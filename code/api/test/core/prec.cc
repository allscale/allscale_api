#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "allscale/api/core/prec.h"
#include "allscale/api/user/arithmetic.h"

using namespace allscale::api::user;

namespace allscale {
namespace api {
namespace core {

	TEST(FunVariants, Basic) {
		auto x = make_fun_variants([](int i) { return i; });
		auto y = make_fun_variants([](int i) { return i + 1; }, [](int i) { return i + 2; });

		EXPECT_TRUE(is_fun_variants<decltype(x)>::value);
		EXPECT_TRUE(is_fun_variants<decltype(y)>::value);
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
						auto a = run(f(x-1));
						auto b = run(f(x-2));
						return done(a.get() + b.get());
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

	TEST(RecOps, FibEagerConvenience) {

		auto fib = prec(
				fun(
					[](int x)->bool { return x < 2; },
					[](int x)->int { return x; },
					[](int x, const auto& f) {
						auto a = run(f(x-1));
						auto b = run(f(x-2));
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
					return done(f(x-1).get() + f(x-2).get());
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


	TEST(RecOps, FibShortLazyAlternative) {

		auto fib = prec(
				[](int x)->bool { return x < 2; },
				[](int x)->int { return fib_seq(x); },
				pick(
					[](int x, const auto& f) {
						return add( f(x-1), f(x-2) );
					},
					[](int x, const auto& f) {
						return add( f(x-2), f(x-1) );
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

	TEST(RecOps, Fill) {

		std::array<int,20> data;

		data.fill(0);

		auto fill = prec(
				[](int x) { return x <= 0; },
				[](int) { },
				[&](int x, const auto& nested) {
					data[x-1] = 12;
					return nested(x-1);
				}
		);

		for(const auto& cur : data) {
			EXPECT_EQ(0,cur);
		}

		fill(20).get();

		for(const auto& cur : data) {
			EXPECT_EQ(12,cur);
		}

	}

	// test support for sequential

	TEST(Treeture, Sequential) {

		int x = 0;

		auto op2 = prec(
				[](int p) { return p < 10; },
				[&](int p) {
					EXPECT_EQ(p,x);
					x++;
				},
				[](int, auto& rec) {
					return sequential(
						rec(0), rec(1)
					);
				}
		);

		EXPECT_EQ(0,x);
		op2(10).get();
		EXPECT_EQ(2,x);

		x = 0;

		auto op3 = prec(
				[](int p) { return p < 10; },
				[&](int p) {
					EXPECT_EQ(p,x);
					x++;
				},
				[](int, auto& rec) {
					return sequential(
						rec(0), rec(1), rec(2)
					);
				}
		);

		EXPECT_EQ(0,x);
		op3(10).get();
		EXPECT_EQ(3,x);

		x = 0;

		auto op4 = prec(
				[](int p) { return p < 10; },
				[&](int p) {
					EXPECT_EQ(p,x);
					x++;
				},
				[](int, auto& rec) {
					return sequential(
						rec(0), rec(1), rec(2), rec(3)
					);
				}
		);

		EXPECT_EQ(0,x);
		op4(10).get();
		EXPECT_EQ(4,x);

	}


	TEST(Treeture, Parallel) {

		std::atomic<int> x;

		x = 0;

		auto op2 = prec(
				[](int p) { return p < 10; },
				[&](int) {
					x++;
				},
				[](int, auto& rec) {
					return parallel(
						rec(0), rec(1)
					);
				}
		);

		EXPECT_EQ(0,x);
		op2(10).get();
		EXPECT_EQ(2,x);

		x = 0;

		auto op3 = prec(
				[](int p) { return p < 10; },
				[&](int) {
					x++;
				},
				[](int, auto& rec) {
					return parallel(
						rec(0), rec(1), rec(2)
					);
				}
		);

		EXPECT_EQ(0,x);
		op3(10).get();
		EXPECT_EQ(3,x);

		x = 0;

		auto op4 = prec(
				[](int p) { return p < 10; },
				[&](int) {
					x++;
				},
				[](int, auto& rec) {
					return parallel(
						rec(0), rec(1), rec(2), rec(3)
					);
				}
		);

		EXPECT_EQ(0,x);
		op4(10).get();
		EXPECT_EQ(4,x);

	}


	// test support for move-only result values

	struct MoveOnly {

		int x;

		MoveOnly() {}
		MoveOnly(int x) : x(x) {}

		MoveOnly(const MoveOnly&) = delete;
		MoveOnly(MoveOnly&&) = default;

		MoveOnly& operator=(const MoveOnly&) = delete;
		MoveOnly& operator=(MoveOnly&&) = default;

	};

	TEST(Prec,MoveOnlySupport) {

		// test properties of result type

		EXPECT_FALSE(std::is_copy_constructible<MoveOnly>::value);
		EXPECT_FALSE(std::is_copy_assignable<MoveOnly>::value);

		EXPECT_TRUE(std::is_move_constructible<MoveOnly>::value);
		EXPECT_TRUE(std::is_move_assignable<MoveOnly>::value);

		auto op = prec(
			[](int p) { return p == 0; },
			[&](int) {
				return MoveOnly(0);
			},
			[](int p, auto& rec) {
				return MoveOnly(rec(p-1).get().x+1);
			}
		);

		// test that the no-movable result type is supported
		EXPECT_EQ(2, op(2).get().x);
		EXPECT_EQ(4, op(4).get().x);

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

	static const int N = 40;

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


	template<typename Iter, typename Filter, typename Map, typename Reduce>
	auto reduceIf(const Iter& a, const Iter& b, const Filter& filter, const Map& map, const Reduce& reduce) {
		using treeture_type = typename utils::lambda_traits<Map>::result_type::treeture_type;
		using result_type = typename utils::lambda_traits<Reduce>::result_type;

		// check that the interval is not empty
		if (a>b) return result_type(0);

		// spawn tasks and collect without heap allocation
		assert_lt(b-a,32);
		std::bitset<32> mask;
		std::array<treeture_type,32> tasks;
		std::size_t j = 0;
		for(Iter i = a; i<b; ++i,++j) {
			if (!filter(i)) continue;
			mask.set(j);
			tasks[j] = map(i);
		}

		// collect results
		result_type res = 0;
		for(std::size_t j = 0; j < std::size_t(b-a); ++j) {
			if (!mask.test(j)) continue;
			res = reduce(res,std::move(tasks[j]).get());
		}
		return res;
	}

	template<typename Iter, typename Filter, typename Map>
	auto sumIf(const Iter& a, const Iter& b, const Filter& filter, const Map& map) {
		return reduceIf(a,b,filter,map,std::plus<int>());
	}

	int nqueens(int size) {

		struct Assignment {

			int column;				// < the number of columns assigned so far
			int row;				// < the row this queen is placed
			const Assignment* rest;	// < the rest of this assignment

			Assignment()
				: column(-1), row(0), rest(nullptr) {}

			Assignment(int row, const Assignment& rest)
				: column(rest.column+1), row(row), rest(&rest) {}

			int size() const {
				return column + 1;
			}

			bool valid(int r) const {
				return valid(r,column+1);
			}

			bool valid(int r, int c) const {
				assert_lt(column,c);
				// check end of assignment
				if (column<0) return true;
				// if in same row => fail
				if (row == r) return false;
				// if on same diagonal => fail
				auto diff = c - column;
				if (row + diff == r || row - diff == r) return false;
				// check nested
				return rest->valid(r,c);
			}
		};

		// create the recursive version
		auto compute = prec(
			[size](const Assignment& args) {
				// check whether the assignment is complete
				return args.size() >= size;
			},
			[](const Assignment&) {
				// if a complete assignment is reached, we have a solution
				return 1;
			},
			[size](const Assignment& a, const auto& rec) {
				return sumIf(0,size,
					[&](int i){ return a.valid(i); },
					[&](int i){ return rec(Assignment(i,a)); }
				);
			}
		);

		// compute the result
		return compute(Assignment()).get();
	}


	TEST(RecOps,NQueens) {

		EXPECT_EQ(     1,nqueens( 1));
		EXPECT_EQ(     0,nqueens( 2));
		EXPECT_EQ(     0,nqueens( 3));
		EXPECT_EQ(     2,nqueens( 4));
		EXPECT_EQ(    10,nqueens( 5));
		EXPECT_EQ(     4,nqueens( 6));
		EXPECT_EQ(    40,nqueens( 7));
		EXPECT_EQ(    92,nqueens( 8));
		EXPECT_EQ(   352,nqueens( 9));
		EXPECT_EQ(   724,nqueens(10));
		EXPECT_EQ(  2680,nqueens(11));
		EXPECT_EQ( 14200,nqueens(12));
		EXPECT_EQ( 73712,nqueens(13));
		EXPECT_EQ(365596,nqueens(14));

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
