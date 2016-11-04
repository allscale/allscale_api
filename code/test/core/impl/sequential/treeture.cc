#include <gtest/gtest.h>

#include "allscale/api/core/impl/sequential/treeture.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace sequential {

	TEST(Treeture,Basic) {

		treeture<void> t1;
		t1.get();

		treeture<int> t2 = 12;
		EXPECT_EQ(12, t2.get());

	}

	TEST(Operator,Done) {

		treeture<void> t1 = done();
		t1.get();

		treeture<int> t2 = done(12);
		EXPECT_EQ(12, t2.get());

	}

	TEST(Operator, Task) {

		treeture<int> t1 = task([](){ return 12; });
		EXPECT_EQ(12, t1.get());

	}

	TEST(Operator, Seq) {

		int x = 3;

		// build a not-yet started sequential task
		auto ls = seq(
				task([&]{ x++; }),
				task([&]{ x*=2; }),
				task([&]{ x-=1; x*=2; })
		);

		// should not be executed yet
		EXPECT_EQ(3,x);

		// the treeture conversion should trigger the execution
		treeture<void> s = ls;
		EXPECT_EQ(14,x);

		// and the get should do nothing
		s.get();
		EXPECT_EQ(14,x);

	}

	TEST(Operation, Sum) {

		auto t = sum(done(4),done(8));
		EXPECT_EQ(12,t.get());

	}

	// --- benchmark ---

	const int N = 38;

	template<int x>
	struct c_fib {
		enum { value = c_fib<x-1>::value + c_fib<x-2>::value };
	};

	template<>
	struct c_fib<0> {
		enum { value = 0 };
	};

	template<>
	struct c_fib<1> {
		enum { value = 1 };
	};

	int s_fib(int x) {
		return (x <= 1) ? x : s_fib(x-1) + s_fib(x-2);
	}

	treeture_factory<int> gen_fib(int x) {
		auto fib = [](int x) {
			return make_lazy_treeture_factory_factory([=]() { return gen_fib(x); });
		};
		if (x <= 1) {
			return done(x);
		}
		return sum(fib(x-1),fib(x-2));
	}

	int p_fib(int x) {
		return gen_fib(x).get();
	}

	TEST(Benchmark,SeqFib) {
		EXPECT_EQ(c_fib<N>::value, s_fib(N));
	}

	TEST(Benchmark,ParFib) {
		EXPECT_EQ(c_fib<N>::value, p_fib(N));
	}

} // end namespace sequential
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
