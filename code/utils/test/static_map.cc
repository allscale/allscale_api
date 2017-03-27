#include <gtest/gtest.h>

#include "allscale/utils/static_map.h"
#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"

namespace allscale {
namespace utils {

	struct A {};
	struct B {};
	struct C {};

	TEST(StaticMap,Size) {

		EXPECT_LE(sizeof(StaticMap<keys<>,int>),4);
		EXPECT_EQ(sizeof(int)*1,sizeof(StaticMap<keys<A>,int>));
		EXPECT_EQ(sizeof(int)*2,sizeof(StaticMap<keys<A,B>,int>));
		EXPECT_EQ(sizeof(int)*3,sizeof(StaticMap<keys<A,B,C>,int>));

	}

	TEST(StaticMap,Basic) {

		StaticMap<keys<A,B,C>,int> map;

		EXPECT_EQ(sizeof(int)*3,sizeof(map));

		auto& valA = map.get<A>();
		auto& valB = map.get<B>();
		auto& valC = map.get<C>();

		valA = 12;
		valB = 14;
		valC = 16;

		EXPECT_EQ(12,map.get<A>());
		EXPECT_EQ(14,map.get<B>());
		EXPECT_EQ(16,map.get<C>());

		EXPECT_NE(&map.get<A>(),&map.get<B>());

	}

	TEST(StaticMap,Constructor) {

		StaticMap<keys<A,B,C>,int> map(12);

		EXPECT_EQ(sizeof(int)*3,sizeof(map));

		EXPECT_EQ(12,map.get<A>());
		EXPECT_EQ(12,map.get<B>());
		EXPECT_EQ(12,map.get<C>());

	}

	TEST(StaticMap,Iterators) {

		StaticMap<keys<A,B,C>,int> map(12);

		EXPECT_EQ(sizeof(int)*3,sizeof(map));

		auto& valA = map.get<A>();
		auto& valB = map.get<B>();
		auto& valC = map.get<C>();

		valA = 12;
		valB = 14;
		valC = 16;

		std::vector<int> res(map.begin(),map.end());
		EXPECT_EQ("[12,14,16]",toString(res));
	}

} // end namespace utils
} // end namespace allscale
