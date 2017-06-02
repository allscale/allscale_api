#include <gtest/gtest.h>

#include "allscale/api/user/data/static_grid.h"
#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"
#include "allscale/utils/printer/pairs.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(StaticGridFragment,Basic) {

		EXPECT_TRUE((core::is_fragment<StaticGridFragment<double,100,200>>::value));

		StaticGridPoint<2> size = {100,200};

		StaticGridRegion<2> region(size,20,30);
		StaticGridFragment<int,100,200> fA(region);

		EXPECT_EQ(size,fA.totalSize());
	}


	TEST(StaticGridRegion,TypeProperties) {

		using namespace detail;

		EXPECT_TRUE(utils::is_value<StaticGridRegion<0>>::value);
		EXPECT_TRUE(utils::is_value<StaticGridRegion<1>>::value);
		EXPECT_TRUE(utils::is_value<StaticGridRegion<2>>::value);
		EXPECT_TRUE(utils::is_value<StaticGridRegion<3>>::value);

		EXPECT_TRUE(utils::is_serializable<StaticGridRegion<0>>::value);
		EXPECT_TRUE(utils::is_serializable<StaticGridRegion<1>>::value);
		EXPECT_TRUE(utils::is_serializable<StaticGridRegion<2>>::value);
		EXPECT_TRUE(utils::is_serializable<StaticGridRegion<3>>::value);

		EXPECT_TRUE(core::is_region<StaticGridRegion<0>>::value);
		EXPECT_TRUE(core::is_region<StaticGridRegion<1>>::value);
		EXPECT_TRUE(core::is_region<StaticGridRegion<2>>::value);
		EXPECT_TRUE(core::is_region<StaticGridRegion<3>>::value);

	}

	TEST(StaticGridFragment,TypeProperties) {

		using namespace detail;

		EXPECT_TRUE((core::is_fragment<StaticGridFragment<int,10>>::value));
		EXPECT_TRUE((core::is_fragment<StaticGridFragment<int,10,20>>::value));
		EXPECT_TRUE((core::is_fragment<StaticGridFragment<int,10,20,30>>::value));

	}


	TEST(StaticGridFragment1D,FragmentTestBasic) {

		StaticGridPoint<1> size = 50;

		StaticGridRegion<1> a(size,5,10);
		StaticGridRegion<1> b(size,8,14);

		testFragment<StaticGridFragment<int,50>>(a,b);

	}

	TEST(StaticGridFragment2D,FragmentTestBasic) {

		StaticGridPoint<2> size = {50,60};

		StaticGridRegion<2> a(size,{5,6},{10,12});
		StaticGridRegion<2> b(size,{8,9},{14,16});

		testFragment<StaticGridFragment<int,50,60>>(a,b);

	}


	TEST(StaticGrid,TypeProperties) {

		using namespace detail;

		EXPECT_TRUE((core::is_data_item<StaticGrid<int,10>>::value));
		EXPECT_TRUE((core::is_data_item<StaticGrid<int,10,20>>::value));
		EXPECT_TRUE((core::is_data_item<StaticGrid<int,10,20,30>>::value));

	}

	TEST(StaticGrid2D, Size) {

		StaticGrid<int,10,20> grid;

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


	TEST(StaticGrid2D, ElementCtorAndDtor) {


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
			StaticGrid<InstanceCounted,10,20> a;

			// now there should be a lot of instances
			EXPECT_EQ(200,InstanceCounted::num_instances);

			// here they should get destroyed
		}

		// check that there are none left
		EXPECT_EQ(0,InstanceCounted::num_instances);
	}

	TEST(StaticGrid2D, ComplexDataStructureCtorDtor) {

		// create a large array
		StaticGrid<std::vector<int>,10,20> a;

		// initialize each value
		for(int i=0; i<10; i++) {
			for(int j=0; j<10; j++) {
				a[{i,j}].push_back(i*j);
			}
		}
	}

	TEST(StaticGrid2D, Move) {

		// create a large array
		StaticGrid<std::vector<int>,10,20> a;

		// initialize each value
		for(int i=0; i<10; i++) {
			for(int j=0; j<10; j++) {
				a[{i,j}].push_back(i*j);
			}
		}

		// move the data to a new instance
		StaticGrid<std::vector<int>,10,20> b(std::move(a));

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

	TEST(StaticGrid2D,ExampleManagement) {

		using Point = StaticGridPoint<2>;
		using Region = StaticGridRegion<2>;
		using Fragment = StaticGridFragment<int,500,1000>;
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

	}

	TEST(StaticGrid2D, PforEach) {

		const std::size_t N = 10;
		const std::size_t M = 20;

		StaticGrid<double, N, M> staticGrid;

		staticGrid.pforEach([](double& element) { element = 3.5; });

		for(std::size_t i = 0; i < N; ++i) {
			for(std::size_t j = 0; j < M; ++j) {
				EXPECT_EQ(3.5, (staticGrid[{i, j}]));
			}
		}

	}

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
