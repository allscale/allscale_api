#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/core/impl/omp_cilk/future.h"

#ifdef __cilk
    #include <cilk/cilk.h>
#endif


namespace allscale {
namespace api {
namespace core {

	TEST(Parec,ImplCheck) {
		EXPECT_EQ("OpenMP/Cilk", PAREC_IMPL);
	}

	TEST(Runtime, DefaultFuture) {
		Future<int> future;
		EXPECT_TRUE(future.isDone());
		EXPECT_EQ(0,future.get());
	}

	TEST(Runtime, SimpleTask) {
		Future<int> future = spawn([]{ return 12; });
		EXPECT_EQ(12,future.get());
	}

	template<typename T>
	T sum(const std::vector<Future<T>>& children) {
		T res = 0;
		for(const auto& cur : children) {
			res += cur.get();
		}
		return res;
	}

	template<typename R, typename ... Tasks>
	Future<R> sum(Future<R>&& first, Tasks&& ... tasks) {
		R(*sum_fun)(const std::vector<Future<R>>&) = sum<R>;
		return aggregate(sum_fun, first, tasks...);
	}

	TEST(Runtime, Spawn) {

		// build a completed task
		Future<int> d = done(10);
		EXPECT_TRUE(d.isDone());
		EXPECT_EQ(10, d.get());

		// build a simple task
		Future<int> f = spawn([](){ return 12; });
		EXPECT_TRUE(f.valid());

		// compute with futures
		EXPECT_EQ(5, sum(done(2),done(3)).get());

		// build a splitable task
		Future<int> g = spawn(
				[](){ return 6 + 8; },
				[](){ return sum(
						spawn([](){ return 8; }),
						done(6)
					);
				}
		);

		// build an aggregate node
		Future<int> h = sum(std::move(f), std::move(g));

		EXPECT_EQ(26, h.get());

	}
//
//	TEST(Runtime, TaskDelayedRunAndSplit) {
//
//		int x = 0;
//		int y = 0;
//		int z = 0;
//
//		// -- simple tasks --
//
//		EXPECT_EQ(0,x);
//		EXPECT_EQ(0,y);
//		EXPECT_EQ(0,z);
//
//		Future<void> a = spawn(
//			[&]{ x++; }
//		);
//
//		EXPECT_EQ(0,x);
//		EXPECT_EQ(0,y);
//		EXPECT_EQ(0,z);
//
//		Future<void> b = spawn(
//			[&]{ y++; }
//		);
//
//		EXPECT_EQ(0,x);
//		EXPECT_EQ(0,y);
//		EXPECT_EQ(0,z);
//
//		b.wait();
//
//		EXPECT_EQ(0,x);
//		EXPECT_EQ(1,y);
//		EXPECT_EQ(0,z);
//
//		a.wait();
//
//		EXPECT_EQ(1,x);
//		EXPECT_EQ(1,y);
//		EXPECT_EQ(0,z);
//
//		a.wait();
//
//		EXPECT_EQ(1,x);
//		EXPECT_EQ(1,y);
//		EXPECT_EQ(0,z);
//
//		// -- auto-wait on distruction --
//		x = y = z = 0;
//
//		EXPECT_EQ(0,x);
//		EXPECT_EQ(0,y);
//		EXPECT_EQ(0,z);
//
//		spawn(
//			[&]{ z++; }
//		);
//
//		EXPECT_EQ(0,x);
//		EXPECT_EQ(0,y);
//		EXPECT_EQ(1,z);
//
//
//
//		// -- splitable tasks --
//
//		{
//			x = y = z = 0;
//
//			EXPECT_EQ(0,x);
//			EXPECT_EQ(0,y);
//			EXPECT_EQ(0,z);
//
//			Future<void> t = spawn(
//				[&]{ x++; },
//				[&]{ return par(
//						spawn([&]{ y++; }),
//						spawn([&]{ z++; })
//					);
//				}
//			);
//
//			EXPECT_EQ(0,x);
//			EXPECT_EQ(0,y);
//			EXPECT_EQ(0,z);
//
//			t.wait();
//
//			if (x == 0) {
//
//				// the task was split
//
//				EXPECT_EQ(0,x);
//				EXPECT_EQ(1,y);
//				EXPECT_EQ(1,z);
//
//				t.wait();
//
//				EXPECT_EQ(0,x);
//				EXPECT_EQ(1,y);
//				EXPECT_EQ(1,z);
//			} else {
//
//				// the task was not split
//
//				EXPECT_EQ(1,x);
//				EXPECT_EQ(0,y);
//				EXPECT_EQ(0,z);
//
//				t.wait();
//
//				EXPECT_EQ(1,x);
//				EXPECT_EQ(0,y);
//				EXPECT_EQ(0,z);
//			}
//
//		}
//
//
//	}
//
//	template<typename Body>
//	Future<void> forEach(int begin, int end, const Body& body) {
//
//		// handle empty case
//		if (begin >= end) {
//			return done();
//		}
//
//		// handle single step case
//		if (begin + 1 == end) {
//			return spawn(
//					[=]{
//						body(begin);
//					}
//			);
//		}
//
//		// handle rest
//		int mid = (begin + end) / 2;
//		return spawn(
//				[=]() {
//					for(int i=begin; i<end; i++) body(i);
//				},
//				[=]() {
//					return par(
//							forEach(begin,mid,body),
//							forEach(mid,end,body)
//					);
//				}
//		);
//	}
//
//
//	TEST(Runtime, ForEach) {
//
//		const int N = 20000;
//
//		std::array<int,N> data;
//		data.fill(10);
//
//		forEach(0,N,[&](int i){
//			data[i]++;
//		});
//
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(11, data[i]);
//		}
//
//	}
//
//	TEST(Runtime, ForEachSplitTest) {
//
//		const int N = 20000;
//
//		std::array<int,N> data;
//		data.fill(10);
//
//		auto As = forEach(0,N,[&](int i){
//			data[i]++;
//		});
//
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(10, data[i]);
//		}
//
//		// As should for sure be split into parallel jobs
//		EXPECT_TRUE(As.isParallel());
//
//		// wait for completion
//		As.wait();
//
//		// check result
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(11, data[i]);
//		}
//
//	}
//
//	template<typename Body>
//	Future<void> forEachAfter(int begin, int end, const Body& body) {
//
//		// handle empty case
//		if (begin >= end) {
//			return done();
//		}
//
//		// handle single step case
//		if (begin + 1 == end) {
//			return spawn(
//					[=]{
//						body(begin);
//					}
//			);
//		}
//
//		// handle rest
//		int mid = (begin + end) / 2;
//		return spawn(
//				[=]() {
//					for(int i=begin; i<end; i++) body(i);
//				},
//				[=]() {
//					return seq(
//							forEachAfter(begin,mid,body),
//							forEachAfter(mid,end,body)
//					);
//				}
//		);
//	}
//
//	TEST(Runtime, ForEachAfter) {
//
//		const int N = 20000;
//
//		std::array<int,N> data;
//		data.fill(0);
//
//		forEachAfter(1,N,[&](int i){
//			data[i] = data[i-1] + 1;
//		});
//
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(i, data[i]);
//		}
//
//	}
//
//	TEST(Runtime, ForEachAfterSplitTest) {
//
//		const int N = 20000;
//
//		std::array<int,N> data;
//		data.fill(0);
//
//		// create parallel task
//		auto As = forEachAfter(1,N,[&](int i){
//			data[i] = data[i-1] + 1;
//		});
//
//		// should not run yet
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(0, data[i]);
//		}
//
//		// As should for sure be split into parallel jobs
//		EXPECT_TRUE(As.isSequence());
//
//		// wait for completion
//		As.wait();
//
//		// check result
//		for(int i=0; i<N; i++) {
//			EXPECT_EQ(i, data[i]);
//		}
//	}
//
//
//	constexpr unsigned const_fib(unsigned n) {
//		return (n <= 1) ? n : (const_fib(n-1) + const_fib(n-2));
//	}
//
//	unsigned fib(unsigned n) {
//		if (n <= 1) return n;
//		return fib(n-1) + fib(n-2);
//	}
//
//	const unsigned STRESS_N = 43;
//	const unsigned STRESS_RES = const_fib(STRESS_N);
//
//	TEST(Runtime, FibSeq) {
//
//		EXPECT_EQ(0, fib(0));
//		EXPECT_EQ(1, fib(1));
//		EXPECT_EQ(1, fib(2));
//		EXPECT_EQ(2, fib(3));
//		EXPECT_EQ(3, fib(4));
//		EXPECT_EQ(5, fib(5));
//		EXPECT_EQ(8, fib(6));
//		EXPECT_EQ(13, fib(7));
//
//		EXPECT_EQ(144, fib(12));
//		EXPECT_EQ(6765, fib(20));
//
//		EXPECT_EQ(STRESS_RES, fib(STRESS_N));
//
//	}
//
//	Future<unsigned> fib_naive(unsigned n) {
//		if (n <= 1) return done(n);
//		return sum(fib_naive(n-1),fib_naive(n-2));
//	}
//
//	TEST(Runtime, FibNaive) {
//
//		EXPECT_EQ(0, fib_naive(0).get());
//		EXPECT_EQ(1, fib_naive(1).get());
//		EXPECT_EQ(1, fib_naive(2).get());
//		EXPECT_EQ(2, fib_naive(3).get());
//		EXPECT_EQ(3, fib_naive(4).get());
//		EXPECT_EQ(5, fib_naive(5).get());
//		EXPECT_EQ(8, fib_naive(6).get());
//		EXPECT_EQ(13, fib_naive(7).get());
//
//		EXPECT_EQ(144, fib_naive(12).get());
//		EXPECT_EQ(6765, fib_naive(20).get());
//
//		// EXPECT_EQ(STRESS_RES, fib_naive(STRESS_N).get());
//	}
//
//	Future<unsigned> fib_split(unsigned n) {
//		if (n <= 1) return done(n);
//		return spawn(
//				[=](){ return fib(n); },
//				[=](){ return sum(fib_split(n-1), fib_split(n-2)); }
//		);
//	}
//
//	TEST(Runtime, FibSplit) {
//		EXPECT_EQ(0, fib_split(0).get());
//		EXPECT_EQ(1, fib_split(1).get());
//		EXPECT_EQ(1, fib_split(2).get());
//		EXPECT_EQ(2, fib_split(3).get());
//		EXPECT_EQ(3, fib_split(4).get());
//		EXPECT_EQ(5, fib_split(5).get());
//		EXPECT_EQ(8, fib_split(6).get());
//		EXPECT_EQ(13, fib_split(7).get());
//
//		EXPECT_EQ(144, fib_split(12).get());
//		EXPECT_EQ(6765, fib_split(20).get());
//
//		EXPECT_EQ(STRESS_RES, fib_split(STRESS_N).get());
//	}
//
//#ifdef __cilk
//
//	unsigned fib_cilk(unsigned n) {
//		if (n <= 1) return n;
//		unsigned a = cilk_spawn fib_cilk(n-1);
//		unsigned b = fib_cilk(n-2);
//		cilk_sync;
//		return a + b;
//	}
//
//	TEST(Runtime, FibCilk) {
//		EXPECT_EQ(0, fib_cilk(0));
//		EXPECT_EQ(1, fib_cilk(1));
//		EXPECT_EQ(1, fib_cilk(2));
//		EXPECT_EQ(2, fib_cilk(3));
//		EXPECT_EQ(3, fib_cilk(4));
//		EXPECT_EQ(5, fib_cilk(5));
//		EXPECT_EQ(8, fib_cilk(6));
//		EXPECT_EQ(13, fib_cilk(7));
//
//		EXPECT_EQ(144, fib_cilk(12));
//		EXPECT_EQ(6765, fib_cilk(20));
//
//		EXPECT_EQ(STRESS_RES, fib_cilk(STRESS_N));
//	}
//
//#endif

} // end namespace core
} // end namespace api
} // end namespace allscale
