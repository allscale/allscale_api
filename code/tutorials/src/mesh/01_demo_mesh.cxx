#include <iostream>
#include <algorithm>
#include <regex>
#include <fstream>
#include <chrono>

#include "allscale/utils/vector.h"
#include "allscale/api/user/data/mesh.h"
#include "allscale/api/user/algorithm/vcycle.h"

#include "01_demo_mesh_utils.h"

using namespace allscale::api::user;

// -- Number of hierarchical levels, pre-smoothing and post-smoothing steps in the V-cycle --
constexpr int NUM_LEVELS = 20;
constexpr int POST_STEPS = 3;
constexpr int PRE_STEPS = 2 + (NUM_LEVELS == 1 ? POST_STEPS : 0);
constexpr int PARTITION_DEPTH = 5;

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
struct CellVolume       : public data::mesh_property<Cell,value_t> {};
struct FaceArea         : public data::mesh_property<Face,value_t> {};
struct FaceVolumeRatio  : public data::mesh_property<Face,value_t> {};
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

// -- type of a mesh --
template<unsigned levels = 1, unsigned PartitionDepth = PARTITION_DEPTH>
using Mesh = typename MeshBuilder<levels>::template mesh_type<PartitionDepth>;

// -- type of the properties of a mesh --
template<typename Mesh>
using MeshProperties = data::MeshProperties<Mesh::levels, typename Mesh::partition_tree_type,
	CellTemperature,
	CellVolume,
	FaceArea,
	FaceVolumeRatio,
	FaceConductivity,
	VertexPosition>;

// -- V-Cycle stage --
template<typename Mesh, unsigned Level>
struct TemperatureStage {
	template<typename NodeKind,typename ValueType>
	using attribute = typename Mesh::template mesh_data_type<NodeKind,ValueType,Level>;

	// Capture mesh & properties
	const Mesh& mesh;
	MeshProperties<Mesh>& properties;

	// Output configuration & checking
	int outputFreq;
	double energySum = -1.0;

	// Cell data
	attribute<Cell, value_t> temperature;
	attribute<Cell, value_t> oldTemperature;
	attribute<Face, value_t> fluxes;

	TemperatureStage(const Mesh& mesh, MeshProperties<Mesh>& properties, int outputFreq)
		: mesh(mesh), properties(properties), outputFreq(outputFreq)
		, temperature(mesh.template createNodeData<Cell, value_t, Level>())
		, oldTemperature(mesh.template createNodeData<Cell, value_t, Level>())
		, fluxes(mesh.template createNodeData<Face, value_t, Level>()) {

		auto& cellTemperature = properties.template get<CellTemperature, Level>();
		mesh.template forAll<Cell, Level>([&](auto c) {
			temperature[c] = cellTemperature[c];
			assert_temperature(temperature[c]) << "While initializing level " << Level;
		});
	}

	void outputResult();

	void jacobiSolver() {
		auto& faceConductivity = properties.template get<FaceConductivity, Level>();
		auto& faceArea         = properties.template get<FaceArea, Level>();
		auto& faceVolumeRatio  = properties.template get<FaceVolumeRatio, Level>();

		// calculation of the per-face flux
		mesh.template pforAll<Face, Level>([&](auto f) {
			auto in = mesh.template getNeighbor<Face_to_Cell_In>(f);
			auto out = mesh.template getNeighbor<Face_to_Cell_Out>(f);

			value_t gradTemperature = temperature[in] - temperature[out];
			value_t flux = faceVolumeRatio[f] * faceConductivity[f] * faceArea[f] * gradTemperature;

			fluxes[f] = flux;
		});

		// update of the per-cell solution
		mesh.template pforAll<Cell, Level>([&](auto c) {
			value_t prevtemp = temperature[c];
			auto subtractingFaces = mesh.template getNeighbors<Face_to_Cell_In>(c);
			for(auto sf : subtractingFaces) {
				temperature[c] -= fluxes[sf];
			}
			auto addingFaces = mesh.template getNeighbors<Face_to_Cell_Out>(c);
			for(auto af : addingFaces) {
				temperature[c] += fluxes[af];
			}

			assert_temperature(temperature[c]) << "On level " << Level << "\nprev temp: " << prevtemp << "\nCell id: " << c.getOrdinal();
		});
	}

	void computeFineToCoarse() {
		outputResult();
		for(int i = 0; i < PRE_STEPS; ++i) jacobiSolver();
	}

	void computeCoarseToFine() {
		for(int i = 0; i < POST_STEPS; ++i) jacobiSolver();
	}

	void restrictFrom(TemperatureStage<Mesh,Level-1>& childStage) {
		mesh.template pforAll<Cell, Level>([&](auto c) {
			auto children = mesh.template getChildren<Parent_to_Child>(c);
			value_t avgTemperature = 0;
			for(auto child : children) {
				avgTemperature += childStage.temperature[child];
			}
			avgTemperature /= children.size();
			temperature[c] = avgTemperature;
			oldTemperature[c] = avgTemperature;
		});
	}

	void prolongateTo(TemperatureStage<Mesh,Level-1>& childStage) {
		value_t correctionFactor = sqrt(8.0) + (Level-1)/8.0;
		mesh.template pforAll<Cell, Level>([&,correctionFactor](auto c) {
			auto children = mesh.template getChildren<Parent_to_Child>(c);
			for(auto child : children) {
				auto preTemp = oldTemperature[c];
				childStage.temperature[child] += (temperature[c] - preTemp) / children.size() * correctionFactor;
				assert_temperature(childStage.temperature[child]) << "Pre child temp: " << preTemp;
			}
		});
	}
};

#include "01_demo_mesh_io.h"

int main(int argc, char** argv) {

	if(argc < 2) {
		std::cout << "Usage: 01_mesh_demo [amf file] [timesteps (default 10)] [output frequency (default 1)]\n";
		return EXIT_SUCCESS;
	}

	int timeSteps = argc > 2 ? atoi(argv[2]) : 10;
	int outputFreq = argc > 3 ? atoi(argv[3]) : 1;

	auto pair = amfloader::loadAMF(argv[1]);
	auto& mesh = pair.first;
	auto& properties = pair.second;

	auto vcycle = algorithm::make_vcycle<TemperatureStage>(mesh, properties, outputFreq);

	std::cout << "Starting simulation...\n";

	vcycle.run(timeSteps);

	return EXIT_SUCCESS;
}
