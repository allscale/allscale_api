#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

#include "allscale/utils/string_utils.h"

#include "allscale/api/user/data/mesh.h"
#include "allscale/api/user/algorithm/vcycle.h"

namespace allscale {
namespace api {
namespace user {
namespace algorithm {

	#include "../data/bar_mesh.inl"


	template<
		typename Mesh,
		unsigned Level
	>
	struct TestOrderStage {

		template<typename NodeKind,typename ValueType>
		using attribute = typename Mesh::template mesh_data_type<NodeKind,ValueType,Level>;

		std::vector<std::string>* ops;

		TestOrderStage(const Mesh&) : ops(nullptr) {}

		void computeFineToCoarse() {
			ops->push_back("C-F2C-" + std::to_string(Level));
		}

		void computeCoarseToFine() {
			ops->push_back("C-C2F-" + std::to_string(Level));
		}


		void restrictFrom(const TestOrderStage<Mesh,Level-1>&) {
			ops->push_back("R-" + std::to_string(Level-1) + "-" + std::to_string(Level));
		}

		void prolongateTo(TestOrderStage<Mesh,Level-1>&) {
			ops->push_back("P-" + std::to_string(Level) + "-" + std::to_string(Level-1));
		}

	};

	TEST(VCycle,TestOrder) {

		const int N = 10;

		using vcycle_type = VCycle<
				TestOrderStage,
				BarMesh<3,10>
			>;

		// create a sample bar, 3 layers
		auto bar = createBarMesh<3,10>(N);

		// create vcycle instance
		vcycle_type vcycle(bar);

		// set up ops buffer
		std::vector<std::string> buffer;
		vcycle.forEachStage([&](unsigned, auto& body){
			body.ops = &buffer;
		});

		// run the cycle
		vcycle.run(2);

		// check the result
		EXPECT_EQ(
			"[C-F2C-0,R-0-1,C-F2C-1,R-1-2,C-F2C-2,P-2-1,C-C2F-1,P-1-0,C-C2F-0,"
		     "C-F2C-0,R-0-1,C-F2C-1,R-1-2,C-F2C-2,P-2-1,C-C2F-1,P-1-0,C-C2F-0]",
			toString(buffer)
		);

	}


	// --- basic vcycle usage ---


	template<
		typename Mesh,
		unsigned Level
	>
	struct TestStage {

		template<typename NodeKind,typename ValueType>
		using attribute = typename Mesh::template mesh_data_type<NodeKind,ValueType,Level>;

		// capture mesh
		const Mesh& mesh;

		// counts the number of updates per node
		attribute<Vertex,int> updateCounters;


		TestStage(const Mesh& mesh)
			: mesh(mesh),
			  updateCounters(mesh.template createNodeData<Vertex,int,Level>()) {

			mesh.template pforAll<Vertex,Level>([&](const auto& cur){
				updateCounters[cur] = 0;
			});

		}

		void computeFineToCoarse() {
			mesh.template pforAll<Vertex,Level>([&](const auto& cur) {
				updateCounters[cur]++;
			});
		}

		void computeCoarseToFine() {
			mesh.template pforAll<Vertex,Level>([&](const auto& cur) {
				updateCounters[cur]++;
			});
		}


		void restrictFrom(const TestStage<Mesh,Level-1>& childStage) {

			// reduction by taking average of children
			mesh.template pforAll<Vertex,Level>([&](auto cur){
				// check that all children have the same number of updates
				const auto& children = mesh.template getChildren<Refine>(cur);
				assert_false(children.empty());

				int numUpdates = childStage.updateCounters[children.front()];
				for(const auto& child : children) {
					EXPECT_EQ(numUpdates,childStage.updateCounters[child]);
				}

				EXPECT_GT(numUpdates,updateCounters[cur]);
				updateCounters[cur] = numUpdates;
			});

		}

		void prolongateTo(TestStage<Mesh,Level-1>& childStage) {

			// prolongation of results to children
			mesh.template pforAll<Vertex,Level>([&](auto cur){
				// check that all children have the same number of updates
				const auto& children = mesh.template getChildren<Refine>(cur);
				assert_false(children.empty());

				int numUpdates = childStage.updateCounters[children.front()];
				for(const auto& child : children) {
					EXPECT_EQ(numUpdates,childStage.updateCounters[child]);
				}

				EXPECT_GT(updateCounters[cur],numUpdates);

				// propagate update counters to child stages
				for(const auto& child : children) {
					childStage.updateCounters[child] = updateCounters[cur];
				}

			});
		}

	};

