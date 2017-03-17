#include <gtest/gtest.h>

#include "allscale/utils/range.h"

namespace allscale {
namespace utils {

	template<typename Iter>
	std::size_t count(const range<Iter>& r) {
		std::size_t count = 0;
		for(auto it = r.begin(); it != r.end(); ++it) {
			count++;
		}
		return count;
	}


	TEST(Range, PlainArray) {

		int data[12] = { 1, 2, 3 };

		using range = range<int*>;

		range e { &data[0],&data[0] };

		EXPECT_TRUE(e.empty());
		EXPECT_EQ(0,count(e));
		EXPECT_EQ(e.size(),count(e));



		range a { &data[0],&data[12] };
		EXPECT_FALSE(a.empty());
		EXPECT_EQ(12,count(a));
		EXPECT_EQ(a.size(),count(a));

		EXPECT_EQ(1, a.front());
		EXPECT_EQ(0, a.back());

	}


	TEST(Range, Vector) {

		std::vector<int> data(12);
		data[0] = 1;
		data[1] = 2;
		data[2] = 3;

		using range = range<std::vector<int>::const_iterator>;

		range e { data.begin(),data.begin() };

		EXPECT_TRUE(e.empty());
		EXPECT_EQ(0,count(e));
		EXPECT_EQ(e.size(),count(e));



		range a { data.begin(),data.end() };
		EXPECT_FALSE(a.empty());
		EXPECT_EQ(12,count(a));
		EXPECT_EQ(a.size(),count(a));

		EXPECT_EQ(1, a.front());
		EXPECT_EQ(0, a.back());

		EXPECT_NE(e,data);
		EXPECT_NE(data,e);

		EXPECT_EQ(a,data);
		EXPECT_EQ(data,a);

	}

} // end namespace utils
} // end namespace allscale
