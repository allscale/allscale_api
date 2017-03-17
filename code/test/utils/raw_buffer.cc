#include <gtest/gtest.h>

#include <type_traits>

#include "allscale/utils/raw_buffer.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace utils {

	TEST(RawBuffer,TypeProperties) {

		EXPECT_TRUE(std::is_copy_constructible<RawBuffer>::value);
		EXPECT_TRUE(std::is_move_constructible<RawBuffer>::value);

		EXPECT_TRUE(std::is_copy_assignable<RawBuffer>::value);
		EXPECT_TRUE(std::is_move_assignable<RawBuffer>::value);

		EXPECT_TRUE(std::is_destructible<RawBuffer>::value);

	}

	TEST(RawBuffer,Basic) {

		int data[5] = { 1, 2, 3, 4, 5 };

		RawBuffer buffer(data);

		// check consuming individual elements
		EXPECT_EQ(1,buffer.consume<int>());
		EXPECT_EQ(2,buffer.consume<int>());

		// consume an array of elements
		int* array = buffer.consumeArray<int>(2);
		EXPECT_EQ(data[2],array[0]);
		EXPECT_EQ(data[3],array[1]);
		EXPECT_EQ(data+2,array);

		// check final element
		EXPECT_EQ(5,buffer.consume<int>());
	}

} // end namespace utils
} // end namespace allscale
