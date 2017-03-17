#include <gtest/gtest.h>

#include "allscale/api/user/data/scalar.h"
#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"
#include "allscale/utils/printer/pairs.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(ScalarRegion,TypeProperties) {

		using namespace detail;

		EXPECT_TRUE(utils::is_value<ScalarRegion>::value);
		EXPECT_TRUE(utils::is_serializable<ScalarRegion>::value);
		EXPECT_TRUE(core::is_region<ScalarRegion>::value);

	}

	TEST(ScalarFragment,TypeProperties) {

		using namespace detail;

		EXPECT_TRUE(core::is_fragment<ScalarFragment<int>>::value);

	}

	TEST(Scalar,TypeProperties) {

		using namespace detail;

		EXPECT_TRUE(core::is_data_item<Scalar<int>>::value);

	}

	TEST(Scalar,ExampleUse) {

		using namespace detail;

		ScalarRegion off = false;
		ScalarRegion on  = true;

		// create two fragments
		ScalarFragment<int> fA(core::no_shared_data(),on);
		ScalarFragment<int> fB(core::no_shared_data(),off);

		// write in first fragment
		Scalar<int> sA = fA.mask();
		sA.set(12);

		// check the value
		EXPECT_EQ(12,sA.get());
		EXPECT_EQ(12,const_cast<const Scalar<int>&>(sA).get());

		// -- move data to second fragment --

		fB.resize(on);
		fB.insert(fA,on);

		// write in first fragment
		Scalar<int> sB = fB.mask();
		EXPECT_EQ(12,sB.get());

	}

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
