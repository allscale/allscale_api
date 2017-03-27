#include <gtest/gtest.h>

#include "allscale/utils/type_list.h"

namespace allscale {
namespace utils {

	struct A {};
	struct B {};
	struct C {};


	TEST(TypeList,Basic) {

		EXPECT_EQ(0,(type_list<>::length));
		EXPECT_EQ(1,(type_list<A>::length));
		EXPECT_EQ(2,(type_list<A,B>::length));
		EXPECT_EQ(3,(type_list<A,B,C>::length));
		EXPECT_EQ(4,(type_list<A,B,C,A>::length));

		EXPECT_TRUE((type_list<>::empty));
		EXPECT_FALSE((type_list<A>::empty));
		EXPECT_FALSE((type_list<A,B>::empty));
		EXPECT_FALSE((type_list<A,B,C>::empty));

	}

	TEST(TypeList,Contains) {

		EXPECT_FALSE((type_list_contains<A,type_list<>>::value));
		EXPECT_FALSE((type_list_contains<B,type_list<>>::value));
		EXPECT_FALSE((type_list_contains<C,type_list<>>::value));

		EXPECT_TRUE((type_list_contains<A,type_list<A>>::value));
		EXPECT_FALSE((type_list_contains<B,type_list<A>>::value));
		EXPECT_FALSE((type_list_contains<C,type_list<A>>::value));

		EXPECT_TRUE((type_list_contains<A,type_list<A,B>>::value));
		EXPECT_TRUE((type_list_contains<B,type_list<A,B>>::value));
		EXPECT_FALSE((type_list_contains<C,type_list<A,B>>::value));

		EXPECT_TRUE((type_list_contains<A,type_list<A,B,C>>::value));
		EXPECT_TRUE((type_list_contains<B,type_list<A,B,C>>::value));
		EXPECT_TRUE((type_list_contains<C,type_list<A,B,C>>::value));

		EXPECT_TRUE((type_list_contains<A,type_list<A,B,C,A>>::value));
		EXPECT_TRUE((type_list_contains<B,type_list<A,B,C,A>>::value));
		EXPECT_TRUE((type_list_contains<C,type_list<A,B,C,A>>::value));


	}

	TEST(TypeList,TypeAt) {

		EXPECT_EQ(typeid(A),typeid(type_at<0,type_list<A>>::type));

		EXPECT_EQ(typeid(A),typeid(type_at<0,type_list<A,B>>::type));
		EXPECT_EQ(typeid(B),typeid(type_at<1,type_list<A,B>>::type));

		EXPECT_EQ(typeid(A),typeid(type_at<0,type_list<A,B,C>>::type));
		EXPECT_EQ(typeid(B),typeid(type_at<1,type_list<A,B,C>>::type));
		EXPECT_EQ(typeid(C),typeid(type_at<2,type_list<A,B,C>>::type));

		EXPECT_EQ(typeid(A),typeid(type_at<0,type_list<A,B,C,A>>::type));
		EXPECT_EQ(typeid(B),typeid(type_at<1,type_list<A,B,C,A>>::type));
		EXPECT_EQ(typeid(C),typeid(type_at<2,type_list<A,B,C,A>>::type));
		EXPECT_EQ(typeid(A),typeid(type_at<3,type_list<A,B,C,A>>::type));

	}


	TEST(TypeList,TypeIndex) {

		EXPECT_EQ(0,(type_index<A,type_list<A>>::value));

		EXPECT_EQ(0,(type_index<A,type_list<A,B>>::value));
		EXPECT_EQ(1,(type_index<B,type_list<A,B>>::value));

		EXPECT_EQ(0,(type_index<A,type_list<A,B,C>>::value));
		EXPECT_EQ(1,(type_index<B,type_list<A,B,C>>::value));
		EXPECT_EQ(2,(type_index<C,type_list<A,B,C>>::value));

		EXPECT_EQ(0,(type_index<A,type_list<A,B,C,A>>::value));
		EXPECT_EQ(1,(type_index<B,type_list<A,B,C,A>>::value));
		EXPECT_EQ(2,(type_index<C,type_list<A,B,C,A>>::value));
		EXPECT_EQ(0,(type_index<A,type_list<A,B,C,A>>::value));

	}


} // end namespace utils
} // end namespace allscale
