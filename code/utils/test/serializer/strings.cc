#include <gtest/gtest.h>

#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {

	TEST(Serializer, String) {

		// check that strings are recognized as serializable
		EXPECT_TRUE(is_serializable<std::string>::value);

		// serialize and de-serialize a string
		std::string str = "Hello World";
		auto archive = serialize(str);
		auto restore = deserialize<std::string>(archive);

		EXPECT_EQ(str,restore);
	}

} // end namespace utils
} // end namespace allscale
