#include <gtest/gtest.h>

#include "allscale/utils/serializer/unordered_maps.h"
#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {

	struct NotSerializable {};


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

		int getX() const {
			return x;
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


} // end namespace utils
} // end namespace allscale

namespace std {

	template<>
	struct hash<allscale::utils::NotSerializable> {
		std::size_t operator()(const allscale::utils::NotSerializable&) const {
			return 0;
		}
	};

	template<>
	struct hash<allscale::utils::SerializableButNotDefaultConstructable> {
		std::size_t operator()(const allscale::utils::SerializableButNotDefaultConstructable& val) const {
			return val.getX();;
		}
	};



} // end namespace std


namespace allscale {
namespace utils {



	TEST(Serializer, StdUnorderedMaps) {

		// check that strings are recognized as serializable
		EXPECT_TRUE((is_serializable<std::unordered_map<int, int>>::value));
		EXPECT_TRUE((is_serializable<std::unordered_map<float,float>>::value));
		EXPECT_TRUE((is_serializable<std::unordered_map<bool,int>>::value));
		EXPECT_TRUE((is_serializable<std::unordered_map<double,float>>::value));

		// but not with non-serializable types
		EXPECT_FALSE((is_serializable<std::unordered_map<double, NotSerializable>>::value));
		EXPECT_FALSE((is_serializable<std::unordered_map<NotSerializable, int>>::value));

		// and with other types
		EXPECT_TRUE((is_serializable<std::unordered_map<std::string,std::string>>::value));

	}

	TEST(Serializer,StdUnorderedMapInt) {
		// serialize and de-serialize a map
		std::unordered_map<int, int> in = {{0, 2}, {12, 929}, {47, 42}};
		auto archive = serialize(in);
		auto out = deserialize<std::unordered_map<int,int>>(archive);

		EXPECT_EQ(in,out);
	}




	TEST(Serializer,StdUnorderedMapNoDefaultConstructor) {

		// make sure the test type is really not default-constructible
		EXPECT_FALSE(std::is_default_constructible<SerializableButNotDefaultConstructable>::value);

		// but it is serializable
		EXPECT_TRUE(is_serializable<SerializableButNotDefaultConstructable>::value);

		// and so is the enclosing map
		EXPECT_TRUE((is_serializable<std::unordered_map<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable>>::value));

		// serialize and de-serialize a string
		std::unordered_map<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable> in {
			{ SerializableButNotDefaultConstructable::get(1), SerializableButNotDefaultConstructable::get(2) },
			{ SerializableButNotDefaultConstructable::get(3), SerializableButNotDefaultConstructable::get(4) },
			{ SerializableButNotDefaultConstructable::get(5), SerializableButNotDefaultConstructable::get(6) }
		};

		auto archive = serialize(in);
		auto out = deserialize<std::unordered_map<SerializableButNotDefaultConstructable, SerializableButNotDefaultConstructable>>(archive);

		EXPECT_EQ(in,out);
	}

} // end namespace utils
} // end namespace allscale
