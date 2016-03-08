#include <gtest/gtest.h>

#include "allscale/api/user/data/grid.h"
#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(GridPoint,Basic) {

		GridPoint<1> a = 3;
		EXPECT_EQ("[3]", toString(a));

		GridPoint<2> b = 5;
		EXPECT_EQ("[5,5]", toString(b));

		GridPoint<2> c = {2,3};
		EXPECT_EQ("[2,3]", toString(c));

		GridPoint<3> d = {2,3,4};
		EXPECT_EQ("[2,3,4]", toString(d));

		GridPoint<4> e = {2,3,4,5};
		EXPECT_EQ("[2,3,4,5]", toString(e));

	}

	TEST(GridBox,Basic) {

		// -- 1D boxes --

		EXPECT_TRUE(GridBox<1>(5,5).empty());
		EXPECT_TRUE(GridBox<1>(5,4).empty());
		EXPECT_FALSE(GridBox<1>(5,6).empty());

		GridBox<1> b1_1(10);
		EXPECT_EQ("[[0] - [10]]",toString(b1_1));

		GridBox<1> b1_2(5,15);
		EXPECT_EQ("[[5] - [15]]",toString(b1_2));


		// -- 2D boxes --

		EXPECT_TRUE(GridBox<2>(GridPoint<2>{3,4},GridPoint<2>{3,4}).empty());
		EXPECT_TRUE(GridBox<2>(GridPoint<2>{3,4},GridPoint<2>{3,5}).empty());
		EXPECT_TRUE(GridBox<2>(GridPoint<2>{3,4},GridPoint<2>{4,4}).empty());
		EXPECT_TRUE(GridBox<2>(GridPoint<2>{3,4},GridPoint<2>{2,5}).empty());
		EXPECT_TRUE(GridBox<2>(GridPoint<2>{3,4},GridPoint<2>{4,3}).empty());

		EXPECT_FALSE(GridBox<2>(GridPoint<2>{3,4},GridPoint<2>{4,5}).empty());

		GridBox<2> b2_1(5);
		EXPECT_EQ("[[0,0] - [5,5]]",toString(b2_1));

		GridBox<2> b2_2(GridPoint<2>{4,5});
		EXPECT_EQ("[[0,0] - [4,5]]",toString(b2_2));

		GridBox<2> b2_3(GridPoint<2>{4,5},GridPoint<2>{8,12});
		EXPECT_EQ("[[4,5] - [8,12]]",toString(b2_3));

	}

	TEST(GridBox1D,IsIntersecting) {

		using Box = GridBox<1>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);

		EXPECT_TRUE(a.intersectsWith(a));
		EXPECT_TRUE(a.intersectsWith(b));
		EXPECT_FALSE(a.intersectsWith(c));

		EXPECT_TRUE(b.intersectsWith(a));
		EXPECT_TRUE(b.intersectsWith(b));
		EXPECT_TRUE(b.intersectsWith(c));

		EXPECT_FALSE(c.intersectsWith(a));
		EXPECT_TRUE(c.intersectsWith(b));
		EXPECT_TRUE(c.intersectsWith(c));

		// nothing intersects with an empty set
		Box e(5,5);
		EXPECT_TRUE(e.empty());
		EXPECT_FALSE(a.intersectsWith(e));
		EXPECT_FALSE(b.intersectsWith(e));
		EXPECT_FALSE(c.intersectsWith(e));

		EXPECT_FALSE(e.intersectsWith(a));
		EXPECT_FALSE(e.intersectsWith(b));
		EXPECT_FALSE(e.intersectsWith(c));

	}

	TEST(GridBox1D,Intersect) {

		using Box = GridBox<1>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);

		EXPECT_EQ("[[3] - [8]]",toString(Box::intersect(a,a)));
		EXPECT_EQ("[[4] - [8]]",toString(Box::intersect(a,b)));
		EXPECT_TRUE(Box::intersect(a,c).empty());

		EXPECT_EQ("[[4] - [8]]",toString(Box::intersect(b,a)));
		EXPECT_EQ("[[4] - [14]]",toString(Box::intersect(b,b)));
		EXPECT_EQ("[[12] - [14]]",toString(Box::intersect(b,c)));

		EXPECT_TRUE(Box::intersect(c,a).empty());
		EXPECT_EQ("[[12] - [14]]",toString(Box::intersect(c,b)));
		EXPECT_EQ("[[12] - [18]]",toString(Box::intersect(c,c)));

		// nothing intersects with an empty set
		Box e(5,5);

		EXPECT_TRUE(Box::intersect(a,e).empty());
		EXPECT_TRUE(Box::intersect(b,e).empty());
		EXPECT_TRUE(Box::intersect(c,e).empty());

		EXPECT_TRUE(Box::intersect(e,a).empty());
		EXPECT_TRUE(Box::intersect(e,b).empty());
		EXPECT_TRUE(Box::intersect(e,c).empty());

	}


	TEST(GridBox1D,Difference) {

		using Box = GridBox<1>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);
		Box d(14,15);

		EXPECT_EQ("[]",toString(Box::difference(a,a)));
		EXPECT_EQ("[[[3] - [4]]]",toString(Box::difference(a,b)));
		EXPECT_EQ("[[[3] - [8]]]",toString(Box::difference(a,c)));
		EXPECT_EQ("[[[3] - [8]]]",toString(Box::difference(a,d)));

		EXPECT_EQ("[[[8] - [14]]]",toString(Box::difference(b,a)));
		EXPECT_EQ("[]",toString(Box::difference(b,b)));
		EXPECT_EQ("[[[4] - [12]]]",toString(Box::difference(b,c)));
		EXPECT_EQ("[[[4] - [14]]]",toString(Box::difference(b,d)));

		EXPECT_EQ("[[[12] - [18]]]",toString(Box::difference(c,a)));
		EXPECT_EQ("[[[14] - [18]]]",toString(Box::difference(c,b)));
		EXPECT_EQ("[]",toString(Box::difference(c,c)));
		EXPECT_EQ("[[[12] - [14]],[[15] - [18]]]",toString(Box::difference(c,d)));

		EXPECT_EQ("[[[14] - [15]]]",toString(Box::difference(d,a)));
		EXPECT_EQ("[[[14] - [15]]]",toString(Box::difference(d,b)));
		EXPECT_EQ("[]",toString(Box::difference(d,c)));
		EXPECT_EQ("[]",toString(Box::difference(d,d)));

	}

	TEST(GridBox1D,Merge) {

		using Box = GridBox<1>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);
		Box d(14,15);

		EXPECT_EQ("[[[3] - [8]]]",toString(Box::merge(a,a)));
		EXPECT_EQ("[[[3] - [4]],[[4] - [14]]]",toString(Box::merge(a,b)));
		EXPECT_EQ("[[[3] - [8]],[[12] - [18]]]",toString(Box::merge(a,c)));
		EXPECT_EQ("[[[3] - [8]],[[14] - [15]]]",toString(Box::merge(a,d)));

		EXPECT_EQ("[[[8] - [14]],[[3] - [8]]]",toString(Box::merge(b,a)));
		EXPECT_EQ("[[[4] - [14]]]",toString(Box::merge(b,b)));
		EXPECT_EQ("[[[4] - [12]],[[12] - [18]]]",toString(Box::merge(b,c)));
		EXPECT_EQ("[[[4] - [14]],[[14] - [15]]]",toString(Box::merge(b,d)));

		EXPECT_EQ("[[[12] - [18]],[[3] - [8]]]",toString(Box::merge(c,a)));
		EXPECT_EQ("[[[14] - [18]],[[4] - [14]]]",toString(Box::merge(c,b)));
		EXPECT_EQ("[[[12] - [18]]]",toString(Box::merge(c,c)));
		EXPECT_EQ("[[[12] - [14]],[[15] - [18]],[[14] - [15]]]",toString(Box::merge(c,d)));

		EXPECT_EQ("[[[14] - [15]],[[3] - [8]]]",toString(Box::merge(d,a)));
		EXPECT_EQ("[[[14] - [15]],[[4] - [14]]]",toString(Box::merge(d,b)));
		EXPECT_EQ("[[[12] - [18]]]",toString(Box::merge(d,c)));
		EXPECT_EQ("[[[14] - [15]]]",toString(Box::merge(d,d)));

	}

	TEST(GridBox2D,IsIntersecting) {

		using Box = GridBox<2>;
		using Point = GridPoint<2>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);

		Box d(Point{4,2},Point{5,6});

		EXPECT_TRUE(a.intersectsWith(a));
		EXPECT_TRUE(a.intersectsWith(b));
		EXPECT_FALSE(a.intersectsWith(c));
		EXPECT_TRUE(a.intersectsWith(d));

		EXPECT_TRUE(b.intersectsWith(a));
		EXPECT_TRUE(b.intersectsWith(b));
		EXPECT_TRUE(b.intersectsWith(c));
		EXPECT_TRUE(b.intersectsWith(d));

		EXPECT_FALSE(c.intersectsWith(a));
		EXPECT_TRUE(c.intersectsWith(b));
		EXPECT_TRUE(c.intersectsWith(c));
		EXPECT_FALSE(c.intersectsWith(d));


		// nothing intersects with an empty set
		Box e(5,5);
		EXPECT_TRUE(e.empty());
		EXPECT_FALSE(a.intersectsWith(e));
		EXPECT_FALSE(b.intersectsWith(e));
		EXPECT_FALSE(c.intersectsWith(e));
		EXPECT_FALSE(d.intersectsWith(e));

		EXPECT_FALSE(e.intersectsWith(a));
		EXPECT_FALSE(e.intersectsWith(b));
		EXPECT_FALSE(e.intersectsWith(c));
		EXPECT_FALSE(e.intersectsWith(d));

	}


	TEST(GridBox2D,Intersect) {

		using Box = GridBox<2>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);

		EXPECT_EQ("[[3,3] - [8,8]]",toString(Box::intersect(a,a)));
		EXPECT_EQ("[[4,4] - [8,8]]",toString(Box::intersect(a,b)));
		EXPECT_TRUE(Box::intersect(a,c).empty());

		EXPECT_EQ("[[4,4] - [8,8]]",toString(Box::intersect(b,a)));
		EXPECT_EQ("[[4,4] - [14,14]]",toString(Box::intersect(b,b)));
		EXPECT_EQ("[[12,12] - [14,14]]",toString(Box::intersect(b,c)));

		EXPECT_TRUE(Box::intersect(c,a).empty());
		EXPECT_EQ("[[12,12] - [14,14]]",toString(Box::intersect(c,b)));
		EXPECT_EQ("[[12,12] - [18,18]]",toString(Box::intersect(c,c)));

		EXPECT_EQ("[[5,4] - [8,12]]", toString(
				Box::intersect(
						Box({2,4},{10,12}),
						Box({5,2},{8,14})
				)
		));

		// nothing intersects with an empty set
		Box e(5,5);

		EXPECT_TRUE(Box::intersect(a,e).empty());
		EXPECT_TRUE(Box::intersect(b,e).empty());
		EXPECT_TRUE(Box::intersect(c,e).empty());

		EXPECT_TRUE(Box::intersect(e,a).empty());
		EXPECT_TRUE(Box::intersect(e,b).empty());
		EXPECT_TRUE(Box::intersect(e,c).empty());

	}


	TEST(GridBox2D,Difference) {

		using Box = GridBox<2>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);
		Box d(14,15);

		EXPECT_EQ("[]",toString(Box::difference(a,a)));
		EXPECT_EQ("[[[3,3] - [4,4]],[[4,3] - [14,4]],[[3,4] - [4,14]]]",toString(Box::difference(a,b)));
		EXPECT_EQ("[[[3,3] - [8,8]]]",toString(Box::difference(a,c)));
		EXPECT_EQ("[[[3,3] - [8,8]]]",toString(Box::difference(a,d)));

		EXPECT_EQ("[[[8,3] - [14,8]],[[3,8] - [8,14]],[[8,8] - [14,14]]]",toString(Box::difference(b,a)));
		EXPECT_EQ("[]",toString(Box::difference(b,b)));
		EXPECT_EQ("[[[4,4] - [12,12]],[[12,4] - [18,12]],[[4,12] - [12,18]]]",toString(Box::difference(b,c)));
		EXPECT_EQ("[[[4,4] - [14,14]]]",toString(Box::difference(b,d)));

		EXPECT_EQ("[[[12,12] - [18,18]]]",toString(Box::difference(c,a)));
		EXPECT_EQ("[[[14,4] - [18,14]],[[4,14] - [14,18]],[[14,14] - [18,18]]]",toString(Box::difference(c,b)));
		EXPECT_EQ("[]",toString(Box::difference(c,c)));
		EXPECT_EQ("[[[12,12] - [14,14]],[[14,12] - [15,14]],[[15,12] - [18,14]],[[12,14] - [14,15]],[[15,14] - [18,15]],[[12,15] - [14,18]],[[14,15] - [15,18]],[[15,15] - [18,18]]]",toString(Box::difference(c,d)));

		EXPECT_EQ("[[[14,14] - [15,15]]]",toString(Box::difference(d,a)));
		EXPECT_EQ("[[[14,14] - [15,15]]]",toString(Box::difference(d,b)));
		EXPECT_EQ("[]",toString(Box::difference(d,c)));
		EXPECT_EQ("[]",toString(Box::difference(d,d)));

	}

	TEST(GridBox2D,Merge) {

		using Box = GridBox<2>;

		Box a(3,8);
		Box b(4,14);
		Box c(12,18);
		Box d(14,15);

		EXPECT_EQ("[[[3,3] - [8,8]]]",toString(Box::merge(a,a)));
		EXPECT_EQ("[[[3,3] - [4,4]],[[4,3] - [14,4]],[[3,4] - [4,14]],[[4,4] - [14,14]]]",toString(Box::merge(a,b)));
		EXPECT_EQ("[[[3,3] - [8,8]],[[12,12] - [18,18]]]",toString(Box::merge(a,c)));
		EXPECT_EQ("[[[3,3] - [8,8]],[[14,14] - [15,15]]]",toString(Box::merge(a,d)));

		EXPECT_EQ("[[[8,3] - [14,8]],[[3,8] - [8,14]],[[8,8] - [14,14]],[[3,3] - [8,8]]]",toString(Box::merge(b,a)));
		EXPECT_EQ("[[[4,4] - [14,14]]]",toString(Box::merge(b,b)));
		EXPECT_EQ("[[[4,4] - [12,12]],[[12,4] - [18,12]],[[4,12] - [12,18]],[[12,12] - [18,18]]]",toString(Box::merge(b,c)));
		EXPECT_EQ("[[[4,4] - [14,14]],[[14,14] - [15,15]]]",toString(Box::merge(b,d)));

		EXPECT_EQ("[[[12,12] - [18,18]],[[3,3] - [8,8]]]",toString(Box::merge(c,a)));
		EXPECT_EQ("[[[14,4] - [18,14]],[[4,14] - [14,18]],[[14,14] - [18,18]],[[4,4] - [14,14]]]",toString(Box::merge(c,b)));
		EXPECT_EQ("[[[12,12] - [18,18]]]",toString(Box::merge(c,c)));
		EXPECT_EQ("[[[12,12] - [14,14]],[[14,12] - [15,14]],[[15,12] - [18,14]],[[12,14] - [14,15]],[[15,14] - [18,15]],[[12,15] - [14,18]],[[14,15] - [15,18]],[[15,15] - [18,18]],[[14,14] - [15,15]]]",toString(Box::merge(c,d)));

		EXPECT_EQ("[[[14,14] - [15,15]],[[3,3] - [8,8]]]",toString(Box::merge(d,a)));
		EXPECT_EQ("[[[14,14] - [15,15]],[[4,4] - [14,14]]]",toString(Box::merge(d,b)));
		EXPECT_EQ("[[[12,12] - [18,18]]]",toString(Box::merge(d,c)));
		EXPECT_EQ("[[[14,14] - [15,15]]]",toString(Box::merge(d,d)));

	}

	TEST(GridRegion,Basic) {

		GridRegion<2> region;
		EXPECT_TRUE(region.empty());
		EXPECT_EQ("{}", toString(region));

		GridRegion<2> cube(10);
		EXPECT_FALSE(cube.empty());
		EXPECT_EQ("{[[0,0] - [10,10]]}", toString(cube));

		GridRegion<2> box(GridPoint<2>{10,20});
		EXPECT_FALSE(box.empty());
		EXPECT_EQ("{[[0,0] - [10,20]]}", toString(box));

		GridRegion<2> box2(GridPoint<2>{5,8},GridPoint<2>{10,20});
		EXPECT_FALSE(box2.empty());
		EXPECT_EQ("{[[5,8] - [10,20]]}", toString(box2));

		GridRegion<2> e1(0);
		EXPECT_TRUE(e1.empty());
		EXPECT_EQ("{}", toString(e1));

		GridRegion<2> e2(2,2);
		EXPECT_TRUE(e2.empty());
		EXPECT_EQ("{}", toString(e2));

	}


	TEST(GridRegion,RegionTest) {

		GridRegion<1> a(5,10);
		GridRegion<1> b(8,14);

		EXPECT_TRUE(utils::is_value<GridRegion<0>>::value);
		EXPECT_TRUE(utils::is_serializable<GridRegion<0>>::value);
		EXPECT_TRUE(core::is_region<GridRegion<0>>::value);

		EXPECT_TRUE(utils::is_value<GridRegion<1>>::value);
		EXPECT_TRUE(utils::is_serializable<GridRegion<1>>::value);
		EXPECT_TRUE(core::is_region<GridRegion<1>>::value);

		EXPECT_TRUE(core::is_region<GridRegion<2>>::value);
		EXPECT_TRUE(core::is_region<GridRegion<3>>::value);

		testRegion(a,b);
	}


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
