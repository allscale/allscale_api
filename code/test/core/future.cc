#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "allscale/api/core/future.h"

namespace allscale {
namespace api {
namespace core {

	TEST(Future, Basic) {

		// a simple done test
		Future<int> test = 12;
		EXPECT_EQ(12, test.get());
		EXPECT_TRUE(test.isAtom());

		// test an aggregate
		EXPECT_EQ(9, add(done(2), done(3), done(4)).get());


		// and a more deeper nested aggregate
		Future<int> computation = add(
				add(done(1), done(2)),
				add(done(3), done(4), done(5))
		);

		// check result
		EXPECT_EQ((1+2)+(3+4+5), computation.get());
		EXPECT_TRUE(computation.isParallel());

	}


	Future<int> naive_fib(int x) {
		if (x <= 1) return done(x);
		return add(naive_fib(x-1),naive_fib(x-2));
	}

	TEST(Future, Fib) {

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


	TEST(Future, Ordering) {

		std::vector<int> res;

		auto w1 = [&](){
			res.push_back(1);
		};
		auto w2 = [&](){
			res.push_back(2);
		};
		auto w3 = [&](){
			res.push_back(3);
		};

		// test an atomic step
		atom(w1);
		EXPECT_EQ(std::vector<int>({1}), res);

		// test a sequence
		seq(
				atom(w2),
				atom(w3),
				atom(w1)
		);
		EXPECT_EQ(std::vector<int>({1,2,3,1}), res);

		res.clear();
		res.resize(3);

		// test a parallel
		par(
				atom([&](){ res[0] = 1; }),
				atom([&](){ res[1] = 2; }),
				atom([&](){ res[2] = 3; })
		);
		EXPECT_EQ(std::vector<int>({1,2,3}), res);

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
