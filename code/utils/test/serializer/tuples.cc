#include <gtest/gtest.h>

#include "allscale/utils/serializer/tuple.h"
#include "allscale/utils/serializer/strings.h"
#include "allscale/utils/serializer/vectors.h"

namespace allscale {
namespace utils {

	struct NotSerializable {};

	TEST(Serializer, StdTuple) {

		// check that pairs are recognized as serializable
		EXPECT_TRUE((is_serializable<std::tuple<>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<int>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<float>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<std::string>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<int, int>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<float, int>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<int, float>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<int, std::string>>::value));
		EXPECT_TRUE((is_serializable<std::tuple<std::string, int>>::value));

		EXPECT_FALSE((is_serializable<std::tuple<NotSerializable>>::value));
		EXPECT_FALSE((is_serializable<std::tuple<int, NotSerializable>>::value));
		EXPECT_FALSE((is_serializable<std::tuple<NotSerializable, int>>::value));


		// no version should be trivially serializable (since layout/organization is unspecified)
		EXPECT_FALSE((is_trivially_serializable<std::tuple<>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::tuple<int, int>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::tuple<float, int>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::tuple<int, float>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::tuple<int, std::string>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::tuple<std::string, int>>::value));

		EXPECT_FALSE((is_trivially_serializable<std::tuple<int, NotSerializable>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::tuple<NotSerializable, int>>::value));

	}

	TEST(Serializer,StdTupleEmpty) {
		// serialize and de-serialize a map
		std::tuple<> in {};
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<>>(archive);
		EXPECT_EQ(in,out);
	}

	TEST(Serializer,StdTupleInt1) {
		// serialize and de-serialize a map
		std::tuple<int> in {2};
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<int>>(archive);
		EXPECT_EQ(in,out);
	}

	TEST(Serializer,StdTupleInt2) {
		// serialize and de-serialize a map
		std::tuple<int, int> in {3, 2};
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<int,int>>(archive);
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


	TEST(Serializer,StdTupleNoDefaultConstructor) {

		// make sure the test type is really not default-constructible
		EXPECT_FALSE(std::is_default_constructible<SerializableButNotDefaultConstructable>::value);

		// but it is serializable
		EXPECT_TRUE(is_serializable<SerializableButNotDefaultConstructable>::value);

		// and so is the enclosing map
		EXPECT_TRUE((is_serializable<std::tuple<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable>>::value));

		// serialize and de-serialize a string
		std::tuple<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable> in
			{ SerializableButNotDefaultConstructable::get(1), SerializableButNotDefaultConstructable::get(2) };

		auto archive = serialize(in);
		auto out = deserialize<std::tuple<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable>>(archive);

		EXPECT_EQ(in,out);
	}



	TEST(Serializer,StdTupleString1) {
		// serialize and de-serialize a map
		std::tuple<std::string> in {"Hello"};
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<std::string>>(archive);
		EXPECT_EQ(in,out);
	}

	TEST(Serializer,StdTupleString2) {
		// serialize and de-serialize a map
		std::tuple<std::string, std::string> in {"Hello", "World"};
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<std::string,std::string>>(archive);
		EXPECT_EQ(in,out);
	}

	TEST(Serializer,StdTupleString3) {
		// serialize and de-serialize a map
		std::tuple<std::string, std::string, std::string> in {"Hello", "World", "Test"};
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<std::string,std::string,std::string>>(archive);
		EXPECT_EQ(in,out);
	}

	TEST(Serializer,StdTupleMixed) {
		// serialize and de-serialize a map
		std::tuple<std::string, int, std::vector<int>> in {"Hello", 12, std::vector<int>() };
		auto archive = serialize(in);
		auto out = deserialize<std::tuple<std::string, int, std::vector<int>>>(archive);
		EXPECT_EQ(in,out);
	}

} // end namespace utils
} // end namespace allscale
