#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/user/data/mesh.h"
#include "allscale/api/user/operator/vcycle.h"

namespace allscale {
namespace api {
namespace user {

	#include "../data/bar_mesh.inl"

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

		void compute() {
			mesh.template pforAll<Vertex,Level>([&](const auto& cur) {
				updateCounters[cur]++;
			});

		}

		void reduce(const TestStage<Mesh,Level-1>& childStage) {

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

		void prolong(TestStage<Mesh,Level-1>& childStage) {

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
			EXPECT_EQ(0,counts[data::NodeRef<Vertex>(i)]);
		}
		vcycle.run(10);

		// now each element should be updated 30x
		for(std::size_t i=0; i<bar.getNumNodes<Vertex>(); ++i) {
			EXPECT_EQ(30,counts[data::NodeRef<Vertex>(i)]);
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

		void compute() {
			mesh.template pforAll<Vertex,Level>([&](const auto& cur) {

				double sum = 0;
				auto neighbors = mesh.template getNeighbors<Edge>(cur);

				for(const auto& x : neighbors) {
					sum += temperature[x];
				}

				double avg = sum / neighbors.size();

				temperature[cur] = temperature[cur] + ((avg - temperature[cur]) * 0.2);

			});

		}

		void reduce(const ExampleTemperatureStage<Mesh,Level-1>& childStage) {

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

		void prolong(ExampleTemperatureStage<Mesh,Level-1>& childStage) {

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

} // end namespace user
} // end namespace api
} // end namespace allscale
