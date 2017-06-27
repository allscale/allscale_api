#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <set>
#include <map>

#include "allscale/api/core/data.h"
#include "allscale/api/user/data/map.h"

namespace allscale {
namespace api {
namespace core {


	TEST(Region, IsRegionTest) {

		// some negative tests
		EXPECT_FALSE(is_region<int>::value);
		EXPECT_FALSE(is_region<std::string>::value);

		// some positive test
		EXPECT_TRUE(is_region<user::data::SetRegion<int>>::value);

	}


	TEST(Fragment, IsFragmentTest) {

		// some negative tests
		EXPECT_FALSE(is_fragment<int>::value);
		EXPECT_FALSE(is_fragment<std::string>::value);
		EXPECT_FALSE(is_fragment<user::data::SetRegion<int>>::value);

		// some positive test
		EXPECT_TRUE((is_fragment<user::data::MapFragment<int,int>>::value));
	}

	struct no_serializable {};

	TEST(SharedData, IsSharedData) {

		// some negative tests
		EXPECT_FALSE(is_shared_data<no_serializable>::value);

		// some positive test
		EXPECT_TRUE(is_shared_data<no_shared_data>::value);

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
