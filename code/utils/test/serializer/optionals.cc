#include <gtest/gtest.h>

#include "allscale/utils/serializer/optionals.h"
#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {

	struct NotSerializable {};


	TEST(Serializer, Optional) {

		// check that pairs are recognized as serializable
		EXPECT_TRUE((is_serializable<optional<int>>::value));
		EXPECT_TRUE((is_serializable<optional<float>>::value));
		EXPECT_TRUE((is_serializable<optional<std::string>>::value));

	}

	TEST(Serializer,OptionalIntNone) {
		// serialize and de-serialize a instance
		optional<int> none;
		auto archive = serialize(none);
		auto out = deserialize<optional<int>>(archive);
		EXPECT_EQ(none,out);
	}

	TEST(Serializer,OptionalIntOne) {
		// serialize and de-serialize a instance
		optional<int> one = 1;
		auto archive = serialize(one);
		auto out = deserialize<optional<int>>(archive);
		EXPECT_EQ(one,out);
	}

} // end namespace utils
} // end namespace allscale
