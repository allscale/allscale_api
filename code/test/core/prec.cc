#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "allscale/api/core/prec.h"

namespace allscale {
namespace api {
namespace core {

	TEST(PickRandom, SimpleTest) {

		std::vector<int> data;
		std::srand(1);
		for(int i =0; i<20; i++) {
			data.push_back(detail::pickRandom(1,2,3,4,5));
		}

		EXPECT_EQ(std::vector<int>({5,2,3,4,5,2,2,2,2,2,1,2,4,5,5,3,4,3,1,4}), data);

	}

	TEST(RecOps, IsFunDef) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		EXPECT_FALSE(utils::is_vector<empty>::value);

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->float { return 0.0; },
				[](empty, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(g)>::value);
	}

	TEST(RecOps, IsFunDefGeneric) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->float { return 0.0; },
				[](empty, const auto&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(g)>::value);
	}

	TEST(RecOps, IsFunDefLazy) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const auto&)->treeture<float> { return 1.0f; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		EXPECT_FALSE(utils::is_vector<empty>::value);

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->treeture<float> { return 0.0f; },
				[](empty, const auto&)->treeture<float> { return 1.0f; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(g)>::value);
	}

	TEST(RecOps, IsRecDef) {
		EXPECT_FALSE(detail::is_rec_def<int>::value);

		EXPECT_TRUE((detail::is_rec_def<rec_defs<int,int>>::value));
		EXPECT_TRUE((detail::is_rec_def<const rec_defs<int,int>>::value));
	}


	int fib_seq(int x) {
		if (x < 2) return x;
		return fib_seq(x-1) + fib_seq(x-2);
	}

	TEST(RecOps, FibEager) {

		auto fib = prec(
				fun(
					[](int x)->bool { return x < 2; },
					[](int x)->int { return x; },
					[](int x, const auto& f) {
						auto a = f(x-1);
						auto b = f(x-2);
						return a.get() + b.get();
					}
				)
		);

		EXPECT_EQ( 0,fib(0).get());
		EXPECT_EQ( 1,fib(1).get());
		EXPECT_EQ( 1,fib(2).get());
		EXPECT_EQ( 2,fib(3).get());
		EXPECT_EQ( 3,fib(4).get());
		EXPECT_EQ( 5,fib(5).get());
		EXPECT_EQ( 8,fib(6).get());
		EXPECT_EQ(13,fib(7).get());
		EXPECT_EQ(21,fib(8).get());
		EXPECT_EQ(34,fib(9).get());

	}

	TEST(RecOps, FibLazy) {

		auto fib = prec(
				fun(
					[](int x)->bool { return x < 2; },
					[](int x)->int { return x; },
					[](int x, const auto& f) {
						return add(f(x-1),f(x-2));
					}
				)
		);

		EXPECT_EQ( 0,fib(0).get());
		EXPECT_EQ( 1,fib(1).get());
		EXPECT_EQ( 1,fib(2).get());
		EXPECT_EQ( 2,fib(3).get());
		EXPECT_EQ( 3,fib(4).get());
		EXPECT_EQ( 5,fib(5).get());
		EXPECT_EQ( 8,fib(6).get());
		EXPECT_EQ(13,fib(7).get());
		EXPECT_EQ(21,fib(8).get());
		EXPECT_EQ(34,fib(9).get());

	}

	TEST(RecOps, FibShort) {

		auto fib = prec(
				[](int x)->bool { return x < 2; },
				[](int x)->int { return x; },
				[](int x, const auto& f) {
					auto a = f(x-1);
					auto b = f(x-2);
					return a.get() + b.get();
				}
		);

		EXPECT_EQ( 0,fib(0).get());
		EXPECT_EQ( 1,fib(1).get());
		EXPECT_EQ( 1,fib(2).get());
		EXPECT_EQ( 2,fib(3).get());
		EXPECT_EQ( 3,fib(4).get());
		EXPECT_EQ( 5,fib(5).get());
		EXPECT_EQ( 8,fib(6).get());
		EXPECT_EQ(13,fib(7).get());
		EXPECT_EQ(21,fib(8).get());
		EXPECT_EQ(34,fib(9).get());

	}

	TEST(RecOps, FibShortLazy) {

		auto fib = prec(
				[](int x)->bool { return x < 2; },
				[](int x)->int { return fib_seq(x); },
				[](int x, const auto& f) {
					return add( f(x-1), f(x-2) );
				}
		);

		EXPECT_EQ( 0,fib(0).get());
		EXPECT_EQ( 1,fib(1).get());
		EXPECT_EQ( 1,fib(2).get());
		EXPECT_EQ( 2,fib(3).get());
		EXPECT_EQ( 3,fib(4).get());
		EXPECT_EQ( 5,fib(5).get());
		EXPECT_EQ( 8,fib(6).get());
		EXPECT_EQ(13,fib(7).get());
		EXPECT_EQ(21,fib(8).get());
		EXPECT_EQ(34,fib(9).get());

	}


	TEST(RecOps, MultipleRecursion) {

		auto def = group(
				// function A
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->int { return 1; },
						[](int, const auto& A, const auto& B, const auto& C)->int {
							EXPECT_EQ(1,A(0).get());
							EXPECT_EQ(2,B(0).get());
							EXPECT_EQ(3,C(0).get());
							return 1;
						}
				),
				// function B
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->int { return 2; },
						[](int, const auto& A, const auto& B, const auto& C)->int {
							EXPECT_EQ(1,A(0).get());
							EXPECT_EQ(2,B(0).get());
							EXPECT_EQ(3,C(0).get());
							return 2;
						}
				),
				// function C
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->int { return 3; },
						[](int, const auto& A, const auto& B, const auto& C)->int {
							EXPECT_EQ(1,A(0).get());
							EXPECT_EQ(2,B(0).get());
							EXPECT_EQ(3,C(0).get());
							return 3;
						}
				)
		);

		auto A = prec<0>(def);
		auto B = prec<1>(def);
		auto C = prec<2>(def);

		EXPECT_EQ(1,A(1).get());
		EXPECT_EQ(2,B(1).get());
		EXPECT_EQ(3,C(1).get());
	}

	TEST(RecOps, MultipleRecursionMultipleTypes) {

		struct A { int x; A(int x=0):x(x){}; };
		struct B { int x; B(int x=0):x(x){}; };
		struct C { int x; C(int x=0):x(x){}; };
		struct D { int x; D(int x=0):x(x){}; };

		auto def = group(
				// function A
				fun(
						[](A x)->bool { return x.x==0; },
						[](A)->int { return 1; },
						[](A, const auto& a, const auto& b, const auto& c, const auto& d)->int {
							EXPECT_EQ(1,a(A()).get());
							EXPECT_EQ(2,b(B()).get());
							EXPECT_EQ(3,c(C()).get());
							EXPECT_EQ(4,d(D()).get());
							return 1;
						}
				),
				// function B
				fun(
						[](B x)->bool { return x.x==0; },
						[](B)->int { return 2; },
						[](B, const auto& a, const auto& b, const auto& c, const auto& d)->int {
							EXPECT_EQ(1,a(A()).get());
							EXPECT_EQ(2,b(B()).get());
							EXPECT_EQ(3,c(C()).get());
							EXPECT_EQ(4,d(D()).get());
							return 2;
						}
				),
				// function C
				fun(
						[](C x)->bool { return x.x==0; },
						[](C)->int { return 3; },
						[](C, const auto& a, const auto& b, const auto& c, const auto& d)->int {
							EXPECT_EQ(1,a(A()).get());
							EXPECT_EQ(2,b(B()).get());
							EXPECT_EQ(3,c(C()).get());
							EXPECT_EQ(4,d(D()).get());
							return 3;
						}
				)
				,
				// function D
				fun(
						[](D x)->bool { return x.x==0; },
						[](D)->int { return 4; },
						[](D, const auto& a, const auto& b, const auto& c, const auto& d)->int {
							EXPECT_EQ(1,a(A()).get());
							EXPECT_EQ(2,b(B()).get());
							EXPECT_EQ(3,c(C()).get());
							EXPECT_EQ(4,d(D()).get());
							return 4;
						}
				)
		);

		auto a = prec<0>(def);
		auto b = prec<1>(def);
		auto c = prec<2>(def);
		auto d = prec<3>(def);

		EXPECT_EQ(1,a(A(1)).get());
		EXPECT_EQ(2,b(B(1)).get());
		EXPECT_EQ(3,c(C(1)).get());
		EXPECT_EQ(4,d(D(1)).get());
	}

	TEST(RecOps, EvenOdd) {

		auto def = group(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return true; },
						[](int x, const auto& , const auto& odd)->bool {
							return odd(x-1).get();
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return false; },
						[](int x, const auto& even, const auto& )->bool {
							return even(x-1).get();
						}
				)
		);

		auto even = prec<0>(def);
		auto odd = prec<1>(def);

		EXPECT_TRUE(even(0).get());
		EXPECT_TRUE(even(2).get());
		EXPECT_TRUE(even(4).get());
		EXPECT_TRUE(even(6).get());
		EXPECT_TRUE(even(8).get());

		EXPECT_FALSE(even(1).get());
		EXPECT_FALSE(even(3).get());
		EXPECT_FALSE(even(5).get());
		EXPECT_FALSE(even(7).get());
		EXPECT_FALSE(even(9).get());

		EXPECT_FALSE(odd(0).get());
		EXPECT_FALSE(odd(2).get());
		EXPECT_FALSE(odd(4).get());
		EXPECT_FALSE(odd(6).get());
		EXPECT_FALSE(odd(8).get());

		EXPECT_TRUE(odd(1).get());
		EXPECT_TRUE(odd(3).get());
		EXPECT_TRUE(odd(5).get());
		EXPECT_TRUE(odd(7).get());
		EXPECT_TRUE(odd(9).get());

	}

	TEST(RecOps, EvenOddLazy) {

		auto def = group(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int x)->bool { return x%2 == 0; },
						[](int x, const auto& , const auto& odd) {
							return odd(x-1);
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int x)->bool { return x%2 == 1; },
						[](int x, const auto& even, const auto& ) {
							return even(x-1);
						}
				)
		);

		auto even = prec<0>(def);
		auto odd = prec<1>(def);

		EXPECT_TRUE(even(0).get());
		EXPECT_TRUE(even(2).get());
		EXPECT_TRUE(even(4).get());
		EXPECT_TRUE(even(6).get());
		EXPECT_TRUE(even(8).get());

		EXPECT_FALSE(even(1).get());
		EXPECT_FALSE(even(3).get());
		EXPECT_FALSE(even(5).get());
		EXPECT_FALSE(even(7).get());
		EXPECT_FALSE(even(9).get());

		EXPECT_FALSE(odd(0).get());
		EXPECT_FALSE(odd(2).get());
		EXPECT_FALSE(odd(4).get());
		EXPECT_FALSE(odd(6).get());
		EXPECT_FALSE(odd(8).get());

		EXPECT_TRUE(odd(1).get());
		EXPECT_TRUE(odd(3).get());
		EXPECT_TRUE(odd(5).get());
		EXPECT_TRUE(odd(7).get());
		EXPECT_TRUE(odd(9).get());

	}

	TEST(RecOps, Even) {

		auto even = prec(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return true; },
						[](int x, const auto& , const auto& odd)->bool {
							return odd(x-1).get();
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return false; },
						[](int x, const auto& even, const auto& )->bool {
							return even(x-1).get();
						}
				)
		);

		EXPECT_TRUE(even(0).get());
		EXPECT_TRUE(even(2).get());
		EXPECT_TRUE(even(4).get());
		EXPECT_TRUE(even(6).get());
		EXPECT_TRUE(even(8).get());

		EXPECT_FALSE(even(1).get());
		EXPECT_FALSE(even(3).get());
		EXPECT_FALSE(even(5).get());
		EXPECT_FALSE(even(7).get());
		EXPECT_FALSE(even(9).get());

	}



	int fib(int x) {
		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { return x; },
					pick(
							[](int x, const auto& f) { return add(f(x-1), f(x-2)); },
							[](int x, const auto& f) { return add(f(x-2), f(x-1)); }
					)
				)
		)(x).get();
	}

	int fac(int x) {
		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { int res =1; for(int i=1; i<=x; ++i) { res*=i; }; return res; },
					[](int x, const auto& f) { return x * f(x-1).get(); }
				)
		)(x).get();
	}

	TEST(RecOps, SimpleTest) {

		EXPECT_EQ(0, fib(0));
		EXPECT_EQ(1, fib(1));
		EXPECT_EQ(1, fib(2));
		EXPECT_EQ(2, fib(3));
		EXPECT_EQ(3, fib(4));
		EXPECT_EQ(5, fib(5));
		EXPECT_EQ(8, fib(6));

		EXPECT_EQ(1, fac(1));
		EXPECT_EQ(2, fac(2));
		EXPECT_EQ(6, fac(3));
		EXPECT_EQ(24, fac(4));

	}

	// ---- application tests --------

	int pfib(int x) {
		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { return x; },
					[](int x, const auto& f) {
						return add(f(x-1),f(x-2));
					}
				)
		)(x).get();
	}

	TEST(RecOps, ParallelTest) {

		EXPECT_EQ(6765, pfib(20));
		EXPECT_EQ(46368, pfib(24));

	}


	// --- check stack memory usage ---

	struct big_params {
		int a[500];
		int x;
		big_params(int x) : x(x) {};
	};

	int sum_seq(big_params p) {
		if (p.x == 0) return 0;
		return sum_seq(p.x-1) + p.x;
	}


	TEST(DISABLED_RecOps, RecursionDepth) {


		auto sum = prec(
				[](big_params p) { return p.x == 0; },
				[](big_params) { return 0; },
				[](big_params p, const auto& rec) {
					return rec(p.x-1).get() + p.x;
				}
		);

		EXPECT_EQ(55,sum(10).get());
		int N = 2068;
		sum_seq(N);
		sum(N).get();

	}


	template<unsigned N>
	struct static_fib {
		enum { value = static_fib<N-1>::value + static_fib<N-2>::value };
	};

	template<>
	struct static_fib<1> {
		enum { value = 1 };
	};

	template<>
	struct static_fib<0> {
		enum { value = 0 };
	};

	int sfib(int x) {
		return (x<2) ? x : sfib(x-1) + sfib(x-2);
	}

	static const int N = 10;

	TEST(ScalingTest, StaticFib) {
		// this should not take any time
		EXPECT_LT(0, static_fib<N>::value);
	}

	TEST(ScalingTest, SequentialFib) {
		EXPECT_EQ(static_fib<N>::value, sfib(N));
	}

	TEST(ScalingTest, ParallelFib) {
		EXPECT_EQ(static_fib<N>::value, pfib(N));
	}


	TEST(DISABLED_WorkerSleepTest, StopAndGo) {
		// Unfortunately, I don't know a simple, portable way to check the
		// actual number of workers -- so this one must be inspected manually
		const int N = 45;
		EXPECT_EQ(static_fib<N>::value, pfib(N));
		EXPECT_EQ(static_fib<N>::value, sfib(N));
		EXPECT_EQ(static_fib<N>::value, pfib(N));

	}


} // end namespace core
} // end namespace api
} // end namespace allscale
