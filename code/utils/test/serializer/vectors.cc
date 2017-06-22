#include <gtest/gtest.h>

#include "allscale/utils/serializer/vectors.h"
#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {

	struct NotSerializable {};


	TEST(Serializer, StdVectors) {

		// check that strings are recognized as serializable
		EXPECT_TRUE((is_serializable<std::vector<int>>::value));
		EXPECT_TRUE((is_serializable<std::vector<float>>::value));
		EXPECT_TRUE((is_serializable<std::vector<bool>>::value));
		EXPECT_TRUE((is_serializable<std::vector<double>>::value));

		// this should work also for a nested vectors
		EXPECT_TRUE((is_serializable<std::vector<std::vector<int>>>::value));

		// and with other types
		EXPECT_TRUE((is_serializable<std::vector<std::string>>::value));

		// but not with non-serializable typse
		EXPECT_FALSE((is_serializable<NotSerializable>::value));
		EXPECT_FALSE((is_serializable<std::vector<NotSerializable>>::value));

	}

	TEST(Serializer,StdVectorInt) {
		// serialize and de-serialize a vector
		std::vector<int> in { 1, 2, 3, 4 };
		auto archive = serialize(in);
		auto out = deserialize<std::vector<int>>(archive);

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


		friend std::ostream& operator<<(std::ostream& out, const SerializableButNotDefaultConstructable& obj) {
			return out << obj.x;
		}

	};


	TEST(Serializer,StdVectorNoDefaultConstructor) {

		// make sure the test type is realy not default-constructable
		EXPECT_FALSE(std::is_default_constructible<SerializableButNotDefaultConstructable>::value);

		// but it is serializable
		EXPECT_TRUE(is_serializable<SerializableButNotDefaultConstructable>::value);

		// and so is the enclosing array
		EXPECT_TRUE((is_serializable<std::vector<SerializableButNotDefaultConstructable>>::value));

		// serialize and de-serialize a string
		std::vector<SerializableButNotDefaultConstructable> in {
			SerializableButNotDefaultConstructable::get(1),
			SerializableButNotDefaultConstructable::get(2),
			SerializableButNotDefaultConstructable::get(3)
		};

		auto archive = serialize(in);
		auto out = deserialize<std::vector<SerializableButNotDefaultConstructable>>(archive);

		EXPECT_EQ(in,out);
	}

	// -- and the other Vector --



	TEST(Serializer, Vectors) {

		// check that strings are recognized as serializable
		EXPECT_TRUE((is_serializable<Vector<int,1>>::value));
		EXPECT_TRUE((is_serializable<Vector<float,2>>::value));
		EXPECT_TRUE((is_serializable<Vector<bool,3>>::value));
		EXPECT_TRUE((is_serializable<Vector<double,4>>::value));

		// this should work also for a nested vectors
		EXPECT_TRUE((is_serializable<Vector<Vector<int,3>,4>>::value));

		// and with other types
		EXPECT_TRUE((is_serializable<Vector<std::string,2>>::value));

		// but not with non-serializable typse
		EXPECT_FALSE((is_serializable<NotSerializable>::value));
		EXPECT_FALSE((is_serializable<Vector<NotSerializable,4>>::value));

	}

	TEST(Serializer,VectorInt) {
		// serialize and de-serialize a vector
		Vector<int,4> in { 1, 2, 3, 4 };
		auto archive = serialize(in);
		auto out = deserialize<Vector<int,4>>(archive);

		EXPECT_EQ(in,out);
	}

	TEST(Serializer,VectorNoDefaultConstructor) {

		// make sure the test type is realy not default-constructable
		EXPECT_FALSE(std::is_default_constructible<SerializableButNotDefaultConstructable>::value);

		// but it is serializable
		EXPECT_TRUE(is_serializable<SerializableButNotDefaultConstructable>::value);

		// and so is the enclosing array
		EXPECT_TRUE((is_serializable<std::vector<SerializableButNotDefaultConstructable>>::value));

		// serialize and de-serialize a string
		Vector<SerializableButNotDefaultConstructable,3> in {
			SerializableButNotDefaultConstructable::get(1),
			SerializableButNotDefaultConstructable::get(2),
			SerializableButNotDefaultConstructable::get(3)
		};

		auto archive = serialize(in);
		auto out = deserialize<Vector<SerializableButNotDefaultConstructable,3>>(archive);

		EXPECT_EQ(in,out);
	}

} // end namespace utils
} // end namespace allscale
