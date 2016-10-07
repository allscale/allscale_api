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

	TEST(RecOps, Functions) {
		auto inc = toFunction([](int x) { return x + 1; });
		EXPECT_EQ(3,inc(2));

		struct empty {};
		auto f = toFunction([](empty) { return 12; });
		EXPECT_EQ(12, f(empty()));
	}

	TEST(RecOps, IsFunDef) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const prec_fun<float(int)>::type&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		EXPECT_FALSE(utils::is_vector<empty>::value);

		typedef const prec_fun<float(empty)>::type& fun_type;

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->float { return 0.0; },
				[](empty, fun_type&)->float { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(g)>::value);
	}

	TEST(RecOps, CallFunDef) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->int { return 12; },
				[](int, const prec_fun<int(int)>::type&)->Future<int> { return done(14); }
		);

		auto g = [=](int) {
			return 0.0;
		};

		EXPECT_EQ(12, f(2,g).get());
	}

	TEST(RecOps, IsFunDefLazy) {

		auto a = [](){return false;};
		EXPECT_FALSE(detail::is_fun_def<decltype(a)>::value);

		auto f = fun(
				[](int)->bool { return true; },
				[=](int)->float { return 0.0; },
				[](int, const prec_fun<float(int)>::type&)->Future<float> { return 1.0; }
		);

		EXPECT_TRUE(detail::is_fun_def<decltype(f)>::value);

		struct empty {};

		EXPECT_FALSE(utils::is_vector<empty>::value);

		typedef prec_fun<float(empty)>::type& fun_type;

		auto g = fun(
				[](empty)->bool { return true; },
				[=](empty)->Future<float> { return 0.0; },
				[](empty, fun_type&)->Future<float> { return 1.0; }
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

	TEST(RecOps, Fib) {

		auto fib = prec(
				fun(
					[](int x)->bool { return x < 2; },
					[](int x)->int { return fib_seq(x); },
					[](int x, const typename prec_fun<int(int)>::type& f)->int {
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
					[](int x)->int { return fib_seq(x); },
					[](int x, const typename prec_fun<int(int)>::type& f)->Future<int> {
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
//
	TEST(RecOps, FibShort) {

		auto fib = prec(
				[](int x)->bool { return x < 2; },
				[](int x)->int { return fib_seq(x); },
				[](int x, const typename prec_fun<int(int)>::type& f)->int {
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
				[](int x, const typename prec_fun<int(int)>::type& f)->Future<int> {
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


	TEST(RecOps, EvenOdd) {

		typedef typename prec_fun<bool(int)>::type test;

		auto def = group(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return true; },
						[](int x, const test& , const test& odd)->bool {
							return odd(x-1).get();
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return false; },
						[](int x, const test& even, const test& )->bool {
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

		typedef typename prec_fun<bool(int)>::type test;

		auto def = group(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int x)->bool { return x%2 == 0; },
						[](int x, const test& , const test& odd)->Future<bool> {
							return odd(x-1);
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int x)->bool { return x%2 == 1; },
						[](int x, const test& even, const test& )->Future<bool> {
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

		typedef typename prec_fun<bool(int)>::type test;

		auto even = prec(
				// even
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return true; },
						[](int x, const test& , const test& odd)->bool {
							return odd(x-1).get();
						}
				),
				// odd
				fun(
						[](int x)->bool { return x == 0; },
						[](int)->bool { return false; },
						[](int x, const test& even, const test& )->bool {
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
		typedef typename std::function<Future<int>(int)> fun_type;

		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { return fib_seq(x); },
					pick(
							[](int x, const fun_type& f) { return add(f(x-1), f(x-2)); },
							[](int x, const fun_type& f) { return add(f(x-2), f(x-1)); }
					)
				)
		)(x).get();
	}

	int fac(int x) {
		typedef typename std::function<Future<int>(int)> fun_type;

		return prec(
				fun(
					[](int x) { return x < 2; },
					[](int x) { int res =1; for(int i=1; i<=x; ++i) { res*=i; }; return res; },
					[](int x, const fun_type& f) { return x * f(x-1).get(); }
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
					[](int x) { return fib_seq(x); },
					[](int x, const typename prec_fun<int(int)>::type& f)->Future<int> {
						return add(f(x-1),f(x-2));
					}
				)
		)(x).get();
	}

	TEST(RecOps, ParallelTest) {

		EXPECT_EQ(6765, pfib(20));
		EXPECT_EQ(46368, pfib(24));

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
