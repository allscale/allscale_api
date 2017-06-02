#include <gtest/gtest.h>

#include <array>
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

	TEST(MemoryMappedInput, TypeTraits) {

		EXPECT_FALSE(std::is_default_constructible<MemoryMappedInput>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<MemoryMappedInput>::value);

		EXPECT_TRUE(std::is_copy_constructible<MemoryMappedInput>::value);
		EXPECT_TRUE(std::is_move_constructible<MemoryMappedInput>::value);

		EXPECT_TRUE(std::is_copy_assignable<MemoryMappedInput>::value);
		EXPECT_TRUE(std::is_move_assignable<MemoryMappedInput>::value);

		EXPECT_TRUE(allscale::utils::is_serializable<MemoryMappedInput>::value);

	}

	TEST(MemoryMappedOutput, TypeTraits) {

		EXPECT_FALSE(std::is_default_constructible<MemoryMappedOutput>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<MemoryMappedOutput>::value);

		EXPECT_TRUE(std::is_copy_constructible<MemoryMappedOutput>::value);
		EXPECT_TRUE(std::is_move_constructible<MemoryMappedOutput>::value);

		EXPECT_TRUE(std::is_copy_assignable<MemoryMappedOutput>::value);
		EXPECT_TRUE(std::is_move_assignable<MemoryMappedOutput>::value);

		EXPECT_TRUE(allscale::utils::is_serializable<MemoryMappedOutput>::value);

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

	TEST(IO, MemoryMappedBuffers) {

		using data = std::array<int,1000>;

		BufferIOManager mgr;

		auto entry = mgr.createEntry("element");


		// -- open a memory mapped buffer for writing --
		auto out = mgr.openMemoryMappedOutput(entry,sizeof(data));

		auto& dataOut = out.access<data>();
		for(std::size_t i=0; i<dataOut.size(); ++i) {
			dataOut[i] = (int)i;
		}

		mgr.close(out);


		// -- open it for reading --

		auto in = mgr.openMemoryMappedInput(entry);
		auto& dataIn = in.access<data>();

		EXPECT_EQ(dataOut,dataIn);

		mgr.close(in);

	}

#ifndef _MSC_VER

	TEST(IO, MemoryMappedFiles) {
		static const size_t N = 1000u;

		using data = std::array<int,N>;

		FileIOManager& mgr = FileIOManager::getInstance();

		auto entry = mgr.createEntry("element", Mode::Text);

		// -- open a memory mapped buffer for writing --
		auto out = mgr.openMemoryMappedOutput(entry,sizeof(data));

		auto& dataOut = out.access<data>();
		for(std::size_t i=0; i<dataOut.size(); ++i) {
			dataOut[i] = (int)i;
		}

		// check memory mapped output array
		for(size_t i = 0; i < N; ++i) {
			EXPECT_EQ(i, dataOut[i]);
		}

		mgr.close(out);

		// -- open it for reading --

		auto in = mgr.openMemoryMappedInput(entry);
		auto& dataIn = in.access<data>();

		EXPECT_EQ(dataOut,dataIn); // file associated with dataOut has been closed and file reopened, check seem useful at this point

		// check memory mapped input array
		for(size_t i = 0; i < N; ++i) {
			EXPECT_EQ(i, dataIn[i]);
		}

		mgr.close(in);

		// delete file
	//	mgr.remove(entry);
	}

#endif


	TEST(DISABLED_IO, LargeFile) {

		// file size: 1GB
		const int size = 1024*1024*1024;
		const int N = size / sizeof(int);

		using data = std::array<int,N>;

		FileIOManager& mgr = FileIOManager::getInstance();

		auto entry = mgr.createEntry("element");


		// -- open a memory mapped buffer for writing --
		auto out = mgr.openMemoryMappedOutput(entry,sizeof(data));

		auto& dataOut = out.access<data>();
		for(std::size_t i=0; i<dataOut.size(); ++i) {
			dataOut[i] = (int)i;
		}

		mgr.close(out);


		// -- open it for reading --

		auto in = mgr.openMemoryMappedInput(entry);
		auto& dataIn = in.access<data>();

		EXPECT_EQ(dataOut,dataIn); // file associated with dataOut has been closed and file reopened, check seem useful at this point

		// check a few pseudo random points
		EXPECT_EQ(0, dataIn[0]);
		EXPECT_EQ(42, dataIn[42]);
		EXPECT_EQ(666, dataIn[666]);
		EXPECT_EQ(1836, dataIn[1836]);
		EXPECT_EQ(65438, dataIn[65438]);
		EXPECT_EQ(321684, dataIn[321684]);
		EXPECT_EQ(9871354, dataIn[9871354]);
		EXPECT_EQ(24684312, dataIn[24684312]);
		EXPECT_EQ(268435455, dataIn[268435455]);

		mgr.close(in);

		// delete file
		mgr.remove(entry);
	}


} // end namespace core
} // end namespace api
} // end namespace allscale
