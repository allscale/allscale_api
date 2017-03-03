#include <gtest/gtest.h>

#include "allscale/utils/static_map.h"

namespace allscale {
namespace utils {

	TEST(StaticMap,Basic) {

		struct A {};
		struct B {};
		struct C {};

		using Keys = keys<A,B,C>;

		EXPECT_EQ(1,sizeof(StaticMap<keys<>,int>));
		EXPECT_EQ(sizeof(int)*1,sizeof(StaticMap<keys<A>,int>));
		EXPECT_EQ(sizeof(int)*2,sizeof(StaticMap<keys<A,B>,int>));
		EXPECT_EQ(sizeof(int)*3,sizeof(StaticMap<keys<A,B,C>,int>));

		StaticMap<Keys,int> map;

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

} // end namespace utils
} // end namespace allscale
