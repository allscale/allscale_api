#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#define ENABLE_PROFILING
#include "allscale/api/core/impl/reference/profiling.h"

#include "allscale/api/core/impl/reference/treeture.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	TEST(ProfilingEnableSwitch, Flag) {
		EXPECT_TRUE(PROFILING_ENABLED);
	}

	TEST(ProfileLogEntry, TypeProperties) {
		EXPECT_TRUE(std::is_trivially_constructible<ProfileLogEntry>::value);
		EXPECT_TRUE(std::is_trivially_copy_constructible<ProfileLogEntry>::value);
		EXPECT_TRUE(std::is_trivially_copy_assignable<ProfileLogEntry>::value);
	}

	void testWriteRead(int N) {

		// create a profiler log
		ProfileLog log;

		EXPECT_EQ(log.begin(),log.end());

		// add some entries
		for(int i=0; i<N; ++i) {
			log << ProfileLogEntry::createTaskStartedEntry(TaskID(i));
		}

		// read the log
		int i = 0;
		for(auto& cur : log) {
			EXPECT_EQ(cur.getKind(), ProfileLogEntry::TaskStarted);
			EXPECT_EQ(i,cur.getTask().getRootID());
			++i;
		}

		EXPECT_EQ(i,N);

	}

	TEST(ProfileLog, WriteReadEmpty) {
		testWriteRead(0);
	}

	TEST(ProfileLog, WriteReadShort) {
		EXPECT_LT(500,ProfileLog::BATCH_SIZE);
		testWriteRead(500);
	}

	TEST(ProfileLog, WriteReadMedium) {
		testWriteRead(ProfileLog::BATCH_SIZE + 500);
	}

	TEST(ProfileLog, WriteReadLong) {
		testWriteRead(ProfileLog::BATCH_SIZE * 10 + 500);
	}

	void testWriteStoreLoadAndRead(int N) {

		// create an in-memory buffer
		std::stringstream buffer(std::ios_base::out | std::ios_base::in | std::ios_base::binary);

		{
			// create a profiler log
			ProfileLog log;

			// add some entries
			for(int i=0; i<N; ++i) {
				log << ProfileLogEntry::createTaskStartedEntry(TaskID(i));
			}

			// store log
			log.saveTo(buffer);
		}

		{
			// reload the log
			auto log = ProfileLog::loadFrom(buffer);

			// read the log
			int i = 0;
			for(auto& cur : log) {
				EXPECT_EQ(cur.getKind(), ProfileLogEntry::TaskStarted);
				EXPECT_EQ(i,cur.getTask().getRootID());
				++i;
			}

			EXPECT_EQ(i,N);

		}
	}

	TEST(ProfileLog, WriteStoreLoadReadEmpty) {
		testWriteStoreLoadAndRead(0);
	}

	TEST(ProfileLog, WriteStoreLoadReadShort) {
		EXPECT_LT(500,ProfileLog::BATCH_SIZE);
		testWriteStoreLoadAndRead(500);
	}

	TEST(ProfileLog, WriteStoreLoadReadMedium) {
		testWriteStoreLoadAndRead(ProfileLog::BATCH_SIZE + 500);
	}

	TEST(ProfileLog, WriteStoreLoadReadLong) {
		testWriteStoreLoadAndRead(ProfileLog::BATCH_SIZE * 10 + 500);
	}

	bool exists(const std::string& filename) {
		std::ifstream f(filename.c_str());
		return f.good();
	}

	TEST(ProfileLog, WorkerPoolProfiling) {

		int poolsize = 0;

		{
			// start up a worker pool
			runtime::WorkerPool pool;

			poolsize = pool.getNumWorkers();
			EXPECT_LE(1,poolsize);

			// shut it down (implicit in destructor)
		}

		// see whether there are logs (all but the first, since no log message in those)
		for(int i=1; i<poolsize; ++i) {
			auto file = getLogFileNameForWorker(i);
			EXPECT_PRED1(exists,file);

			// delete the file
			std::remove(file.c_str());
		}

		// there is no additional log file (bad test)
		EXPECT_FALSE(exists(getLogFileNameForWorker(poolsize)));

	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
