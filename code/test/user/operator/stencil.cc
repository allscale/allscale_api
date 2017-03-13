#include <gtest/gtest.h>

#include <vector>

#include "allscale/utils/string_utils.h"
#include "allscale/api/user/operator/stencil.h"
#include "allscale/api/user/data/grid.h"

namespace allscale {
namespace api {
namespace user {

	// --- basic parallel stencil usage ---

	// TODO: type-parameterize this test over the implementations

	template <typename T>
	class Stencil : public ::testing::Test {
	};

	TYPED_TEST_CASE_P(Stencil);

	TYPED_TEST_P(Stencil,Basic) {

		using Impl = TypeParam;

		const int N = 1000;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			std::vector<int> data(N);
			for(int& x : data) x = 0;

			// run the stencil
			stencil<Impl>(data, T, [](int time, int pos, const std::vector<int>& data){

				// check that input arrays are up-to-date
				if (pos > 0) EXPECT_EQ(time,data[pos-1]);
				EXPECT_EQ(time,data[pos]);
				if (pos < N-1) EXPECT_EQ(time,data[pos+1]);

				// increase the time step of current sell
				return data[pos] + 1;
			});

			// check final state
			for(const int x : data) {
				EXPECT_EQ(T,x);
			}

		}

	}

	TYPED_TEST_P(Stencil,Grid1D) {

		using Impl = TypeParam;

		const int N = 1000;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,1> data(N);
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [](int time, const data::GridPoint<1>& pos, const data::Grid<int,1>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					auto p = pos + data::GridPoint<1>{dx};
					if (p[0] < 0 || p[0] >= N) continue;
					EXPECT_EQ(time,data[p]) << "Position " << p;
				}

				// increase the time step of current sell
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
			data::Grid<int,2> data(N);
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [](int time, const data::GridPoint<2>& pos, const data::Grid<int,2>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						auto p = pos + data::GridPoint<2>{dx,dy};
						if (p[0] < 0 || p[0] >= N) continue;
						if (p[1] < 0 || p[1] >= N) continue;
						EXPECT_EQ(time,data[p]) << "Position " << p << " - offset: " << dx << "/" << dy;
					}
				}

				// increase the time step of current sell
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
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,3> data(N);
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [](int time, const data::GridPoint<3>& pos, const data::Grid<int,3>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						for(int dz = -1; dz <= 1; ++dz) {
							auto p = pos + data::GridPoint<3>{dx,dy,dz};
							if (p[0] < 0 || p[0] >= N) continue;
							if (p[1] < 0 || p[1] >= N) continue;
							if (p[2] < 0 || p[2] >= N) continue;
							EXPECT_EQ(time,data[p]) << "Position " << p << " - offset: " << dx << "/" << dy << "/" << dz;
						}
					}
				}

				// increase the time step of current sell
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

		const int N = 10;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			data::Grid<int,4> data(N);
			data.forEach([](int& x){
				x = 0;
			});

			// run the stencil
			stencil<Impl>(data, T, [](int time, const data::GridPoint<4>& pos, const data::Grid<int,4>& data){

				// check that input arrays are up-to-date
				for(int dx = -1; dx <= 1; ++dx) {
					for(int dy = -1; dy <= 1; ++dy) {
						for(int dz = -1; dz <= 1; ++dz) {
							for(int dw = -1; dw <= 1; ++dw) {
								auto p = pos + data::GridPoint<4>{dx,dy,dz,dw};
								if (p[0] < 0 || p[0] >= N) continue;
								if (p[1] < 0 || p[1] >= N) continue;
								if (p[2] < 0 || p[2] >= N) continue;
								if (p[3] < 0 || p[3] >= N) continue;
								EXPECT_EQ(time,data[p]) << "Position " << p << " - offset: " << dx << "/" << dy << "/" << dz << "/" << dw;
							}
						}
					}
				}

				// increase the time step of current sell
				return data[pos] + 1;
			});

			// check final state
			data.forEach([T](int x){
				EXPECT_EQ(T,x);
			});

		}

	}

	TYPED_TEST_P(Stencil,DefaultImpl) {

		const int N = 1000;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 , (int)(2.5 * N) }) {

			// initialize the data buffer
			std::vector<int> data(N);
			for(int& x : data) x = 0;

			// run the stencil
			stencil(data, T, [](int time, int pos, const std::vector<int>& data){

				// check that input arrays are up-to-date
				if (pos > 0) EXPECT_EQ(time,data[pos-1]);
				EXPECT_EQ(time,data[pos]);
				if (pos < N-1) EXPECT_EQ(time,data[pos+1]);

				// increase the time step of current sell
				return data[pos] + 1;
			});

			// check final state
			for(const int x : data) {
				EXPECT_EQ(T,x);
			}

		}

	}



	// TODO:
	//  - generalize data structure
	//  - support boundary handling
	//  - return a treeture wrapper
	//    - how to link multiple instances
	//  - integrate observers?
	//  - integrate inputs through captures


	REGISTER_TYPED_TEST_CASE_P(
			Stencil,
		    Basic,
		    DefaultImpl,
		    Grid1D,
			Grid2D,
			Grid3D,
			Grid4D
	);

//	using test_params = ::testing::Types<iterative_stencil,recursive_stencil>;
	using test_params = ::testing::Types<
			implementation::coarse_grained_iterative,
			implementation::fine_grained_iterative
		>;
	INSTANTIATE_TYPED_TEST_CASE_P(Test,Stencil,test_params);



	// -- recursive stencil related tests ----------------------------------------------


	TEST(Base,Basic) {

		using namespace implementation::detail;

		data::Vector<int64_t,3> size = { 4, 5, 6 };
		Base<3> base = Base<3>::full(size);

		EXPECT_FALSE(base.empty());
		EXPECT_EQ(4*5*6,base.size());

		EXPECT_EQ("[0-4,0-5,0-6]",toString(base));

	}



	TEST(Zoid,Basic) {

		using namespace implementation::detail;

		data::Vector<int64_t,3> size = { 4, 5, 6 };
		Base<3> base = Base<3>::full(size);
		Zoid<3> zoid(base,1,0,2);

		EXPECT_EQ("Zoid([0-4,0-5,0-6],[1,1,1],0-2)",toString(zoid));

	}

} // end namespace user
} // end namespace api
} // end namespace allscale
