
#include <vector>
#include <array>
#include <map>
#include <inttypes.h>

#include "allscale/utils/assert.h"

namespace amfloader {
	// file format structures
	#pragma pack(push, 1)
	struct FVertex {
		double x, y, z;
	};
	struct FCell {
		int32_t level;
		double temperature;
		double conductivity;
		std::array<int32_t,25> in_face_ids;
		std::array<int32_t,25> out_face_ids;
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
			assert_true(file) << "Could not open " << fname << " for reading";
			__allscale_unused size_t readVal = fread(&ret.header, sizeof(ret.header), 1, file);
			assert_eq(readVal, 1);
			assert_eq(ret.header.magic_number, 0xA115ca1e) << fname << " - magic number in header doesn't match";
			assert_eq(ret.header.num_levels, NUM_LEVELS) << fname << " - mismatch between file number of levels and C++ NUM_LEVELS";
			std::cout << "File info - " << ret.header.num_levels << " Levels // " << ret.header.num_vertices << " Vertices\n";

			auto loadList = [&](size_t count, size_t elem_size, auto& target, const char* name) {
				target.resize(count);
				__allscale_unused size_t readVals = fread(target.data(), elem_size, count, file);
				assert_eq(readVals, count);
				uint32_t magic = 0;
				readVals = fread(&magic, sizeof(uint32_t), 1, file);
				assert_eq(readVals, 1);
				assert_eq(magic, 0xA115ca1e) << fname << " - magic number after " << name << " list invalid";
			};

			loadList(ret.header.num_vertices, sizeof(FVertex), ret.vertices, "vertex");
			assert_eq((size_t)ret.header.num_vertices, ret.vertices.size());

