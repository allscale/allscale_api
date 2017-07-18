#include <gtest/gtest.h>

#include "allscale/utils/serializer.h"

namespace allscale {
namespace utils {

	struct Serializable {
		static Serializable load(ArchiveReader&) {
			return Serializable();
		}
		void store(ArchiveWriter&) const {}
	};

	struct NonSerializable {

	};

	struct LoadOnly {
		static Serializable load(ArchiveReader&) {
			return Serializable();
		}
	};

	struct StoreOnly {
		void store(ArchiveWriter&) const {}
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

	TEST(Archive, TypeTraits) {

		// nothing should be trivial
		EXPECT_FALSE(std::is_trivially_constructible<Archive>::value);
		EXPECT_FALSE(std::is_trivially_copy_assignable<Archive>::value);
		EXPECT_FALSE(std::is_trivially_move_assignable<Archive>::value);
		EXPECT_FALSE(std::is_trivially_destructible<Archive>::value);


		// it should only be move-able
		EXPECT_FALSE(std::is_default_constructible<Archive>::value);
		EXPECT_TRUE(std::is_copy_constructible<Archive>::value);
		EXPECT_TRUE(std::is_copy_assignable<Archive>::value);

		EXPECT_TRUE(std::is_move_constructible<Archive>::value);
		EXPECT_TRUE(std::is_move_assignable<Archive>::value);
		EXPECT_TRUE(std::is_destructible<Archive>::value);

	}

	TEST(ArchiveWriter, TypeTraits) {

		// check non-trivial versions
		EXPECT_TRUE(std::is_default_constructible<ArchiveWriter>::value);
		EXPECT_FALSE(std::is_copy_constructible<ArchiveWriter>::value);
		EXPECT_FALSE(std::is_copy_assignable<ArchiveWriter>::value);

		EXPECT_TRUE(std::is_move_constructible<ArchiveWriter>::value);
		EXPECT_TRUE(std::is_move_assignable<ArchiveWriter>::value);
		EXPECT_TRUE(std::is_destructible<ArchiveWriter>::value);

	}

	TEST(ArchiveReader, TypeTraits) {

		// it should only be move-able
		EXPECT_FALSE(std::is_default_constructible<ArchiveReader>::value);
		EXPECT_FALSE(std::is_copy_constructible<ArchiveReader>::value);
		EXPECT_FALSE(std::is_copy_assignable<ArchiveReader>::value);

		EXPECT_TRUE(std::is_move_constructible<ArchiveReader>::value);
		EXPECT_TRUE(std::is_move_assignable<ArchiveReader>::value);
		EXPECT_TRUE(std::is_destructible<ArchiveReader>::value);

	}

	TEST(Archive, WriteRead) {

		// test basic application of archive

		int x = 12;

		ArchiveWriter writer;
		writer.write(x);

		Archive a = std::move(writer).toArchive();

		ArchiveReader reader1(a);
		EXPECT_EQ(x,reader1.read<int>());

		ArchiveReader reader2(a);
		EXPECT_EQ(x,reader2.read<int>());
	}

	TEST(SerializeDeserialize, Int) {

		int x = 10;

		Archive a = serialize(x);
		EXPECT_EQ(x,deserialize<int>(a));

	}

} // end namespace utils
} // end namespace allscale
