#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "allscale/api/core/treeture.h"

namespace allscale {
namespace api {
namespace core {

	TEST(BitQueue, Basic) {
		BitQueue queue;

		// check put and pop
		int x = 577;
		for(int i=0; i<32; i++) {
			queue.put(x & (1<<i));
		}

		int y = 0;
		for(int i=0; i<32; i++) {
			if (queue.pop()) {
				y = y | (1<<i);
			}
		}

		EXPECT_EQ(x,y);

		// check the get function
		for(int i=0; i<32; i++) {
			queue.put(x & (1<<i));
		}

		y = 0;
		for(int i=0; i<32; i++) {
			if (queue.get()) {
				y = y | (1<<i);
			}
			queue.pop();
		}

		EXPECT_EQ(x,y);
	}

	TEST(Treeture, Basic) {

		// a simple done test
		treeture test = done();
		test.wait();

		int x = 0;
		treeture job = spawn([&](){ x++; });
		EXPECT_EQ(0,x);
		job.wait();
		EXPECT_EQ(1,x);
		job.wait();
		EXPECT_EQ(1,x);

		int y = 0;
		treeture another = spawn(
				[&](){ y++; },
				[&](){ return spawn([&](){ y++; }); }
		);
		EXPECT_EQ(0,y);
		another.wait();
		EXPECT_EQ(1,y);
		another.wait();
		EXPECT_EQ(1,y);

	}


	TEST(Treeture, Navigation) {

		treeture test = done();
		test.descentLeft().descentRight();
		test.wait();

		int x = 0;
		treeture job = spawn([&](){ x++; });
		EXPECT_EQ(0,x);
		job.wait();
		EXPECT_EQ(1,x);
		job.wait();
		EXPECT_EQ(1,x);
		job.getLeft().getRight().wait();
		EXPECT_EQ(1,x);

		job = spawn([&](){ x++; });
		EXPECT_EQ(1,x);
		job.getLeft().getRight().wait();
		EXPECT_EQ(2,x);

	}

	TEST(Treeture, Sequence) {

		int x = 0;

		sequence(
			[&]() { EXPECT_EQ(0,x); x++; },
			[&]() { EXPECT_EQ(1,x); x++; },
			[&]() { EXPECT_EQ(2,x); x++; },
			[&]() { EXPECT_EQ(3,x); x++; },
			[&]() { EXPECT_EQ(4,x); x++; }
		).wait();

		EXPECT_EQ(5,x);
	}

	TEST(Treeture, Parallel) {

		int x = 0;
		int y = 0;
		int z = 0;

		parallel(
			[&]() { EXPECT_EQ(0,x); x++; },
			[&]() { EXPECT_EQ(0,y); y++; },
			[&]() { EXPECT_EQ(0,z); z++; }
		).wait();

		EXPECT_EQ(1,x);
		EXPECT_EQ(1,y);
		EXPECT_EQ(1,z);

	}


	TEST(Treeture, Nested) {

		int x = 0;
		int y = 0;
		int z = 0;

		parallel(
			[&]() { EXPECT_EQ(0,x); x++; },
			sequence(
				[&]() { EXPECT_EQ(0,y); y++; },
				[&]() { EXPECT_EQ(1,y); y++; },
				[&]() { EXPECT_EQ(2,y); y++; }
			),
			[&]() { EXPECT_EQ(0,z); z++; }
		).wait();

		EXPECT_EQ(1,x);
		EXPECT_EQ(3,y);
		EXPECT_EQ(1,z);

	}

//	treeture<int> naive_fib(int x) {
//		if (x <= 1) return done(x);
//		return add(naive_fib(x-1),naive_fib(x-2));
//	}

//	TEST(Treeture, Fib) {
//
//		EXPECT_EQ(1, naive_fib(1).get());
//		EXPECT_EQ(1, naive_fib(2).get());
//		EXPECT_EQ(2, naive_fib(3).get());
//		EXPECT_EQ(3, naive_fib(4).get());
//		EXPECT_EQ(5, naive_fib(5).get());
//		EXPECT_EQ(8, naive_fib(6).get());
//		EXPECT_EQ(13, naive_fib(7).get());
//		EXPECT_EQ(21, naive_fib(8).get());
//
//		EXPECT_EQ(832040, naive_fib(30).get());
//
//	}


//	TEST(Treeture, Ordering) {
//
//		std::vector<int> res;
//
//		auto w1 = [&](){
//			res.push_back(1);
//		};
//		auto w2 = [&](){
//			res.push_back(2);
//		};
//		auto w3 = [&](){
//			res.push_back(3);
//		};
//
//		// test an atomic step
//		atom(w1);
//		EXPECT_EQ(std::vector<int>({1}), res);
//
//		// test a sequence
//		seq(
//				atom(w2),
//				atom(w3),
//				atom(w1)
//		);
//		EXPECT_EQ(std::vector<int>({1,2,3,1}), res);
//
//		res.clear();
//		res.resize(3);
//
//		// test a parallel
//		par(
//				atom([&](){ res[0] = 1; }),
//				atom([&](){ res[1] = 2; }),
//				atom([&](){ res[2] = 3; })
//		);
//		EXPECT_EQ(std::vector<int>({1,2,3}), res);
//
//	}

//	TEST(Treeture, TaskReferences) {
//
//		using Ref = typename treeture<int>::task_reference;
//
//		Ref t;
//		EXPECT_FALSE(t.valid());
//
//		{
//			treeture<int> f = spawn([]() { return 12; });
//
//			// obtain task reference of treeture
//			t = f.getTaskReference();
//
//			EXPECT_TRUE(t.valid());
//
//			EXPECT_FALSE(f.isDone());
//			EXPECT_FALSE(t.isDone());
//
//			t.wait();
//			EXPECT_TRUE(f.isDone());
//			EXPECT_TRUE(t.isDone());
//		}
//
//		// task reference has to survive the treeture if necessary
//		EXPECT_TRUE(t.valid());
//		EXPECT_TRUE(t.isDone());
//
//	}

} // end namespace core
} // end namespace api
} // end namespace allscale
