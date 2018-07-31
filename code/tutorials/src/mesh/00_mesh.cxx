#include <iostream>

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
struct Node {};

struct LeftBoundaryFace {};
struct RightBoundaryFace {};

// - connections -
struct Face_2_Cell_Left  : public data::edge<Face,Cell> {};
struct Face_2_Cell_Right : public data::edge<Face,Cell> {};

struct BoundaryFace_2_Cell_Left  : public data::edge<RightBoundaryFace,Cell> {};
struct BoundaryFace_2_Cell_Right : public data::edge<LeftBoundaryFace,Cell> {};
struct Cell_2_Face_Left		: public data::edge<Cell,Face> {};
struct Cell_2_Face_Right	: public data::edge<Cell,Face> {};

struct Node_2_Cell : public data::edge<Node,Cell> {};
struct Face_2_Node : public data::edge<Face,Node> {};

// - inter-layer connections -
struct Cell_2_Child : public data::hierarchy<Cell,Cell> {};
struct LeftBoundaryFace_2_Child : public data::hierarchy<LeftBoundaryFace,LeftBoundaryFace> {};
struct RightBoundaryFace_2_Child : public data::hierarchy<RightBoundaryFace,RightBoundaryFace> {};

// -- property data --
struct CellVolume : public data::mesh_property<Cell,value_t> {};
struct CellCenter : public data::mesh_property<Cell,Point> {};

struct FaceSurface : public data::mesh_property<Face,value_t> {};

struct LeftBoundaryFaceSurface : public data::mesh_property<LeftBoundaryFace,value_t> {};
struct LeftBoundaryFaceCenter  : public data::mesh_property<LeftBoundaryFace,Point> {};

struct RightBoundaryFaceSurface : public data::mesh_property<RightBoundaryFace,value_t> {};
struct RightBoundaryFaceCenter  : public data::mesh_property<RightBoundaryFace,Point> {};

struct NodePosition : public data::mesh_property<Node,Point> {};

// - define the mesh and builder -
template<unsigned levels = 1>
using MeshBuilder = data::MeshBuilder<
	data::nodes<
		Cell,
		Face,
		Node,
		LeftBoundaryFace,
		RightBoundaryFace
	>,
	data::edges<
		Face_2_Cell_Left,
		Face_2_Cell_Right,
		BoundaryFace_2_Cell_Left,
		BoundaryFace_2_Cell_Right,
		Node_2_Cell,
		Face_2_Node,
		Cell_2_Face_Left,
		Cell_2_Face_Right
	>,
	data::hierarchies<
		Cell_2_Child,
		LeftBoundaryFace_2_Child,
		RightBoundaryFace_2_Child
	>,
	levels
>;

// -- the type of a mesh --
template<unsigned levels = 1, unsigned PartitionDepth = 0>
using Mesh = typename MeshBuilder<levels>::template mesh_type<PartitionDepth>;

