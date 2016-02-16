#include <gtest/gtest.h>

#include "allscale/api/user/data/vector.h"

#include "allscale/utils/string_utils.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	TEST(Vector,Basic) {

		using Point = Vector<int,3>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values

		Point p0 = 0;
		EXPECT_EQ("[0,0,0]", toString(p0));

		Point p1 = 1;
		EXPECT_EQ("[1,1,1]", toString(p1));

		Point p2 = {1,2,3};
		EXPECT_EQ("[1,2,3]", toString(p2));


		// -- assignment --

		p1 = p2;
		EXPECT_EQ("[1,2,3]", toString(p1));
		EXPECT_EQ("[1,2,3]", toString(p2));


		// -- equality --
		EXPECT_EQ(p1,p2);
		EXPECT_NE(p0,p1);

		// -- addition --
		p1 = p1 + p2;
		EXPECT_EQ("[2,4,6]", toString(p1));

		p1 += p1;
		EXPECT_EQ("[4,8,12]", toString(p1));

		// -- subtraction --
		p1 = p1 - p2;
		EXPECT_EQ("[3,6,9]", toString(p1));

		p1 -= p2;
		EXPECT_EQ("[2,4,6]", toString(p1));


		// scalar multiplication
		p1 = p1 * 2;
		EXPECT_EQ("[4,8,12]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[8,16,24]", toString(p1));

	}

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
