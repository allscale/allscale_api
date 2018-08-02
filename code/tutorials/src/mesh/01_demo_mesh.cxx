#include <iostream>
#include <algorithm>
#include <regex>
#include <fstream>

#include "allscale/utils/vector.h"
#include "allscale/api/user/data/mesh.h"
#include "allscale/api/user/algorithm/vcycle.h"


using namespace allscale::api::user;


// -- Number of hierarchical levels in the mesh data structure --
const int NUM_LEVELS = 3;

// -- define types to model the topology of meshes --
using value_t = double;

using Point = allscale::utils::Vector<value_t, 3>;

value_t norm(const Point& a) {
	return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

value_t dist(const Point& a, const Point& b) {
	return norm(a - b);
}

Point cross(const Point& a, const Point& b) {
	return Point{
			a.y*b.z-a.z*b.y,
			a.z*b.x-a.x*b.z,
			a.x*b.y-a.y*b.x
	};
}

value_t area(const Point& a, const Point& b, const Point& c) {
	return norm(cross(a-b,a-c))/2;
}

// - elements -
struct Cell {};
struct Face {};
struct Vertex {};

// - per-cell connections -
struct Cell_to_Vertex   : public data::edge<Cell, Vertex> {}; // 8x
struct Cell_to_Face_In  : public data::edge<Cell, Face> {};   // 3x
struct Cell_to_Face_Out : public data::edge<Cell, Face> {};   // 3x

// - per-face connections -
struct Face_to_Cell_In  : public data::edge<Face, Cell> {}; // 1x
struct Face_to_Cell_Out : public data::edge<Face, Cell> {}; // 1x

// - inter-layer connections -
struct Parent_to_Child : public data::hierarchy<Cell,Cell> {};

// -- property data --
struct CellTemperature  : public data::mesh_property<Cell,value_t> {};
struct FaceArea         : public data::mesh_property<Face,value_t> {};
struct FaceConductivity : public data::mesh_property<Face,value_t> {};
struct VertexPosition   : public data::mesh_property<Vertex, Point> {};

// - define the mesh and builder -
template<unsigned levels = 1>
using MeshBuilder = data::MeshBuilder<
	data::nodes<
		Cell,
		Face,
		Vertex
	>,
	data::edges<
		Cell_to_Vertex,
		Cell_to_Face_In,
		Cell_to_Face_Out,
		Face_to_Cell_In,
		Face_to_Cell_Out
	>,
	data::hierarchies<
		Parent_to_Child
	>,
	levels
>;

// -- the type of a mesh --
template<unsigned levels = 1, unsigned PartitionDepth = 0>
using Mesh = typename MeshBuilder<levels>::template mesh_type<PartitionDepth>;

// -- type of the properties of a mesh --
template<typename Mesh>
using MeshProperties = data::MeshProperties<Mesh::levels, typename Mesh::partition_tree_type,
	CellTemperature,
	FaceArea,
	FaceConductivity,
	VertexPosition>;

// -- V-Cycle stage --
template<
	typename Mesh,
	unsigned Level
>
struct TemperatureStage {
	template<typename NodeKind,typename ValueType>
	using attribute = typename Mesh::template mesh_data_type<NodeKind,ValueType,Level>;

	// capture mesh
	const Mesh& mesh;
	// capture mesh properties
	MeshProperties<Mesh>& properties;

	// Cell data
	attribute<Cell, value_t> temperature;
	attribute<Cell, value_t> temperatureBuffer;
	attribute<Face, value_t> fluxes;

	TemperatureStage(const Mesh& mesh, MeshProperties<Mesh>& properties)
		: mesh(mesh), properties(properties)
		, temperature(mesh.template createNodeData<Cell, value_t, Level>())
		, temperatureBuffer(mesh.template createNodeData<Cell, value_t, Level>())
		, fluxes(mesh.template createNodeData<Face, value_t, Level>()) {

		// -- initialize attributes --

		// simulation parameters
		int TF = 511;		// < first temperature
		int T0 = 20;		// < initial temperature

		bool first = true;
		mesh.template forAll<Cell, Level>([&](auto c) {
			if(first) {
				temperature[c] = TF;
				first = false;
			}
			else {
				temperature[c] = T0;
			}
		});
	}

	void jacobiSolver() {
		auto& faceConductivity = properties.template get<FaceConductivity, Level>();
		auto& faceArea         = properties.template get<FaceArea, Level>();

		// calculation of the flux
		mesh.template pforAll<Face, Level>([&](auto f){
			auto in = mesh.template getNeighbor<Face_to_Cell_In>(f);
			auto out = mesh.template getNeighbor<Face_to_Cell_Out>(f);

			value_t gradTemperature = temperature[out] - temperature[in];
			value_t flux = faceConductivity[f] * faceArea[f] * gradTemperature;

			fluxes[f] = flux;
		});

		// update of solution
		mesh.template pforAll<Cell, Level>([&](auto c){
			auto subtractingFaces = mesh.template getNeighbors<Face_to_Cell_In>(c);
			for(auto sf : subtractingFaces) {
				temperature[c] += fluxes[sf];
			}
			auto addingFaces = mesh.template getNeighbors<Face_to_Cell_Out>(c);
			for(auto af : addingFaces) {
				temperature[c] -= fluxes[af];
			}
		});
	}

	void computeFineToCoarse() {
		if(Level == 0) {
			//mesh.template forAll<Cell, Level>([&](auto c){
			//	printf("%6.2lf, ", temperature[c]);
			//});

			static int fileId = 0;
			constexpr char* filePrefix = "step";
			constexpr char* fileSuffix = ".obj";
			constexpr char* mtlFile = "ramp.mtl";
			char fn[64];
			snprintf(fn, sizeof(fn), "%s%03d%s", filePrefix, fileId++, fileSuffix);
			std::ofstream out(fn);

			// header
			out << "mtllib " << mtlFile << "\n";

			auto& vertexPosition = properties.template get<VertexPosition, Level>();

			int vertexId = 1; // FORTRAN programmers made .obj
			mesh.template forAll<Cell, Level>([&](auto c) {
				out << "\n# cell ---\n";
				// set color according to temperature
				out << "usemtl r" << (int)temperature[c] << "\n";
				auto vertices = mesh.template getNeighbors<Cell_to_Vertex>(c);
				for(auto v : vertices) {
					const auto& vp = vertexPosition[v];
					out << "v " << vp.x << " " << vp.y << " " << vp.z << "\n";
				}
				std::stringstream ss;
				ss << "f ~0 ~3 ~4 ~7\n"
				   << "f ~0 ~3 ~2 ~1\n"
				   << "f ~0 ~1 ~6 ~7\n"
				   << "f ~3 ~2 ~5 ~4\n"
				   << "f ~4 ~5 ~6 ~7\n"
				   << "f ~1 ~2 ~5 ~6\n";
				std::string faces = ss.str();
				for(int i = 0; i < 8; ++i) {
					char toReplace[8], replacement[8];
					snprintf(toReplace, sizeof(toReplace), "~%d", i);
					snprintf(replacement, sizeof(replacement), "%d", vertexId+i);
					faces = std::regex_replace(faces, std::regex(toReplace), replacement);
				}
				out << faces;

				vertexId += 8;
			});

			out << "\n";
		}

		jacobiSolver();
	}

	void computeCoarseToFine() {
		jacobiSolver();
	}

	void restrictFrom(TemperatureStage<Mesh,Level-1>& childStage) {
		mesh.template pforAll<Cell, Level>([&](auto c) {
			// set to average temperature of children, and store the average for prolongation
			auto children = mesh.template getChildren<Parent_to_Child>(c);
			value_t avgTemperature = 0;
			for(auto child : children) {
				avgTemperature += childStage.temperature[child];
			}
			avgTemperature /= children.size();
			temperature[c] = avgTemperature;
			temperatureBuffer[c] = avgTemperature;
		});
	}

	void prolongateTo(TemperatureStage<Mesh,Level-1>& childStage) {
		mesh.template pforAll<Cell, Level>([&](auto c) {
			auto children = mesh.template getChildren<Parent_to_Child>(c);

			value_t diff = temperature[c] - temperatureBuffer[c];

			for(auto child: children) {
				childStage.temperature[child] += diff;
			}
		});
	}
};

namespace detail {
	std::pair<Mesh<NUM_LEVELS>, MeshProperties<Mesh<NUM_LEVELS>>> createTube(const int N);
}


namespace {
	// file format structures
	#pragma pack(push, 1)
	struct FVertex {
		double x, y, z;
	};
	struct FCell {
		int32_t level;
		double temperature;
		double conductivity;
		std::array<int32_t,3> in_face_ids;
		std::array<int32_t,3> out_face_ids;
		std::array<int32_t,8> vertex_ids;
		std::array<int32_t,8> child_cell_ids;
	};
	struct FFace {
		int32_t level;
		double area;
		int32_t in_cell_id;
		int32_t out_cell_id;
	};
	struct FHeader {
		uint32_t magic_number;
		int32_t num_levels;
		int32_t num_vertices;
	};
	struct FLevelHeader {
		uint32_t magic_number;
		int32_t level;
		int32_t num_cells;
		int32_t num_faces;
	};
	#pragma pack(pop)

	struct AMFFile {
		FHeader header;
		std::vector<FVertex> vertices;
		std::vector<FCell> cells[NUM_LEVELS];
		std::vector<FFace> faces[NUM_LEVELS];

		static AMFFile load(const std::string& fname) {
			AMFFile ret;
			auto file = fopen(fname.c_str(), "rb");
			fread(&ret.header, sizeof(ret.header), 1, file);
			assert_eq(ret.header.magic_number, 0xA115ca1e) << fname << " - magic number in header doesn't match";
			assert_eq(ret.header.num_levels, NUM_LEVELS) << fname << " - mismatch between file number of levels and C++ NUM_LEVELS";
			std::cout << "File info - " << ret.header.num_levels << " Levels // " << ret.header.num_vertices << " Vertices\n";

			auto loadList = [&](int count, int elem_size, auto& target, const char* name) {
				target.resize(count);
				fread(target.data(), elem_size, count, file);
				uint32_t magic = 0;
				std::size_t ret = fread(&magic, sizeof(uint32_t), 1, file);
				assert_eq(magic, 0xA115ca1e) << fname << " - magic number after " << name << " list invalid";
			};

			loadList(ret.header.num_vertices, sizeof(FVertex), ret.vertices, "vertex");
			assert_eq(ret.header.num_vertices, ret.vertices.size());

			for(int i = 0; i < NUM_LEVELS; ++i) {
				FLevelHeader levelHeader;
				fread(&levelHeader, sizeof(levelHeader), 1, file);
				assert_eq(levelHeader.magic_number, 0xA115ca1e) << fname << " - magic number in per-level header doesn't match";
				assert_eq(levelHeader.level, i) << " - level id mismatch";
				loadList(levelHeader.num_cells, sizeof(FCell), ret.cells[i], "cell");
				assert_eq(levelHeader.num_cells, ret.cells[i].size());
				loadList(levelHeader.num_faces, sizeof(FFace), ret.faces[i], "face");
				assert_eq(levelHeader.num_faces, ret.faces[i].size());
			}

			return ret;
		}
	};

	template<typename Builder, unsigned Level>
	class MeshFromFileBuilder {

		using CellRef = typename data::NodeRef<Cell,Level>;
		using FaceRef = typename data::NodeRef<Face,Level>;
		using VertexRef = typename data::NodeRef<Vertex,Level>;

		std::vector<VertexRef> vertices;
		std::vector<CellRef> cells;
		std::vector<FaceRef> faces;

		const AMFFile& amfFile;
		MeshFromFileBuilder<Builder, Level+1> subBuilder;

		template<unsigned Level>
		struct VertexAssembler {
			void assembleVertices(Builder& builder, MeshFromFileBuilder<Builder, Level>& levelBuilder) { }
		};
		template<>
		struct VertexAssembler<0> {
			void assembleVertices(Builder& builder, MeshFromFileBuilder<Builder, Level>& levelBuilder) {
				// create vertices
				for(size_t i = 0; i < levelBuilder.amfFile.vertices.size(); ++i) {
					levelBuilder.vertices.push_back(builder.template create<Vertex,0>());
				}
				// link cells to vertices
				for(size_t id = 0; id < levelBuilder.amfFile.cells[Level].size(); ++id) {
					const auto& cell = levelBuilder.amfFile.cells[Level][id];

					for(const auto& vtxId : cell.vertex_ids) {
						if(vtxId != -1) {
							builder.template link<Cell_to_Vertex>(levelBuilder.cells[id], levelBuilder.vertices[vtxId]);
						}
					}
				}
			}
		};

		template<unsigned Level>
		struct HierarchyLinker {
			void linkHierarchy(Builder& builder, MeshFromFileBuilder<Builder, Level>& levelBuilder, MeshFromFileBuilder<Builder, Level+1>& subBuilder) {
				// link cells to child cells
				for(size_t id = 0; id < levelBuilder.amfFile.cells[Level+1].size(); ++id) {
					const auto& amfCell = levelBuilder.amfFile.cells[Level+1][id];
					const auto& targetCell = subBuilder.getCell(static_cast<int>(id));
					for(const auto childCellId : amfCell.child_cell_ids) {
						if(childCellId != -1) {
							builder.template link<Parent_to_Child>(targetCell, levelBuilder.cells[childCellId]);
						}
					}
				}
			}
		};
		template<>
		struct HierarchyLinker<NUM_LEVELS-1> {
			void linkHierarchy(Builder& builder, MeshFromFileBuilder<Builder, Level>& levelBuilder, MeshFromFileBuilder<Builder, Level+1>& subBuilder) { }
		};

	  public:

		MeshFromFileBuilder(const AMFFile& amfFile) : amfFile(amfFile), subBuilder(amfFile) { }

		CellRef getCell(int idx) {
			return cells[idx];
		}

		void assembleMesh(Builder& builder) {

			// create cells
			for(const auto& cell : amfFile.cells[Level]) {
				cells.push_back(builder.template create<Cell, Level>());
				assert_eq(cell.level, Level) << "Cell level mismatch";
			}

			// create faces
			for(const auto& face : amfFile.faces[Level]) {
				faces.push_back(builder.template create<Face, Level>());
				assert_eq(face.level, Level) << "Face level mismatch";
			}

			// link cells to faces inward and outward
			for(size_t id = 0; id < amfFile.cells[Level].size(); ++id) {
				const auto& cell = amfFile.cells[Level][id];
				for(const auto& inFaceID : cell.in_face_ids) {
					if(inFaceID != -1) {
						builder.template link<Cell_to_Face_In>(cells[id], faces[inFaceID]);
					}
				}
				for(const auto& outFaceID : cell.out_face_ids) {
					if(outFaceID != -1) {
						builder.template link<Cell_to_Face_Out>(cells[id], faces[outFaceID]);
					}
				}
			}

			VertexAssembler<Level>{}.assembleVertices(builder, *this);
			subBuilder.assembleMesh(builder);
			HierarchyLinker<Level>{}.linkHierarchy(builder, *this, subBuilder);

			printf("Finished geometry for level %lu - %10lu vertices %10lu cells %10lu faces\n", Level, (unsigned)vertices.size(), (unsigned)cells.size(), (unsigned)faces.size());
		}

		void addVertexProperties(Mesh<NUM_LEVELS>& mesh, MeshProperties<Mesh<NUM_LEVELS>>& properties) {
			// vertex properties
			auto& vertexPosition = properties.template get<VertexPosition,Level>();
			for(int vid = 0; vid < vertices.size(); ++vid) {
				const auto& mVtx = vertices[vid];
				const auto& fVtx = amfFile.vertices[vid];
				vertexPosition[mVtx] = { fVtx.x, fVtx.y, fVtx.z };
			}
		}

		void addPropertyData(Mesh<NUM_LEVELS>& mesh, MeshProperties<Mesh<NUM_LEVELS>>& properties) {
			// cell properties
			auto& cellTemperature = properties.template get<CellTemperature,Level>();
			for(int cid = 0; cid < cells.size(); ++cid) {
				const auto& mCell = cells[cid];
				const auto& fCell = amfFile.cells[Level][cid];
				cellTemperature[mCell] = fCell.temperature;
			}

			// face properties
			auto& faceArea = properties.template get<FaceArea,Level>();
			auto& faceConductivity = properties.template get<FaceConductivity,Level>();
			for(int fid = 0; fid < faces.size(); ++fid) {
				const auto& mFace = faces[fid];
				const auto& fFace = amfFile.faces[Level][fid];
				faceArea[mFace] = fFace.area;
				auto avgCellConductivity = (amfFile.cells[Level][fFace.in_cell_id].conductivity + amfFile.cells[Level][fFace.out_cell_id].conductivity) / 2.0;
				faceConductivity[mFace] = avgCellConductivity;
			}

			// initialize sublevel properties
			subBuilder.addPropertyData(mesh, properties);
		}
	};
	template<typename Builder>
	class MeshFromFileBuilder<Builder, NUM_LEVELS> {
	  public:
		MeshFromFileBuilder(const AMFFile&) { }
		data::NodeRef<Cell,NUM_LEVELS> getCell(int idx) { return {}; }
		void assembleMesh(Builder& builder) { }
		void addPropertyData(Mesh<NUM_LEVELS>& mesh, MeshProperties<Mesh<NUM_LEVELS>>& properties) { }
	};

	std::pair<Mesh<NUM_LEVELS>, MeshProperties<Mesh<NUM_LEVELS>>> loadAMF(const std::string& filename) {

		// load file
		auto file = AMFFile::load(filename);

		// create geometric information
		MeshBuilder<NUM_LEVELS> builder;
		MeshFromFileBuilder<MeshBuilder<NUM_LEVELS>, 0> fileBuilder(file);
		fileBuilder.assembleMesh(builder);
		auto mesh = std::move(builder).build();

		// create properties
		auto properties = mesh.template createKnownProperties<MeshProperties<decltype(mesh)>>();
		fileBuilder.addVertexProperties(mesh, properties);
		fileBuilder.addPropertyData(mesh, properties);

		return std::make_pair(std::move(mesh), std::move(properties));
	}
}


int main() {
	loadAMF(R"(Z:\allscale\git\allscale-compiler\api\scripts\demo\mesh.amf)");

	//// the number of simulated steps
	//const int S = 10;

	//// create the mesh and properties
	//auto pair = detail::createTube(N);

	//auto& mesh = pair.first;
	//auto& properties = pair.second;

	//using vcycle_type = algorithm::VCycle<
	//	TemperatureStage,
	//	std::remove_reference<decltype(mesh)>::type
	//>;

	//vcycle_type vcycle(mesh, properties);

	//// -- simulation --
	//// run S iterations
	//vcycle.run(S);

    return EXIT_SUCCESS;
}
