#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/core/io.h"

#include "allscale/api/user/algorithm/pfor.h"

#include "allscale/utils/serializer.h"
#include "allscale/utils/string_utils.h"
#include "allscale/utils/vector.h"
#include "allscale/utils/printer/vectors.h"

namespace allscale {
namespace api {
namespace user {
namespace algorithm {

	// --- scanner tests ---

	TEST(Scanner,ScanOrder1D) {

		auto range = detail::range<int>(0,100);

		int last = -1;
		range.forEach([&](int x){
			EXPECT_EQ(last+1,x);
			last = x;
		});

		EXPECT_EQ(99,last);
	}

	namespace {

		template<typename T, std::size_t D>
		utils::Vector<T,D> inc(const utils::Vector<T,D>& in, const T& limit) {
			auto res = in;
			for(int i = D-1; i>=0; --i) {
				res[i]++;
				if (res[i] < limit) return res;
				res[i] = 0;
			}
			return res;
		}

	}


	TEST(Scanner,ScanOrder2D) {

		using Point = utils::Vector<int,2>;
		auto range = detail::range<Point>(Point(0,0),Point(100,100));

		Point last(0,-1);
		range.forEach([&](Point x){
			EXPECT_EQ(inc(last,100),x);
			last = x;
		});

		EXPECT_EQ(Point(99,99),last);
	}

	TEST(Scanner,ScanOrder3D) {

		using Point = utils::Vector<int,3>;
		auto range = detail::range<Point>(Point(0,0,0),Point(100,100,100));

		Point last(0,0,-1);
		range.forEach([&](Point x){
			EXPECT_EQ(inc(last,100),x);
			last = x;
		});

		EXPECT_EQ(Point(99,99,99),last);
	}


	// --- basic parallel loop usage ---


