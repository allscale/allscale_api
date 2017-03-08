#include <gtest/gtest.h>

#include <type_traits>

#include "allscale/api/core/io.h"
#include "allscale/utils/serializer.h"

namespace allscale {
namespace api {
namespace core {

	TEST(InputStream, TypeTraits) {

		EXPECT_FALSE(std::is_default_constructible<InputStream>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<InputStream>::value);

		EXPECT_FALSE(std::is_copy_constructible<InputStream>::value);
		EXPECT_TRUE(std::is_move_constructible<InputStream>::value);

		EXPECT_FALSE(std::is_copy_assignable<InputStream>::value);
		EXPECT_FALSE(std::is_move_assignable<InputStream>::value);

		EXPECT_TRUE(allscale::utils::is_serializable<InputStream>::value);

	}


	TEST(OutputStream, TypeTraits) {

		EXPECT_FALSE(std::is_default_constructible<OutputStream>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<OutputStream>::value);

		EXPECT_FALSE(std::is_copy_constructible<OutputStream>::value);
		EXPECT_TRUE(std::is_move_constructible<OutputStream>::value);

		EXPECT_FALSE(std::is_copy_assignable<OutputStream>::value);
		EXPECT_FALSE(std::is_move_assignable<OutputStream>::value);

		EXPECT_TRUE(allscale::utils::is_serializable<OutputStream>::value);

	}

	bool exists(const std::string& name) {
		FileIOManager& mgr = FileIOManager::getInstance();
		return mgr.exists(mgr.createEntry(name));
	}

	bool notExists(const std::string& name) {
		return !exists(name);
	}

	TEST(IO, Buffers_Text) {

		BufferIOManager manager;

		Entry text = manager.createEntry("text", Mode::Text);
		auto out = manager.openOutputStream(text);
		EXPECT_TRUE(out << 'a');

		auto out2 = manager.getOutputStream(text);
		EXPECT_TRUE(out2.atomic([](auto& out) {
			out << 'b';
			out << 'c';
		}));

		manager.close(out);

		auto in = manager.openInputStream(text);
		char x;
		EXPECT_TRUE(in >> x);
		EXPECT_EQ('a',x);
		EXPECT_TRUE(in >> x);
		EXPECT_EQ('b',x);
		EXPECT_TRUE(in >> x);
		EXPECT_EQ('c',x);

		// nevermore
		EXPECT_FALSE(in >> x);

		manager.close(in);
	}

	TEST(IO, Buffers_Binary) {

		BufferIOManager manager;

		Entry binary = manager.createEntry("text", Mode::Binary);
		auto out = manager.openOutputStream(binary);
		out.write(1);

		auto out2 = manager.getOutputStream(binary);
		out2.atomic([](auto& out) {
			out.write(2);
			out.write(3);
		});

		manager.close(out);

		auto in = manager.openInputStream(binary);
		int x;
		EXPECT_TRUE(x = in.read<int>());
		EXPECT_EQ(1,x);
		EXPECT_TRUE(x = in.read<int>());
		EXPECT_EQ(2,x);
		EXPECT_TRUE(x = in.read<int>());
		EXPECT_EQ(3,x);

		manager.close(in);
	}

	TEST(IO, File_Text) {

		FileIOManager& manager = FileIOManager::getInstance();

		Entry text = manager.createEntry("text", Mode::Text);
		auto out = manager.openOutputStream(text);
		EXPECT_TRUE(out << 'a');

		auto out2 = manager.getOutputStream(text);
		EXPECT_TRUE(out2.atomic([](auto& out) {
			out << 'b';
			out << 'c';
		}));

		manager.close(out);

		auto in = manager.openInputStream(text);
		char x;
		EXPECT_TRUE(in >> x);
		EXPECT_EQ('a',x);
		EXPECT_TRUE(in >> x);
		EXPECT_EQ('b',x);
		EXPECT_TRUE(in >> x);
		EXPECT_EQ('c',x);

		// nevermore
		EXPECT_FALSE(in >> x);

		manager.close(in);

		// check existence of file
		EXPECT_PRED1(exists, "text");

		// delete the file
		manager.remove(text);

		// check existence of file
		EXPECT_PRED1(notExists, "text");

	}

	TEST(IO, File_Binary) {

		FileIOManager& manager = FileIOManager::getInstance();

		Entry binary = manager.createEntry("binary", Mode::Binary);
		auto out = manager.openOutputStream(binary);
		out.write(1);

		auto out2 = manager.getOutputStream(binary);
		out2.atomic([](auto& out) {
			out.write(2);
			out.write(3);
		});

		manager.close(out);

		auto in = manager.openInputStream(binary);
		int x;
		EXPECT_TRUE(x = in.read<int>());
		EXPECT_EQ(1,x);
		EXPECT_TRUE(x = in.read<int>());
		EXPECT_EQ(2,x);
		EXPECT_TRUE(x = in.read<int>());
		EXPECT_EQ(3,x);

		manager.close(in);

		// check existence of file
		EXPECT_PRED1(exists, "binary");

		// delete the file
		manager.remove(binary);

		// check existence of file
		EXPECT_PRED1(notExists, "binary");

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
