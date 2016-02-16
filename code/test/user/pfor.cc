#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/user/pfor.h"
#include "allscale/api/user/data/vector.h"

namespace allscale {
namespace api {
namespace user {


	TEST(PFor,Basic) {
		const int N = 200;

		// -- initialize data --

		std::array<int,N> data;
		data.fill(0);

		// check that all are 0
		for(const auto& cur : data) {
			EXPECT_EQ(0,cur);
		}

		// -- direct execution --

		// increase all by 1
		pfor(0,N,[&](const int& i) {
			data[i]++;
		});

		// check that all have been updated
		for(const auto& cur : data) {
			EXPECT_EQ(1,cur);
		}


		// -- delayed execution --

		// increase all by 1
		auto As = pfor(0,N,[&](const int& i) {
			data[i]++;
		});

		// check that all have been updated
		for(const auto& cur : data) {
			EXPECT_EQ(1,cur);
		}

		// trigger execution
		As.wait();

		// check that all have been updated
		for(const auto& cur : data) {
			EXPECT_EQ(2,cur);
		}

	}

	TEST(PFor,Container) {
		const int N = 200;

		// create data
		std::vector<int> data(N);

		// initialize data
		pfor(data,[](int& x) {
			x = 10;
		});

		// check state
		for(const auto& cur : data) {
			EXPECT_EQ(10,cur);
		}

		auto As = pfor(data,[](int& x) { x = 20; });

		// check state
		for(const auto& cur : data) {
			EXPECT_EQ(10,cur);
		}

		As.wait();

		// check state
		for(const auto& cur : data) {
			EXPECT_EQ(20,cur);
		}

	}

	TEST(PFor, Vector) {

		const int N = 200;

		using Point = data::Vector<int,3>;
		using Grid = std::array<std::array<std::array<int,N>,N>,N>;

		Point zero = 0;
		Point full = N;

		// create data
		Grid* data = new Grid();

		// initialize the data
		for(int i=0; i<N; i++) {
			for(int j=0; j<N; j++) {
				for(int k=0; k<N; k++) {
					(*data)[i][j][k] = 5;
				}
			}
		}

		// update data in parallel
		pfor(zero,full,[&](const Point& p){
			(*data)[p[0]][p[1]][p[2]]++;
		});

		// check that all has been covered
		for(int i=0; i<N; i++) {
			for(int j=0; j<N; j++) {
				for(int k=0; k<N; k++) {
					EXPECT_EQ(6,(*data)[i][j][k]);
				}
			}
		}

		delete data;

	}

} // end namespace user
} // end namespace api
} // end namespace allscale
