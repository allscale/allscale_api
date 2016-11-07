#include <gtest/gtest.h>

#include "allscale/api/core/impl/reference/treeture.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

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

		treeture<int> t1 = spawn([](){ return 12; });
		EXPECT_EQ(12, t1.get());

	}

	TEST(Operator, Sequential) {

		int x = 3;

		// build a not-yet started sequential tasks
		auto ls = sequential(
				spawn([&]{ x++; }),
				spawn([&]{ x*=2; }),
				spawn([&]{ x-=1; x*=2; })
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
		auto ls = parallel(
				spawn([&]{ EXPECT_EQ(3,x); x++; }),
				spawn([&]{ EXPECT_EQ(4,y); y++; }),
				spawn([&]{ EXPECT_EQ(5,z); z++; })
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

	const int N = 20;

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
		if (x <= 1) {
			return done(x);
		}
		return sum(gen_fib(x-1),gen_fib(x-2));
	}

	int p_fib(int x) {
		return gen_fib(x).release().get();
	}

	TEST(Benchmark,SeqFib) {
		EXPECT_EQ(c_fib<N>::value, s_fib(N));
	}

	TEST(Benchmark,ParFib) {
		EXPECT_EQ(c_fib<N>::value, p_fib(N));
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