	TEST(Pfor,Basic) {
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

	TEST(Pfor,Integrals) {

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

	TEST(Pfor,Container) {
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


	TEST(Pfor, Array) {

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


	TEST(Pfor, Vector) {

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


	TEST(Pfor, SyncNoDependency_2D) {

		const int N = 50;
		const int T = 10;

		using Point = utils::Vector<int,2>;

		std::array<std::array<std::array<bool,N>,N>,T> updated;

		// start with an initialization
		for(int t=0; t<T; t++) {
			for(int i=0; i<N; i++) {
				for(int j=0; j<N; j++) {
					updated[t][i][j] = false;
				}
			}
		}

		// start with a completed loop
		auto ref = detail::loop_reference<Point>();

		// run the time loop
		Point min {0,0};
		Point max {N,N};

		std::vector<detail::iteration_reference<Point>> refs;

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[t,&updated](const Point& p) {

				// we can only check that we have not been here before
				EXPECT_FALSE(updated[t][p.x][p.y])
					<< "t=" << t << ", i=" << p.x << ", j=" << p.y;

				// but we record that we have been here
				updated[t][p.x][p.y] = true;

			},no_sync());

			refs.push_back(ref);
		}

		// wait for completion
		for(auto& ref : refs) {
			ref.wait();
		}

		// at this point everything should be done
		for(int t=0; t<T; t++) {
			for(int i=0; i<N; i++) {
				for(int j=0; j<N; j++) {
					EXPECT_TRUE(updated[t][i][j])
							<< "t=" << t << ", i=" << i << ", j=" << j;
				}
			}
		}
	}


	TEST(Pfor, SyncConjunction) {
		const int N = 10000;

		std::vector<int> dataA(N);
		std::vector<int> dataB(N);
		std::vector<int> dataC(N);

		// check 0-init
		for(int i=0; i<N; i++) {
			EXPECT_EQ(0,dataA[i]);
			EXPECT_EQ(0,dataB[i]);
			EXPECT_EQ(0,dataC[i]);
		}

		// start 3 parallel loops updating the vectors

		auto As = pfor(0,N,[&](int i) {
			dataA[i] = 1;
		});

		auto Bs = pfor(0,N,[&](int i) {
			dataB[i] = 1;
		});

		auto Cs = pfor(0,N,[&](int i) {
			dataC[i] = 1;
		});

		// start a third loop updating depending on all three previous loops
		auto Xs = pfor(0,N,[&](int i) {
				EXPECT_EQ(1,dataA[i]);
				EXPECT_EQ(1,dataB[i]);
				EXPECT_EQ(1,dataC[i]);
			}, sync_all(
				one_on_one(As),
				one_on_one(Bs),
				one_on_one(Cs)
			)
		);

		// also just on two of those
		auto Ys = pfor(0,N,[&](int i) {
				EXPECT_EQ(1,dataA[i]);
				EXPECT_EQ(1,dataC[i]);
			}, sync_all(
				one_on_one(As),
				one_on_one(Cs)
			)
		);

		// or just a single
		auto Zs = pfor(0,N,[&](int i) {
				EXPECT_EQ(1,dataC[i]);
			}, sync_all(
				one_on_one(Cs)
			)
		);

		// or even none
		auto Ws = pfor(0,N,[&](int) {
			}, sync_all(
			)
		);

		// also for mixed dependencies
		auto Vs = pfor(0,N,[&](int i) {
				EXPECT_EQ(1,dataA[i]);
				EXPECT_EQ(1,dataB[i]);
				if (i > 0)   { EXPECT_EQ(1,dataB[i-1]); }
				if (i < N-1) { EXPECT_EQ(1,dataB[i+1]); }
			}, sync_all(
				one_on_one(As),
				full_neighborhood_sync(Bs)
			)
		);

	}

	// a utility function to generate arbitrary loop references
	template<typename T>
	detail::range<T> make_range(const T& from, const T& to) {
		return detail::range<T>(from,to);
	}

	// a utility function to generate arbitrary loop references
	template<typename T>
	detail::loop_reference<T> make_loop_ref(const T& from, const T& to) {
		return detail::loop_reference<T>(make_range(from,to), core::done());
	}

	TEST(SyncOneOnOne, Explicit_1D) {

		auto full = make_loop_ref(0,100);

		// create an initial dependency
		auto dep = one_on_one(full);
		EXPECT_EQ("[0,100)",toString(dep.getCenterRange()));

		// test a clean split
		auto parts = dep.split(make_range(0,50),make_range(50,100));
		EXPECT_EQ("[0,50)",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[50,100)",toString(parts.right.getCenterRange()));

		// test an split that does not hit the center
		parts = dep.split(make_range(0,40),make_range(40,100));
		EXPECT_EQ("[0,50)",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[0,100)",toString(parts.right.getCenterRange()));

		// and in the other direction
		parts = dep.split(make_range(0,80),make_range(80,100));
		EXPECT_EQ("[0,100)",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[50,100)",toString(parts.right.getCenterRange()));

	}

	TEST(SyncOneOnOne, Explicit_2D) {

		using point = utils::Vector<int,2>;

		auto full = make_loop_ref(point{0,0},point{100,100});

		// create an initial dependency
		auto dep = one_on_one(full);
		EXPECT_EQ("[[0,0],[100,100])",toString(dep.getCenterRange()));

		// test a clean split
		auto parts = dep.split(make_range(point{0,0},point{50,100}),make_range(point{50,0},point{100,100}));
		EXPECT_EQ("[[0,0],[50,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[50,0],[100,100])",toString(parts.right.getCenterRange()));

		// test an split that does not hit the center
		parts = dep.split(make_range(point{0,0},point{40,100}),make_range(point{40,0},point{100,100}));
		EXPECT_EQ("[[0,0],[50,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[0,0],[100,100])",toString(parts.right.getCenterRange()));

		// and in the other direction
		parts = dep.split(make_range(point{0,0},point{80,100}),make_range(point{80,0},point{100,100}));
		EXPECT_EQ("[[0,0],[100,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[50,0],[100,100])",toString(parts.right.getCenterRange()));

		// test split along the wrong dimension
		parts = dep.split(make_range(point{0,0},point{100,50}),make_range(point{0,50},point{100,100}));
		EXPECT_EQ("[[0,0],[100,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[0,0],[100,100])",toString(parts.right.getCenterRange()));

	}

	TEST(SyncOneOnOne, Explicit_3D) {

		using point = utils::Vector<int,3>;

		auto full = make_loop_ref(point{0,0,0},point{100,100,100});

		// create an initial dependency
		auto dep = one_on_one(full);
		EXPECT_EQ("[[0,0,0],[100,100,100])",toString(dep.getCenterRange()));

		// test a clean split
		auto parts = dep.split(make_range(point{0,0,0},point{50,100,100}),make_range(point{50,0,0},point{100,100,100}));
		EXPECT_EQ("[[0,0,0],[50,100,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[50,0,0],[100,100,100])",toString(parts.right.getCenterRange()));

		// test an split that does not hit the center
		parts = dep.split(make_range(point{0,0,0},point{40,100,100}),make_range(point{40,0,0},point{100,100,100}));
		EXPECT_EQ("[[0,0,0],[50,100,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[0,0,0],[100,100,100])",toString(parts.right.getCenterRange()));

		// and in the other direction
		parts = dep.split(make_range(point{0,0,0},point{80,100,100}),make_range(point{80,0,0},point{100,100,100}));
		EXPECT_EQ("[[0,0,0],[100,100,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[50,0,0],[100,100,100])",toString(parts.right.getCenterRange()));

		// test split along the wrong dimension
		parts = dep.split(make_range(point{0,0,0},point{100,50,100}),make_range(point{0,50,0},point{100,100,100}));
		EXPECT_EQ("[[0,0,0],[100,100,100])",  toString(parts.left.getCenterRange()));
		EXPECT_EQ("[[0,0,0],[100,100,100])",toString(parts.right.getCenterRange()));

	}


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


	TEST(Pfor, SyncOneOnOne_2D_DifferentExtends) {

		const int N = 50;
		const int T = 10;

		using Point = utils::Vector<int,2>;

		Point size = {N,2*N};

		std::array<std::array<int,2*N>,N> bufferA;
		std::array<std::array<int,2*N>,N> bufferB;

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


	TEST(Pfor, SyncOneOnOne_3D) {

		const int N = 20;
		const int T = 10;

		using Point = utils::Vector<int,3>;

		Point size = {N,N,N};

		std::array<std::array<std::array<int,N>,N>,N> bufferA;
		std::array<std::array<std::array<int,N>,N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y][p.z] = 0;
			(*B)[p.x][p.y][p.z] = -1;
		});

		// run the time loop
		for(int t=0; t<T; ++t) {
			ref = pfor(Point{1,1,1},Point{N-1,N-1,N-1},[A,B,t](const Point& p) {

				EXPECT_EQ(t,(*A)[p.x][p.y][p.z]);
				EXPECT_EQ(t-1,(*B)[p.x][p.y][p.z]);

				(*B)[p.x][p.y][p.z]=t+1;

			},one_on_one(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(Point{1,1,1},Point{N-1,N-1,N-1},[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y][p.z]);
		},one_on_one(ref));

	}


	TEST(Pfor, SyncOneOnOne_3D_DifferentExtends) {

		const int N = 10;
		const int T = 10;

		using Point = utils::Vector<int,3>;

		Point size = {N,2*N,3*N};

		std::array<std::array<std::array<int,3*N>,2*N>,N> bufferA;
		std::array<std::array<std::array<int,3*N>,2*N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y][p.z] = 0;
			(*B)[p.x][p.y][p.z] = -1;
		});

		// run the time loop
		for(int t=0; t<T; ++t) {
			ref = pfor(Point{1,1,1},Point{N-1,2*N-1,3*N-1},[A,B,t](const Point& p) {

				EXPECT_EQ(t,(*A)[p.x][p.y][p.z]);
				EXPECT_EQ(t-1,(*B)[p.x][p.y][p.z]);

				(*B)[p.x][p.y][p.z]=t+1;

			},one_on_one(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(Point{1,1,1},Point{N-1,2*N-1,3*N-1},[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y][p.z]);
		},one_on_one(ref));

	}



	TEST(SyncSmallNeighborhood, Explicit_1D) {

		auto full = make_loop_ref(0,100);

		// create an initial dependency
		auto dep = small_neighborhood_sync(full);
		EXPECT_EQ("[[0,100)]",toString(dep.getRanges()));

		// test a clean split
		auto parts = dep.split(make_range(0,50),make_range(50,100));
		EXPECT_EQ("[[0,50),[50,100)]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[50,100),[0,50)]",toString(parts.right.getRanges()));

		// test an split that does not hit the center
		parts = dep.split(make_range(0,40),make_range(40,100));
		EXPECT_EQ("[[0,50),[50,100)]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,100)]",toString(parts.right.getRanges()));

		// and in the other direction
		parts = dep.split(make_range(0,80),make_range(80,100));
		EXPECT_EQ("[[0,100)]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[50,100),[0,50)]",toString(parts.right.getRanges()));

		// test a split outside the full range
		parts = dep.split(make_range(0,120),make_range(120,240));
		EXPECT_EQ("[[0,100)]",toString(parts.left.getRanges()));

		// TODO: this could be empty!
		EXPECT_EQ("[[0,100)]",toString(parts.right.getRanges()));

		// - split a second level -
		auto part = dep.split(make_range(0,50),make_range(50,100)).left;
		EXPECT_EQ("[[0,50),[50,100)]",toString(part.getRanges()));

		// split it even
		parts = part.split(make_range(0,25),make_range(25,50));
		EXPECT_EQ("[[0,25),[25,50)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[25,50),[0,25),[50,75)]",toString(parts.right.getRanges()));

		// split it odd
		parts = part.split(make_range(0,20),make_range(20,50));
		EXPECT_EQ("[[0,25),[25,50)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,50),[50,100)]",toString(parts.right.getRanges()));

		// split it odd in the other direction
		parts = part.split(make_range(0,40),make_range(40,50));
		EXPECT_EQ("[[0,50),[50,100)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[25,50),[0,25),[50,75)]",toString(parts.right.getRanges()));

		// - and a third level -
		part = part.split(make_range(0,25),make_range(25,50)).right;
		EXPECT_EQ("[[25,50),[0,25),[50,75)]",toString(part.getRanges()));

		// split evenly
		parts = part.split(make_range(25,37),make_range(37,50));
		EXPECT_EQ("[[25,37),[12,25),[37,50)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[37,50),[25,37),[50,62)]",toString(parts.right.getRanges()));

	}

	TEST(SyncSmallNeighborhood, Explicit_2D) {

		using point = utils::Vector<int,2>;

		auto full = make_loop_ref(point{0,0},point{100,100});

		// create an initial dependency
		auto dep = small_neighborhood_sync(full);
		EXPECT_EQ("[[[0,0],[100,100])]",toString(dep.getRanges()));

		// test a clean split
		auto parts = dep.split(make_range(point{0,0},point{50,100}),make_range(point{50,0},point{100,100}));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[[50,0],[100,100]),[[0,0],[50,100])]",toString(parts.right.getRanges()));

		// test an split that does not hit the center
		parts = dep.split(make_range(point{0,0},point{40,100}),make_range(point{40,0},point{100,100}));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]", toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[100,100])]",toString(parts.right.getRanges()));

		// and in the other direction
		parts = dep.split(make_range(point{0,0},point{80,100}),make_range(point{80,0},point{100,100}));
		EXPECT_EQ("[[[0,0],[100,100])]", toString(parts.left.getRanges()));
		EXPECT_EQ("[[[50,0],[100,100]),[[0,0],[50,100])]",toString(parts.right.getRanges()));

		// test split along the wrong dimension
		parts = dep.split(make_range(point{0,0},point{100,50}),make_range(point{0,50},point{100,100}));
		EXPECT_EQ("[[[0,0],[100,100])]", toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[100,100])]",toString(parts.right.getRanges()));


