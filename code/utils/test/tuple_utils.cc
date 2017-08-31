#include <gtest/gtest.h>

#include "allscale/utils/tuple_utils.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace utils {

	TEST(Tuple,ForEach) {

		auto tuple = std::make_tuple(0,2,4.3);

		EXPECT_EQ("(0,2,4.3)",toString(tuple));

		forEach(tuple,[](auto& cur) {
			cur += 1;
		});

		EXPECT_EQ("(1,3,5.3)",toString(tuple));

		double sum = 0;
		forEach(const_cast<const std::tuple<int,int,double>&>(tuple),[&](const auto cur) {
			sum += cur;
		});

		EXPECT_DOUBLE_EQ(9.3,sum);
	}


} // end namespace utils
} // end namespace allscale
