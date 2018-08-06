#include <iostream>
#include <algorithm>
#include <regex>
#include <fstream>
#include <chrono>

#include "allscale/utils/vector.h"
#include "allscale/api/user/data/mesh.h"
#include "allscale/api/user/algorithm/vcycle.h"


using namespace allscale::api::user;

#define assert_between(__low, __v, __high)                                                                                                                    \
	if(__allscale_unused auto __allscale_temp_object_ = insieme::utils::detail::LazyAssertion((__low) <= (__v) && (__v) <= (__high)))                         \
	std::cerr << "\nAssertion " #__low " <= " #__v " <= " #__high " of " __FILE__ ":" __allscale_xstr_(__LINE__) " failed!\n\t" #__v " = " << (__v) << "\n"

// -- Number of hierarchical levels in the mesh data structure --
const int NUM_LEVELS = 3;

// -- define types to model the topology of meshes --
using value_t = double;

using Point = allscale::utils::Vector<value_t, 3>;

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

		bool first = true;
		auto& cellTemperature = properties.template get<CellTemperature, Level>();
		mesh.template forAll<Cell, Level>([&](auto c) {
			temperature[c] = cellTemperature[c];
			assert_between(0, temperature[c], 511) << "While initializing level " << Level;
		});
	}

	void jacobiSolver() {
		auto& faceConductivity = properties.template get<FaceConductivity, Level>();
		auto& faceArea         = properties.template get<FaceArea, Level>();

		// calculation of the flux
		mesh.template pforAll<Face, Level>([&](auto f){
			auto in = mesh.template getNeighbor<Face_to_Cell_In>(f);
			auto out = mesh.template getNeighbor<Face_to_Cell_Out>(f);

			assert_between(0, temperature[in], 511) << "On level " << Level;
			assert_between(0, temperature[out], 511) << "On level " << Level;

			value_t gradTemperature = temperature[out] - temperature[in];
			value_t flux = faceConductivity[f] * faceArea[f] * gradTemperature;

			fluxes[f] = flux;
		});

		// update of solution
		mesh.template pforAll<Cell, Level>([&](auto c){
			auto subtractingFaces = mesh.template getNeighbors<Face_to_Cell_In>(c);
			std::stringstream subtractingFluxes;
			for(auto sf : subtractingFaces) {
				temperature[c] += fluxes[sf];
				subtractingFluxes << fluxes[sf] << ", ";
			}
			auto addingFaces = mesh.template getNeighbors<Face_to_Cell_Out>(c);
			std::stringstream addingFluxes;
			for(auto af : addingFaces) {
				temperature[c] -= fluxes[af];
				addingFluxes << fluxes[af] << ", ";
			}

			assert_between(0, temperature[c], 511) << "On level " << Level << "\n adding fluxes: " << addingFluxes.str() << "\n subtracting fluxes: " << subtractingFluxes.str();
		});
	}

	void computeFineToCoarse() {
		if(Level == 0) {
			static int fileId = 0;
			constexpr const char* filePrefix = "step";
			constexpr const char* fileSuffix = ".obj";
			constexpr const char* mtlFile = "ramp.mtl";
			char fn[64];
			snprintf(fn, sizeof(fn), "%s%03d%s", filePrefix, fileId++, fileSuffix);
			std::cout << "Starting file dump to " << fn << "\n";
			auto start = std::chrono::high_resolution_clock::now();
			std::ofstream out(fn);

			// header
			out << "mtllib " << mtlFile << "\n";

			auto& vertexPosition = properties.template get<VertexPosition, Level>();

			mesh.template forAll<Vertex, Level>([&](auto v) {
				const auto& vp = vertexPosition[v];
				out << "v " << vp.x << " " << vp.y << " " << vp.z << "\n";
			});
			mesh.template forAll<Cell, Level>([&](auto c) {
				// set color according to temperature
				out << "\nusemtl r" << (int)temperature[c] << "\n";
				auto vertexRange = mesh.template getNeighbors<Cell_to_Vertex>(c);
				std::vector<allscale::api::user::data::NodeRef<Vertex>> vertices(vertexRange.begin(), vertexRange.end());
				auto vp = [&](int i) { return vertices[i].getOrdinal() + 1; };
				out << "f " << vp(0) << " " << vp(1) << " " << vp(3) << " " << vp(2) << "\n";
				out << "f " << vp(0) << " " << vp(4) << " " << vp(5) << " " << vp(1) << "\n";
				out << "f " << vp(0) << " " << vp(4) << " " << vp(6) << " " << vp(2) << "\n";
				out << "f " << vp(4) << " " << vp(5) << " " << vp(7) << " " << vp(6) << "\n";
				out << "f " << vp(1) << " " << vp(5) << " " << vp(7) << " " << vp(3) << "\n";
				out << "f " << vp(2) << " " << vp(6) << " " << vp(7) << " " << vp(3) << "\n";
			});
			out << "\n";

			auto end = std::chrono::high_resolution_clock::now();
			std::cout << "File dump took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " ms.\n";
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
			assert_between(0, avgTemperature, 511);
			temperature[c] = avgTemperature;
			temperatureBuffer[c] = avgTemperature;
		});
	}

	void prolongateTo(TemperatureStage<Mesh,Level-1>& childStage) {
		mesh.template pforAll<Cell, Level>([&](auto c) {
			auto children = mesh.template getChildren<Parent_to_Child>(c);

			value_t diff = temperature[c] - temperatureBuffer[c];

			for(auto child: children) {
				auto pretemp = childStage.temperature[child];
				//childStage.temperature[child] += diff;
				childStage.temperature[child] += (temperature[c] - childStage.temperature[child]);
				assert_between(0, childStage.temperature[child], 511) << "Pre child temp: " << pretemp << "\n diff: " << diff;
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
				fread(&magic, sizeof(uint32_t), 1, file);
				assert_eq(magic, 0xA115ca1e) << fname << " - magic number after " << name << " list invalid";
			};

			loadList(ret.header.num_vertices, sizeof(FVertex), ret.vertices, "vertex");
			assert_eq((size_t)ret.header.num_vertices, ret.vertices.size());

			for(int i = 0; i < NUM_LEVELS; ++i) {
				FLevelHeader levelHeader;
				fread(&levelHeader, sizeof(levelHeader), 1, file);
				assert_eq(levelHeader.magic_number, 0xA115ca1e) << fname << " - magic number in per-level header doesn't match";
				assert_eq(levelHeader.level, i) << " - level id mismatch";
				loadList(levelHeader.num_cells, sizeof(FCell), ret.cells[i], "cell");
				assert_eq((size_t)levelHeader.num_cells, ret.cells[i].size());
				loadList(levelHeader.num_faces, sizeof(FFace), ret.faces[i], "face");
				assert_eq((size_t)levelHeader.num_faces, ret.faces[i].size());
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

		template<unsigned VLevel>
		struct VertexAssembler {
			void assembleVertices(Builder& builder, MeshFromFileBuilder<Builder, VLevel>& levelBuilder) { }
		};
		template<>
		struct VertexAssembler<0> {
			void assembleVertices(Builder& builder, MeshFromFileBuilder<Builder, 0>& levelBuilder) {
				// create vertices
				for(size_t i = 0; i < levelBuilder.amfFile.vertices.size(); ++i) {
					levelBuilder.vertices.push_back(builder.template create<Vertex,0>());
				}
				// link cells to vertices
				for(size_t id = 0; id < levelBuilder.amfFile.cells[0].size(); ++id) {
					const auto& cell = levelBuilder.amfFile.cells[0][id];

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
						builder.template link<Face_to_Cell_In>(faces[inFaceID], cells[id]);
					}
				}
				for(const auto& outFaceID : cell.out_face_ids) {
					if(outFaceID != -1) {
						builder.template link<Cell_to_Face_Out>(cells[id], faces[outFaceID]);
						builder.template link<Face_to_Cell_Out>(faces[outFaceID], cells[id]);
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
				//faceArea[mFace] = fFace.area;
				//assert_between(0.9, faceArea[mFace], pow(4,Level) + 0.1) << "While loading level " << Level;
				faceArea[mFace] = fFace.area / pow(4,Level);
				assert_between(0.0, faceArea[mFace], 1.0) << "While loading level " << Level;
				auto avgCellConductivity = (amfFile.cells[Level][fFace.in_cell_id].conductivity + amfFile.cells[Level][fFace.out_cell_id].conductivity) / 2.0;
				assert_between(0.19, avgCellConductivity, 0.21) << "While loading level " << Level;
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
	// the number of simulated steps
	const int S = 10;

	auto pair = loadAMF(R"(Z:\allscale\git\allscale-compiler\api\scripts\demo\mesh.amf)");

	auto& mesh = pair.first;
	auto& properties = pair.second;

	using vcycle_type = algorithm::VCycle<
		TemperatureStage,
		std::remove_reference<decltype(mesh)>::type
	>;

	std::cout << "Starting simulation...\n";

	vcycle_type vcycle(mesh, properties);

	// -- simulation --
	vcycle.run(S);

    return EXIT_SUCCESS;
}
