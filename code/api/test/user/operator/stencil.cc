#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/user/operator/stencil.h"

namespace allscale {
namespace api {
namespace user {

	// --- basic parallel stencil usage ---

	TEST(Stencil,Basic) {

		const int N = 1000;

		// test for an even and an odd number of time steps
		for(int T : { 40 , 41 }) {

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

} // end namespace user
} // end namespace api
} // end namespace allscale
