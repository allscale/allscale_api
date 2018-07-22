#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/user/algorithm/stencil.h"
#include "allscale/api/user/data/grid.h"

#include "allscale/utils/string_utils.h"
#include "allscale/utils/bitmanipulation.h"

namespace allscale {
namespace api {
namespace user {
namespace algorithm {

	// --- basic parallel stencil usage ---

	template <typename T>
	class Stencil : public ::testing::Test {
	};

	TYPED_TEST_CASE_P(Stencil);

	TYPED_TEST_P(Stencil,Vector) {

		using Impl = TypeParam;

		const std::size_t N = 500;
		const int I = 10;

		// test for an even and an odd number of time steps
		for(std::size_t T : { (std::size_t)40 , (std::size_t)41 , (std::size_t)(2.5 * N) }) {

			// initialize the data buffer
			std::vector<int> data(N);
			for(int& x : data) x = I;

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, std::size_t pos, const std::vector<int>& data) {

				// check that input arrays are up-to-date
				if(pos > 0) { EXPECT_EQ(I + time, data[pos - 1]) << "Position: " << pos << " - 1 = " << (pos - 1); }
				EXPECT_EQ(I + time, data[pos]) << "Position: " << pos;
				if(pos < N - 1) { EXPECT_EQ(I + time, data[pos + 1]) << "Position: " << pos << " + 1 = " << (pos + 1); }

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			for(std::size_t i = 0; i<N; i++) {
				EXPECT_EQ(I+T,data[i]) << "Position " << i;
			}

		}

	}

	TYPED_TEST_P(Stencil,Grid1D) {

		using Impl = TypeParam;

		const int N = 500;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,1> data(N);
			
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, const data::GridPoint<1>& pos, const data::Grid<int,1>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					data::GridPoint<1> offset{dx};
					auto p = pos + offset;
					if (p[0] < 0 || p[0] >= N) continue;
					EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
				}

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}

	TYPED_TEST_P(Stencil,Grid2D) {

		using Impl = TypeParam;

		const int N = 50;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,2> data({N,N+10});
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, const data::GridPoint<2>& pos, const data::Grid<int,2>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						data::GridPoint<2> offset{dx,dy};
						auto p = pos + offset;
						if (p[0] < 0 || p[0] >= N) continue;
						if (p[1] < 0 || p[1] >= N) continue;
						EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
					}
				}

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}


	TYPED_TEST_P(Stencil,Grid3D) {

		using Impl = TypeParam;

		const int N = 20;

		// test for an even and an odd number of time steps
		for(int T : { 20 , 21 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,3> data({N,N+2,N+3});
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, const data::GridPoint<3>& pos, const data::Grid<int,3>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						for(int dz = -1; dz <= 1; ++dz) {
							data::GridPoint<3> offset{dx,dy,dz};
							auto p = pos + offset;
							if (p[0] < 0 || p[0] >= N) continue;
							if (p[1] < 0 || p[1] >= N) continue;
							if (p[2] < 0 || p[2] >= N) continue;
							EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
						}
					}
				}

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}


	TYPED_TEST_P(Stencil,Grid4D) {

		using Impl = TypeParam;

		const int N = 8;

		// test for an even and an odd number of time steps
		for(int T : { 20 , 21 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,4> data({N,N+1,N+2,N+3});
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, const data::GridPoint<4>& pos, const data::Grid<int,4>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						for(int dz = -1; dz <= 1; ++dz) {
							for(int dw = -1; dw <= 1; ++dw) {
								data::GridPoint<4> offset{dx,dy,dz,dw};
								auto p = pos + offset;
								if (p[0] < 0 || p[0] >= N) continue;
								if (p[1] < 0 || p[1] >= N) continue;
								if (p[2] < 0 || p[2] >= N) continue;
								if (p[3] < 0 || p[3] >= N) continue;
								EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
							}
						}
					}
				}

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}

	TYPED_TEST_P(Stencil,Grid5D) {

		using Impl = TypeParam;

		const int N = 4;

		// test for an even and an odd number of time steps
		for(int T : { 20 , 21 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,5> data({N,N+1,N+2,N+3,N+4});
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, const data::GridPoint<5>& pos, const data::Grid<int,5>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						for(int dz = -1; dz <= 1; ++dz) {
							for(int dw = -1; dw <= 1; ++dw) {
								for(int dv = -1; dv <= 1; ++dv) {
									data::GridPoint<5> p{pos[0] + dx, pos[1] + dy, pos[2] + dz, pos[3] + dw, pos[4] + dv};
									if (p[0] < 0 || p[0] >= N) continue;
									if (p[1] < 0 || p[1] >= N) continue;
									if (p[2] < 0 || p[2] >= N) continue;
									if (p[3] < 0 || p[3] >= N) continue;
									if (p[4] < 0 || p[4] >= N) continue;
									EXPECT_EQ(time,data[p]) << "Position = " << p;
								}
							}
						}
					}
				}

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}


