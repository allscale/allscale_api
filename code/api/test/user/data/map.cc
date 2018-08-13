#include <gtest/gtest.h>

#include <typeinfo>
#include "allscale/api/user/data/map.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(SetRegion,Basic) {

		SetRegion<int> a,b;
		a.add(5);
		a.add(6);

		b.add(6);
		b.add(7);

		testRegion(a,b);

	}

	TEST(MapFragment,Basic) {

		SetRegion<int> a,b;
		a.add(5);
		a.add(6);

		b.add(6);
		b.add(7);

		testFragment<MapFragment<int,int>>(a,b);

	}

	TEST(Map,Basic) {

		// check whether Map implements the data_item concept (syntax)
		EXPECT_TRUE((core::is_data_item<Map<int,int>>::value));



		// TODO: semantic data item checks

	}

	TEST(Map,Interact) {

		SetRegion<int> keys;
		keys.add(2);
		keys.add(3);
		keys.add(5);


		// create a map data item
		Map<int,int> data(keys);

		// use map data item
		data[2] = 12;
		data[3] = 14;
		data[5] = 18;

	}



	TEST(Map,ExampleManagement) {

		// create several ranges
		SetRegion<int> a, b;
		for(int i=0; i<5; i++) {
			a.add(i);
		}
		for(int i=5; i<10; i++) {
			b.add(i);
		}

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
		fC.insertRegion(fB,c);

		SetRegion<int> nb;
		nb.add(3,4,5,6,7);
		fB.resize(nb);
		fB.insertRegion(fA,SetRegion<int>::intersect(a,nb));

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

	}


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