	TEST(VCycle,TestRun) {

		const int N = 1000;

		using vcycle_type = VCycle<
				TestStage,
				BarMesh<3,10>
			>;

		// create a sample bar, 3 layers
		auto bar = createBarMesh<3,10>(N);

		// create vcycle instance
		vcycle_type vcycle(bar);
		auto& counts = vcycle.getStageBody().updateCounters;

		// counters should all be initially 0
		for(std::size_t i=0; i<bar.getNumNodes<Vertex>(); ++i) {
			EXPECT_EQ(0,counts[data::NodeRef<Vertex>((data::node_index_t)i)]);
		}
		vcycle.run(10);

		// now each element should be updated 30x
		for(std::size_t i=0; i<bar.getNumNodes<Vertex>(); ++i) {
			EXPECT_EQ(50,counts[data::NodeRef<Vertex>((data::node_index_t)i)]);
		}

	}


	template<
		typename Mesh,
		unsigned Level
	>
	struct ExampleTemperatureStage {

		template<typename NodeKind,typename ValueType>
		using attribute = typename Mesh::template mesh_data_type<NodeKind,ValueType,Level>;

		// capture mesh
		const Mesh& mesh;

		// the temperature of vertexes
		attribute<Vertex,double> temperature;


		ExampleTemperatureStage(const Mesh& mesh)
			: mesh(mesh),
			  temperature(mesh.template createNodeData<Vertex,double,Level>()) {

			mesh.template pforAll<Vertex,Level>([&](const auto& cur){
				// init temperature
				temperature[cur] = 0.0;
			});

		}

		void computeFineToCoarse() {
			mesh.template pforAll<Vertex,Level>([&](const auto& cur) {

				double sum = 0;
				auto neighbors = mesh.template getSinks<Edge>(cur);

				for(const auto& x : neighbors) {
					sum += temperature[x];
				}

				double avg = sum / neighbors.size();

				temperature[cur] = temperature[cur] + ((avg - temperature[cur]) * 0.2);

			});

		}

		void computeCoarseToFine() {
			// nothing to do
		}

		void restrictFrom(const ExampleTemperatureStage<Mesh,Level-1>& childStage) {

			// reduction by taking average of children
			mesh.template pforAll<Vertex,Level>([&](auto cur){
				// compute the average temperature of children
				double newTemp = 0.0;
				const auto& children = mesh.template getChildren<Refine>(cur);
				assert_false(children.empty());
				for(const auto& e : children) {
					newTemp += childStage.temperature[e];
				}
				temperature[cur] = newTemp / children.size();
			});

		}

		void prolongateTo(ExampleTemperatureStage<Mesh,Level-1>& childStage) {

			// prolongation of results to children
			mesh.template pforAll<Vertex,Level>([&](auto cur){
				// compute the average temperature of children
				double sum = 0.0;
				const auto& children = mesh.template getChildren<Refine>(cur);
				assert_false(children.empty());
				for(const auto& e : children) {
					sum += childStage.temperature[e];
				}
				auto oldTemp = sum / children.size();

				// compute difference to new temperature
				double diff = temperature[cur] - oldTemp;

				// distribute difference to finer temperature grid
				for(auto s : children) {
					childStage.temperature[s] += diff;
				}

			});

		}

	};


	TEST(VCycle,TemperatureSimulation) {

		const int N = 100000;
		const int T = 10;

		using vcycle_type = VCycle<
				ExampleTemperatureStage,
				BarMesh<3,10>
			>;

		// create a sample bar, 3 layers
		auto bar = createBarMesh<3,10>(N);

		// create a v-cycle instance
		vcycle_type vcycle(bar);

		// set off a nuke in the center
		vcycle.getStageBody().temperature[data::NodeRef<Vertex>((N*4)/2)] = 10000;

		// run the diffusion simulation
		vcycle.run(T);
	}

	template<
		typename Mesh,
		unsigned Level
	>
	struct TestValueStage {
		int value;

		TestValueStage(const Mesh&) : value(42) {}

		void computeFineToCoarse() {}

		void computeCoarseToFine() {}

		void restrictFrom(const TestValueStage<Mesh,Level-1>&) {}

		void prolongateTo(TestValueStage<Mesh,Level-1>&) {}

	};

	TEST(VCycle,ForEachStage) {

		const int N = 10;

		using vcycle_type = VCycle<
				TestValueStage,
				BarMesh<5,10>
			>;

		// create a sample bar, 5 layers
		auto bar = createBarMesh<5,10>(N);

		// create vcycle instance
		vcycle_type vcycle(bar);

		// check if value was initialized correctly
		vcycle.forEachStage([&](unsigned level, auto& body){
			EXPECT_EQ(body.value, 42);
			body.value = level;
		});


		int i = 0;
		const_cast<const vcycle_type&>(vcycle).forEachStage([&](unsigned level, auto& body){
			i++;
			EXPECT_EQ(body.value, level);
		});

		EXPECT_EQ(i, 5);


	}

} // end namespace algorithm
} // end namespace user
} // end namespace api
} // end namespace allscale