	TYPED_TEST_P(Stencil,DefaultImpl) {

		const int N = 500;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			std::vector<int> data(N);
			for(int& x : data) x = 0;

			// run the stencil
			stencil(data, T, [=](time_t time, std::size_t pos, const std::vector<int>& data){

				// check that input arrays are up-to-date
				if(pos > 0) { EXPECT_EQ(time, data[pos - 1]); }
				EXPECT_EQ(time,data[pos]);
				if(pos < N - 1) { EXPECT_EQ(time, data[pos + 1]); }

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			for(const int x : data) {
				EXPECT_EQ(T,x);
			}

		}

	}

	TYPED_TEST_P(Stencil,Grid3D_Boundary) {

		using Impl = TypeParam;

		const int N = 20;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,3> data({N,N+2,N+3});
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T,

					// inner part
					[=](time_t time, const data::GridPoint<3>& pos, const data::Grid<int,3>& data){

						// check that this is an inner position
						EXPECT_FALSE(
								pos[0] == 0 || pos[0] == N-1 ||
								pos[1] == 0 || pos[1] == N+1 ||
								pos[2] == 0 || pos[2] == N+2
							) << "Position " << pos << " should not be a boundary position!";

						// check that input arrays are up-to-date
						for(int dx = -1; dx <= 1; ++dx) {
							for(int dy = -1; dy <= 1; ++dy) {
								for(int dz = -1; dz <= 1; ++dz) {
									data::GridPoint<3> offset{dx,dy,dz};
									auto p = pos + offset;
									if (p[0] < 0 || p[0] >= N) continue;
									if (p[1] < 0 || p[1] >= N+2) continue;
									if (p[2] < 0 || p[2] >= N+3) continue;
									EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
								}
							}
						}

						// increase the time step of current cell
						return data[pos] + 1;
					},

					// boundary update
					[=](time_t time, const data::GridPoint<3>& pos, const data::Grid<int,3>& data){

						// check that this is an inner position
						EXPECT_TRUE(
								pos[0] == 0 || pos[0] == N-1 ||
								pos[1] == 0 || pos[1] == N+1 ||
								pos[2] == 0 || pos[2] == N+2
							) << "Position " << pos << " should be a boundary position!";

						// check that input arrays are up-to-date
						for(int dx = -1; dx <= 1; ++dx) {
							for(int dy = -1; dy <= 1; ++dy) {
								for(int dz = -1; dz <= 1; ++dz) {
									data::GridPoint<3> offset{dx,dy,dz};
									auto p = pos + offset;
									if (p[0] < 0 || p[0] >= N) continue;
									if (p[1] < 0 || p[1] >= N+2) continue;
									if (p[2] < 0 || p[2] >= N+3) continue;
									EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
								}
							}
						}

						// increase the time step of current cell
						return data[pos] + 1;
					}
			);

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}