// -- type of the properties of a mesh --
template<typename Mesh>
using MeshProperties = data::MeshProperties<Mesh::levels, typename Mesh::partition_tree_type,
	CellVolume,
	CellCenter,
	FaceSurface,
	LeftBoundaryFaceSurface,
	LeftBoundaryFaceCenter,
	RightBoundaryFaceSurface,
	RightBoundaryFaceCenter,
	NodePosition>;

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
	bool print_temperature = false;

	// Cell data
	attribute<Cell, value_t> temperature;
	attribute<Cell, value_t> residual;
	attribute<Cell, value_t> timeStep;
	attribute<Cell, value_t> conductivity;
	attribute<Cell, value_t> static_temperature;
	attribute<Cell, value_t> oldSol;
	attribute<Cell, value_t> temperatureBuffer;

	attribute<Face,value_t> fluxes;
	attribute<LeftBoundaryFace, value_t> lbFaceConductivity;
	attribute<RightBoundaryFace, value_t> rbFaceConductivity;

	attribute<LeftBoundaryFace,value_t> left_boundary_temperature;
	attribute<RightBoundaryFace,value_t> right_boundary_temperature;

	TemperatureStage(const Mesh& mesh, MeshProperties<Mesh>& properties, bool printTemp = false)
		: mesh(mesh), properties(properties), print_temperature(printTemp)
		, temperature(mesh.template createNodeData<Cell, value_t, Level>())
		, residual(mesh.template createNodeData<Cell, value_t, Level>())
		, timeStep(mesh.template createNodeData<Cell, value_t, Level>())
		, conductivity(mesh.template createNodeData<Cell, value_t, Level>())
		, static_temperature(mesh.template createNodeData<Cell, value_t, Level>())
		, oldSol(mesh.template createNodeData<Cell, value_t, Level>())
		, temperatureBuffer(mesh.template createNodeData<Cell, value_t, Level>())
		, fluxes(mesh.template createNodeData<Face, value_t, Level>())
		, lbFaceConductivity(mesh.template createNodeData<LeftBoundaryFace, value_t, Level>())
		, rbFaceConductivity(mesh.template createNodeData<RightBoundaryFace, value_t, Level>())
		, left_boundary_temperature(mesh.template createNodeData<LeftBoundaryFace, value_t, Level>())
		, right_boundary_temperature(mesh.template createNodeData<RightBoundaryFace, value_t, Level>()) {

		// -- initialize attributes --

		// simulation parameters
		int TL = 10;		// < temperature on the left
		int TR = 30;		// < temperature on the right
		int T0 = 20;		// < initial temperature

		mesh.template pforAll<Cell, Level>([&](auto c) {
			static_temperature[c] = T0;
			temperature[c] = T0;
			oldSol[c] = T0;
		});


		mesh.template pforAll<Cell, Level>([&](auto c) { conductivity[c] = 0.2; });		// a simple constant for now
		mesh.template pforAll<LeftBoundaryFace, Level>([&](auto f) { lbFaceConductivity[f] = 0.2; });
		mesh.template pforAll<RightBoundaryFace, Level>([&](auto f) { rbFaceConductivity[f] = 0.2; });
		// fix temperature on left boundary
		mesh.template pforAll<LeftBoundaryFace, Level>([&](auto f) { left_boundary_temperature[f] = TL; });
		// fix temperature on right boundary
		mesh.template pforAll<RightBoundaryFace, Level>([&](auto f) { right_boundary_temperature[f] = TR; });
	}

	void computeFineToCoarse() {
		//actual computation
		const value_t CFL = 1;

		auto& cellVol = properties.template get<CellVolume, Level>();
		auto& cellCenter = properties.template get<CellCenter, Level>();
		auto& faceSurf = properties.template get<FaceSurface, Level>();
		auto& lbfaceSurf = properties.template get<LeftBoundaryFaceSurface, Level>();
		auto& lbfaceCenter = properties.template get<LeftBoundaryFaceCenter, Level>();
		auto& rbfaceSurf = properties.template get<RightBoundaryFaceSurface, Level>();
		auto& rbfaceCenter = properties.template get<RightBoundaryFaceCenter, Level>();

		bool firstStage = true;

		// run Runge-Kutta stage
		for(value_t coefficient : { 0.25, 0.333, 0.5, 1.0 }) {

			// reset residual to 0
			mesh.template pforAll<Cell, Level>([&](auto c){ residual[c] = 0; });

			// special treatment of first stage
			if (firstStage) {

				// time step computation

				// reset timeStep buffer
				mesh.template pforAll<Cell, Level>([&](auto c){ timeStep[c] = 0; });

				// internal faces
				mesh.template pforAll<Face, Level>([&](auto f){
					timeStep[mesh.template getNeighbor<Face_2_Cell_Left>(f)] += faceSurf[f] * faceSurf[f];
					timeStep[mesh.template getNeighbor<Face_2_Cell_Right>(f)] += faceSurf[f] * faceSurf[f];
				});

				// left boundary faces
				mesh.template forAll<LeftBoundaryFace, Level>([&](auto f){
					timeStep[mesh.template getNeighbor<BoundaryFace_2_Cell_Right>(f)] += lbfaceSurf[f] * lbfaceSurf[f];
				});

				// right boundary faces
				mesh.template forAll<RightBoundaryFace, Level>([&](auto f) {
					timeStep[mesh.template getNeighbor<BoundaryFace_2_Cell_Left>(f)] += rbfaceSurf[f] * rbfaceSurf[f];
				});

				// incorporate volume
				mesh.template pforAll<Cell, Level>([&](auto c){
					timeStep[c] *= conductivity[c] / cellVol[c];
					timeStep[c] = CFL / timeStep[c];
				});

				// save the reference solution
				mesh.template pforAll<Cell, Level>([&](auto c) { oldSol[c] = temperature[c]; });

				firstStage = false;
			}

			// calculation of the residual
			// - inner faces -
			mesh.template forAll<Face, Level>([&](auto f){ // parallelize by storing flux and writing to left and right in two separate, parallel loops
				auto left = mesh.template getNeighbor<Face_2_Cell_Left>(f);
				auto right = mesh.template getNeighbor<Face_2_Cell_Right>(f);

				value_t face_conductivity = 0.5 * (conductivity[left] + conductivity[right]);
				value_t distance = dist(cellCenter[left],cellCenter[right]);
				value_t gradTemperature = (temperature[right] - temperature[left]) / distance;
				value_t flux = face_conductivity * gradTemperature * faceSurf[f];

				fluxes[f] = flux;
			});

			mesh.template pforAll<Cell, Level>([&](auto c){
				auto subtractingFaces = mesh.template getNeighbors<Cell_2_Face_Left>(c);
				for(auto sf : subtractingFaces) {
					residual[c] -= fluxes[sf];
				}
				auto addingFaces = mesh.template getNeighbors<Cell_2_Face_Right>(c);
				for(auto af : addingFaces) {
					residual[c] += fluxes[af];
				}
			});


			// - boundary faces -

			// the left faces
			mesh.template pforAll<LeftBoundaryFace, Level>([&](auto f){ // parallelize as each face has only one cell to write to
				auto right = mesh.template getNeighbor<BoundaryFace_2_Cell_Right>(f);

				value_t face_conductivity = 0.5 * (conductivity[right] + lbFaceConductivity[f]);
				value_t distance = dist(cellCenter[right], lbfaceCenter[f]);
				value_t gradTemperature = (temperature[right] - left_boundary_temperature[f]) / distance;
				value_t flux = face_conductivity * gradTemperature * lbfaceSurf[f];

				residual[right] += flux;
			});

			// the right faces
			mesh.template pforAll<RightBoundaryFace, Level>([&](auto f){ // parallelize as each face has only one cell to write to
				auto left = mesh.template getNeighbor<BoundaryFace_2_Cell_Left>(f);

				value_t face_conductivity = 0.5 * (conductivity[left] + rbFaceConductivity[f]);
				value_t distance = dist(cellCenter[left], rbfaceCenter[f]);
				value_t gradTemperature = (right_boundary_temperature[f] - temperature[left]) / distance;
				value_t flux = face_conductivity * gradTemperature * rbfaceSurf[f];

				residual[left] -= flux;
			});

			// update of solution
			mesh.template pforAll<Cell, Level>([&](auto c) {
				temperature[c] = oldSol[c] - coefficient * timeStep[c] * residual[c];
			});

			// update boundary condition
			// -- no change since constant --

			// update conductivity if temperature dependent
			// -- no change since constant --

		} // end of stage

		if (print_temperature) {
			std::cout << Level << " fine to coarse -> ";
			mesh.template forAll<Cell, Level>([&](auto c){
				std::cout << temperature[c] << " ";
			});
			std::cout << std::endl << std::endl;
		}
	}

	void computeCoarseToFine() {
		if (print_temperature) {
			std::cout << Level << " corarse to fine -> ";
			mesh.template forAll<Cell, Level>([&](auto c){
				std::cout << temperature[c] << " ";
			});
			std::cout << std::endl << std::endl;
		}
	}

	void restrictFrom(TemperatureStage<Mesh,Level-1>& childStage) {
			mesh.template forAll<Cell, Level>([&](auto c) {
			temperature[c] = 0;

			auto children = mesh.template getChildren<Cell_2_Child>(c);

			int count = 0;

			for(auto& child: children) {
				temperature[c] += childStage.temperature[child];
				count++;
			}
			temperature[c] /= count;
			temperatureBuffer[c] = temperature[c];
		});

		if (print_temperature) {
			std::cout << Level << " restrict from -> ";
			mesh.template forAll<Cell, Level>([&](auto c){
				std::cout << temperature[c] << " ";
			});
			std::cout << std::endl << std::endl;
		}
	}

	void prolongateTo(TemperatureStage<Mesh,Level-1>& childStage) {
		mesh.template forAll<Cell, Level>([&](auto c) {
			auto children = mesh.template getChildren<Cell_2_Child>(c);

			value_t diff = temperature[c] - temperatureBuffer[c];

			for(auto& child: children) {
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
		using NodeRef = typename data::NodeRef<Node,level>;
		using LeftBoundaryFaceRef = typename data::NodeRef<LeftBoundaryFace,level>;
		using RightBoundaryFaceRef = typename data::NodeRef<RightBoundaryFace,level>;

		std::vector<CellRef> cells;
		std::vector<FaceRef> faces;
		std::vector<NodeRef> nodes;

		LeftBoundaryFaceRef lb;
		RightBoundaryFaceRef rb;

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
				builder.template link<Face_2_Cell_Left>(faces[i], cells[i]);
				builder.template link<Cell_2_Face_Left>(cells[i], faces[i]);
			}

			// link faces with cells upward
			for(int i=0; i<length-1; i++) {
				builder.template link<Face_2_Cell_Right>(faces[i], cells[i+1]);
				builder.template link<Cell_2_Face_Right>(cells[i+1], faces[i]);
			}

			// create and link boundary faces
			lb = builder.template create<LeftBoundaryFace,level>();
			builder.template link<BoundaryFace_2_Cell_Right>(lb,cells.front());

			rb = builder.template create<RightBoundaryFace,level>();
			builder.template link<BoundaryFace_2_Cell_Left>(rb,cells.back());


			// -- nodes --

			// create 4 nodes per face + 8 for the boundary faces
			int numNodes = (length-1)*4+8;
			for(int i=0; i<numNodes; i++) {
				nodes.push_back(builder.template create<Node,level>());
			}

			// link faces to nodes
			for(int i=4; i<numNodes-4; i+=4) {
				builder.template link<Face_2_Node>(faces[i/4-1], nodes[i]);
				builder.template link<Face_2_Node>(faces[i/4-1], nodes[i+1]);
				builder.template link<Face_2_Node>(faces[i/4-1], nodes[i+2]);
				builder.template link<Face_2_Node>(faces[i/4-1], nodes[i+3]);
			}

			// link nodes to cell centers
			for(int i=0; i<numNodes; i+=4) {
				auto cellId = i / 4;
				if (cellId > 0) {
					auto leftCell = cells[cellId - 1];
					builder.template link<Node_2_Cell>(nodes[i],   leftCell);
					builder.template link<Node_2_Cell>(nodes[i+1], leftCell);
					builder.template link<Node_2_Cell>(nodes[i+2], leftCell);
					builder.template link<Node_2_Cell>(nodes[i+3], leftCell);
				}
				if (cellId < length) {
					auto rightCell = cells[cellId];
					builder.template link<Node_2_Cell>(nodes[i+3], rightCell);
					builder.template link<Node_2_Cell>(nodes[i+2], rightCell);
					builder.template link<Node_2_Cell>(nodes[i+1], rightCell);
					builder.template link<Node_2_Cell>(nodes[i],   rightCell);
				}
			}

		}

		template<unsigned L, typename ... Properties>
		void addPropertyData(Mesh<L>& mesh, data::MeshProperties<L, typename Mesh<L>::partition_tree_type, Properties...>& properties) {

			// compute half the width of a cell on this layer
			value_t cell_width = (1 << level);

			// setup cell volume
			auto& cellVolume = properties.template get<CellVolume,level>();
			mesh.template pforAll<Cell,level>([&](auto c) { cellVolume[c] = 1; });			// all the same size

			// setup cell centers
			auto& cellCenter = properties.template get<CellCenter,level>();
			value_t posX = cell_width/2;
			for(auto c : cells) {
				cellCenter[c] = Point{ posX, 0, 0 };
				posX += cell_width;
			}

			// setup face surfaces
			auto& faceSurfaces = properties.template get<FaceSurface,level>();
			mesh.template pforAll<Face,level>([&](auto f) { faceSurfaces[f] = 1; });		// all the same size

			// create surfaces of left boundary faces
			auto& lbFaceSurfaces = properties.template get<LeftBoundaryFaceSurface,level>();
			mesh.template pforAll<LeftBoundaryFace,level>([&](auto f) { lbFaceSurfaces[f] = 1; });

			// left boundary center
			auto& lbFaceCenters = properties.template get<LeftBoundaryFaceCenter,level>();
			lbFaceCenters[lb] = Point{ cellCenter[cells.front()].x - (cell_width/2), 0, 0 };

			// create surfaces of right boundary faces
			auto& rbFaceSurfaces = properties.template get<RightBoundaryFaceSurface,level>();
			mesh.template pforAll<RightBoundaryFace,level>([&](auto f) { rbFaceSurfaces[f] = 1; });

			// right boundary center
			auto& rbFaceCenters = properties.template get<RightBoundaryFaceCenter,level>();
			rbFaceCenters[rb] = Point{ cellCenter[cells.back()].x + (cell_width/2), 0, 0 };

			// node positions
			size_t numNodes = nodes.size();
			auto& nodePositions = properties.template get<NodePosition,level>();
			for(size_t i=0; i<numNodes; i+=4) {
				value_t x = (i / 4) * cell_width;
				nodePositions[nodes[i+0]] = Point{ x , +0.5, +0.5 };
				nodePositions[nodes[i+1]] = Point{ x , -0.5, +0.5 };
				nodePositions[nodes[i+2]] = Point{ x , -0.5, -0.5 };
				nodePositions[nodes[i+3]] = Point{ x , +0.5, -0.5 };
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
				builder.template link<Cell_2_Child>(this->cells[i/2], subLayerBuilder.cells[i]);
			}

			// also link boundary cells
			builder.template link<LeftBoundaryFace_2_Child>(this->lb, subLayerBuilder.lb);
			builder.template link<RightBoundaryFace_2_Child>(this->rb, subLayerBuilder.rb);
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

		MeshProperties<decltype(mesh)> properties = mesh.template createProperties<CellVolume,
			CellCenter,
			FaceSurface,
			LeftBoundaryFaceSurface,
			LeftBoundaryFaceCenter,
			RightBoundaryFaceSurface,
			RightBoundaryFaceCenter,
			NodePosition
		>();

		tubeBuilder.template addPropertyData<NUM_LEVELS>(mesh, properties);

		return std::make_pair(std::move(mesh), std::move(properties));
	}
}