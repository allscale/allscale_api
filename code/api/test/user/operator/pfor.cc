#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/core/io.h"

#include "allscale/api/user/operator/pfor.h"

#include "allscale/utils/serializer.h"
#include "allscale/utils/string_utils.h"
#include "allscale/utils/vector.h"

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

		// trigger execution
		As.wait();

		// check that all have been updated
		for(const auto& cur : data) {
			EXPECT_EQ(2,cur);
		}

	}

	template<typename Iter>
	void testIntegral() {
		const int N = 100;

		std::vector<int> data(N);
		for(std::size_t i = 0; i<N; ++i) {
			data[i] = 0;
		}

		for(std::size_t i = 0; i<N; ++i) {
			EXPECT_EQ(0,data[i]);
		}

		pfor(Iter(0),Iter(100),[&](Iter i){
			data[i] = 1;
		});

		for(std::size_t i = 0; i<N; ++i) {
			EXPECT_EQ(1,data[i]);
		}
	}

	TEST(PFor,Integrals) {

		testIntegral<char>();
		testIntegral<short>();
		testIntegral<int>();
		testIntegral<long>();
		testIntegral<long long>();

		testIntegral<unsigned char>();
		testIntegral<unsigned short>();
		testIntegral<unsigned int>();
		testIntegral<unsigned long>();
		testIntegral<unsigned long long>();

		testIntegral<char16_t>();
		testIntegral<char32_t>();
		testIntegral<wchar_t>();

		testIntegral<int8_t>();
		testIntegral<int16_t>();
		testIntegral<int32_t>();
		testIntegral<int64_t>();

		testIntegral<uint8_t>();
		testIntegral<uint16_t>();
		testIntegral<uint32_t>();
		testIntegral<uint64_t>();

		testIntegral<std::size_t>();
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

		As.wait();

		// check state
		for(const auto& cur : data) {
			EXPECT_EQ(20,cur);
		}

	}


	TEST(PFor, Array) {

		const int N = 100;

		using Point = std::array<int,3>;
		using Grid = std::array<std::array<std::array<int,N>,N>,N>;

		Point zero = {{0,0,0}};
		Point full = {{N,N,N}};

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
					EXPECT_EQ(6,(*data)[i][j][k])
							<< "Position: " << i << "/" << j << "/" << k;
				}
			}
		}

		delete data;

	}


	TEST(PFor, Vector) {

		const int N = 100;

		using Point = utils::Vector<int,3>;
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
					EXPECT_EQ(6,(*data)[i][j][k])
							<< "Position: " << i << "/" << j << "/" << k;
				}
			}
		}

		delete data;

	}



	// --- loop iteration sync ---

	TEST(Pfor, SyncOneOnOne) {

		const int N = 10000;
		const bool enable_log = false;

//		const int N = 10;
//		const bool enable_log = true;

		std::mutex outLock;
		auto log = [&](const std::string& str, int i) {
			if (!enable_log) return;
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
			EXPECT_EQ(0,data[i]) << "Index: " << i;
			data[i] = 1;
		}, one_on_one(As));

		auto Cs = pfor(0,N,[&](int i) {
			log("C",i);
			EXPECT_EQ(1,data[i]) << "Index: " << i;
			data[i] = 2;
		}, one_on_one(Bs));

		Cs.wait();

		for(int i=0; i<N; i++) {
			EXPECT_EQ(2, data[i]) << "Index: " << i;
		}
	}


	TEST(Pfor, SyncOneOnOne_DifferentSize) {

		const int N = 10000;

		std::vector<int> data(N+20);

		auto As = pfor(0,N,[&](int i) {
			data[i] = 0;
		});

		// test a smaller one
		auto Bs = pfor(0,N-1,[&](int i) {
			EXPECT_EQ(0,data[i]) << "Index: " << i;
			data[i] = 1;
		}, one_on_one(As));

		// and an even smaller one
		auto Cs = pfor(0,N-2,[&](int i) {
			EXPECT_EQ(1,data[i]) << "Index: " << i;
			data[i] = 2;
		}, one_on_one(Bs));

		// and a bigger one
		auto Ds = pfor(0,N+20,[&](int i) {
			if(i < N - 2) { EXPECT_EQ(2, data[i]) << "Index: " << i; }
			else if(i < N - 1) { EXPECT_EQ(1, data[i]) << "Index: " << i; }
			else if(i < N) { EXPECT_EQ(0, data[i]) << "Index: " << i; }
			data[i] = 3;
		}, one_on_one(Cs));

		Ds.wait();

		for(int i=0; i<N+20; i++) {
			EXPECT_EQ(3, data[i]) << "Index: " << i;
		}
	}

	TEST(Pfor, SyncOneOnOne_2D) {

		const int N = 50;
		const int T = 10;

		using Point = utils::Vector<int,2>;

		Point size = {N,N};

		std::array<std::array<int,N>,N> bufferA;
		std::array<std::array<int,N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y] = 0;
			(*B)[p.x][p.y] = -1;
		});

		// run the time loop
		for(int t=0; t<T; ++t) {
			ref = pfor(Point{1,1},Point{N-1,N-1},[A,B,t](const Point& p) {

				EXPECT_EQ(t,(*A)[p.x][p.y]);
				EXPECT_EQ(t-1,(*B)[p.x][p.y]);

				(*B)[p.x][p.y]=t+1;

			},one_on_one(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(Point{1,1},Point{N-1,N-1},[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y]);
		},one_on_one(ref));

	}


	TEST(Pfor, SyncNeighbor) {
		const int N = 10000;

		std::vector<int> dataA(N);
		std::vector<int> dataB(N);

		auto As = pfor(0,N,[&](int i) {
			dataA[i] = 1;
		});

		auto Bs = pfor(0,N,[&](int i) {

			// the neighborhood of i has to be completed in A
			if (i != 0) {
				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(1,dataA[i])   << "Index: " << i;

			if (i != N-1) {
				EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;
			}

			dataB[i] = 2;
		}, neighborhood_sync(As));

		auto Cs = pfor(0,N,[&](int i) {

			// the neighborhood of i has to be completed in B
			if (i != 0) {
				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(2,dataB[i])   << "Index: " << i;

			if (i != N-1) {
				EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;
			}

			dataA[i] = 3;
		}, neighborhood_sync(Bs));

		// trigger execution
		Cs.wait();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(3, dataA[i]);
			EXPECT_EQ(2, dataB[i]);
		}
	}


	TEST(Pfor, SyncNeighbor_DifferentSize) {
		const int N = 10000;

		std::vector<int> dataA(N+20);
		std::vector<int> dataB(N+20);

		auto As = pfor(0,N,[&](int i) {
			dataA[i] = 1;
		});

		auto Bs = pfor(0,N-1,[&](int i) {

			// the neighborhood of i has to be completed in A
			if (i != 0) {
				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(1,dataA[i])   << "Index: " << i;

			EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;

			dataB[i] = 2;
		}, neighborhood_sync(As));

		auto Cs = pfor(0,N-2,[&](int i) {

			// the neighborhood of i has to be completed in B
			if (i != 0) {
				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(2,dataB[i])   << "Index: " << i;

			EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;

			dataA[i] = 3;
		}, neighborhood_sync(Bs));

		// also try a larger range
		auto Ds = pfor(0,N+20,[&](int i) {

			// the neighborhood of i has to be completed in A
			if (i != 0 && i <= N-2 ) {
				EXPECT_EQ(3,dataA[i-1]) << "Index: " << i;
			} else if ( i != 0 && i < N ) {
				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
			}

			if (i < N-2) {
				EXPECT_EQ(3,dataA[i])   << "Index: " << i;
			} else if (i < N) {
				EXPECT_EQ(1,dataA[i])   << "Index: " << i;
			}

			if (i != N-1 && i < N-3) {
				EXPECT_EQ(3,dataA[i+1]) << "Index: " << i;
			} else if (i != N-1 && i < N) {
				EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;
			}

			dataB[i] = 4;

		}, neighborhood_sync(Cs));

		// trigger execution
		Ds.wait();

		// check result
		for(int i=0; i<N-2; i++) {
			EXPECT_EQ(3, dataA[i]);
		}
		for(int i=N-2; i<N-1; i++) {
			EXPECT_EQ(1, dataA[i]);
		}
		for(int i=0; i<N+20; i++) {
			EXPECT_EQ(4, dataB[i]);
		}
	}


	TEST(Pfor, SyncNeighbor_2D) {

		const int N = 50;
		const int T = 10;

		using Point = utils::Vector<int,2>;

		Point size = {N,N};

		std::array<std::array<int,N>,N> bufferA;
		std::array<std::array<int,N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y] = 0;
			(*B)[p.x][p.y] = -1;
		});

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        ref = pfor(Point{1,1},Point{N-1,N-1},[A,B,t,N](const Point& p) {

	        	if (p.x != 1   && p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x-1][p.y-1]); }
	        	if (              p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x  ][p.y-1]); }
	        	if (p.x != N-2 && p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x+1][p.y-1]); }

	        	if (p.x != N-2 && p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x+1][p.y-1]); }
	        	if (p.x != N-2              )  { EXPECT_EQ(t,(*A)[p.x+1][p.y  ]); }
	        	if (p.x != N-2 && p.y != N-2)  { EXPECT_EQ(t,(*A)[p.x+1][p.y+1]); }

	        	if (p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x][p.y-1]); }
	        	if (p.y != N-2)  { EXPECT_EQ(t,(*A)[p.x][p.y+1]); }

	        	EXPECT_EQ(t,(*A)[p.x][p.y]);
	        	EXPECT_EQ(t-1,(*B)[p.x][p.y]);

	        	(*B)[p.x][p.y]=t+1;

	        },neighborhood_sync(ref));

	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(Point{1,1},Point{N-1,N-1},[T,A](const Point& p){
	    	EXPECT_EQ(T,(*A)[p.x][p.y]);
	    },neighborhood_sync(ref));

	}


	// --- stencil variants --.

	const int N = 10000;
	const int T = 100;

	TEST(Pfor, Stencil_Barrier) {

		int* A = new int[N];
		int* B = new int[N];

		// start with an initialization
		pfor(0,N,[A,B](int i){
			A[i] = 0;
			B[i] = -1;
		});

		// run the time loop
	    for(int t=0; t<T; ++t) {
			pfor(1, N - 1, [A, B, t](int i) {

				if(i != 1) { EXPECT_EQ(t, A[i - 1]); }
				EXPECT_EQ(t, A[i]);
				if(i != N - 2) { EXPECT_EQ(t, A[i + 1]); }

	        	EXPECT_EQ(t-1,B[i]);

	        	B[i]=t+1;

	        });
	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(1,N-1,[A](int i){
	    	EXPECT_EQ(T,A[i]);
	    });

	    delete [] A;
		delete [] B;
	}

	TEST(Pfor, Stencil_Fine_Grained) {

		int* A = new int[N];
		int* B = new int[N];

		// start with an initialization
		auto ref = pfor(0,N,[A,B](int i){
			A[i] = 0;
			B[i] = -1;
		});

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        ref = pfor(1,N-1,[A,B,t](int i) {

				if(i != 1) { EXPECT_EQ(t, A[i - 1]); }
	        	EXPECT_EQ(t,A[i]);
				if(i != N - 2) { EXPECT_EQ(t, A[i + 1]); }

	        	EXPECT_EQ(t-1,B[i]);

	        	B[i]=t+1;

	        },neighborhood_sync(ref));
	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(1,N-1,[A](int i){
	    	EXPECT_EQ(T,A[i]);
	    },neighborhood_sync(ref));

	    delete [] A;
	    delete [] B;
	}


	TEST(Range,Covers) {

		using range = detail::range<int>;

		auto covers = [](const range& a, const range& b) {
			return a.covers(b);
		};
		auto not_covers = [](const range& a, const range& b) {
			return !a.covers(b);
		};

		// sub-ranges
		EXPECT_PRED2(covers,range(2,5),range(2,2));
		EXPECT_PRED2(covers,range(2,5),range(2,3));
		EXPECT_PRED2(covers,range(2,5),range(2,4));
		EXPECT_PRED2(covers,range(2,5),range(2,5));

		EXPECT_PRED2(covers,range(2,5),range(2,5));
		EXPECT_PRED2(covers,range(2,5),range(3,5));
		EXPECT_PRED2(covers,range(2,5),range(4,5));
		EXPECT_PRED2(covers,range(2,5),range(5,5));

		// always cover empty ranges
		EXPECT_PRED2(covers,range(2,5),range(1,1));
		EXPECT_PRED2(covers,range(2,5),range(2,2));
		EXPECT_PRED2(covers,range(2,5),range(6,6));

		// negative cases
		EXPECT_PRED2(not_covers,range(2,5),range(2,6));
		EXPECT_PRED2(not_covers,range(2,5),range(1,2));
		EXPECT_PRED2(not_covers,range(2,5),range(1,3));
		EXPECT_PRED2(not_covers,range(2,5),range(3,6));
		EXPECT_PRED2(not_covers,range(2,5),range(8,9));

	}

	TEST(Range,Covers_2D) {

		using point = utils::Vector<int,2>;
		using range = detail::range<point>;

		auto covers = [](const range& a, const range& b) {
			return a.covers(b);
		};
		auto not_covers = [](const range& a, const range& b) {
			return !a.covers(b);
		};

		// equal range
		EXPECT_PRED2(covers,range({2,3},{5,6}),range({2,3},{5,6}));

		// sub-ranges
		EXPECT_PRED2(covers,range({2,3},{5,6}),range({2,3},{4,5}));
		EXPECT_PRED2(covers,range({2,3},{5,6}),range({2,3},{5,4}));

		// empty ranges
		EXPECT_PRED2(covers,range({2,3},{5,6}),range({2,3},{8,3}));
		EXPECT_PRED2(covers,range({2,3},{5,6}),range({2,3},{2,9}));

		EXPECT_PRED2(covers,range({2,3},{5,6}),range({1,3},{1,3}));
		EXPECT_PRED2(covers,range({2,3},{5,6}),range({2,9},{2,9}));

		// negative cases
		EXPECT_PRED2(not_covers,range({2,3},{5,6}),range({4,5},{8,9}));

		// once on each edge
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({1,6},{4,7}));
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({4,6},{6,7}));
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({3,4},{4,7}));
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({3,6},{6,9}));

		// once over each corner
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({1,4},{4,7}));
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({3,4},{6,7}));
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({3,4},{6,7}));
		EXPECT_PRED2(not_covers,range({2,5},{5,8}),range({3,6},{6,9}));

	}


	TEST(Range,GrowAndShrink) {

		using range = detail::range<int>;

		range limit(0,5);
		range a(1,2);

		EXPECT_EQ("[0,5)",toString(limit));
		EXPECT_EQ("[1,2)",toString(a));

		EXPECT_EQ("[0,3)",toString(a.grow(limit)));
		EXPECT_EQ("[0,4)",toString(a.grow(limit).grow(limit)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit).grow(limit).grow(limit)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit).grow(limit).grow(limit).grow(limit)));

		EXPECT_EQ("[0,4)",toString(a.grow(limit,2)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit,3)));
		EXPECT_EQ("[0,5)",toString(a.grow(limit,4)));



		EXPECT_EQ("[2,2)",toString(a.shrink()));
		EXPECT_EQ("[1,4)",toString(limit.shrink()));
		EXPECT_EQ("[2,3)",toString(limit.shrink().shrink()));
		EXPECT_EQ("[3,3)",toString(limit.shrink().shrink().shrink()));

		EXPECT_EQ("[2,3)",toString(limit.shrink(2)));
		EXPECT_EQ("[3,3)",toString(limit.shrink(3)));

	}

	TEST(Range,GrowAndShrink_2D) {

		using range = detail::range<std::array<int,2>>;

		range limit({{0,2}},{{5,7}});
		range a({{1,4}},{{2,5}});

		EXPECT_EQ("[[0,2],[5,7])",toString(limit));
		EXPECT_EQ("[[1,4],[2,5])",toString(a));

		EXPECT_EQ("[[0,3],[3,6])",toString(a.grow(limit)));
		EXPECT_EQ("[[0,2],[4,7])",toString(a.grow(limit).grow(limit)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit).grow(limit).grow(limit)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit).grow(limit).grow(limit).grow(limit)));

		EXPECT_EQ("[[0,2],[4,7])",toString(a.grow(limit,2)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit,3)));
		EXPECT_EQ("[[0,2],[5,7])",toString(a.grow(limit,4)));


		EXPECT_EQ("[[2,5],[2,5])",toString(a.shrink()));

		EXPECT_EQ("[[1,3],[4,6])",toString(limit.shrink()));
		EXPECT_EQ("[[2,4],[3,5])",toString(limit.shrink().shrink()));
		EXPECT_EQ("[[3,5],[3,5])",toString(limit.shrink().shrink().shrink()));
		EXPECT_EQ("[[4,6],[4,6])",toString(limit.shrink().shrink().shrink().shrink()));

		EXPECT_EQ("[[1,3],[4,6])",toString(limit.shrink()));
		EXPECT_EQ("[[2,4],[3,5])",toString(limit.shrink(2)));
		EXPECT_EQ("[[3,5],[3,5])",toString(limit.shrink(3)));
		EXPECT_EQ("[[4,6],[4,6])",toString(limit.shrink(4)));

	}

	TEST(Pfor, ParallelTextFile) {

		unsigned N = 1000;
		core::FileIOManager& manager = core::FileIOManager::getInstance();

		// generate output data
		core::Entry text = manager.createEntry("text.txt", core::Mode::Text);
		auto out = manager.openOutputStream(text);
		std::vector<int> toBeWritten;
		for(unsigned i = 0; i < N; ++i) {
			toBeWritten.push_back(i);
		}

		// write file
		user::pfor(toBeWritten, [&](auto c) {
			EXPECT_TRUE(out.atomic([c](auto& out) {
				out << c << " ";
			}));
		});

		manager.close(out);

		// read file
		auto in = manager.openInputStream(text);
		std::set<int> readFromFile;
		int x;
		for(unsigned i = 0; i < N; ++i) {
			EXPECT_TRUE(in >> x);

			readFromFile.insert(x);
		}

		// check data read from file
		for(int val : toBeWritten) {
			readFromFile.erase(val);
		}
		EXPECT_TRUE(readFromFile.empty());

		// nevermore
		EXPECT_FALSE(in >> x);

		manager.close(in);

		// check existence of file
		EXPECT_TRUE(manager.exists(text));

		// delete the file
	 	manager.remove(text);

		// check existence of file
		EXPECT_FALSE(manager.exists(text));

	}


	TEST(PForWithBoundary, Basic1D) {

		const int N = 100;

		std::atomic<int> countInner(0);
		std::atomic<int> countBoundary(0);

		// update data in parallel
		pforWithBoundary(0,N,
			// the inner case
			[&](int i){
				EXPECT_TRUE(0<i && i<N-1) << "Invalid i: " << i;
				countInner++;
			},
			// the boundary case
			[&](int i){
				EXPECT_TRUE(0==i || i==N-1) << "Invalid i: " << i;
				countBoundary++;
			}
		);

		EXPECT_EQ(98,countInner);
		EXPECT_EQ(2,countBoundary);

	}

	TEST(PForWithBoundary, Basic2D) {

		const int N = 100;

		using Point = utils::Vector<int,2>;

		Point zero = 0;
		Point full = N;


		std::atomic<int> countInner(0);
		std::atomic<int> countBoundary(0);

		// update data in parallel
		pforWithBoundary(zero,full,
			// the inner case
			[&](const Point& p){
				EXPECT_TRUE(0<p.x && p.x<N-1 && 0<p.y && p.y<N-1) << "Invalid p: " << p;
				countInner++;
			},
			// the boundary case
			[&](const Point& p){
				EXPECT_TRUE(0==p.x || p.x==N-1 || 0==p.y || p.y==N-1) << "Invalid p: " << p;
				countBoundary++;
			}
		);

		EXPECT_EQ(98*98,countInner);
		EXPECT_EQ(100*100 - 98*98,countBoundary);

	}


	TEST(PForWithBoundary, Basic3D) {

		const int N = 100;

		using Point = utils::Vector<int,3>;

		Point zero = 0;
		Point full = N;


		std::atomic<int> countInner(0);
		std::atomic<int> countBoundary(0);

		// update data in parallel
		pforWithBoundary(zero,full,
			// the inner case
			[&](const Point& p){
				EXPECT_TRUE(0<p.x && p.x<N-1 && 0<p.y && p.y<N-1 && 0<p.z && p.z<N-1) << "Invalid p: " << p;
				countInner++;
			},
			// the boundary case
			[&](const Point& p){
				EXPECT_TRUE(0==p.x || p.x==N-1 || 0==p.y || p.y==N-1 || 0==p.z || p.z==N-1) << "Invalid p: " << p;
				countBoundary++;
			}
		);

		EXPECT_EQ(98*98*98,countInner);
		EXPECT_EQ(100*100*100 - 98*98*98,countBoundary);

	}


	TEST(PForWithBoundary, SyncNeighbor) {
		const int N = 10000;

		std::vector<int> dataA(N);
		std::vector<int> dataB(N);

		auto As = pfor(0,N,[&](int i) {
			dataA[i] = 1;
		});

		auto Bs = pforWithBoundary(0,N,
			[&](int i) {

				// this is an inner node
				EXPECT_TRUE(0 < i && i < N-1) << "Invalid i: " << i;

				EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
				EXPECT_EQ(1,dataA[i])   << "Index: " << i;
				EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;

				dataB[i] = 2;
			},
			[&](int i) {

				// this is an boundary node
				EXPECT_TRUE(0 == i || i == N-1) << "Invalid i: " << i;

				// the neighborhood of i has to be completed in A
				if (i != 0) {
					EXPECT_EQ(1,dataA[i-1]) << "Index: " << i;
				}

				EXPECT_EQ(1,dataA[i])   << "Index: " << i;

				if (i != N-1) {
					EXPECT_EQ(1,dataA[i+1]) << "Index: " << i;
				}

				dataB[i] = 2;
			},
			neighborhood_sync(As)
		);

		auto Cs = pforWithBoundary(0,N,
			[&](int i) {

				// this is an inner node
				EXPECT_TRUE(0 < i && i < N-1) << "Invalid i: " << i;

				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
				EXPECT_EQ(2,dataB[i])   << "Index: " << i;
				EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;

				dataA[i] = 3;
			},
			[&](int i) {

				// this is an boundary node
				EXPECT_TRUE(0 == i || i == N-1) << "Invalid i: " << i;

				// the neighborhood of i has to be completed in A
				if (i != 0) {
					EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
				}

				EXPECT_EQ(2,dataB[i])   << "Index: " << i;

				if (i != N-1) {
					EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;
				}

				dataA[i] = 3;
			},
			neighborhood_sync(Bs)
		);

		// trigger execution
		Cs.wait();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(3, dataA[i]);
			EXPECT_EQ(2, dataB[i]);
		}
	}


	TEST(Pfor,After) {

		const int N = 10;

		int* A = new int[N];
		int* B = new int[N];

		// start with an initialization
		auto ref = pfor(0,N,[A,B](int i){
			A[i] = 0;
			B[i] = -1;
		});

		int counter = 0;

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        ref = pfor(1,N-1,[A,B,t,N](int i) {

				if(i != 1) { EXPECT_EQ(t, A[i - 1]); }
	        	EXPECT_EQ(t,A[i]);
				if(i != N - 2) { EXPECT_EQ(t, A[i + 1]); }

	        	EXPECT_EQ(t-1,B[i]);

	        	B[i]=t+1;

	        },neighborhood_sync(ref));

	        // plug in after
	        if (t % 2 == 0) {
	        	ref = after(ref,N/2,[B,t,&counter,N]{
	        		EXPECT_EQ(t+1,B[N/2]);
	        		counter++;
	        	});
	        }

	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(1,N-1,[A](int i){
	    	EXPECT_EQ(T,A[i]);
	    },neighborhood_sync(ref));

	    EXPECT_EQ(counter,T/2);

	    delete [] A;
	    delete [] B;

	}


	TEST(Pfor,After2D) {

		const int N = 10;

		using Point = utils::Vector<int,2>;

		Point size = {N,N};
		Point center = {N/2,N/2};

		std::array<std::array<int,N>,N> bufferA;
		std::array<std::array<int,N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y] = 0;
			(*B)[p.x][p.y] = -1;
		});

		int counter = 0;

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        ref = pfor(Point{1,1},Point{N-1,N-1},[A,B,t,N](const Point& p) {

	        	if (p.x != 1   && p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x-1][p.y-1]); }
	        	if (              p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x  ][p.y-1]); }
	        	if (p.x != N-2 && p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x+1][p.y-1]); }

	        	if (p.x != N-2 && p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x+1][p.y-1]); }
	        	if (p.x != N-2              )  { EXPECT_EQ(t,(*A)[p.x+1][p.y  ]); }
	        	if (p.x != N-2 && p.y != N-2)  { EXPECT_EQ(t,(*A)[p.x+1][p.y+1]); }

	        	if (p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x][p.y-1]); }
	        	if (p.y != N-2)  { EXPECT_EQ(t,(*A)[p.x][p.y+1]); }

	        	EXPECT_EQ(t,(*A)[p.x][p.y]);
	        	EXPECT_EQ(t-1,(*B)[p.x][p.y]);

	        	(*B)[p.x][p.y]=t+1;

	        },neighborhood_sync(ref));

	        // plug in after
	        if (t % 2 == 0) {
	        	ref = after(ref,center,[&,B,t]{
	        		EXPECT_EQ(t+1,(*B)[center.x][center.y]);
	        		counter++;
	        	});
	        }

	        std::swap(A,B);
	    }

	    // check the final state
	    pfor(Point{1,1},Point{N-1,N-1},[A](const Point& p){
	    	EXPECT_EQ(T,(*A)[p.x][p.y]);
	    },neighborhood_sync(ref));

	    EXPECT_EQ(counter,T/2);

	}


	TEST(Pfor,LazyLoopTest) {

		// check whether loops are really processed asynchronously

		const int N = 10;
		const int T = 5;
		const int X = N/2;

		int counter = 0;

		// start with an initialization
		auto ref = pfor(0,N,[&](int i){
			std::this_thread::sleep_for(std::chrono::seconds(1));
			if (i==X) counter++;
		});

		// run the time loop
	    for(int t=0; t<T; ++t) {
	        ref = pfor(1,N-1,[&](int i) {
	        	if (i==X) counter++;
	        },neighborhood_sync(ref));
	    }

	    // check the final state
	    ref = pfor(1,N-1,[&](int i){
	    	if (i==X) counter++;
	    },neighborhood_sync(ref));

	    // should not be done by now
	    EXPECT_EQ(0,counter);

	    // now wait for ref
	    ref.wait();

	    // now all the steps should be done
	    EXPECT_EQ(T+2,counter);

	}

	TEST(Pfor,OverlapTest_Barrier) {

		const int N = 100;
		const int T = 10;

		std::atomic<int> maxTime(0);
		std::atomic<bool> overlapDetected(false);

		for(int t=0; t<T; ++t) {
			pfor(0,N,[&,t](int i){

				// check whether the current time is less than the encountered max
				while(true) {
					int curMaxTime = maxTime.load();
					if (t < curMaxTime) {
						overlapDetected = true;
					}

					if (maxTime.compare_exchange_strong(curMaxTime,std::max(curMaxTime,t))) break;
				}

				// introduce some unbalanced delay
				std::this_thread::sleep_for(std::chrono::microseconds(i));
			});
		}

		// there should have been some overlap
		EXPECT_FALSE(overlapDetected.load());
	}

	TEST(Pfor,OverlapTest_OneOnOne) {

		const int N = 100;
		const int T = 10;

		detail::loop_reference<int> ref;

		std::atomic<int> maxTime(0);
		std::atomic<bool> overlapDetected(false);

		for(int t=0; t<T; ++t) {
			ref = pfor(0,N,[&,t](int i){

				// check whether the current time is less than the encountered max
				while(true) {
					int curMaxTime = maxTime.load();
					if (t < curMaxTime) {
						overlapDetected = true;
					}

					if (maxTime.compare_exchange_strong(curMaxTime,std::max(curMaxTime,t))) break;
				}

				// introduce some unbalanced delay
				std::this_thread::sleep_for(std::chrono::microseconds(i));
			},one_on_one(ref));
		}

		ref.wait();

		// there should have been some overlap
		EXPECT_TRUE(overlapDetected.load());
	}

	TEST(Pfor,OverlapTest_NeighborSync) {

		const int N = 100;
		const int T = 10;

		detail::loop_reference<int> ref;

		std::atomic<int> maxTime(0);
		std::atomic<bool> overlapDetected(false);

		for(int t=0; t<T; ++t) {
			ref = pfor(0,N,[&,t](int i){

				// check whether the current time is less than the encountered max
				while(true) {
					int curMaxTime = maxTime.load();
					if (t < curMaxTime) {
						overlapDetected = true;
					}

					if (maxTime.compare_exchange_strong(curMaxTime,std::max(curMaxTime,t))) break;
				}

				// introduce some unbalanced delay
				std::this_thread::sleep_for(std::chrono::microseconds(i));
			},neighborhood_sync(ref));
		}

		ref.wait();

		// there should have been some overlap
		EXPECT_TRUE(overlapDetected.load());
	}

} // end namespace user
} // end namespace api
} // end namespace allscale
