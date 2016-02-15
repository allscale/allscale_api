#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/core/impl/simple/runtime.h"

namespace allscale {
namespace api {
namespace core {

	TEST(TaskQueue, Basic) {

		runtime::SimpleQueue<int,3> queue;

		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(0, queue.size());

		std::cout << queue << "\n";

		EXPECT_TRUE(queue.push_front(12));
		std::cout << queue << "\n";
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1, queue.size());

		EXPECT_EQ(12, queue.pop_front());
		std::cout << queue << "\n";

		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(0, queue.size());

		EXPECT_TRUE(queue.push_front(12));

		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1, queue.size());

		std::cout << queue << "\n";
		EXPECT_EQ(12, queue.pop_back());
		std::cout << queue << "\n";

	}

	TEST(TaskQueue, Size) {

		runtime::SimpleQueue<int,3> queue;

		EXPECT_EQ(0, queue.size());
		queue.push_front(1);
		EXPECT_EQ(1, queue.size());
		queue.push_front(1);
		EXPECT_EQ(2, queue.size());
		queue.push_front(1);
		EXPECT_EQ(3, queue.size());

		for (int i =0; i<10; i++) {
			queue.pop_front();
			EXPECT_EQ(2, queue.size());
			queue.pop_front();
			EXPECT_EQ(1, queue.size());

			queue.push_front(1);
			EXPECT_EQ(2, queue.size());
			queue.push_front(1);
			EXPECT_EQ(3, queue.size());
		}

	}


