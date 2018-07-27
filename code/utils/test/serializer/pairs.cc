#include <gtest/gtest.h>

#include "allscale/utils/serializer/pairs.h"
#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {

	struct NotSerializable {};


	TEST(Serializer, StdPairs) {

		// check that pairs are recognized as serializable
		EXPECT_TRUE((is_serializable<std::pair<int, int>>::value));
		EXPECT_TRUE((is_serializable<std::pair<float, int>>::value));
		EXPECT_TRUE((is_serializable<std::pair<int, float>>::value));
		EXPECT_TRUE((is_serializable<std::pair<int, std::string>>::value));
		EXPECT_TRUE((is_serializable<std::pair<std::string, int>>::value));

		EXPECT_FALSE((is_serializable<std::pair<int, NotSerializable>>::value));
		EXPECT_FALSE((is_serializable<std::pair<NotSerializable, int>>::value));



		// check support for trivially serializable elements
		EXPECT_TRUE((is_trivially_serializable<std::pair<int, int>>::value));
		EXPECT_TRUE((is_trivially_serializable<std::pair<float, int>>::value));
		EXPECT_TRUE((is_trivially_serializable<std::pair<int, float>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::pair<int, std::string>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::pair<std::string, int>>::value));

		EXPECT_FALSE((is_trivially_serializable<std::pair<int, NotSerializable>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::pair<NotSerializable, int>>::value));

	}

	TEST(Serializer,StdPairInt) {
		// serialize and de-serialize a map
		std::pair<int, int> in {0, 2};
		auto archive = serialize(in);
		auto out = deserialize<std::pair<int,int>>(archive);
		EXPECT_EQ(in,out);
	}


	class SerializableButNotDefaultConstructable {

		int x;

		SerializableButNotDefaultConstructable(int x) : x(x) {}

	public:

		static SerializableButNotDefaultConstructable get(int x) {
			return SerializableButNotDefaultConstructable(x);
		}

		static SerializableButNotDefaultConstructable load(ArchiveReader& reader) {
			return SerializableButNotDefaultConstructable(reader.read<int>());
		}

		void store(ArchiveWriter& writer) const {
			writer.write<int>(x);
		}

		bool operator==(const SerializableButNotDefaultConstructable& other) const {
			return x == other.x;
		}

		bool operator<(const SerializableButNotDefaultConstructable& other) const {
			return x < other.x;
		}


		friend std::ostream& operator<<(std::ostream& out, const SerializableButNotDefaultConstructable& obj) {
			return out << obj.x;
		}

	};


	TEST(Serializer,StdPairNoDefaultConstructor) {

		// make sure the test type is really not default-constructible
		EXPECT_FALSE(std::is_default_constructible<SerializableButNotDefaultConstructable>::value);

		// but it is serializable
		EXPECT_TRUE(is_serializable<SerializableButNotDefaultConstructable>::value);

		// and so is the enclosing map
		EXPECT_TRUE((is_serializable<std::pair<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable>>::value));

		// serialize and de-serialize a string
		std::pair<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable> in
			{ SerializableButNotDefaultConstructable::get(1), SerializableButNotDefaultConstructable::get(2) };

		auto archive = serialize(in);
		auto out = deserialize<std::pair<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable>>(archive);

		EXPECT_EQ(in,out);
	}

} // end namespace utils
} // end namespace allscale
