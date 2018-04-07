#include <gtest/gtest.h>

#include "allscale/utils/string_utils.h"
#include "allscale/utils/vector.h"

#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace utils {


	TEST(Vector, Vector2DLayout) {

		using Point = Vector<int, 2>;

		Point p;

		ASSERT_EQ(&(p[0]), &(p.x));
		ASSERT_EQ(&(p[1]), &(p.y));

		p[0] = 1;
		p[1] = 2;

		Point p2 = p;

		EXPECT_EQ(1, p.x);
		EXPECT_EQ(2, p.y);

		EXPECT_EQ(1, p2.x);
		EXPECT_EQ(2, p2.y);
	}

	TEST(Vector, Vector3DLayout) {

		using Point = Vector<int, 3>;

		Point p;

		ASSERT_EQ(&(p[0]), &(p.x));
		ASSERT_EQ(&(p[1]), &(p.y));
		ASSERT_EQ(&(p[2]), &(p.z));

		p[0] = 1;
		p[1] = 2;
		p[2] = 3;

		Point p2 = p;

		EXPECT_EQ(1, p.x);
		EXPECT_EQ(2, p.y);
		EXPECT_EQ(3, p.z);

		EXPECT_EQ(1, p2.x);
		EXPECT_EQ(2, p2.y);
		EXPECT_EQ(3, p2.z);
	}

	TEST(Vector, Basic2D) {

		using Point = Vector<int, 2>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values
		(void)p;

		Point p0 = 0;
		EXPECT_EQ("[0,0]", toString(p0));

		Point p1 = 1;
		EXPECT_EQ("[1,1]", toString(p1));

		Point p2 = { 1,2 };
		EXPECT_EQ("[1,2]", toString(p2));


		// -- assignment --

		p1 = p2;
		EXPECT_EQ("[1,2]", toString(p1));
		EXPECT_EQ("[1,2]", toString(p2));


		// -- equality --
		EXPECT_EQ(p1, p2);
		EXPECT_NE(p0, p1);

		// -- addition --
		p1 = p1 + p2;
		EXPECT_EQ("[2,4]", toString(p1));

		p1 += p1;
		EXPECT_EQ("[4,8]", toString(p1));

		// -- subtraction --
		p1 = p1 - p2;
		EXPECT_EQ("[3,6]", toString(p1));

		p1 -= p2;
		EXPECT_EQ("[2,4]", toString(p1));


		// scalar multiplication
		p1 = p1 * 2;
		EXPECT_EQ("[4,8]", toString(p1));
		p1 = 2 * p1;
		EXPECT_EQ("[8,16]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[16,32]", toString(p1));

		// scalar division
		p1 = p1 / 2;
		EXPECT_EQ("[8,16]", toString(p1));
		p1 /= 2;
		EXPECT_EQ("[4,8]", toString(p1));

	}

	TEST(Vector, Basic3D) {

		using Point = Vector<int,3>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values
		(void)p;

		Point p0 = 0;
		EXPECT_EQ("[0,0,0]", toString(p0));

		Point p1 = 1;
		EXPECT_EQ("[1,1,1]", toString(p1));

		Point p2 = {1,2,3};
		EXPECT_EQ("[1,2,3]", toString(p2));


		// -- assignment --

		p1 = p2;
		EXPECT_EQ("[1,2,3]", toString(p1));
		EXPECT_EQ("[1,2,3]", toString(p2));


		// -- equality --
		EXPECT_EQ(p1,p2);
		EXPECT_NE(p0,p1);

		// -- addition --
		p1 = p1 + p2;
		EXPECT_EQ("[2,4,6]", toString(p1));

		p1 += p1;
		EXPECT_EQ("[4,8,12]", toString(p1));

		// -- subtraction --
		p1 = p1 - p2;
		EXPECT_EQ("[3,6,9]", toString(p1));

		p1 -= p2;
		EXPECT_EQ("[2,4,6]", toString(p1));


		// scalar multiplication
		p1 = p1 * 2;
		EXPECT_EQ("[4,8,12]", toString(p1));
		p1 = 2 * p1;
		EXPECT_EQ("[8,16,24]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[16,32,48]", toString(p1));

		// scalar division
		p1 = p1 / 2;
		EXPECT_EQ("[8,16,24]", toString(p1));
		p1 /= 2;
		EXPECT_EQ("[4,8,12]", toString(p1));

		// cross product
		p1 = {1,2,3};
		p2 = {2,3,4};
		auto p3 = crossProduct(p1, p2);
		EXPECT_EQ("[-1,2,-1]", toString(p3));

		Vector<double, 3> temp = { 1,2,3 };
		const Vector<double, 3>& temp2 = temp;
		auto res = temp2 * 2.0;
		EXPECT_EQ("[2,4,6]", toString(res));

	}

	TEST(Vector, BasicND) {

		using Point = Vector<int,5>;

		// -- constructors --

		Point p;		// just works, no restrictions on the values
		(void)p;

		Point p0 = 0;
		EXPECT_EQ("[0,0,0,0,0]", toString(p0));

		Point p1 = 1;
		EXPECT_EQ("[1,1,1,1,1]", toString(p1));

		Point p2 = {1,2,3,4,5};
		EXPECT_EQ("[1,2,3,4,5]", toString(p2));


		// -- assignment --

		p1 = p2;
		EXPECT_EQ("[1,2,3,4,5]", toString(p1));
		EXPECT_EQ("[1,2,3,4,5]", toString(p2));


		// -- equality --
		EXPECT_EQ(p1,p2);
		EXPECT_NE(p0,p1);

		// -- addition --
		p1 = p1 + p2;
		EXPECT_EQ("[2,4,6,8,10]", toString(p1));

		p1 += p1;
		EXPECT_EQ("[4,8,12,16,20]", toString(p1));

		// -- subtraction --
		p1 = p1 - p2;
		EXPECT_EQ("[3,6,9,12,15]", toString(p1));

		p1 -= p2;
		EXPECT_EQ("[2,4,6,8,10]", toString(p1));


		// scalar multiplication
		p1 = p1 * 2;
		EXPECT_EQ("[4,8,12,16,20]", toString(p1));
		p1 = 2 * p1;
		EXPECT_EQ("[8,16,24,32,40]", toString(p1));

		p1 *= 2;
		EXPECT_EQ("[16,32,48,64,80]", toString(p1));

		// scalar division
		p1 = p1 / 2;
		EXPECT_EQ("[8,16,24,32,40]", toString(p1));
		p1 /= 2;
		EXPECT_EQ("[4,8,12,16,20]", toString(p1));

	}

	TEST(Vector, MathUtilities) {
		using Point = Vector<int,3>;

		Point p1 = 16;
		EXPECT_EQ("768", toString(sumOfSquares(p1)));

		Point p2 = 4;
		EXPECT_EQ("[64,64,64]", toString(elementwiseProduct(p1,p2)));
		EXPECT_EQ("[4,4,4]", toString(elementwiseDivision(p1,p2)));

	}

	template<typename T>
	void testStoreLoad(const T& val) {
		auto archive = serialize(val);
		EXPECT_EQ(val,deserialize<T>(archive));
	}

	TEST(Vector, Serializable) {

		EXPECT_TRUE((is_serializable<Vector<int,0>>::value));
		EXPECT_TRUE((is_serializable<Vector<int,1>>::value));
		EXPECT_TRUE((is_serializable<Vector<int,2>>::value));
		EXPECT_TRUE((is_serializable<Vector<int,3>>::value));
		EXPECT_TRUE((is_serializable<Vector<int,4>>::value));

		testStoreLoad(Vector<int,0>());
		testStoreLoad(Vector<int,1>(10));
		testStoreLoad(Vector<int,2>(10,20));
		testStoreLoad(Vector<int,2>(20,10));
		testStoreLoad(Vector<int,3>(10,20,30));
		testStoreLoad(Vector<int,4>{10,20,30,40});

		testStoreLoad(Vector<short,0>());
		testStoreLoad(Vector<short,1>(10));
		testStoreLoad(Vector<short,2>(10,20));
		testStoreLoad(Vector<short,2>(20,10));
		testStoreLoad(Vector<short,3>(10,20,30));
		testStoreLoad(Vector<short,4>{10,20,30,40});
	}

	// -- more complex serialization cases --

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

		// and so is the enclosing vector
		EXPECT_TRUE((is_serializable<Vector<SerializableButNotDefaultConstructable,2>>::value));

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

	TEST(Serializer,VectorTriviallySerializable) {

		EXPECT_TRUE((is_serializable<Vector<int,2>>::value));
		EXPECT_TRUE((is_trivially_serializable<Vector<int,2>>::value));

		EXPECT_TRUE((is_serializable<Vector<std::string,2>>::value));
		EXPECT_FALSE((is_trivially_serializable<Vector<std::string,2>>::value));

	}


} // end namespace utils
} // end namespace allscale