		// split another level
		auto part = dep.split(make_range(point{0,0},point{50,100}),make_range(point{50,0},point{100,100})).left;
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(part.getRanges()));

		// split at the right position
		parts = part.split(make_range(point{0,0},point{50,50}),make_range(point{0,50},point{50,100}));
		EXPECT_EQ("[[[0,0],[50,50]),[[50,0],[100,50]),[[0,50],[50,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,50],[50,100]),[[50,50],[100,100]),[[0,0],[50,50])]",toString(parts.right.getRanges()));

		parts = part.split(make_range(point{0,0},point{50,40}),make_range(point{0,40},point{50,100}));
		EXPECT_EQ("[[[0,0],[50,50]),[[50,0],[100,50]),[[0,50],[50,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(parts.right.getRanges()));

		parts = part.split(make_range(point{0,0},point{50,60}),make_range(point{0,60},point{50,100}));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,50],[50,100]),[[50,50],[100,100]),[[0,0],[50,50])]",toString(parts.right.getRanges()));

	}

	TEST(SyncSmallNeighborhood, Explicit_3D) {

		using point = utils::Vector<int,3>;

		auto full = make_loop_ref(point{0,0,0},point{100,100,100});

		// create an initial dependency
		auto dep = small_neighborhood_sync(full);
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(dep.getRanges()));

		// test a clean split
		auto parts = dep.split(make_range(point{0,0,0},point{50,100,100}),make_range(point{50,0,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[50,100,100]),[[50,0,0],[100,100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[50,0,0],[100,100,100]),[[0,0,0],[50,100,100])]",toString(parts.right.getRanges()));

		// test an split that does not hit the center
		parts = dep.split(make_range(point{0,0,0},point{40,100,100}),make_range(point{40,0,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[50,100,100]),[[50,0,0],[100,100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(parts.right.getRanges()));

		// and in the other direction
		parts = dep.split(make_range(point{0,0,0},point{80,100,100}),make_range(point{80,0,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[[50,0,0],[100,100,100]),[[0,0,0],[50,100,100])]",toString(parts.right.getRanges()));

		// test split along the wrong dimension
		parts = dep.split(make_range(point{0,0,0},point{100,50,100}),make_range(point{0,50,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(parts.right.getRanges()));

	}

	template<typename I, std::size_t Dims, std::size_t width>
	void testExhaustive(const small_neighborhood_sync_dependency<utils::Vector<I,Dims>, width>& dependency, const detail::range<utils::Vector<I,Dims>>& full, const detail::range<utils::Vector<I,Dims>>& range, std::size_t depth) {

		// check that the current range is covered by the given dependency
		auto coverage = dependency.getRanges();

		// a utility to test coverage
		auto isCovered = [&](const auto& p) {
			// what is not in the full range is always covered
			if (!full.covers(p)) return true;
			// if it is covered by full, it must be covered by some dependency
			for(const auto& cur : coverage) {
				if (cur.covers(p)) return true;
			}
			// not covered, we are done
			return false;
		};

		// check the currently covered range
		range.forEach([&](const auto& p){

			EXPECT_PRED1(isCovered,p) << "Point " << p << " in range " << range << " at depth " << depth << " not covered by " << coverage << "\n";

			// check left and right neighbor in each dimension
			for(std::size_t i=0; i<Dims; ++i) {
				auto s = p;
				for(std::size_t j = 1; j<=width; j++) {
					s[i] = p[i] - (int)j;
					EXPECT_PRED1(isCovered,s) << "Point " << s << " in hull of range " << range << " at depth " << depth << " not covered by " << coverage << "\n";
					s[i] = p[i] + (int)j;
					EXPECT_PRED1(isCovered,s) << "Point " << s << " in hull of range " << range << " at depth " << depth << " not covered by " << coverage << "\n";
				}
			}
		});


		// TODO: narrow down dependency size
//		// also check that the coverage is not too large
//		std::size_t center_size = range.size();
//		std::size_t sum = 0;
//		for(const auto& cur : coverage) {
//			sum += cur.size();
//		}
//		EXPECT_LE(sum, 5 * center_size)
//			<< "Large dependency for range " << range << ": " << coverage << "\n";


		// simulate loop descent
		if (range.size() <= 1) return;

		// process fragments
		auto parts = detail::range_spliter<utils::Vector<I,Dims>>::split(depth,range);
		auto deps = dependency.split(parts.left,parts.right);
		testExhaustive(deps.left,full,parts.left,depth+1);
		testExhaustive(deps.right,full,parts.right,depth+1);

	}

	template<typename I, typename Dependency>
	void testExhaustive(const Dependency& dependency, const detail::range<I>& range) {
		testExhaustive(dependency,dependency.getCenterRange(),range,0);
	}

	template<typename width>
	class SyncSmallNeighborhood : public ::testing::Test {};

	TYPED_TEST_CASE_P(SyncSmallNeighborhood);

	TYPED_TEST_P(SyncSmallNeighborhood, Exhaustive_1D) {

		using point = utils::Vector<int,1>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{0},point{100})						// < the simulated loop
		);

		// check a loop of different size
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{0},point{101})						// < the simulated loop
		);

		// check an offsetted loop
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{10},point{110})						// < the simulated loop
		);

		// check a completely independent range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{200},point{400})						// < the simulated loop
		);

	}


	TYPED_TEST_P(SyncSmallNeighborhood, Exhaustive_2D) {

		using point = utils::Vector<int,2>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{50,50})						// < the simulated loop
		);

		// check a loop of different size
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{51,51})						// < the simulated loop
		);

		// check an offsetted loop
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{10,10},point{60,60})					// < the simulated loop
		);

		// check a completely independent range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{100,100},point{200,200})					// < the simulated loop
		);

		// check non-quadratic range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{20,80})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{20,80})						// < the simulated loop
		);

		// check non-quadratic range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{20,80})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{21,81})						// < the simulated loop
		);

		// check non-quadratic range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0},point{20,80})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{80,20})						// < the simulated loop
		);
	}


	TYPED_TEST_P(SyncSmallNeighborhood, Exhaustive_3D) {

		using point = utils::Vector<int,3>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{30,30,30})						// < the simulated loop
		);

		// check a slightly larger loop than dependency
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{31,31,31})						// < the simulated loop
		);

		// check a slightly smaller loop than dependency
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{29,29,29})						// < the simulated loop
		);

		// check an offseted loop
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{10,10,10},point{40,40,40})					// < the simulated loop
		);

		// test a completely different range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{50,50,50},point{60,60,60})					// < the simulated loop
		);

		// test a non-cube shape
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{20,30,40})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{20,30,40})						// < the simulated loop
		);

		// and a non-cube shape with small differences
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{20,30,40})),	// < the loop range depending on
				detail::range<point>(point{1,2,3},point{21,32,43})						// < the simulated loop
		);
	}


	TYPED_TEST_P(SyncSmallNeighborhood, Exhaustive_4D) {

		using point = utils::Vector<int,4>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{0,0,0,0},point{5,8,10,12})					// < the simulated loop
		);

		// check a slightly larger loop than dependency
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{0,0,0,0},point{6,9,11,13})					// < the simulated loop
		);

		// check a slightly smaller loop than dependency
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{0,0,0,0},point{4,7,9,11})					// < the simulated loop
		);

		// check an offseted loop
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{2,2,2,2},point{7,10,12,14})					// < the simulated loop
		);

		// test a completely different range
		testExhaustive(
				small_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{12,4,8,9},point{15,7,11,12})					// < the simulated loop
		);

	}

	REGISTER_TYPED_TEST_CASE_P(
			SyncSmallNeighborhood,
			Exhaustive_1D,
			Exhaustive_2D,
			Exhaustive_3D,
			Exhaustive_4D
	);

	using WidthRange = ::testing::Types<
			std::integral_constant<std::size_t,0>,
			std::integral_constant<std::size_t,1>,
			std::integral_constant<std::size_t,2>,
			std::integral_constant<std::size_t,3>
		>;
	INSTANTIATE_TYPED_TEST_CASE_P(Parameterized, SyncSmallNeighborhood, WidthRange);

	TEST(Pfor, SyncSmallNeighborhood) {
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
		}, small_neighborhood_sync(As));

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
		}, small_neighborhood_sync(Bs));

		// trigger execution
		Cs.wait();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(3, dataA[i]);
			EXPECT_EQ(2, dataB[i]);
		}
	}

	TEST(Pfor, SyncSmallNeighborhood_DifferentSize) {
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
		}, small_neighborhood_sync(As));

		auto Cs = pfor(0,N-2,[&](int i) {

			// the neighborhood of i has to be completed in B
			if (i != 0) {
				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(2,dataB[i])   << "Index: " << i;

			EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;

			dataA[i] = 3;
		}, small_neighborhood_sync(Bs));

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

		}, small_neighborhood_sync(Cs));

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

	TEST(Pfor, SyncSmallNeighborhood_2D) {

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
		Point min {1,1};
		Point max {N-1,N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check small neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						if (abs(i) + abs(j) <= 1) {
							Point r = p + Point{i,j};
							if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
								EXPECT_EQ(t,(*A)[r.x][r.y]) << "Point: " << r;
							}
						}
					}
				}

				(*B)[p.x][p.y]=t+1;

			},small_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y]);
		},small_neighborhood_sync(ref));

	}

	TEST(Pfor, SyncSmallNeighborhood_2D_DifferentExtends) {

		const int N = 30;
		const int T = 10;

		using Point = utils::Vector<int,2>;

		Point size = {N,2*N};

		std::array<std::array<int,2*N>,N> bufferA;
		std::array<std::array<int,2*N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y] = 0;
			(*B)[p.x][p.y] = -1;
		});

		// run the time loop
		Point min {1,1};
		Point max {N-1,2*N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check small neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						if (abs(i) + abs(j) <= 1) {
							Point d { i, j };
							Point r = p + d;
							if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
								EXPECT_EQ(t,(*A)[r.x][r.y]) << "Point: " << p << " + " << d << " = " << r;
							}
						}
					}
				}

				(*B)[p.x][p.y]=t+1;

			},small_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y]);
		},small_neighborhood_sync(ref));

	}

	TEST(Pfor, SyncSmallNeighborhood_3D) {

		const int N = 20;
		const int T = 10;

		using Point = utils::Vector<int,3>;

		Point size = {N,N,N};

		std::array<std::array<std::array<int,N>,N>,N> bufferA;
		std::array<std::array<std::array<int,N>,N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y][p.z] = 0;
			(*B)[p.x][p.y][p.z] = -1;
		});

		// run the time loop
		Point min {1,1,1};
		Point max {N-1,N-1,N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check small neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						for (int k : { -1, 0, 1 }) {
							if (abs(i) + abs(j) + abs(k) <= 1) {
								Point r = p + Point{i,j,k};
								if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
									EXPECT_EQ(t,(*A)[r.x][r.y][r.z]) << "Point: " << r;
								}
							}
						}
					}
				}

				(*B)[p.x][p.y][p.z]=t+1;

			},small_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y][p.z]);
		},small_neighborhood_sync(ref));

	}

	TEST(Pfor, SyncSmallNeighborhood_3D_DifferentExtends) {

		const int N = 10;
		const int T = 10;

		using Point = utils::Vector<int,3>;

		Point size = {N,2*N,3*N};

		std::array<std::array<std::array<int,3*N>,2*N>,N> bufferA;
		std::array<std::array<std::array<int,3*N>,2*N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y][p.z] = 0;
			(*B)[p.x][p.y][p.z] = -1;
		});

		// run the time loop
		Point min {1,1,1};
		Point max {N-1,2*N-1,3*N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check small neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						for (int k : { -1, 0, 1 }) {
							if (abs(i) + abs(j) + abs(k) <= 1) {
								Point r = p + Point{i,j,k};
								if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
									EXPECT_EQ(t,(*A)[r.x][r.y][r.z]) << "Point: " << r;
								}
							}
						}
					}
				}

				(*B)[p.x][p.y][p.z]=t+1;

			},small_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y][p.z]);
		},small_neighborhood_sync(ref));

	}



	TEST(SyncFullNeighborhood, Explicit_1D) {

		auto full = make_loop_ref(0,100);

		// create an initial dependency
		auto dep = full_neighborhood_sync(full);
		EXPECT_EQ("[[0,100)]",toString(dep.getRanges()));

		// test a clean split
		auto parts = dep.split(make_range(0,50),make_range(50,100));
		EXPECT_EQ("[[0,50),[50,100)]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,50),[50,100)]",toString(parts.right.getRanges()));

		// test an split that does not hit the center
		parts = dep.split(make_range(0,40),make_range(40,100));
		EXPECT_EQ("[[0,50),[50,100)]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,100)]",toString(parts.right.getRanges()));

		// and in the other direction
		parts = dep.split(make_range(0,80),make_range(80,100));
		EXPECT_EQ("[[0,100)]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,50),[50,100)]",toString(parts.right.getRanges()));

		// test a split outside the full range
		parts = dep.split(make_range(0,120),make_range(120,240));
		EXPECT_EQ("[[0,100)]",toString(parts.left.getRanges()));

		// TODO: this could be empty!
		EXPECT_EQ("[[0,100)]",toString(parts.right.getRanges()));

		// - split a second level -
		auto part = dep.split(make_range(0,50),make_range(50,100)).left;
		EXPECT_EQ("[[0,50),[50,100)]",toString(part.getRanges()));

		// split it even
		parts = part.split(make_range(0,25),make_range(25,50));
		EXPECT_EQ("[[0,25),[25,50)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,25),[25,50),[50,75)]",toString(parts.right.getRanges()));

		parts = dep.split(make_range(0,50),make_range(50,100)).right.split(make_range(50,75),make_range(75,100));
		EXPECT_EQ("[[25,50),[50,75),[75,100)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[50,75),[75,100)]",toString(parts.right.getRanges()));

		// split it odd
		parts = part.split(make_range(0,20),make_range(20,50));
		EXPECT_EQ("[[0,25),[25,50)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,50),[50,100)]",toString(parts.right.getRanges()));

		// split it odd in the other direction
		parts = part.split(make_range(0,40),make_range(40,50));
		EXPECT_EQ("[[0,50),[50,100)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[0,25),[25,50),[50,75)]",toString(parts.right.getRanges()));

		// - and a third level -
		part = part.split(make_range(0,25),make_range(25,50)).right;
		EXPECT_EQ("[[0,25),[25,50),[50,75)]",toString(part.getRanges()));

		// split evenly
		parts = part.split(make_range(25,37),make_range(37,50));
		EXPECT_EQ("[[12,25),[25,37),[37,50)]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[25,37),[37,50),[50,62)]",toString(parts.right.getRanges()));

	}

	TEST(SyncFullNeighborhood, Explicit_2D) {

		using point = utils::Vector<int,2>;

		auto full = make_loop_ref(point{0,0},point{100,100});

		// create an initial dependency
		auto dep = full_neighborhood_sync(full);
		EXPECT_EQ("[[[0,0],[100,100])]",toString(dep.getRanges()));

		// test a clean split
		auto parts = dep.split(make_range(point{0,0},point{50,100}),make_range(point{50,0},point{100,100}));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(parts.right.getRanges()));

		// test an split that does not hit the center
		parts = dep.split(make_range(point{0,0},point{40,100}),make_range(point{40,0},point{100,100}));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]", toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[100,100])]",toString(parts.right.getRanges()));

		// and in the other direction
		parts = dep.split(make_range(point{0,0},point{80,100}),make_range(point{80,0},point{100,100}));
		EXPECT_EQ("[[[0,0],[100,100])]", toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(parts.right.getRanges()));

		// test split along the wrong dimension
		parts = dep.split(make_range(point{0,0},point{100,50}),make_range(point{0,50},point{100,100}));
		EXPECT_EQ("[[[0,0],[100,100])]", toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[100,100])]",toString(parts.right.getRanges()));


		// split another level
		auto part = dep.split(make_range(point{0,0},point{50,100}),make_range(point{50,0},point{100,100})).left;
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(part.getRanges()));

		// split at the right position
		parts = part.split(make_range(point{0,0},point{50,50}),make_range(point{0,50},point{50,100}));
		EXPECT_EQ("[[[0,0],[50,50]),[[50,0],[100,50]),[[0,50],[50,100]),[[50,50],[100,100])]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[50,50]),[[50,0],[100,50]),[[0,50],[50,100]),[[50,50],[100,100])]",toString(parts.right.getRanges()));

		parts = part.split(make_range(point{0,0},point{50,40}),make_range(point{0,40},point{50,100}));
		EXPECT_EQ("[[[0,0],[50,50]),[[50,0],[100,50]),[[0,50],[50,100]),[[50,50],[100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",toString(parts.right.getRanges()));

		parts = part.split(make_range(point{0,0},point{50,60}),make_range(point{0,60},point{50,100}));
		EXPECT_EQ("[[[0,0],[50,100]),[[50,0],[100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0],[50,50]),[[50,0],[100,50]),[[0,50],[50,100]),[[50,50],[100,100])]",toString(parts.right.getRanges()));

	}

	TEST(SyncFullNeighborhood, Explicit_3D) {

		using point = utils::Vector<int,3>;

		auto full = make_loop_ref(point{0,0,0},point{100,100,100});

		// create an initial dependency
		auto dep = full_neighborhood_sync(full);
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(dep.getRanges()));

		// test a clean split
		auto parts = dep.split(make_range(point{0,0,0},point{50,100,100}),make_range(point{50,0,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[50,100,100]),[[50,0,0],[100,100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0,0],[50,100,100]),[[50,0,0],[100,100,100])]",toString(parts.right.getRanges()));

		// test an split that does not hit the center
		parts = dep.split(make_range(point{0,0,0},point{40,100,100}),make_range(point{40,0,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[50,100,100]),[[50,0,0],[100,100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(parts.right.getRanges()));

		// and in the other direction
		parts = dep.split(make_range(point{0,0,0},point{80,100,100}),make_range(point{80,0,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0,0],[50,100,100]),[[50,0,0],[100,100,100])]",toString(parts.right.getRanges()));

		// test split along the wrong dimension
		parts = dep.split(make_range(point{0,0,0},point{100,50,100}),make_range(point{0,50,0},point{100,100,100}));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",  toString(parts.left.getRanges()));
		EXPECT_EQ("[[[0,0,0],[100,100,100])]",toString(parts.right.getRanges()));

	}

	template<typename I,std::size_t radius>
	void testExhaustive(const full_neighborhood_sync_dependency<I,radius>& dependency, const detail::range<I>& full, const detail::range<I>& range, std::size_t depth) {

		// check that the current range is covered by the given dependency
		auto coverage = dependency.getRanges();

		// skip checks if current range is empty
		if (range.empty()) return;

		// check the currently covered range
		range.grow(full,radius).forEach([&](const auto& p){
			bool covered = false;
			for(const auto& cur : coverage) {
				covered = covered || cur.covers(p);
			}
			EXPECT_TRUE(covered) << "Point " << p << " in range " << range << " at depth " << depth << " not covered by " << coverage << "\n";
		});

		// simulate loop descent
		if (range.size() <= 1) return;

		// process fragments
		auto parts = detail::range_spliter<I>::split(depth,range);
		auto deps = dependency.split(parts.left,parts.right);
		testExhaustive(deps.left,full,parts.left,depth+1);
		testExhaustive(deps.right,full,parts.right,depth+1);

	}

	template<typename width>
	class SyncFullNeighborhood : public ::testing::Test {};

	TYPED_TEST_CASE_P(SyncFullNeighborhood);


	TYPED_TEST_P(SyncFullNeighborhood, Exhaustive_1D) {

		using point = utils::Vector<int,1>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{0},point{100})						// < the simulated loop
		);

		// check a loop of different size
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{0},point{101})						// < the simulated loop
		);

		// check an offsetted loop
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{10},point{110})						// < the simulated loop
		);

		// check a completely independent range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0},point{100})),	// < the loop range depending on
				detail::range<point>(point{200},point{400})						// < the simulated loop
		);

	}


	TYPED_TEST_P(SyncFullNeighborhood, Exhaustive_2D) {

		using point = utils::Vector<int,2>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{50,50})						// < the simulated loop
		);

		// check a loop of different size
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{51,51})						// < the simulated loop
		);

		// check an offset loop
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{10,10},point{60,60})					// < the simulated loop
		);

		// check a completely independent range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{50,50})),	// < the loop range depending on
				detail::range<point>(point{100,100},point{200,200})					// < the simulated loop
		);

		// check non-quadratic range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{20,80})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{20,80})						// < the simulated loop
		);

		// check non-quadratic range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{20,80})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{21,81})						// < the simulated loop
		);

		// check non-quadratic range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0},point{20,80})),	// < the loop range depending on
				detail::range<point>(point{0,0},point{80,20})						// < the simulated loop
		);
	}


	TYPED_TEST_P(SyncFullNeighborhood, Exhaustive_3D) {

		using point = utils::Vector<int,3>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{30,30,30})						// < the simulated loop
		);

		// check a slightly larger loop than dependency
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{31,31,31})						// < the simulated loop
		);

		// check a slightly smaller loop than dependency
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{29,29,29})						// < the simulated loop
		);

		// check an offseted loop
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{10,10,10},point{40,40,40})					// < the simulated loop
		);

		// test a completely different range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{30,30,30})),	// < the loop range depending on
				detail::range<point>(point{50,50,50},point{60,60,60})					// < the simulated loop
		);

		// test a non-cube shape
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{20,30,40})),	// < the loop range depending on
				detail::range<point>(point{0,0,0},point{20,30,40})						// < the simulated loop
		);

		// and a non-cube shape with small differences
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0},point{20,30,40})),	// < the loop range depending on
				detail::range<point>(point{1,2,3},point{21,32,43})						// < the simulated loop
		);
	}


	TYPED_TEST_P(SyncFullNeighborhood, Exhaustive_4D) {

		using point = utils::Vector<int,4>;
		enum { width = TypeParam::value };

		// check a dependency / loop combination exhaustive
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{0,0,0,0},point{5,8,10,12})					// < the simulated loop
		);

		// check a slightly larger loop than dependency
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{0,0,0,0},point{6,9,11,13})					// < the simulated loop
		);

		// check a slightly smaller loop than dependency
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{0,0,0,0},point{4,7,9,11})					// < the simulated loop
		);

		// check an offseted loop
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{2,2,2,2},point{7,10,12,14})					// < the simulated loop
		);

		// test a completely different range
		testExhaustive(
				full_neighborhood_sync<width>(make_loop_ref(point{0,0,0,0},point{5,8,10,12})),	// < the loop range depending on
				detail::range<point>(point{12,4,8,9},point{15,7,11,12})					// < the simulated loop
		);

	}


	REGISTER_TYPED_TEST_CASE_P(
			SyncFullNeighborhood,
			Exhaustive_1D,
			Exhaustive_2D,
			Exhaustive_3D,
			Exhaustive_4D
	);

	INSTANTIATE_TYPED_TEST_CASE_P(Parameterized, SyncFullNeighborhood, WidthRange);


	TEST(Pfor, SyncFullNeighbor) {
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
		}, full_neighborhood_sync(As));

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
		}, full_neighborhood_sync(Bs));

		// trigger execution
		Cs.wait();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(3, dataA[i]);
			EXPECT_EQ(2, dataB[i]);
		}
	}


	TEST(Pfor, SyncFullNeighborhood_DifferentSize) {
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
		}, full_neighborhood_sync(As));

		auto Cs = pfor(0,N-2,[&](int i) {

			// the neighborhood of i has to be completed in B
			if (i != 0) {
				EXPECT_EQ(2,dataB[i-1]) << "Index: " << i;
			}

			EXPECT_EQ(2,dataB[i])   << "Index: " << i;

			EXPECT_EQ(2,dataB[i+1]) << "Index: " << i;

			dataA[i] = 3;
		}, full_neighborhood_sync(Bs));

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

		}, full_neighborhood_sync(Cs));

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


	TEST(Pfor, SyncFullNeighborhood_2D) {

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
		Point min {1,1};
		Point max {N-1,N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check full neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						Point r = p + Point{i,j};
						if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
							EXPECT_EQ(t,(*A)[r.x][r.y]) << "Point: " << r;
						}
					}
				}

				(*B)[p.x][p.y]=t+1;

			},full_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y]);
		},full_neighborhood_sync(ref));

	}

	TEST(Pfor, SyncFullNeighborhood_2D_DifferentExtends) {

		const int N = 30;
		const int T = 10;

		using Point = utils::Vector<int,2>;

		Point size = {N,2*N};

		std::array<std::array<int,2*N>,N> bufferA;
		std::array<std::array<int,2*N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y] = 0;
			(*B)[p.x][p.y] = -1;
		});

		// run the time loop
		Point min {1,1};
		Point max {N-1,2*N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check full neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						Point r = p + Point{i,j};
						if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
							EXPECT_EQ(t,(*A)[r.x][r.y]) << "Point: " << r;
						}
					}
				}

				(*B)[p.x][p.y]=t+1;

			},full_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y]);
		},full_neighborhood_sync(ref));

	}


	TEST(Pfor, SyncFullNeighborhood_3D) {

		const int N = 20;
		const int T = 10;

		using Point = utils::Vector<int,3>;

		Point size = {N,N,N};

		std::array<std::array<std::array<int,N>,N>,N> bufferA;
		std::array<std::array<std::array<int,N>,N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y][p.z] = 0;
			(*B)[p.x][p.y][p.z] = -1;
		});

		// run the time loop
		Point min {1,1,1};
		Point max {N-1,N-1,N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check full neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						for (int k : { -1, 0, 1 }) {
							Point r = p + Point{i,j,k};
							if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
								EXPECT_EQ(t,(*A)[r.x][r.y][r.z]) << "Point: " << r;
							}
						}
					}
				}

				(*B)[p.x][p.y][p.z]=t+1;

			},full_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y][p.z]);
		},full_neighborhood_sync(ref));

	}

	TEST(Pfor, SyncFullNeighborhood_3D_DifferentExtends) {

		const int N = 10;
		const int T = 10;

		using Point = utils::Vector<int,3>;

		Point size = {N,2*N,3*N};

		std::array<std::array<std::array<int,3*N>,2*N>,N> bufferA;
		std::array<std::array<std::array<int,3*N>,2*N>,N> bufferB;

		auto* A = &bufferA;
		auto* B = &bufferB;

		// start with an initialization
		auto ref = pfor(size,[A,B](const Point& p){
			(*A)[p.x][p.y][p.z] = 0;
			(*B)[p.x][p.y][p.z] = -1;
		});

		// run the time loop
		Point min {1,1,1};
		Point max {N-1,2*N-1,3*N-1};

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,min,max](const Point& p) {

				// check full neighborhood
				for(int i : { -1, 0, 1 }) {
					for (int j : { -1, 0, 1 }) {
						for (int k : { -1, 0, 1 }) {
							Point r = p + Point{i,j,k};
							if (min.dominatedBy(r) && r.strictlyDominatedBy(max)) {
								EXPECT_EQ(t,(*A)[r.x][r.y][r.z]) << "Point: " << r;
							}
						}
					}
				}

				(*B)[p.x][p.y][p.z]=t+1;

			},full_neighborhood_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y][p.z]);
		},full_neighborhood_sync(ref));

	}


	TEST(Pfor, SyncAfterAll_2D) {

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
		Point min {0,0};
		Point max {N,N};

		detail::range<Point> r(min,max);

		for(int t=0; t<T; ++t) {
			ref = pfor(min,max,[A,B,t,&r](const Point& p) {

				// check full neighborhood
				r.forEach([&](const Point& s) {
					EXPECT_EQ(t,(*A)[s.x][s.y]) << "Point: " << r << " - error for " << s;
				});

				(*B)[p.x][p.y]=t+1;

			},after_all_sync(ref));

			std::swap(A,B);
		}

		// check the final state
		pfor(min,max,[T,A](const Point& p){
			EXPECT_EQ(T,(*A)[p.x][p.y]);
		},after_all_sync(ref));

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

			},small_neighborhood_sync(ref));
			std::swap(A,B);
		}

		// check the final state
		pfor(1,N-1,[A](int i){
			EXPECT_EQ(T,A[i]);
		},small_neighborhood_sync(ref));

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
		pfor(toBeWritten, [&](auto c) {
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


	TEST(PforWithBoundary, Basic1D) {

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

	TEST(PforWithBoundary, Basic2D) {

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


	TEST(PforWithBoundary, Basic3D) {

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


	TEST(PforWithBoundary, SyncFullNeighborhood) {
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
			small_neighborhood_sync(As)
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
			small_neighborhood_sync(Bs)
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

		// only declared static to silence MSVC errors...
		static const int N = 10;

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
			ref = pfor(1,N-1,[A,B,t](int i) {

				if(i != 1) { EXPECT_EQ(t, A[i - 1]); }
				EXPECT_EQ(t,A[i]);
				if(i != N - 2) { EXPECT_EQ(t, A[i + 1]); }

				EXPECT_EQ(t-1,B[i]);

				B[i]=t+1;

			},small_neighborhood_sync(ref));

			// plug in after
			if (t % 2 == 0) {
				ref = after(ref,N/2,[B,t,&counter]{
					EXPECT_EQ(t+1,B[N/2]);
					counter++;
				});
			}

			std::swap(A,B);
		}

		// check the final state
		pfor(1,N-1,[A](int i){
			EXPECT_EQ(T,A[i]);
		},small_neighborhood_sync(ref));

		EXPECT_EQ(counter,T/2);

		delete [] A;
		delete [] B;

	}


	TEST(Pfor,After2D) {

		// only declared static to silence MSVC errors...
		static const int N = 10;

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
			ref = pfor(Point{1,1},Point{N-1,N-1},[A,B,t](const Point& p) {

				if (			  p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x  ][p.y-1]); }
				if (p.x != N-2			  )  { EXPECT_EQ(t,(*A)[p.x+1][p.y  ]); }
				if (p.y != 1  )  { EXPECT_EQ(t,(*A)[p.x][p.y-1]); }
				if (p.y != N-2)  { EXPECT_EQ(t,(*A)[p.x][p.y+1]); }

				EXPECT_EQ(t,(*A)[p.x][p.y]);
				EXPECT_EQ(t-1,(*B)[p.x][p.y]);

				(*B)[p.x][p.y]=t+1;

			},small_neighborhood_sync(ref));

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
		},small_neighborhood_sync(ref));

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
			},small_neighborhood_sync(ref));
		}

		// check the final state
		ref = pfor(1,N-1,[&](int i){
			if (i==X) counter++;
		},small_neighborhood_sync(ref));

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
			},small_neighborhood_sync(ref));
		}

		ref.wait();

		// there should have been some overlap
		EXPECT_TRUE(overlapDetected.load());
	}


} // end namespace algorithm
} // end namespace user
} // end namespace api
} // end namespace allscale
