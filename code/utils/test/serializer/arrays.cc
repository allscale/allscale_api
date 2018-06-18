#include <gtest/gtest.h>

#include <memory>

#include "allscale/utils/serializer/arrays.h"
#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {

	struct NotSerializable {};


	TEST(Serializer, Arrays) {

		// check that strings are recognized as serializable
		EXPECT_TRUE((is_serializable<std::array<int,0>>::value));
		EXPECT_TRUE((is_serializable<std::array<int,1>>::value));
		EXPECT_TRUE((is_serializable<std::array<int,2>>::value));
		EXPECT_TRUE((is_serializable<std::array<int,4>>::value));

		// this should work also for a nested array
		EXPECT_TRUE((is_serializable<std::array<std::array<int,5>,4>>::value));

		// and with other types
		EXPECT_TRUE((is_serializable<std::array<std::string,4>>::value));

		// but not with non-serializable types
		EXPECT_FALSE((is_serializable<NotSerializable>::value));
		EXPECT_FALSE((is_serializable<std::array<NotSerializable,4>>::value));

	}

	TEST(Serializer,ArraysInt) {
		// serialize and de-serialize a string
		std::array<int,4> in { { 1, 2, 3, 4 } };
		auto archive = serialize(in);
		auto out = deserialize<std::array<int,4>>(archive);

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

	};


	TEST(Serializer,ArraysNoDefaultConstructor) {

		// make sure the test type is realy not default-constructable
		EXPECT_FALSE(std::is_default_constructible<SerializableButNotDefaultConstructable>::value);

		// but it is serializable
		EXPECT_TRUE(is_serializable<SerializableButNotDefaultConstructable>::value);

		// and so is the enclosing array
		EXPECT_TRUE((is_serializable<std::array<SerializableButNotDefaultConstructable,3>>::value));

		// serialize and de-serialize a string
		std::array<SerializableButNotDefaultConstructable,3> in {
			{ SerializableButNotDefaultConstructable::get(1),
			  SerializableButNotDefaultConstructable::get(2),
			  SerializableButNotDefaultConstructable::get(3) }
		};

		auto archive = serialize(in);
		auto out = deserialize<std::array<SerializableButNotDefaultConstructable,3>>(archive);

		EXPECT_EQ(in,out);
	}

	TEST(Serializer,TrivialSerializable) {

		EXPECT_TRUE(is_serializable<int>::value);
		EXPECT_TRUE(is_serializable<std::string>::value);

		EXPECT_TRUE((is_serializable<std::array<int,20>>::value));
		EXPECT_TRUE((is_serializable<std::array<std::string,20>>::value));


		EXPECT_TRUE(is_trivially_serializable<int>::value);
		EXPECT_FALSE(is_trivially_serializable<std::string>::value);

		EXPECT_TRUE((is_trivially_serializable<std::array<int,20>>::value));
		EXPECT_FALSE((is_trivially_serializable<std::array<std::string,20>>::value));

	}

	TEST(Serializer,LargeArrayTrivial) {

		const std::size_t N = 1<<20;

		// make sure the point is serializable but not trivially serializable
		EXPECT_TRUE(is_serializable<int>::value);
		EXPECT_TRUE(is_trivially_serializable<int>::value);

		// create a large instance and serialize it
		using ary = std::array<int,N>;
		using ptr = std::unique_ptr<ary>;

		ptr p = std::make_unique<ary>();
		auto archive = serialize(*p);
		ptr q = std::make_unique<ary>(deserialize<ary>(archive));

		EXPECT_EQ(*p,*q);

	}

	struct Point {
		static Point load(ArchiveReader&) {
			return {};
		}
		void store(ArchiveWriter&) const {
			// nothing to do
		}
		bool operator==(const Point&) const {
			return true;
		}
	};

	TEST(Serializer,LargeArrayNonTrivial) {

		// Known bug: too large arrays of non-trivially serializable data
		// can not be reloaded due to extensive template unfolding

		const std::size_t N = 1<<4;

		// make sure the point is serializable but not trivially serializable
		EXPECT_TRUE(is_serializable<Point>::value);
		EXPECT_FALSE(is_trivially_serializable<Point>::value);

		// create a large instance and serialize it
		using ary = std::array<Point,N>;
		using ptr = std::unique_ptr<ary>;

		ptr p = std::make_unique<ary>();
		auto archive = serialize(*p);
		ptr q = std::make_unique<ary>(deserialize<ary>(archive));

		EXPECT_EQ(*p,*q);

	}

} // end namespace utils
} // end namespace allscale