			for(int i = 0; i < NUM_LEVELS; ++i) {
				FLevelHeader levelHeader;
				__allscale_unused size_t readVals = fread(&levelHeader, sizeof(levelHeader), 1, file);
				assert_eq(readVals, 1);
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

	// forward declarations
	template<typename Builder, unsigned Level>
	class MeshFromFileBuilder;

	template<typename Builder, unsigned VLevel>
	struct VertexAssembler {
		void assembleVertices(Builder& builder, MeshFromFileBuilder<Builder, VLevel>& levelBuilder);
	};
	template<typename Builder, unsigned Level>
	struct HierarchyLinker {
		void linkHierarchy(Builder& builder, MeshFromFileBuilder<Builder, Level>& levelBuilder, MeshFromFileBuilder<Builder, Level+1>& subBuilder);
	};
	template<typename MeshT, typename MeshPropertiesT, unsigned Level>
	struct VolumePropertyCalculator {
		void setVolume(const MeshT& mesh, MeshPropertiesT& properties);
	};

	template<typename Builder, unsigned Level>
	class MeshFromFileBuilder {

		template<typename VBuilder, unsigned VLevel>
		friend struct VertexAssembler;
		template<typename HBuilder, unsigned HLevel>
		friend struct HierarchyLinker;

		using CellRef = typename data::NodeRef<Cell,Level>;
		using FaceRef = typename data::NodeRef<Face,Level>;
		using VertexRef = typename data::NodeRef<Vertex,Level>;

		std::vector<VertexRef> vertices;
		std::vector<CellRef> cells;
		std::vector<FaceRef> faces;

		const AMFFile& amfFile;
		MeshFromFileBuilder<Builder, Level+1> subBuilder;

	  public:

		MeshFromFileBuilder(const AMFFile& amfFile) : amfFile(amfFile), subBuilder(amfFile) { }

		CellRef getCell(int idx) {
			return cells[idx];
		}

		void assembleMesh(Builder& builder) {

			// create cells
			for(__allscale_unused const auto& cell : amfFile.cells[Level]) {
				cells.push_back(builder.template create<Cell, Level>());
				assert_eq(cell.level, Level) << "Cell level mismatch";
			}

			// create faces
			for(__allscale_unused const auto& face : amfFile.faces[Level]) {
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

			VertexAssembler<Builder, Level>{}.assembleVertices(builder, *this);
			subBuilder.assembleMesh(builder);
			HierarchyLinker<Builder, Level>{}.linkHierarchy(builder, *this, subBuilder);

			printf("Finished geometry for level %2u - %10u vertices %10u cells %10u faces\n", Level, (unsigned)vertices.size(), (unsigned)cells.size(), (unsigned)faces.size());
		}

		void addVertexProperties(MeshProperties<Mesh<NUM_LEVELS>>& properties) {
			// vertex properties
			auto& vertexPosition = properties.template get<VertexPosition,Level>();
			for(size_t vid = 0; vid < vertices.size(); ++vid) {
				const auto& mVtx = vertices[vid];
				const auto& fVtx = amfFile.vertices[vid];
				vertexPosition[mVtx] = { fVtx.x, fVtx.y, fVtx.z };
			}
		}

		void addPropertyData(const Mesh<NUM_LEVELS>& mesh, MeshProperties<Mesh<NUM_LEVELS>>& properties) {
			// cell properties
			auto& cellTemperature = properties.template get<CellTemperature,Level>();
			auto& cellVolume = properties.template get<CellVolume,Level>();
			for(size_t cid = 0; cid < cells.size(); ++cid) {
				const auto& mCell = cells[cid];
				const auto& fCell = amfFile.cells[Level][cid];
				cellTemperature[mCell] = fCell.temperature;
			}
			VolumePropertyCalculator<Mesh<NUM_LEVELS>, MeshProperties<Mesh<NUM_LEVELS>>, Level>{}.setVolume(mesh, properties);

			// face properties
			auto& faceArea = properties.template get<FaceArea,Level>();
			auto& faceConductivity = properties.template get<FaceConductivity,Level>();
			auto& faceVolumeRatio = properties.template get<FaceVolumeRatio,Level>();
			for(size_t fid = 0; fid < faces.size(); ++fid) {
				const auto& mFace = faces[fid];
				const auto& fFace = amfFile.faces[Level][fid];
				faceArea[mFace] = fFace.area / (pow(4,Level)*2);
				assert_between(0.0, faceArea[mFace], 1.0) << "While loading level " << Level;
				auto avgCellConductivity = (amfFile.cells[Level][fFace.in_cell_id].conductivity + amfFile.cells[Level][fFace.out_cell_id].conductivity) / 2.0;
				assert_between(0.0, avgCellConductivity, 1.0/6.0) << "While loading level " << Level << "\n(Total potential conductivity to a cell from 6 faces must not be greater than 1)";
				faceConductivity[mFace] = avgCellConductivity;

				auto inVolume = cellVolume[mesh.template getNeighbor<Face_to_Cell_In>(mFace)];
				auto outVolume = cellVolume[mesh.template getNeighbor<Face_to_Cell_Out>(mFace)];
				auto largerVolume = std::max(inVolume, outVolume);
				auto smallerVolume = std::min(inVolume, outVolume);
				faceVolumeRatio[mFace] = smallerVolume / largerVolume;
			}

			// initialize sublevel properties
			subBuilder.addPropertyData(mesh, properties);
		}
	};
	template<typename Builder>
	class MeshFromFileBuilder<Builder, NUM_LEVELS> {
	  public:
		MeshFromFileBuilder(const AMFFile&) { }
		data::NodeRef<Cell,NUM_LEVELS> getCell(int) { return {}; }
		void assembleMesh(Builder&) { }
		void addPropertyData(const Mesh<NUM_LEVELS>&, MeshProperties<Mesh<NUM_LEVELS>>&) { }
	};


	template<typename Builder, unsigned VLevel>
	void VertexAssembler<Builder, VLevel>::assembleVertices(Builder&, MeshFromFileBuilder<Builder, VLevel>&) {
		// vertices are only assembled at level 0
	}
	template<typename Builder>
	struct VertexAssembler<Builder, 0> {
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

	template<typename Builder, unsigned Level>
	void HierarchyLinker<Builder, Level>::linkHierarchy(Builder& builder, MeshFromFileBuilder<Builder, Level>& levelBuilder, MeshFromFileBuilder<Builder, Level+1>& subBuilder) {
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
	template<typename Builder>
	struct HierarchyLinker<Builder, NUM_LEVELS-1> {
		void linkHierarchy(Builder&, MeshFromFileBuilder<Builder, NUM_LEVELS-1>&, MeshFromFileBuilder<Builder, NUM_LEVELS>&) { }
	};

	template<typename MeshT, typename MeshPropertiesT, unsigned Level>
	void VolumePropertyCalculator<MeshT, MeshPropertiesT, Level>::setVolume(const MeshT& mesh, MeshPropertiesT& properties) {
		auto& cellVolume = properties.template get<CellVolume,Level>();
		mesh.template forAll<Cell, Level>([&](const auto& c) {
			value_t vol = 0;
			for(auto child : mesh.template getChildren<Parent_to_Child>(c)) {
				auto& subVolume = properties.template get<CellVolume,Level-1>();
				vol += subVolume[child];
			}
			cellVolume[c] = vol;
		});
	}
	template<typename MeshT, typename MeshPropertiesT>
	struct VolumePropertyCalculator<MeshT, MeshPropertiesT, 0> {
		void setVolume(const MeshT& mesh, MeshPropertiesT& properties) {
			auto& cellVolume = properties.template get<CellVolume,0>();
			mesh.template forAll<Cell>([&](const auto& c) {
				cellVolume[c] = 1;
			});
		}
	};

	std::pair<Mesh<NUM_LEVELS>, MeshProperties<Mesh<NUM_LEVELS>>> loadAMF(const std::string& filename) {

		// load file
		auto file = AMFFile::load(filename);

		// create geometric information
		MeshBuilder<NUM_LEVELS> builder;
		MeshFromFileBuilder<MeshBuilder<NUM_LEVELS>, 0> fileBuilder(file);
		fileBuilder.assembleMesh(builder);
		auto mesh = std::move(builder).build<PARTITION_DEPTH>();

		// create properties
		auto properties = mesh.template createKnownProperties<MeshProperties<decltype(mesh)>>();
		fileBuilder.addVertexProperties(properties);
		fileBuilder.addPropertyData(mesh, properties);

		return std::make_pair(std::move(mesh), std::move(properties));
	}
}

enum class OutputFormat {
	Unset,
	None,
	AVF,
	CSV,
	OBJ
};
static const std::map<OutputFormat, const char*> fmtSuffixes {
	{ OutputFormat::AVF, "avf" },
	{ OutputFormat::CSV, "csv" },
	{ OutputFormat::OBJ, "obj" },
};

template<typename Mesh, unsigned Level>
void TemperatureStage<Mesh, Level>::outputResult() {
	static OutputFormat fmt = OutputFormat::Unset;
	if(fmt == OutputFormat::Unset) {
		fmt = OutputFormat::None;
		if(getenv("OUTPUT_AVF")) fmt = OutputFormat::AVF;
		else if(getenv("OUTPUT_CSV")) fmt = OutputFormat::CSV;
		else if(getenv("OUTPUT_OBJ")) fmt = OutputFormat::OBJ;
	}
	if(fmt == OutputFormat::None) return;

	static auto simStartTime = std::chrono::high_resolution_clock::now();
	static long long simTime = 0;
	simTime += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - simStartTime).count();

	if(Level == 0) {
		bool checkResult = getenv("CHECK_RESULT");
		if(checkResult) {
			double sum = 0.0;
			mesh.template forAll<Cell, Level>([&](auto c) {
				sum += temperature[c];
			});
			if(energySum > 0.0) {
				assert_between(-0.1, sum - energySum, 0.1) << "Lost/gained too much energy!\n From " << energySum << " to " << sum;
			}
			energySum = sum;
		}

		static int fileId = 0;
		if((fmt != OutputFormat::None) && fileId % outputFreq == 0) {
			constexpr const char* filePrefix = "step";
			constexpr const char* mtlFile = "ramp.mtl";
			const char* fileSuffix = fmtSuffixes.at(fmt);
			char fn[64];
			snprintf(fn, sizeof(fn), "%s%03d.%s", filePrefix, fileId, fileSuffix);
			auto start = std::chrono::high_resolution_clock::now();
			std::ofstream out(fn, std::ios::binary);

			auto& vertexPosition = properties.template get<VertexPosition, Level>();

			switch(fmt) {
				case OutputFormat::AVF: {
					if(fileId == 0) { // only write geometry on step 0
						std::ofstream g("geom.avf", std::ios::binary);
						g << "Mesh geometry information: " << mesh.template getNumNodes<Cell>() << " Cells / " <<  mesh.template getNumNodes<Face>() << " Faces / " << Mesh::levels << " Levels\n";
						mesh.template forAll<Cell, Level>([&](auto c) {
							if(mesh.template getNeighbors<Cell_to_Face_In>(c).size() + mesh.template getNeighbors<Cell_to_Face_Out>(c).size() < 6) {
								auto v = mesh.template getNeighbors<Cell_to_Vertex>(c).front();
								auto& vp = vertexPosition[v];
								g << vp.x << "," << vp.y << "," << vp.z << "\n";
							}
						});
					}
					char buffer[512];
					snprintf(buffer, sizeof(buffer), "Step %3d - Simulation execution time %12lld ms\n", fileId, simTime);
					out << buffer;
					mesh.template forAll<Cell, Level>([&](auto c) {
						if(mesh.template getNeighbors<Cell_to_Face_In>(c).size() + mesh.template getNeighbors<Cell_to_Face_Out>(c).size() < 6) {
							float temp = static_cast<float>(temperature[c]);
							out.write((const char*)&temp, sizeof(float));
						}
					});
					break;
				}
				case OutputFormat::CSV: {
					out << "x,y,z,temp\n";
					mesh.template forAll<Cell, Level>([&](auto c) {
						auto vertexRange = mesh.template getNeighbors<Cell_to_Vertex>(c);
						std::vector<data::NodeRef<Vertex, Level>> vertices(vertexRange.begin(), vertexRange.end());
						auto& vp = vertexPosition[vertices[0]];
						out << vp.x << "," << vp.y << "," << vp.z << "," << temperature[c] << "\n";
					});
					break;
				}
				case OutputFormat::OBJ: {
					out << "mtllib " << mtlFile << "\n";
					mesh.template forAll<Vertex, Level>([&](auto v) {
						const auto& vp = vertexPosition[v];
						out << "v " << vp.x << " " << vp.y << " " << vp.z << "\n";
					});
					mesh.template forAll<Cell, Level>([&](auto c) {
						auto vertexRange = mesh.template getNeighbors<Cell_to_Vertex>(c);
						std::vector<data::NodeRef<Vertex, Level>> vertices(vertexRange.begin(), vertexRange.end());
						// set color according to temperature
						out << "\nusemtl r" << (temperature[c] > MAX_TEMP ? 31337 : (int)temperature[c]) << "\n";
						auto vp = [&](int i) { return vertices[i].getOrdinal() + 1; };
						out << "f " << vp(0) << " " << vp(1) << " " << vp(3) << " " << vp(2) << "\n";
						out << "f " << vp(0) << " " << vp(4) << " " << vp(5) << " " << vp(1) << "\n";
						out << "f " << vp(0) << " " << vp(4) << " " << vp(6) << " " << vp(2) << "\n";
						out << "f " << vp(4) << " " << vp(5) << " " << vp(7) << " " << vp(6) << "\n";
						out << "f " << vp(1) << " " << vp(5) << " " << vp(7) << " " << vp(3) << "\n";
						out << "f " << vp(2) << " " << vp(6) << " " << vp(7) << " " << vp(3) << "\n";
					});
					break;
				}
				default: break;
			}

			auto end = std::chrono::high_resolution_clock::now();
			std::cout << "File dumped to " << fn << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms.\n";
		}
		fileId++;
		simStartTime = std::chrono::high_resolution_clock::now();
	}
}
