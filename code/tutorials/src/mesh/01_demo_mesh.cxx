#include <iostream>
#include <algorithm>

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
		int TF = 120;		// < first temperature
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
			mesh.template forAll<Cell, Level>([&](auto c){
				printf("%6.2lf, ", temperature[c]);
			});
			std::cout << "\n";
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

int main() {

	// the length of the simulated tube
	const int N = 20;

	// the number of simulated steps
	const int S = 10;

	// create the mesh and properties
	auto pair = detail::createTube(N);

	auto& mesh = pair.first;
	auto& properties = pair.second;

	using vcycle_type = algorithm::VCycle<
		TemperatureStage,
		std::remove_reference<decltype(mesh)>::type
	>;

	vcycle_type vcycle(mesh, properties);

	// -- simulation --
	// run S iterations
	vcycle.run(S);

    return EXIT_SUCCESS;
}

namespace detail {
	// -- class to build the mesh --
	template<typename Builder, unsigned Level>
	class TubeLayerBuilderBase {

		template<typename B, unsigned L>
		friend class TubeLayerBuilder;

		enum { level = Level - 1 };

		using CellRef = typename data::NodeRef<Cell,level>;
		using FaceRef = typename data::NodeRef<Face,level>;
		using VertexRef = typename data::NodeRef<Vertex,level>;

		std::vector<CellRef> cells;
		std::vector<FaceRef> faces;
		std::vector<VertexRef> vertices;

	public:

		void assembleMesh(Builder& builder, int length) {

			// -- cells --

			// create cells
			for(int i=0; i<length; i++) {
				cells.push_back(builder.template create<Cell,level>());
			}

			// -- faces --

			// create faces
			for(int i=0; i<length-1; i++) {
				faces.push_back(builder.template create<Face,level>());
			}

			// link faces with cells downward
			for(int i=0; i<length-1; i++) {
				builder.template link<Face_to_Cell_In>(faces[i], cells[i]);
				builder.template link<Cell_to_Face_In>(cells[i], faces[i]);
			}

			// link faces with cells upward
			for(int i=0; i<length-1; i++) {
				builder.template link<Face_to_Cell_Out>(faces[i], cells[i+1]);
				builder.template link<Cell_to_Face_Out>(cells[i+1], faces[i]);
			}

			// -- vertices --

			// create 4 nodes per face + 8 for the boundary faces
			int numNodes = (length-1)*4+8;
			for(int i=0; i<numNodes; i++) {
				vertices.push_back(builder.template create<Vertex,level>());
			}

			// link cells to vertices
			for(int i=0; i<numNodes; i+=4) {
				auto cellId = i % 4;
				if(cellId > 0) {
					auto leftCell = cells[cellId - 1];
					builder.template link<Cell_to_Vertex>(leftCell, vertices[i+0]);
					builder.template link<Cell_to_Vertex>(leftCell, vertices[i+1]);
					builder.template link<Cell_to_Vertex>(leftCell, vertices[i+2]);
					builder.template link<Cell_to_Vertex>(leftCell, vertices[i+3]);
				}
				if(cellId < length) {
					auto rightCell = cells[cellId];
					builder.template link<Cell_to_Vertex>(rightCell, vertices[i+3]);
					builder.template link<Cell_to_Vertex>(rightCell, vertices[i+2]);
					builder.template link<Cell_to_Vertex>(rightCell, vertices[i+1]);
					builder.template link<Cell_to_Vertex>(rightCell, vertices[i+0]);
				}
			}

		}

		template<unsigned L, typename ... Properties>
		void addPropertyData(Mesh<L>& mesh, data::MeshProperties<L, typename Mesh<L>::partition_tree_type, Properties...>& properties) {

			// compute half the width of a cell on this layer
			value_t cell_width = (1 << level);

			// setup area surface and conductivity
			auto& faceArea = properties.template get<FaceArea,level>();
			auto& faceConductivity = properties.template get<FaceConductivity,level>();
			mesh.template pforAll<Face,level>([&](auto f) {
				faceArea[f] = 1;
				faceConductivity[f] = 0.2;
			});

			// node positions
			size_t numNodes = vertices.size();
			auto& vertexPositions = properties.template get<VertexPosition,level>();
			for(size_t i=0; i<numNodes; i+=4) {
				value_t x = (i / 4) * cell_width;
				vertexPositions[vertices[i+0]] = Point{ x , +0.5, +0.5 };
				vertexPositions[vertices[i+1]] = Point{ x , -0.5, +0.5 };
				vertexPositions[vertices[i+2]] = Point{ x , -0.5, -0.5 };
				vertexPositions[vertices[i+3]] = Point{ x , +0.5, -0.5 };
			}

		}
	};

	template<typename Builder, unsigned Layer>
	class TubeLayerBuilder : public TubeLayerBuilderBase<Builder,Layer> {

		using super = TubeLayerBuilderBase<Builder,Layer>;

		// the builder building the parent node level
		TubeLayerBuilder<Builder, Layer-1> subLayerBuilder;

	public:

		void assembleMesh(Builder& builder, int length) {

			// build this layer
			super::assembleMesh(builder, length >> (Layer-1));		// halve number of nodes each layer

			// build next lower layer
			subLayerBuilder.assembleMesh(builder, length);

			// link parent and child cells
			size_t numCells = subLayerBuilder.cells.size();
			for(size_t i=0; i<numCells; i++) {
				builder.template link<Parent_to_Child>(this->cells[i/2], subLayerBuilder.cells[i]);
			}
		}

		template<unsigned L, typename ... Properties>
		void addPropertyData(Mesh<L>& mesh, data::MeshProperties<L, typename Mesh<L>::partition_tree_type, Properties...>& properties) {

			// add the data of this layer
			super::template addPropertyData<L>(mesh, properties);

			// and of the parent layer
			subLayerBuilder.template addPropertyData<L>(mesh, properties);

		}

	};

	template<typename Builder>
	class TubeLayerBuilder<Builder,1> : public TubeLayerBuilderBase<Builder,1> {

	};


	//create mesh
	std::pair<Mesh<NUM_LEVELS>, MeshProperties<Mesh<NUM_LEVELS>>> createTube(const int N) {
		MeshBuilder<NUM_LEVELS> builder;
		TubeLayerBuilder<decltype(builder), NUM_LEVELS> tubeBuilder;

		// - assemble the mesh -
		tubeBuilder.assembleMesh(builder, N);

		// - create geometric information -
		auto mesh = std::move(builder).build();

		MeshProperties<decltype(mesh)> properties = mesh.template createProperties<
			CellTemperature,
			FaceArea,
			FaceConductivity,
			VertexPosition
		>();

		tubeBuilder.template addPropertyData<NUM_LEVELS>(mesh, properties);

		return std::make_pair(std::move(mesh), std::move(properties));
	}
}