#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/core/impl/reference/treeture.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	TEST(Parec,ImplCheck) {
		EXPECT_EQ("Reference SharedMemory", getImplementationName());
	}

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

	TEST(Runtime, SimpleTask) {
		treeture<int> future = spawn([]{ return 12; });
		EXPECT_EQ(12,future.get());
	}

	TEST(Runtime, ValueTask) {
		treeture<int> future = done(12);
		EXPECT_EQ(12,future.get());
	}

	template<typename T>
	unreleased_treeture<T> add(unreleased_treeture<T>&& a, unreleased_treeture<T>&& b) {
		return combine(std::move(a),std::move(b),[](T a, T b) { return a + b; });
	}

	TEST(Runtime, Spawn) {

		// build a completed task
		treeture<int> d = done(10);
		EXPECT_EQ(10, d.get());

		// build a simple task
		auto f = spawn([](){ return 12; });

		// compute with futures
		EXPECT_EQ(5, add(done(2),done(3)).get());

		// build a splitable task
		auto g = spawn(
				[](){ return 6 + 8; },
				[](){ return add(
						spawn([](){ return 8; }),
						done(6)
					);
				}
		);

		// build an aggregate node
		treeture<int> h = add(std::move(f), std::move(g));

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

		treeture<void> a = spawn(
			[&]{ x++; }
		);

		EXPECT_EQ(0,y);
		EXPECT_EQ(0,z);

		treeture<void> b = spawn(
			[&]{ y++; }
		);

		EXPECT_EQ(0,z);

		b.get();

		EXPECT_EQ(1,y);
		EXPECT_EQ(0,z);

		a.get();

		EXPECT_EQ(1,x);
		EXPECT_EQ(1,y);
		EXPECT_EQ(0,z);

		a.get();

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
		).get();

		EXPECT_EQ(0,x);
		EXPECT_EQ(0,y);
		EXPECT_EQ(1,z);



		// -- splitable tasks --

		{
			x = y = z = 0;

			EXPECT_EQ(0,x);
			EXPECT_EQ(0,y);
			EXPECT_EQ(0,z);

			treeture<void> t = spawn(
				[&]{ x++; },
				[&]{ return parallel(
						spawn([&]{ y++; }),
						spawn([&]{ z++; })
					);
				}
			);

			EXPECT_EQ(0,x);
			EXPECT_EQ(0,y);
			EXPECT_EQ(0,z);

			t.get();

			if (x == 0) {

				// the task was split

				EXPECT_EQ(0,x);
				EXPECT_EQ(1,y);
				EXPECT_EQ(1,z);

				t.get();

				EXPECT_EQ(0,x);
				EXPECT_EQ(1,y);
				EXPECT_EQ(1,z);
			} else {

				// the task was not split

				EXPECT_EQ(1,x);
				EXPECT_EQ(0,y);
				EXPECT_EQ(0,z);

				t.get();

				EXPECT_EQ(1,x);
				EXPECT_EQ(0,y);
				EXPECT_EQ(0,z);
			}

		}


	}

	template<typename Body>
	unreleased_treeture<void> forEach(int begin, int end, const Body& body) {

		// handle empty case
		if (begin >= end) {
			return done();
		}

		// handle single step case
		if (begin + 1 == end) {
			return spawn(
					[=]{
						body(begin);
					}
			);
		}

		// handle rest
		int mid = (begin + end) / 2;
		return spawn(
				[=]() {
					for(int i=begin; i<end; i++) body(i);
				},
				[=]() {
					return parallel(
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
		}).get();

		for(int i=0; i<N; i++) {
			EXPECT_EQ(11, data[i]);
		}

	}

	TEST(Runtime, ForEachSplitTest) {

		const int N = 20000;

		std::array<int,N> data;
		data.fill(10);

		auto As = forEach(0,N,[&](int i){
			data[i]++;
		});

		for(int i=0; i<N; i++) {
			EXPECT_EQ(10, data[i]);
		}

		// wait for completion
		std::move(As).get();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(11, data[i]);
		}

	}

	template<typename Body>
	unreleased_treeture<void> forEachAfter(int begin, int end, const Body& body) {

		// handle empty case
		if (begin >= end) {
			return done();
		}

		// handle single step case
		if (begin + 1 == end) {
			return spawn(
					[=]{
						body(begin);
					}
			);
		}

		// handle rest
		int mid = (begin + end) / 2;
		return spawn(
				[=]() {
					for(int i=begin; i<end; i++) body(i);
				},
				[=]() {
					return sequential(
							forEachAfter(begin,mid,body),
							forEachAfter(mid,end,body)
					);
				}
		);
	}

	TEST(Runtime, ForEachAfter) {

		const int N = 20000;

		std::array<int,N> data;
		data.fill(0);

		forEachAfter(1,N,[&](int i){
			data[i] = data[i-1] + 1;
		}).get();

		for(int i=0; i<N; i++) {
			EXPECT_EQ(i, data[i]);
		}

	}

	TEST(Runtime, ForEachAfterSplitTest) {

		const int N = 20000;

		std::array<int,N> data;
		data.fill(0);

		// create parallel task
		auto As = forEachAfter(1,N,[&](int i){
			data[i] = data[i-1] + 1;
		});

		// should not run yet
		for(int i=0; i<N; i++) {
			EXPECT_EQ(0, data[i]);
		}

		// wait for completion
		std::move(As).get();

		// check result
		for(int i=0; i<N; i++) {
			EXPECT_EQ(i, data[i]);
		}
	}


	constexpr unsigned const_fib(unsigned n) {
		return (n <= 1) ? n : (const_fib(n-1) + const_fib(n-2));
	}

	unsigned fib(unsigned n) {
		if (n <= 1) return n;
		return fib(n-1) + fib(n-2);
	}

	const unsigned STRESS_N = 43;
	const unsigned STRESS_RES = const_fib(STRESS_N);

	TEST(Runtime, FibSeq) {

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

	unreleased_treeture<unsigned> fib_naive(unsigned n) {
		if (n <= 1) return done(n);
		return add(fib_naive(n-1),fib_naive(n-2));
	}

	TEST(Runtime, FibNaive) {

		EXPECT_EQ(0, fib_naive(0).get());
		EXPECT_EQ(1, fib_naive(1).get());
		EXPECT_EQ(1, fib_naive(2).get());
		EXPECT_EQ(2, fib_naive(3).get());
		EXPECT_EQ(3, fib_naive(4).get());
		EXPECT_EQ(5, fib_naive(5).get());
		EXPECT_EQ(8, fib_naive(6).get());
		EXPECT_EQ(13, fib_naive(7).get());

		EXPECT_EQ(144, fib_naive(12).get());
		EXPECT_EQ(6765, fib_naive(20).get());

		// EXPECT_EQ(STRESS_RES, fib_naive(STRESS_N).get());
	}

	unreleased_treeture<unsigned> fib_split(unsigned n) {
		if (n <= 1) return done(n);
		return spawn(
				[=](){ return fib(n); },
				[=](){ return add(fib_split(n-1), fib_split(n-2)); }
		);
	}

	TEST(Runtime, FibSplit) {
		EXPECT_EQ(0, fib_split(0).get());
		EXPECT_EQ(1, fib_split(1).get());
		EXPECT_EQ(1, fib_split(2).get());
		EXPECT_EQ(2, fib_split(3).get());
		EXPECT_EQ(3, fib_split(4).get());
		EXPECT_EQ(5, fib_split(5).get());
		EXPECT_EQ(8, fib_split(6).get());
		EXPECT_EQ(13, fib_split(7).get());

		EXPECT_EQ(144, fib_split(12).get());
		EXPECT_EQ(6765, fib_split(20).get());

		EXPECT_EQ(STRESS_RES, fib_split(STRESS_N).get());
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
