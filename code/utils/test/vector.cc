#include <gtest/gtest.h>

#include "allscale/utils/string_utils.h"
#include "allscale/utils/vector.h"

namespace allscale {
namespace utils {


	TEST(Vector, Vector2DLayout) {

		using Point = Vector<int, 2>;

		Point p;

		ASSERT_EQ(&(p[0]), &(p.x));
		ASSERT_EQ(&(p[1]), &(p.y));

		p[0] = 1;
		p[1] = 2;

		Point p2 = p;

		EXPECT_EQ(1, p.x);
		EXPECT_EQ(2, p.y);

		EXPECT_EQ(1, p2.x);
		EXPECT_EQ(2, p2.y);
	}

	TEST(Vector, Vector3DLayout) {

		using Point = Vector<int, 3>;

		Point p;

		ASSERT_EQ(&(p[0]), &(p.x));
		ASSERT_EQ(&(p[1]), &(p.y));
		ASSERT_EQ(&(p[2]), &(p.z));

		p[0] = 1;
		p[1] = 2;
		p[2] = 3;

		Point p2 = p;

		EXPECT_EQ(1, p.x);
		EXPECT_EQ(2, p.y);
		EXPECT_EQ(3, p.z);

		EXPECT_EQ(1, p2.x);
		EXPECT_EQ(2, p2.y);
		EXPECT_EQ(3, p2.z);
	}

	TEST(Vector, Basic2D) {

		using Point = Vector<int, 2>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values
		(void)p;

		Point p0 = 0;
		EXPECT_EQ("[0,0]", toString(p0));

		Point p1 = 1;
		EXPECT_EQ("[1,1]", toString(p1));

		Point p2 = { 1,2 };
		EXPECT_EQ("[1,2]", toString(p2));


		// -- assignment --

		p1 = p2;
		EXPECT_EQ("[1,2]", toString(p1));
		EXPECT_EQ("[1,2]", toString(p2));


		// -- equality --
		EXPECT_EQ(p1, p2);
		EXPECT_NE(p0, p1);

		// -- addition --
		p1 = p1 + p2;
		EXPECT_EQ("[2,4]", toString(p1));

		p1 += p1;
		EXPECT_EQ("[4,8]", toString(p1));

		// -- subtraction --
		p1 = p1 - p2;
		EXPECT_EQ("[3,6]", toString(p1));

		p1 -= p2;
		EXPECT_EQ("[2,4]", toString(p1));


		// scalar multiplication
		p1 = p1 * 2;
		EXPECT_EQ("[4,8]", toString(p1));
		p1 = 2 * p1;
		EXPECT_EQ("[8,16]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[16,32]", toString(p1));

		// scalar division
		p1 = p1 / 2;
		EXPECT_EQ("[8,16]", toString(p1));
		p1 /= 2;
		EXPECT_EQ("[4,8]", toString(p1));

	}

	TEST(Vector, Basic3D) {

		using Point = Vector<int,3>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values
		(void)p;

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
		p1 = 2 * p1;
		EXPECT_EQ("[8,16,24]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[16,32,48]", toString(p1));

		// scalar division
		p1 = p1 / 2;
		EXPECT_EQ("[8,16,24]", toString(p1));
		p1 /= 2;
		EXPECT_EQ("[4,8,12]", toString(p1));

		// cross product
		p1 = {1,2,3};
		p2 = {2,3,4};
		auto p3 = crossProduct(p1, p2);
		EXPECT_EQ("[-1,2,-1]", toString(p3));

		Vector<double, 3> temp = { 1,2,3 };
		const Vector<double, 3>& temp2 = temp;
		auto res = temp2 * 2.0;
		EXPECT_EQ("[2,4,6]", toString(res));

	}

	TEST(Vector, BasicND) {

		using Point = Vector<int,5>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values
		(void)p;

		Point p0 = 0;
		EXPECT_EQ("[0,0,0,0,0]", toString(p0));

		Point p1 = 1;
		EXPECT_EQ("[1,1,1,1,1]", toString(p1));

		Point p2 = {1,2,3,4,5};
		EXPECT_EQ("[1,2,3,4,5]", toString(p2));


		// -- assignment --

		p1 = p2;
		EXPECT_EQ("[1,2,3,4,5]", toString(p1));
		EXPECT_EQ("[1,2,3,4,5]", toString(p2));


		// -- equality --
		EXPECT_EQ(p1,p2);
		EXPECT_NE(p0,p1);

		// -- addition --
		p1 = p1 + p2;
		EXPECT_EQ("[2,4,6,8,10]", toString(p1));

		p1 += p1;
		EXPECT_EQ("[4,8,12,16,20]", toString(p1));

		// -- subtraction --
		p1 = p1 - p2;
		EXPECT_EQ("[3,6,9,12,15]", toString(p1));

		p1 -= p2;
		EXPECT_EQ("[2,4,6,8,10]", toString(p1));


		// scalar multiplication
		p1 = p1 * 2;
		EXPECT_EQ("[4,8,12,16,20]", toString(p1));
		p1 = 2 * p1;
		EXPECT_EQ("[8,16,24,32,40]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[16,32,48,64,80]", toString(p1));

		// scalar division
		p1 = p1 / 2;
		EXPECT_EQ("[8,16,24,32,40]", toString(p1));
		p1 /= 2;
		EXPECT_EQ("[4,8,12,16,20]", toString(p1));

	}

	TEST(Vector, MathUtilities) {
		using Point = Vector<int,3>;

		Point p1 = 16;
		EXPECT_EQ("768", toString(sumOfSquares(p1)));

		Point p2 = 4;
		EXPECT_EQ("[64,64,64]", toString(elementwiseProduct(p1,p2)));
		EXPECT_EQ("[4,4,4]", toString(elementwiseDivision(p1,p2)));

	}

} // end namespace utils
} // end namespace allscale