	TEST(TaskQueue, Order) {


		runtime::SimpleQueue<int,3> queue;

		// fill queue in the front
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_front(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the back
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(1,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(3,queue.pop_back());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

		// fill queue in the front again
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_front(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the front
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(3,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1,queue.pop_front());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

		// fill queue in the back
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_back(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the front
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(1,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(3,queue.pop_front());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

		// fill queue in the back again
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_back(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the back
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(3,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1,queue.pop_back());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

	}

	TEST(Runtime, DefaultFuture) {
		Future<int> future;
		EXPECT_TRUE(future.isDone());
		EXPECT_EQ(0,future.get());
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
		return aggregate(sum<R>, first, tasks...);
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

	TEST(Runtime, TaskDelayedRunAndSplit) {

		int x = 0;
		int y = 0;
		int z = 0;

		// -- simple tasks --

		EXPECT_EQ(0,x);
		EXPECT_EQ(0,y);
		EXPECT_EQ(0,z);

		Future<void> a = spawn(
			[&]{ x++; }
		);

		EXPECT_EQ(0,x);
		EXPECT_EQ(0,y);
		EXPECT_EQ(0,z);

		Future<void> b = spawn(
			[&]{ y++; }
		);

		EXPECT_EQ(0,x);
		EXPECT_EQ(0,y);
		EXPECT_EQ(0,z);

		b.wait();

		EXPECT_EQ(0,x);
		EXPECT_EQ(1,y);
		EXPECT_EQ(0,z);

		a.wait();

		EXPECT_EQ(1,x);
		EXPECT_EQ(1,y);
		EXPECT_EQ(0,z);

		a.wait();

		EXPECT_EQ(1,x);
		EXPECT_EQ(1,y);
		EXPECT_EQ(0,z);

		// -- auto-wait on distruction --
		x = y = z = 0;

		EXPECT_EQ(0,x);
		EXPECT_EQ(0,y);
		EXPECT_EQ(0,z);

		spawn(
			[&]{ z++; }
		);

		EXPECT_EQ(0,x);
		EXPECT_EQ(0,y);
		EXPECT_EQ(1,z);



		// -- splitable tasks --

		{
			x = y = z = 0;

			EXPECT_EQ(0,x);
			EXPECT_EQ(0,y);
			EXPECT_EQ(0,z);

			Future<void> t = spawn(
				[&]{ x++; },
				[&]{ return par(
						spawn([&]{ y++; }),
						spawn([&]{ z++; })
					);
				}
			);

			EXPECT_EQ(0,x);
			EXPECT_EQ(0,y);
			EXPECT_EQ(0,z);

			t.wait();

			if (x == 0) {

				// the task was split

				EXPECT_EQ(0,x);
				EXPECT_EQ(1,y);
				EXPECT_EQ(1,z);

				t.wait();

				EXPECT_EQ(0,x);
				EXPECT_EQ(1,y);
				EXPECT_EQ(1,z);
			} else {

				// the task was not split

				EXPECT_EQ(1,x);
				EXPECT_EQ(0,y);
				EXPECT_EQ(0,z);

				t.wait();

				EXPECT_EQ(1,x);
				EXPECT_EQ(0,y);
				EXPECT_EQ(0,z);
			}

		}


	}

	template<typename Body>
	Future<void> forEach(int begin, int end, const Body& body) {

		// handle empty case
		if (begin >= end) {
			return done();
		}

		// handle single step case
		if (begin + 1 == end) {
			return spawn(
					[=]{
//						std::cout << "Running element " << begin << "\n";
						body(begin);
					}
			);
		}

		// handle rest
		int mid = (begin + end) / 2;
		return spawn(
				[=]() {
//					std::cout << "Running range " << begin << " to " << end << "\n";
					for(int i=begin; i<end; i++) body(i);
				},
				[=]() {
					return par(
							forEach(begin,mid,body),
							forEach(mid,end,body)
					);
				}
		);
	}


	TEST(Runtime, ForEach) {

		const int N = 20000;

		std::array<int,N> data;
		data.fill(10);

		forEach(0,N,[&](int i){
			data[i]++;
		});

		for(int i=0; i<N; i++) {
			EXPECT_EQ(11, data[i]);
		}

	}


	constexpr unsigned const_fib(unsigned n) {
		return (n <= 1) ? n : (const_fib(n-1) + const_fib(n-2));
	}

	unsigned fib(unsigned n) {
		if (n <= 1) return n;
		return fib(n-1) + fib(n-2);
	}

//	const unsigned STRESS_N = 20;
	const unsigned STRESS_N = 43;
	const unsigned STRESS_RES = const_fib(STRESS_N);

	TEST(Runtime, Fib) {

		EXPECT_EQ(0, fib(0));
		EXPECT_EQ(1, fib(1));
		EXPECT_EQ(1, fib(2));
		EXPECT_EQ(2, fib(3));
		EXPECT_EQ(3, fib(4));
		EXPECT_EQ(5, fib(5));
		EXPECT_EQ(8, fib(6));
		EXPECT_EQ(13, fib(7));

		EXPECT_EQ(144, fib(12));
		EXPECT_EQ(6765, fib(20));

		EXPECT_EQ(STRESS_RES, fib(STRESS_N));

	}

//	Future<unsigned> naive_fib(unsigned n) {
//		if (n <= 1) return done(n);
//		return sum(naive_fib(n-1),naive_fib(n-2));
//	}
//
//	TEST(Runtime, NaiveFib) {
//
//		EXPECT_EQ(0, naive_fib(0).get());
//		EXPECT_EQ(1, naive_fib(1).get());
//		EXPECT_EQ(1, naive_fib(2).get());
//		EXPECT_EQ(2, naive_fib(3).get());
//		EXPECT_EQ(3, naive_fib(4).get());
//		EXPECT_EQ(5, naive_fib(5).get());
//		EXPECT_EQ(8, naive_fib(6).get());
//		EXPECT_EQ(13, naive_fib(7).get());
//
//		EXPECT_EQ(144, naive_fib(12).get());
//		EXPECT_EQ(6765, naive_fib(20).get());
//
//		EXPECT_EQ(STRESS_RES, naive_fib(STRESS_N).get());
//	}

	Future<unsigned> better_fib(unsigned n) {
		if (n <= 1) return done(n);
		return spawn(
				[=](){ return fib(n); },
				[=](){ return sum(better_fib(n-1), better_fib(n-2)); }
		);

//		Future<unsigned> res;
//		if (n <= 1) res = std::move(done(n));
//		else res = std::move(
//				spawn(
//						[=](){ return fib(n); },
//						[=](){
//							auto res = sum(better_fib(n-1), better_fib(n-2));
//							std::cout << "sum-task(" << n << ") = " << res << "\n";
//							return std::move(res);
//						}
//				)
//		);
//
//		std::cout << "fib(" << n << ") = " << res << "\n";
//
//		return std::move(res);
	}

	TEST(Runtime, BetterFib) {
//		EXPECT_EQ(0, better_fib(0).get());
//		EXPECT_EQ(1, better_fib(1).get());
//		EXPECT_EQ(1, better_fib(2).get());
//		EXPECT_EQ(2, better_fib(3).get());
//		EXPECT_EQ(3, better_fib(4).get());
//		EXPECT_EQ(5, better_fib(5).get());
//		EXPECT_EQ(8, better_fib(6).get());
//		EXPECT_EQ(13, better_fib(7).get());
//
//		EXPECT_EQ(144, better_fib(12).get());
//		EXPECT_EQ(6765, better_fib(20).get());

		EXPECT_EQ(STRESS_RES, better_fib(STRESS_N).get());
	}


} // end namespace core
} // end namespace api
} // end namespace allscale
