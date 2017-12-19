#include <gtest/gtest.h>

#include "allscale/api/core/impl/reference/treeture.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	TEST(TaskDependencyManager, TypeProperties) {

		using Mgr = TaskDependencyManager<10>;

		EXPECT_TRUE(std::is_default_constructible<Mgr>::value);
		EXPECT_FALSE(std::is_trivially_constructible<Mgr>::value);
		EXPECT_FALSE(std::is_copy_constructible<Mgr>::value);
		EXPECT_FALSE(std::is_move_constructible<Mgr>::value);
		EXPECT_FALSE(std::is_copy_assignable<Mgr>::value);
		EXPECT_FALSE(std::is_move_assignable<Mgr>::value);

	}

	TEST(TaskFamily, TypeProperties) {

		EXPECT_TRUE(std::is_default_constructible<TaskFamily>::value);
		EXPECT_FALSE(std::is_trivially_constructible<TaskFamily>::value);
		EXPECT_FALSE(std::is_copy_constructible<TaskFamily>::value);
		EXPECT_FALSE(std::is_move_constructible<TaskFamily>::value);
		EXPECT_FALSE(std::is_copy_assignable<TaskFamily>::value);
		EXPECT_FALSE(std::is_move_assignable<TaskFamily>::value);

	}


	TEST(Treeture,Basic) {

		treeture<void> t1;
		t1.get();

		treeture<int> t2 = 12;
		EXPECT_EQ(12, t2.get());

	}

	TEST(Operator,Done) {

		treeture<void> t1;
		t1.get();

		treeture<int> t2 = done(12);
		EXPECT_EQ(12, t2.get());

	}


	TEST(Operator, Task) {

		treeture<int> t1 = spawn<false>([](){ return 12; });
		EXPECT_EQ(12, t1.get());

	}

	TEST(Operator, Sequential) {

		int x = 3;

		// build a not-yet started sequential tasks
		auto ls = seq(
				spawn<false>([&]{ x++; }),
				spawn<false>([&]{ x*=2; }),
				spawn<false>([&]{ x-=1; x*=2; })
		);

		// should not be executed yet
		EXPECT_EQ(3,x);

		// the treeture conversion should trigger the execution
		treeture<void> s = std::move(ls);
		s.wait();
		EXPECT_EQ(14,x);

		// and the get should do nothing
		s.get();
		EXPECT_EQ(14,x);

	}

	TEST(Operator, Parallel) {

		int x = 3;
		int y = 4;
		int z = 5;

		// build a not-yet started sequential tasks
		auto ls = par(
				spawn<false>([&]{ EXPECT_EQ(3,x); x++; }),
				spawn<false>([&]{ EXPECT_EQ(4,y); y++; }),
				spawn<false>([&]{ EXPECT_EQ(5,z); z++; })
		);

		// should not be executed yet
		EXPECT_EQ(3,x);
		EXPECT_EQ(4,y);
		EXPECT_EQ(5,z);

		// the treeture conversion should trigger the execution
		treeture<void> s = std::move(ls);

		// and we can wait for the result
		s.wait();
		EXPECT_EQ(4,x);
		EXPECT_EQ(5,y);
		EXPECT_EQ(6,z);
	}

	unreleased_treeture<int> sum(unreleased_treeture<int>&& a, unreleased_treeture<int>&& b) {
		return combine(std::move(a),std::move(b),[](int a, int b) { return a + b; });
	}


	TEST(Operation, Sum) {

		treeture<int> t = sum(done(4),done(8));
		EXPECT_EQ(12,t.get());

	}

	TEST(Treeture, FireAndForget) {

		// just trigger a task and forget about the result
		spawn<true>([]{}).release();

	}

	TEST(Treeture, Dependencies) {

		int x = 0;

		treeture<void> a = spawn<true>([&]{
			EXPECT_EQ(0,x);
			x++;
		});

		treeture<void> b = spawn<true>(after(a), [&]{
			EXPECT_EQ(1,x);
			x++;
		});

		treeture<void> c = spawn<true>(after(b), [&]{
			EXPECT_EQ(2,x);
			x++;
		});

		treeture<void> d = spawn<true>(after(a,b,c), [&]{
			EXPECT_EQ(3,x);
			x++;
		});

		d.get();
		EXPECT_EQ(4,x);
	}


	// --- benchmark ---

	const int N = 16;

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

	TEST(Benchmark,SeqFib) {
		EXPECT_EQ(c_fib<N>::value, s_fib(N));
	}

	unreleased_treeture<int> gen_fib_naive(int x) {
		if (x <= 1) {
			return done(x);
		}
		return sum(gen_fib_naive(x-1),gen_fib_naive(x-2));
	}

	int p_fib_naive(int x) {
		return gen_fib_naive(x).release().get();
	}

	TEST(Benchmark,ParFibNaive) {
		EXPECT_EQ(c_fib<N>::value, p_fib_naive(N));
	}

	unreleased_treeture<int> gen_fib(int x) {
		if (x <= 1) {
			return done(x);
		}
		return spawn<false>(
				[=](){ return s_fib(x); },
				[=](){ return sum(gen_fib(x-1),gen_fib(x-2)); }
		);
	}

	int p_fib(int x) {
		return gen_fib(x).release().get();
	}


	TEST(Benchmark,ParFib) {
		EXPECT_EQ(c_fib<N>::value, p_fib(N));
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
