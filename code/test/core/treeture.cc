#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "allscale/api/core/treeture.h"

namespace allscale {
namespace api {
namespace core {

	TEST(Treeture, Immediates) {

		treeture<void> t1 = done();
		t1.wait();

		treeture<int> t2 = done(12);
		EXPECT_EQ(12, t2.get());

		treeture<std::string> t3 = done(std::string("Hello"));
		EXPECT_EQ("Hello",t3.get());
	}

	TEST(Treeture, SimpleAction) {

		// test a simple computation action
		treeture<int> t1 = spawn([](){ return 12; });
		EXPECT_EQ(12, t1.get());

		// test a void action
		int x = 0;
		treeture<void> t2 = spawn([&](){ x = 1; });
		EXPECT_EQ(0,x);
		t2.get();
		EXPECT_EQ(1,x);

	}

	TEST(Treeture, SplitableAction) {

		// test a simple computation action
		treeture<int> t1 = spawn(
				[](){ return 12; },
				[](){ return spawn([](){ return 12; }); }
		);
		EXPECT_EQ(12, t1.get());

		// test a void action
		int x = 0;
		treeture<void> t2 = spawn(
				[&](){ x = 1; },
				[&](){ return spawn([&](){ x = 1; }); }
		);
		EXPECT_EQ(0,x);
		t2.get();
		EXPECT_EQ(1,x);
	}


	TEST(Treeture, Navigation) {

		auto test = done();
		test.descentLeft().descentRight();
		test.wait();

		int x = 0;
		auto job = spawn([&](){ x++; });
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
			spawn([&]() { EXPECT_EQ(0,x); x++; }),
			spawn([&]() { EXPECT_EQ(1,x); x++; }),
			spawn([&]() { EXPECT_EQ(2,x); x++; }),
			spawn([&]() { EXPECT_EQ(3,x); x++; }),
			spawn([&]() { EXPECT_EQ(4,x); x++; })
		).wait();

		EXPECT_EQ(5,x);
	}

	TEST(Treeture, Parallel) {

		int x = 0;
		int y = 0;
		int z = 0;

		parallel(
			spawn([&]() { EXPECT_EQ(0,x); x++; }),
			spawn([&]() { EXPECT_EQ(0,y); y++; }),
			spawn([&]() { EXPECT_EQ(0,z); z++; })
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
			spawn([&]() { EXPECT_EQ(0,x); x++; }),
			sequence(
				spawn([&]() { EXPECT_EQ(0,y); y++; }),
				spawn([&]() { EXPECT_EQ(1,y); y++; }),
				spawn([&]() { EXPECT_EQ(2,y); y++; })
			),
			spawn([&]() { EXPECT_EQ(0,z); z++; })
		).wait();

		EXPECT_EQ(1,x);
		EXPECT_EQ(3,y);
		EXPECT_EQ(1,z);

	}

	TEST(Treeture, Add) {

		auto t1 = add(
			spawn([]{ return 12; }),
			spawn([]{ return 14; })
		);

		EXPECT_EQ(26,t1.get());

		auto t2 = add(
			spawn([]{ return 1.2; }),
			spawn([]{ return 4.3; })
		);

		EXPECT_EQ(1.2+4.3,t2.get());

	}

	treeture<int> naive_fib(int x) {
		if (x <= 1) return done(x);
		return add(naive_fib(x-1),naive_fib(x-2));
	}

	TEST(Treeture, NaiveFib) {

		EXPECT_EQ(1, naive_fib(1).get());
		EXPECT_EQ(1, naive_fib(2).get());
		EXPECT_EQ(2, naive_fib(3).get());
		EXPECT_EQ(3, naive_fib(4).get());
		EXPECT_EQ(5, naive_fib(5).get());
		EXPECT_EQ(8, naive_fib(6).get());
		EXPECT_EQ(13, naive_fib(7).get());
		EXPECT_EQ(21, naive_fib(8).get());

		EXPECT_EQ(832040, naive_fib(30).get());

	}

	int fib(int x) {
		if (x<=1) return x;
		return fib(x-1)+fib(x-2);
	}

	treeture<int> pfib(int x) {
		if (x<=1) return done(x);
		return spawn(
				[=]() { return fib(x); },
				[=]() {
					return add(pfib(x-1),pfib(x-2));
				}
		);
	}

	TEST(Treeture, SplitFib) {

		EXPECT_EQ(1, pfib(1).get());
		EXPECT_EQ(1, pfib(2).get());
		EXPECT_EQ(2, pfib(3).get());
		EXPECT_EQ(3, pfib(4).get());
		EXPECT_EQ(5, pfib(5).get());
		EXPECT_EQ(8, pfib(6).get());
		EXPECT_EQ(13, pfib(7).get());
		EXPECT_EQ(21, pfib(8).get());

		EXPECT_EQ(832040, pfib(30).get());

	}

	#ifndef BENCH_VALUE
	#define BENCH_VALUE 40
	#endif

	int N = BENCH_VALUE;

	TEST(Treeture, Bench_Seq) {
		EXPECT_NE(0,fib(N));
	}

	TEST(Treeture, Bench_Par) {
		EXPECT_NE(0,pfib(N).get());
	}

	TEST(Treeture, Ordering) {

		std::vector<int> res;

		// test an atomic step
		spawn([&](){ res.push_back(1); }).get();
		EXPECT_EQ(std::vector<int>({1}), res);

		// test a sequence
		sequence(
			spawn([&](){ res.push_back(2); }),
			spawn([&](){ res.push_back(3); }),
			spawn([&](){ res.push_back(1); })
		).get();
		EXPECT_EQ(std::vector<int>({1,2,3,1}), res);

		res.clear();
		res.resize(3);

		// test a parallel
		parallel(
			spawn([&](){ res[0] = 1; }),
			spawn([&](){ res[1] = 2; }),
			spawn([&](){ res[2] = 3; })
		).get();
		EXPECT_EQ(std::vector<int>({1,2,3}), res);

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
