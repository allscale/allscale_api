#include <gtest/gtest.h>

#include "allscale/utils/functional_utils.h"

namespace allscale {
namespace utils {

	using namespace detail;

	TEST(LambdaTrait, PureFunctionTypes) {
		// test pure function types
		EXPECT_TRUE(typeid(lambda_traits<int()>::result_type) == typeid(int));
		EXPECT_TRUE(lambda_traits<int()>::arity == 0);

		EXPECT_TRUE(typeid(lambda_traits<int(float)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int(float)>::argument_type) == typeid(float));
		EXPECT_TRUE(typeid(type_at<0, lambda_traits<int(float)>::argument_types>::type) == typeid(float));
		EXPECT_TRUE(lambda_traits<int(float)>::arity == 1);

		EXPECT_TRUE(typeid(lambda_traits<int(float, bool)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int(float, bool)>::arg1_type) == typeid(float));
		EXPECT_TRUE(typeid(lambda_traits<int(float, bool)>::arg2_type) == typeid(bool));
		EXPECT_TRUE(typeid(type_at<0, lambda_traits<int(float, bool)>::argument_types>::type) == typeid(float));
		EXPECT_TRUE(typeid(type_at<1, lambda_traits<int(float, bool)>::argument_types>::type) == typeid(bool));
		EXPECT_TRUE(lambda_traits<int(float, bool)>::arity == 2);
	}

	TEST(LambdaTrait, FunctionPointerTypes) {
		// test function pointers
		EXPECT_TRUE(typeid(lambda_traits<int (*)()>::result_type) == typeid(int));
		EXPECT_TRUE(lambda_traits<int (*)()>::arity == 0);

		EXPECT_TRUE(typeid(lambda_traits<int (*)(float, bool)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int (*)(float, bool)>::arg1_type) == typeid(float));
		EXPECT_TRUE(typeid(lambda_traits<int (*)(float, bool)>::arg2_type) == typeid(bool));
		EXPECT_TRUE(typeid(type_at<0, lambda_traits<int (*)(float, bool)>::argument_types>::type) == typeid(float));
		EXPECT_TRUE(typeid(type_at<1, lambda_traits<int (*)(float, bool)>::argument_types>::type) == typeid(bool));
		EXPECT_TRUE(lambda_traits<int (*)(float, bool)>::arity == 2);


		// test const function pointers
		EXPECT_TRUE(typeid(lambda_traits<int (*const)()>::result_type) == typeid(int));
		EXPECT_TRUE(lambda_traits<int (*const)()>::arity == 0);

		EXPECT_TRUE(typeid(lambda_traits<int (*const)(float, bool)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int (*const)(float, bool)>::arg1_type) == typeid(float));
		EXPECT_TRUE(typeid(lambda_traits<int (*const)(float, bool)>::arg2_type) == typeid(bool));
		EXPECT_TRUE(typeid(type_at<0, lambda_traits<int (*const)(float, bool)>::argument_types>::type) == typeid(float));
		EXPECT_TRUE(typeid(type_at<1, lambda_traits<int (*const)(float, bool)>::argument_types>::type) == typeid(bool));
		EXPECT_TRUE(lambda_traits<int (*const)(float, bool)>::arity == 2);
	}

	TEST(LambdaTrait, MemberFunctionPointerTypes) {
		class A {};

		// test member-function pointers
		EXPECT_TRUE(typeid(lambda_traits<int (A::*)()>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*)()>::class_type) == typeid(A));
		EXPECT_TRUE(lambda_traits<int (A::*)()>::arity == 0);

		EXPECT_TRUE(typeid(lambda_traits<int (A::*)(float, bool)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*)(float, bool)>::arg1_type) == typeid(float));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*)(float, bool)>::arg2_type) == typeid(bool));
		EXPECT_TRUE(typeid(type_at<0, lambda_traits<int (A::*)(float, bool)>::argument_types>::type) == typeid(float));
		EXPECT_TRUE(typeid(type_at<1, lambda_traits<int (A::*)(float, bool)>::argument_types>::type) == typeid(bool));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*)(float, bool)>::class_type) == typeid(A));
		EXPECT_TRUE(lambda_traits<int (A::*)(float, bool)>::arity == 2);


		// test const member-function pointers
		EXPECT_TRUE(typeid(lambda_traits<int (A::*const)()>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*const)()>::class_type) == typeid(A));
		EXPECT_TRUE(lambda_traits<int (A::*const)()>::arity == 0);

		EXPECT_TRUE(typeid(lambda_traits<int (A::*const)(float, bool)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*const)(float, bool)>::arg1_type) == typeid(float));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*const)(float, bool)>::arg2_type) == typeid(bool));
		EXPECT_TRUE(typeid(type_at<0, lambda_traits<int (A::*const)(float, bool)>::argument_types>::type) == typeid(float));
		EXPECT_TRUE(typeid(type_at<1, lambda_traits<int (A::*const)(float, bool)>::argument_types>::type) == typeid(bool));
		EXPECT_TRUE(typeid(lambda_traits<int (A::*const)(float, bool)>::class_type) == typeid(A));
		EXPECT_TRUE(lambda_traits<int (A::*const)(float, bool)>::arity == 2);
	}

	TEST(LambdaTrait, FunctionObjects) {
		// test function objects
		EXPECT_TRUE(typeid(decltype(&std::logical_not<bool>::operator())) == typeid(bool (std::logical_not<bool>::*)(const bool&) const));
		EXPECT_TRUE(typeid(lambda_traits<std::logical_not<bool>>::result_type) == typeid(bool));
		EXPECT_TRUE(typeid(lambda_traits<std::logical_not<bool>>::arg1_type) == typeid(bool));
		EXPECT_TRUE(typeid(lambda_traits<std::logical_not<bool>>::class_type) == typeid(std::logical_not<bool>));
		EXPECT_TRUE(lambda_traits<std::logical_not<bool>>::arity == 1);
	}

	TEST(LambdaTrait, Lambda) {
		// test lambdas
		auto lambda = [](int x) { return x; };
		EXPECT_TRUE(typeid(lambda_traits<decltype(lambda)>::result_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<decltype(lambda)>::arg1_type) == typeid(int));
		EXPECT_TRUE(typeid(lambda_traits<decltype(lambda)>::class_type) == typeid(decltype(lambda)));
		EXPECT_TRUE(lambda_traits<decltype(lambda)>::arity == 1);
	}

	TEST(LambdaTrait, GenericLambda) {

		auto fun1 = [](auto x){ return x; };
		EXPECT_EQ(1,lambda_traits<decltype(fun1)>::arity);

		auto fun2 = [](auto x, auto y){ return x+y; };
		EXPECT_EQ(2,lambda_traits<decltype(fun2)>::arity);

		auto fun3 = [](auto x, auto y, auto z){ return x+y+z; };
		EXPECT_EQ(3,lambda_traits<decltype(fun3)>::arity);

		auto funA = [](int x, auto y){ return x+y; };
		EXPECT_EQ(2,lambda_traits<decltype(funA)>::arity);

		auto funB = [](auto x, int y){ return x+y; };
		EXPECT_EQ(2,lambda_traits<decltype(funB)>::arity);

		auto funC = [](auto x, int y, auto z){ return x+y+z; };
		EXPECT_EQ(3,lambda_traits<decltype(funC)>::arity);

	}

} // end namespace utils
} // end namespace allscale
