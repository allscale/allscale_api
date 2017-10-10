#include <gtest/gtest.h>

#include <atomic>

#include <chrono>
#include <thread>

#include "allscale/api/user/algorithm/async.h"
#include "allscale/api/core/io.h"

namespace allscale {
namespace api {
namespace user {
namespace algorithm {

	// --- basic async usage ---

	TEST(Async,Basic) {

		auto job = async([]{ return 12; });
		EXPECT_EQ(12,job.get());

	}


	TEST(Async,SideEffects) {

		std::atomic<int> counter(0);

		EXPECT_EQ(0,counter.load());
		core::treeture<void> task = async([&]{
				counter = 1;
		});

		// the given task should be valid
		EXPECT_TRUE(task.isValid());

		// wait for the task to complete
		task.wait();

		// check whether side-effects took place
		EXPECT_EQ(1,counter.load());
		EXPECT_TRUE(task.isDone());

	}


	TEST(Async, Dependencies) {
		std::atomic<int> counter(0);

		auto a = async([&]() {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			counter = 0;
		});

		auto b = async(allscale::api::core::after(a), [&]() {
			ASSERT_EQ(0, counter.load());
			std::this_thread::sleep_for(std::chrono::seconds(1));
			counter = 1;
		});

		auto c = async(allscale::api::core::after(b), [&]() {
			ASSERT_EQ(1, counter.load());
			counter = 2;
		});

		c.wait();

		EXPECT_EQ(2, counter.load());
	}


	TEST(Async,ExecuteOnce) {

		std::atomic<int> counter(0);

		for(int i=0; i<100; i++) {
			EXPECT_EQ(i,counter.load());
			auto job = async([&]{
				counter++;
			});
			EXPECT_TRUE(job.isValid());
			job.wait();
			EXPECT_EQ(i+1,counter.load());
		}

	}

	TEST(Async, WriteFile) {
		const std::string filename("asyncTest.dat");
		core::FileIOManager& manager = core::FileIOManager::getInstance();
		core::Entry binary = manager.createEntry(filename, core::Mode::Binary);

		core::treeture<void> asyncWrite = async([&] {
			// create output stream
			auto fout = manager.openOutputStream(binary);

			// write data
			fout.write<int>(7);

			// close output stream
			manager.close(fout);
		});

		// the given task should be valid
		EXPECT_TRUE(asyncWrite.isValid());

		// wait for the task to complete
		asyncWrite.wait();

		// check file content
		auto fin = manager.openInputStream(binary);
		EXPECT_EQ(7, fin.read<int>());
		manager.close(fin);

		manager.remove(binary);
	}

} // end namespace algorithm
} // end namespace user
} // end namespace api
} // end namespace allscale
