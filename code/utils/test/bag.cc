#include <gtest/gtest.h>

#include "allscale/utils/bag.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace utils {

	TEST(Bag,Basic) {

		Bag<int> b;

		EXPECT_TRUE(b.empty());
		EXPECT_EQ(0, b.size());

		EXPECT_FALSE(b.contains(12));
		EXPECT_FALSE(b.contains(14));
		EXPECT_FALSE(b.contains(16));


		b.insert(12);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(1, b.size());

		EXPECT_TRUE(b.contains(12));
		EXPECT_FALSE(b.contains(14));
		EXPECT_FALSE(b.contains(16));


		b.insert(14);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(2, b.size());

		EXPECT_TRUE(b.contains(12));
		EXPECT_TRUE(b.contains(14));
		EXPECT_FALSE(b.contains(16));


		b.insert(12);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(3, b.size());

		EXPECT_TRUE(b.contains(12));
		EXPECT_TRUE(b.contains(14));
		EXPECT_FALSE(b.contains(16));


		// remove some items

		b.remove(12);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(2, b.size());

		EXPECT_TRUE(b.contains(12));
		EXPECT_TRUE(b.contains(14));
		EXPECT_FALSE(b.contains(16));


		b.remove(12);
		EXPECT_FALSE(b.empty());
		EXPECT_EQ(1, b.size());

		EXPECT_FALSE(b.contains(12));
		EXPECT_TRUE(b.contains(14));
		EXPECT_FALSE(b.contains(16));


		b.remove(14);
		EXPECT_TRUE(b.empty());
		EXPECT_EQ(0, b.size());

		EXPECT_FALSE(b.contains(12));
		EXPECT_FALSE(b.contains(14));
		EXPECT_FALSE(b.contains(16));

	}


	TEST(Bag,Iterators) {

		Bag<int> b;

		// check the empty bag
		EXPECT_EQ(b.begin(),b.end());

		// insert some stuff
		for(int i = 0; i<10; ++i) {
			for(int j=0; j<i; ++j) {
				b.insert(i);
			}
		}

		int counts[10] = { 0 };

		for(const auto& cur : b) {
			counts[cur]++;
		}

		for(int i=0; i<10; ++i) {
			EXPECT_EQ(counts[i],i) << "Off for value " << i;
		}

	}

	TEST(Bag, String) {

		Bag<int> a;

		EXPECT_EQ("{}",toString(a));

		a.insert(12);
		EXPECT_EQ("{12}",toString(a));

		a.insert(14);
		EXPECT_EQ("{12,14}",toString(a));

		a.insert(12);
		EXPECT_EQ("{12,14,12}",toString(a));

	}

	TEST(Bag, UpdateAndFilter) {

		Bag<int> b;

		for(int i=0; i<10; i++) {
			b.insert(i);
		}

		EXPECT_EQ(10, b.size());

		// filter out all odd numbers
		b.filter([](int i) { return (i % 2) == 1; });

		EXPECT_EQ(5, b.size());

		for(int i = 1; i<10; i+=2) {
			EXPECT_TRUE(b.contains(i)) << "Missing " << i;
		}

		// increase all by one and filter even values
		b.updateFilter([](int& cur) {
			cur++;
			return (cur % 3) != 0;
		});

		EXPECT_EQ(4, b.size());

		for(int i : { 2, 4, 8, 10 }) {
			EXPECT_TRUE(b.contains(i)) << "Missing " << i;
		}
	}


} // end namespace utils
} // end namespace allscale
