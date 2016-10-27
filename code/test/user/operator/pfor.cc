#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/user/operator/pfor.h"
#include "allscale/api/user/data/vector.h"

namespace allscale {
namespace api {
namespace user {

	// --- basic parallel loop usage ---


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
		pfor(0,N,[&](int i) {
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

		// check that non so far has been updated
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


//	TEST(PFor, Handler) {
//
//		const int N = 10;
//
//		// create data
//		std::vector<int> data(N);
//
//		// initialize data
//		auto As = pfor(data,[](int& x) {
//			x = 10;
//		});
//
//		// check state
//		for(const auto& cur : data) {
//			EXPECT_EQ(0,cur);
//		}
//
//		// wait for first half
//		As.getLeft().wait();
//
////		// check state
////		for(int i=0; i<N/2; i++) {
////			EXPECT_EQ(10,data[i]) << "i=" << i;
////		}
////		for(int i=N/2; i<N; i++) {
////			EXPECT_EQ(0,data[i]) << "i=" << i;
////		}
//
//		// wait for second half
//		As.getRight().wait();
//
//		// check state
//		for(const auto& cur : data) {
//			EXPECT_EQ(10,cur);
//		}
//
//	}

	// --- loop iteration sync ---

	TEST(DISABLED_Pfor, SyncOneOnOne) {
		const int N = 10;

		std::mutex outLock;
		auto log = [&](const std::string& str, int i) {
			std::lock_guard<std::mutex> lock(outLock);
			std::cerr << str << i << "\n";
		};

		std::vector<int> data(N);

		auto As = pfor(0,N,[&](int i) {
			log("A",i);
			data[i] = 0;
		});

		auto Bs = pfor(0,N,[&](int i) {
			log("B",i);
			EXPECT_EQ(0,data[i]);
			data[i] = 1;
		}, one_on_one(As));

		auto Cs = pfor(0,N,[&](int i) {
			log("C",i);
			EXPECT_EQ(1,data[i]);
			data[i] = 2;
		}, one_on_one(Bs));

		Cs.wait();

		for(int i=0; i<N; i++) {
			EXPECT_EQ(2, data[i]);
		}
	}

//	TEST(Pfor, SyncNeighbor) {
//		const int N = 20000;
//
//		std::vector<int> dataA(N);
//		std::vector<int> dataB(N);
//
//		auto As = pfor(0,N,[&](int i) {
//			dataA[i] = 1;
//		});
//
//		auto Bs = pfor(0,N,[&](int i) {
//
//			// the neighborhood of i has to be completed in A
//			if (i != 0) {
//				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
//			}
//
//			EXPECT_EQ(1,dataA[i])   << "Index: " << i;
//
//			if (i != N-1) {
//				EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;
//			}
//
//			dataB[i] = 2;
//		}, neighborhood_sync(As));
//
//		auto Cs = pfor(0,N,[&](int i) {
//
//			// the neighborhood of i has to be completed in B
//			if (i != 0) {
//				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
//			}
//
//			EXPECT_EQ(2,dataB[i])   << "Index: " << i;
//
//			if (i != N-1) {
//				EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;
//			}
//
//			dataA[i] = 3;
//		}, neighborhood_sync(Bs));
//
//		// nothing has to be started yet
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(0, dataA[i]);
//			EXPECT_EQ(0, dataB[i]);
//		}
//
//		// trigger execution
//		Cs.wait();
//
//		// check result
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(3, dataA[i]);
//			EXPECT_EQ(2, dataB[i]);
//		}
//	}

} // end namespace user
} // end namespace api
} // end namespace allscale
