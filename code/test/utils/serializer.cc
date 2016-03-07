#include <gtest/gtest.h>

#include "allscale/utils/serializer.h"

namespace allscale {
namespace utils {

	struct Serializable {
		static Serializable load(Archive&) {
			return Serializable();
		}
		void store(Archive&) const {}
	};

	struct NonSerializable {

	};

	struct LoadOnly {
		static Serializable load(Archive&) {
			return Serializable();
		}
	};

	struct StoreOnly {
		void store(Archive&) const {}
	};

	TEST(Data, Serializable) {

		// check primitive types
		EXPECT_TRUE(is_serializable<int>::value);
		EXPECT_TRUE(is_serializable<char>::value);
		EXPECT_TRUE(is_serializable<unsigned>::value);
		EXPECT_TRUE(is_serializable<float>::value);
		EXPECT_TRUE(is_serializable<double>::value);

		EXPECT_TRUE(is_serializable<const int>::value);
		EXPECT_TRUE(is_serializable<const float>::value);
		EXPECT_TRUE(is_serializable<const double>::value);


		// make sure pointers can not be serialized
		EXPECT_FALSE(is_serializable<int*>::value);

		// check
		EXPECT_TRUE(is_serializable<Serializable>::value);
		EXPECT_FALSE(is_serializable<NonSerializable>::value);
		EXPECT_FALSE(is_serializable<LoadOnly>::value);
		EXPECT_FALSE(is_serializable<StoreOnly>::value);

	}

} // end namespace utils
} // end namespace allscale