	TYPED_TEST_P(Stencil,Grid2D_Tuning) {

		using Impl = TypeParam;

		const int N = 20;

		// run one layer of iterations
		for(int T : { N/2 }) {

			// initialize the data buffer
			data::Grid<int,2> data({N,N});
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [=](time_t time, const data::GridPoint<2>& pos, const data::Grid<int,2>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						data::GridPoint<2> offset{dx,dy};
						auto p = pos + offset;
						if (p[0] < 0 || p[0] >= N) continue;
						if (p[1] < 0 || p[1] >= N) continue;
						EXPECT_EQ(time,data[p]) << "Position " << pos << " + " << offset << " = " << p;
					}
				}

				// increase the time step of current cell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}


	TYPED_TEST_P(Stencil,Grid2D_Observer) {

		using Impl = TypeParam;

		// only declared static to silence MSVC errors...
		static const int N = 100;

		// run one layer of iterations
		for(int T : { N/2 }) {

			// initialize the data buffer
			data::Grid<int,2> data({N,N});
			data.forEach([](int& x){
				x = 0;
			});

			// count the number of collected observations
			int observationCounterA = 0;
			int observationCounterB = 0;

			// run the stencil
			stencil<Impl>(data, T,
				[=](time_t, const data::GridPoint<2>& pos, const data::Grid<int,2>& data){
					// increase the time step of current cell
					return data[pos] + 1;
				},
				observer(
					[](time_t t) { return t % 10 == 0; },
					[](const data::GridPoint<2>& loc) { return loc.x == N/2 && loc.y == N/3; },
					[&observationCounterA](time_t t, const data::GridPoint<2>& loc, int& value) {
						EXPECT_EQ(0,t%10);
						EXPECT_EQ(N/2,loc.x);
						EXPECT_EQ(N/3,loc.y);
						EXPECT_EQ(t+1,value);

						EXPECT_EQ(observationCounterA * 10, t);
						observationCounterA++;
					}
				),
				observer(
					[](time_t t) { return t % 8 == 0; },
					[](const data::GridPoint<2>& loc) { return loc.x == N/4 && loc.y == N/2; },
					[&observationCounterB](time_t t, const data::GridPoint<2>& loc, int& value) {
						EXPECT_EQ(0,t%8);
						EXPECT_EQ(N/4,loc.x);
						EXPECT_EQ(N/2,loc.y);
						EXPECT_EQ(t+1,value);

						EXPECT_EQ(observationCounterB * 8, t);
						observationCounterB++;
					}
				)
			);

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

			// check number of collected observations
			EXPECT_EQ(5,observationCounterA);
			EXPECT_EQ(7,observationCounterB);
		}

	}


	// TODO:
	//  - return a treeture wrapper
	//    - how to link multiple instances
	//  - integrate observers?
	//  - integrate inputs through captures


	REGISTER_TYPED_TEST_CASE_P(
			Stencil,
		    Vector,
		    DefaultImpl,
		    Grid1D,
			Grid2D,
			Grid3D,
			Grid4D,
			Grid5D,
			Grid3D_Boundary,
			Grid2D_Tuning,
			Grid2D_Observer
	);

	using test_params = ::testing::Types<
			implementation::sequential_iterative,
			implementation::coarse_grained_iterative,
			implementation::fine_grained_iterative,
			implementation::sequential_recursive,
			implementation::parallel_recursive
		>;
	INSTANTIATE_TYPED_TEST_CASE_P(Test,Stencil,test_params);



	// -- recursive stencil related tests ----------------------------------------------


	TEST(Base,Basic) {

		using namespace implementation::detail;

		utils::Vector<int64_t,3> size = { 4, 5, 6 };
		Base<3> base = Base<3>::full(size);

		EXPECT_FALSE(base.empty());
		EXPECT_EQ(4*5*6,base.size());

		EXPECT_EQ("[0-4,0-5,0-6]",toString(base));

	}



	TEST(Zoid,Basic) {

		using namespace implementation::detail;

		utils::Vector<int64_t,3> size = { 4, 5, 6 };
		Base<3> base = Base<3>::full(size);
		Zoid<3> zoid(base,1,0,2);

		EXPECT_EQ("Zoid([0-4,0-5,0-6],[1,1,1],0-2)",toString(zoid));

	}

	TEST(ExecutionPlan,EvaluationOrder) {
		const bool debug = false;

		using namespace implementation::detail;

		auto cur = [=](std::size_t idx, const auto& ... deps) {
			// for debugging
			if (debug) std::cout << idx << " depends on " << std::array<std::size_t,sizeof...(deps)>({{deps...}}) << "\n";

			// expect number of dependencies to be equivalent to number of bits in index
			EXPECT_EQ(utils::countOnes((unsigned)idx),sizeof...(deps));

			// expect that each dependency is only 1 bit off
			for(const auto& cur : std::array<std::size_t,sizeof...(deps)>({{ deps ... }})) {
				// check that idx is a super-set of bits
				EXPECT_EQ(cur, idx & cur) << idx;
				// check that there is one bit different
				EXPECT_EQ(1,utils::countOnes((unsigned)(cur ^ idx)));
			}

		};

		ExecutionPlan<1>::enumTaskGraph(cur);
		if (debug) std::cout << "\n";
		ExecutionPlan<2>::enumTaskGraph(cur);
		if (debug) std::cout << "\n";
		ExecutionPlan<3>::enumTaskGraph(cur);
		if (debug) std::cout << "\n";
		ExecutionPlan<4>::enumTaskGraph(cur);
		if (debug) std::cout << "\n";
		ExecutionPlan<5>::enumTaskGraph(cur);

	}

} // end namespace algorithm
} // end namespace user
} // end namespace api
} // end namespace allscale
