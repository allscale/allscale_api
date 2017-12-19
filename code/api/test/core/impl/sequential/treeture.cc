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

		treeture<int> t1 = spawn([](){ return 12; });
		EXPECT_EQ(12, t1.get());

	}

	TEST(Operator, Seq) {

		int x = 3;

		// build a not-yet started sequential tasks
		auto ls = seq(
				spawn([&]{ x++; }),
				spawn([&]{ x*=2; }),
				spawn([&]{ x-=1; x*=2; })
		);

		// should not be executed yet
		EXPECT_EQ(3,x);

		// the treeture conversion should trigger the execution
		treeture<void> s = std::move(ls);
		EXPECT_EQ(14,x);

		// and the get should do nothing
		s.get();
		EXPECT_EQ(14,x);

	}


	template<typename AA, typename BA>
	auto sum(lazy_unreleased_treeture<int,AA>&& a, lazy_unreleased_treeture<int,BA>&& b) {
		return combine(std::move(a),std::move(b),[](int a, int b) { return a+b; });
	}


	TEST(Operation, Sum) {

		auto t = sum(done(4),done(8));
		EXPECT_EQ(12,t.get());

	}

	TEST(Treeture, Dependencies) {

		int x = 0;

		treeture<void> a = spawn([&]{
			EXPECT_EQ(0,x);
			x++;
		});

		treeture<void> b = spawn(after(a), [&]{
			EXPECT_EQ(1,x);
			x++;
		});

		treeture<void> c = spawn(after(b), [&]{
			EXPECT_EQ(2,x);
			x++;
		});

		treeture<void> d = spawn(after(a,b,c), [&]{
			EXPECT_EQ(3,x);
			x++;
		});

		d.get();
		EXPECT_EQ(4,x);
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

	unreleased_treeture<int> gen_fib(int x) {
		auto fib = [](int x) {
			return make_lazy_unreleased_treeture([=]() { return gen_fib(x); });
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
