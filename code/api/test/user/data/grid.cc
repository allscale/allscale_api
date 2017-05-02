#include <gtest/gtest.h>

#include "allscale/api/user/data/grid.h"
#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"
#include "allscale/utils/printer/pairs.h"

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
		EXPECT_EQ("[[[3,3] - [4,4]],[[4,3] - [8,4]],[[3,4] - [4,8]]]",toString(Box::difference(a,b)));
		EXPECT_EQ("[[[3,3] - [8,8]]]",toString(Box::difference(a,c)));
		EXPECT_EQ("[[[3,3] - [8,8]]]",toString(Box::difference(a,d)));

		EXPECT_EQ("[[[8,4] - [14,8]],[[4,8] - [8,14]],[[8,8] - [14,14]]]",toString(Box::difference(b,a)));
		EXPECT_EQ("[]",toString(Box::difference(b,b)));
		EXPECT_EQ("[[[4,4] - [12,12]],[[12,4] - [14,12]],[[4,12] - [12,14]]]",toString(Box::difference(b,c)));
		EXPECT_EQ("[[[4,4] - [14,14]]]",toString(Box::difference(b,d)));

		EXPECT_EQ("[[[12,12] - [18,18]]]",toString(Box::difference(c,a)));
		EXPECT_EQ("[[[14,12] - [18,14]],[[12,14] - [14,18]],[[14,14] - [18,18]]]",toString(Box::difference(c,b)));
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
		EXPECT_EQ("[[[3,3] - [4,4]],[[4,3] - [8,4]],[[3,4] - [4,8]],[[4,4] - [14,14]]]",toString(Box::merge(a,b)));
		EXPECT_EQ("[[[3,3] - [8,8]],[[12,12] - [18,18]]]",toString(Box::merge(a,c)));
		EXPECT_EQ("[[[3,3] - [8,8]],[[14,14] - [15,15]]]",toString(Box::merge(a,d)));

		EXPECT_EQ("[[[8,4] - [14,8]],[[4,8] - [8,14]],[[8,8] - [14,14]],[[3,3] - [8,8]]]",toString(Box::merge(b,a)));
		EXPECT_EQ("[[[4,4] - [14,14]]]",toString(Box::merge(b,b)));
		EXPECT_EQ("[[[4,4] - [12,12]],[[12,4] - [14,12]],[[4,12] - [12,14]],[[12,12] - [18,18]]]",toString(Box::merge(b,c)));
		EXPECT_EQ("[[[4,4] - [14,14]],[[14,14] - [15,15]]]",toString(Box::merge(b,d)));

		EXPECT_EQ("[[[12,12] - [18,18]],[[3,3] - [8,8]]]",toString(Box::merge(c,a)));
		EXPECT_EQ("[[[14,12] - [18,14]],[[12,14] - [14,18]],[[14,14] - [18,18]],[[4,4] - [14,14]]]",toString(Box::merge(c,b)));
		EXPECT_EQ("[[[12,12] - [18,18]]]",toString(Box::merge(c,c)));
		EXPECT_EQ("[[[12,12] - [14,14]],[[14,12] - [15,14]],[[15,12] - [18,14]],[[12,14] - [14,15]],[[15,14] - [18,15]],[[12,15] - [14,18]],[[14,15] - [15,18]],[[15,15] - [18,18]],[[14,14] - [15,15]]]",toString(Box::merge(c,d)));

		EXPECT_EQ("[[[14,14] - [15,15]],[[3,3] - [8,8]]]",toString(Box::merge(d,a)));
		EXPECT_EQ("[[[14,14] - [15,15]],[[4,4] - [14,14]]]",toString(Box::merge(d,b)));
		EXPECT_EQ("[[[12,12] - [18,18]]]",toString(Box::merge(d,c)));
		EXPECT_EQ("[[[14,14] - [15,15]]]",toString(Box::merge(d,d)));

	}

	TEST(GridBox1D, Area) {

		using Box = GridBox<1>;

		EXPECT_EQ(5,Box(7,12).area());
		EXPECT_EQ(0,Box(7,7).area());
		EXPECT_EQ(0,Box(7,0).area());

	}

	TEST(GridBox2D, Area) {

		using Box = GridBox<2>;

		EXPECT_EQ(25,Box(7,12).area());
		EXPECT_EQ(10,Box({7,9},{12,11}).area());
		EXPECT_EQ(0,Box(7,7).area());
		EXPECT_EQ(0,Box(7,0).area());

	}


	TEST(GridBox1D, Fuse) {

		using Box = GridBox<1>;

		EXPECT_TRUE(Box::areFusable<0>(Box(3,5),Box(5,8)));
		EXPECT_TRUE(Box::areFusable<0>(Box(3,7),Box(7,8)));
		EXPECT_TRUE(Box::areFusable<0>(Box(3,8),Box(8,8)));
		EXPECT_TRUE(Box::areFusable<0>(Box(5,8),Box(3,5)));

		EXPECT_FALSE(Box::areFusable<0>(Box(3,7),Box(8,8)));
		EXPECT_FALSE(Box::areFusable<0>(Box(3,6),Box(7,8)));
		EXPECT_FALSE(Box::areFusable<0>(Box(7,8),Box(3,6)));

		EXPECT_EQ(Box(3,8), Box::fuse<0>(Box(3,5),Box(5,8)));
		EXPECT_EQ(Box(3,8), Box::fuse<0>(Box(5,8),Box(3,5)));

	}

	TEST(GridBox2D, Fuse) {

		using Box = GridBox<2>;

		// test first dimension
		EXPECT_TRUE(Box::areFusable<0>(Box({3,3},{5,5}),Box({5,3},{8,5})));
		EXPECT_TRUE(Box::areFusable<0>(Box({5,3},{8,5}),Box({3,3},{5,5})));

		EXPECT_EQ(Box({3,3},{8,5}), Box::fuse<0>(Box({3,3},{5,5}),Box({5,3},{8,5})));
		EXPECT_EQ(Box({3,3},{8,5}), Box::fuse<0>(Box({5,3},{8,5}),Box({3,3},{5,5})));

		EXPECT_FALSE(Box::areFusable<1>(Box({3,3},{5,5}),Box({5,3},{8,5})));
		EXPECT_FALSE(Box::areFusable<1>(Box({5,3},{8,5}),Box({3,3},{5,5})));


		// test second dimension
		EXPECT_TRUE(Box::areFusable<1>(Box({3,3},{5,5}),Box({3,5},{5,8})));
		EXPECT_TRUE(Box::areFusable<1>(Box({3,5},{5,8}),Box({3,3},{5,5})));

		EXPECT_EQ(Box({3,3},{5,8}), Box::fuse<1>(Box({3,3},{5,5}),Box({3,5},{5,8})));
		EXPECT_EQ(Box({3,3},{5,8}), Box::fuse<1>(Box({3,5},{5,8}),Box({3,3},{5,5})));

		EXPECT_FALSE(Box::areFusable<0>(Box({3,3},{5,5}),Box({3,5},{5,8})));
		EXPECT_FALSE(Box::areFusable<0>(Box({3,5},{5,8}),Box({3,3},{5,5})));

	}

	TEST(GridBox1D, ScanByLine) {

		using Point = GridPoint<1>;
		using Box = GridBox<1>;

		std::vector<std::pair<Point,Point>> points;
		Box(5,10).scanByLines([&](const Point& a, const Point& b){
			points.push_back({a,b});
		});

		EXPECT_EQ("[[[5],[10]]]",toString(points));

	}

	TEST(GridBox2D, ScanByLine) {

		using Point = GridPoint<2>;
		using Box = GridBox<2>;

		std::vector<std::pair<Point,Point>> points;
		Box({5,10},{8,40}).scanByLines([&](const Point& a, const Point& b){
			points.push_back({a,b});
		});

		EXPECT_EQ("[[[5,10],[5,40]],[[6,10],[6,40]],[[7,10],[7,40]]]",toString(points));

	}

	TEST(GridBox3D, ScanByLine) {

		using Point = GridPoint<3>;
		using Box = GridBox<3>;

		std::vector<std::pair<Point,Point>> points;
		Box({2,5,10},{4,8,40}).scanByLines([&](const Point& a, const Point& b){
			points.push_back({a,b});
		});

		EXPECT_EQ("[[[2,5,10],[2,5,40]],[[2,6,10],[2,6,40]],[[2,7,10],[2,7,40]],[[3,5,10],[3,5,40]],[[3,6,10],[3,6,40]],[[3,7,10],[3,7,40]]]",toString(points));

	}

	TEST(GridRegion,Basic) {

		GridPoint<2> size = 50;

		GridRegion<2> region;
		EXPECT_TRUE(region.empty());
		EXPECT_EQ("{}", toString(region));

		GridRegion<2> empty(size);
		EXPECT_TRUE(empty.empty());
		EXPECT_EQ("{}", toString(empty));

		GridRegion<2> cube(size,10);
		EXPECT_FALSE(cube.empty());
		EXPECT_EQ("{[[0,0] - [10,10]]}", toString(cube));

		GridRegion<2> box(size,GridPoint<2>{10,20});
		EXPECT_FALSE(box.empty());
		EXPECT_EQ("{[[0,0] - [10,20]]}", toString(box));

		GridRegion<2> box2(size,GridPoint<2>{5,8},GridPoint<2>{10,20});
		EXPECT_FALSE(box2.empty());
		EXPECT_EQ("{[[5,8] - [10,20]]}", toString(box2));

		GridRegion<2> e1(size,0);
		EXPECT_TRUE(e1.empty());
		EXPECT_EQ("{}", toString(e1));

		GridRegion<2> e2(size,2,2);
		EXPECT_TRUE(e2.empty());
		EXPECT_EQ("{}", toString(e2));

	}

	TEST(GridRegion,Compress) {

		using Region = GridRegion<2>;

		GridPoint<2> size = 50;

		Region a(size,{3,3},{5,5});
		Region b(size,{5,5},{8,8});
		Region c(size,{3,5},{5,8});
		Region d(size,{5,3},{8,5});

		Region ab = Region::merge(a,b);
		Region cd = Region::merge(c,d);

		EXPECT_EQ("{[[3,3] - [5,5]],[[5,5] - [8,8]]}",toString(ab));
		EXPECT_EQ("{[[3,5] - [5,8]],[[5,3] - [8,5]]}",toString(cd));


		Region abc = Region::merge(ab,c);
		Region abd = Region::merge(ab,d);

		EXPECT_EQ("{[[3,3] - [5,8]],[[5,5] - [8,8]]}",toString(abc));
		EXPECT_EQ("{[[3,3] - [5,5]],[[5,3] - [8,8]]}",toString(abd));

		EXPECT_EQ("{[[3,3] - [8,8]]}",toString(Region::merge(ab,cd)));
		EXPECT_EQ("{[[3,3] - [8,8]]}",toString(Region::merge(abc,d)));

	}

	TEST(GridRegion,BoundingBox) {

		using Region = GridRegion<2>;

		GridPoint<2> size = 50;

		Region a(size,{3,3},{5,5});
		Region b(size,{5,5},{8,8});
		Region c(size,{3,5},{5,8});
		Region d(size,{5,3},{8,5});

		Region ab = Region::merge(a,b);
		Region cd = Region::merge(c,d);

		Region f = Region::merge(ab,cd);

		EXPECT_EQ(f.boundingBox(),ab.boundingBox());
		EXPECT_EQ(f.boundingBox(),cd.boundingBox());
	}

	TEST(GridRegion,RegionTestBasic) {

		EXPECT_TRUE(utils::is_value<GridRegion<0>>::value);
		EXPECT_TRUE(utils::is_serializable<GridRegion<0>>::value);
		EXPECT_TRUE(core::is_region<GridRegion<0>>::value);

		EXPECT_TRUE(utils::is_value<GridRegion<1>>::value);
		EXPECT_TRUE(utils::is_serializable<GridRegion<1>>::value);
		EXPECT_TRUE(core::is_region<GridRegion<1>>::value);

		EXPECT_TRUE(core::is_region<GridRegion<2>>::value);
		EXPECT_TRUE(core::is_region<GridRegion<3>>::value);

	}

	TEST(GridRegion1D,RegionTestBasic) {

		GridPoint<1> size = 50;

		GridRegion<1> a(size,5,10);
		GridRegion<1> b(size,8,14);
		testRegion(a,b);

		a = GridRegion<1>(size,7,10);
		b = GridRegion<1>(size,6,8);
		testRegion(a,b);
	}

	TEST(GridRegion2D,RegionTestBasic) {

		GridPoint<2> size = 50;

		GridRegion<2> a(size,5,10);
		GridRegion<2> b(size,8,14);
		testRegion(a,b);

		// mirrored
		a = GridRegion<2>(size,8,14);
		b = GridRegion<2>(size,5,10);
		testRegion(a,b);

		// rotated left
		a = GridRegion<2>(size,5,10);
		b = GridRegion<2>(size,{6,3},{12,8});
		testRegion(a,b);

		// rotated right
		a = GridRegion<2>(size,{6,3},{12,8});
		b = GridRegion<2>(size,5,10);
		testRegion(a,b);

		// the cross
		a = GridRegion<2>(size,{4,2},{10,12});
		b = GridRegion<2>(size,{2,4},{12,10});
		testRegion(a,b);

	}

	TEST(GridRegion3D,RegionTestBasic) {

		GridPoint<3> size = 50;

		GridRegion<3> a(size,5,10);
		GridRegion<3> b(size,8,14);
		testRegion(a,b);
	}

	TEST(GridFragment,Basic) {

		EXPECT_TRUE((core::is_fragment<GridFragment<double,2>>::value));

		GridPoint<2> size = 50;

		GridRegion<2> region(size,20,30);
		GridFragment<int,2> fA(region);

	}

	TEST(GridFragment1D,FragmentTestBasic) {

		GridPoint<1> size = 50;

		GridRegion<1> a(size,5,10);
		GridRegion<1> b(size,8,14);

		testFragment<GridFragment<int,1>>(a,b);

	}

	TEST(GridFragment2D,FragmentTestBasic) {

		GridPoint<2> size = {50,60};

		GridRegion<2> a(size,{5,6},{10,12});
		GridRegion<2> b(size,{8,9},{14,16});

		testFragment<GridFragment<int,2>>(a,b);

	}


	TEST(Grid,TypeProperties) {

		EXPECT_TRUE((core::is_data_item<Grid<int,1>>::value));
		EXPECT_TRUE((core::is_data_item<Grid<int,2>>::value));
		EXPECT_TRUE((core::is_data_item<Grid<int,3>>::value));

	}

	TEST(Grid2D, Size) {

		Grid<int,2> grid({10,20});

		EXPECT_EQ("[10,20]",toString(grid.size()));


	}

	struct InstanceCounted {

		static int num_instances;

		InstanceCounted() {
			num_instances++;
		}

		~InstanceCounted() {
			num_instances--;
		}

	};

	int InstanceCounted::num_instances = 0;


	TEST(Grid2D, ElementCtorAndDtor) {


		// ----- test the Instance Counted class --------

		// start with assuming that there are no instances
		EXPECT_EQ(0,InstanceCounted::num_instances);

		// check that the constructor works as expected
		{
			InstanceCounted a;
			EXPECT_EQ(1,InstanceCounted::num_instances);
		}

		// also check that the destructor works as expected
		EXPECT_EQ(0,InstanceCounted::num_instances);


		// ---------- test the Large Array ---------------

		{

			// create a large array
			Grid<InstanceCounted,2> a({10,20});

			// now there should be a lot of instances
			EXPECT_EQ(200,InstanceCounted::num_instances);

			// here they should get destroyed
		}

		// check that there are none left
		EXPECT_EQ(0,InstanceCounted::num_instances);
	}

	TEST(Grid2D, ComplexDataStructureCtorDtor) {

		// create a large array
		Grid<std::vector<int>,2> a({10,20});

		// initialize each value
		for(int i=0; i<10; i++) {
			for(int j=0; j<10; j++) {
				a[{i,j}].push_back(i*j);
			}
		}
	}

	TEST(Grid2D, Move) {

		// create a large array
		Grid<std::vector<int>,2> a({10,20});

		// initialize each value
		for(int i=0; i<10; i++) {
			for(int j=0; j<10; j++) {
				a[{i,j}].push_back(i*j);
			}
		}

		// move the data to a new instance
		Grid<std::vector<int>,2> b(std::move(a));

		// check the content
		for(int i=0; i<10; i++) {
			for(int j=0; j<10; j++) {
				EXPECT_EQ(1,(b[{i,j}].size()));
				EXPECT_EQ(i*j,(b[{i,j}].front()));
			}
		}

		// now move-assign the values
		a = std::move(b);

		// check the content
		for(int i=0; i<10; i++) {
			for(int j=0; j<10; j++) {
				EXPECT_EQ(1,(a[{i,j}].size()));
				EXPECT_EQ(i*j,(a[{i,j}].front()));
			}
		}
	}



	TEST(Grid2D,ExampleManagement) {

		using Point = GridPoint<2>;
		using Region = GridRegion<2>;
		using Fragment = GridFragment<int,2>;
		using SharedData = core::no_shared_data;

		SharedData shared;

		// total size:
		Point size = { 500, 1000 };

		// upper half
		Region partA(size, {0,0},{250,1000});

		// lower half
		Region partB(size, {250,0},{500,1000});

		// check that the coordinates are correct
		Region full = Region::merge(partA,partB);
		EXPECT_EQ("{[[0,0] - [500,1000]]}",toString(full));

		// create fragments
		Fragment fA(shared,partA);
		Fragment fB(shared,partB);

		// initialize
		fA.mask().forEach([](auto& p) { p = 0; });
		fB.mask().forEach([](auto& p) { p = 0; });

		// fill the data set
		for(int t = 1; t<10; t++) {

			auto a = fA.mask();
			for(unsigned i=0; i<250; i++) {
				for(unsigned j=0; j<1000; j++) {
					EXPECT_EQ((i*j*(t-1)),(a[{i,j}]));
					a[{i,j}] = i * j * t;
				}
			}

			auto b = fB.mask();
			for(unsigned i=250; i<500; i++) {
				for(unsigned j=0; j<1000; j++) {
					EXPECT_EQ((i*j*(t-1)),(b[{i,j}]));
					b[{i,j}] = i * j * t;
				}
			}

		}

		// --- alter data distribution ---


		Region newPartA = Region(size, {0,0}, {250,750});
		Region newPartB = Region(size, {250,0}, {500,750});
		Region newPartC = Region(size, {0,750}, {500,1000});
		EXPECT_EQ(full,Region::merge(newPartA,newPartB,newPartC));

		Fragment fC(shared,newPartC);

		// move data from A and B to C
		fC.insert(fA,Region::intersect(newPartC,partA));
		fC.insert(fB,Region::intersect(newPartC,partB));

		// shrink A and B
		fA.resize(newPartA);
		fB.resize(newPartB);

		for(int t = 10; t<20; t++) {

			auto a = fA.mask();
			for(unsigned i=0; i<250; i++) {
				for(unsigned j=0; j<750; j++) {
					EXPECT_EQ((i*j*(t-1)),(a[{i,j}]));
					a[{i,j}] = i * j * t;
				}
			}

			auto b = fB.mask();
			for(unsigned i=250; i<500; i++) {
				for(unsigned j=0; j<750; j++) {
					EXPECT_EQ((i*j*(t-1)),(b[{i,j}]));
					b[{i,j}] = i * j * t;
				}
			}

			auto c = fC.mask();
			for(unsigned i=0; i<500; i++) {
				for(unsigned j=750; j<1000; j++) {
					EXPECT_EQ((i*j*(t-1)),(c[{i,j}]));
					c[{i,j}] = i * j * t;
				}
			}
		}

/*

		EXPECT_TRUE(SetRegion<int>::intersect(a,b).empty());

		// create fragments
		MapFragment<int,int> fA(a);
		MapFragment<int,int> fB(b);

		// do some computation
		for(int t = 0; t<10; t++) {

			Map<int,int> a = fA.mask();
			for(int i=0; i<5; i++) {
				EXPECT_EQ(a[i],t);
				a[i]++;
			}

			Map<int,int> b = fB.mask();
			for(int i=5; i<10; i++) {
				EXPECT_EQ(b[i],t);
				b[i]++;
			}

		}

		// ------------------------------------------------

		// now re-balance fragments by introducing a new fragment C
		SetRegion<int> c;
		c.add(8,9);
		MapFragment<int,int> fC(c);
		fC.insert(fB,c);

		SetRegion<int> nb;
		nb.add(3,4,5,6,7);
		fB.resize(nb);
		fB.insert(fA,SetRegion<int>::intersect(a,nb));

		SetRegion<int> na = SetRegion<int>::difference(a,nb);
		fA.resize(na);

		// ------------------------------------------------

		// do some computation on the re-shaped distribution
		for(int t = 10; t<20; t++) {

			Map<int,int> a = fA.mask();
			for(int i=0; i<3; i++) {
				EXPECT_EQ(a[i],t);
				a[i]++;
			}

			Map<int,int> b = fB.mask();
			for(int i=3; i<8; i++) {
				EXPECT_EQ(b[i],t);
				b[i]++;
			}

			Map<int,int> c = fC.mask();
			for(int i=8; i<10; i++) {
				EXPECT_EQ(c[i],t);
				c[i]++;
			}

		}

*/

	}



} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
