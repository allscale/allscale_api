#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "allscale/api/core/impl/reference/profiling.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	TEST(ProfileLogEntry, TypeProperties) {
		EXPECT_TRUE(std::is_trivially_constructible<ProfileLogEntry>::value);
		EXPECT_TRUE(std::is_trivially_copy_constructible<ProfileLogEntry>::value);
		EXPECT_TRUE(std::is_trivially_copy_assignable<ProfileLogEntry>::value);
	}

	TEST(ProfileLog, WriteRead) {

		// the number of entries to be tested
		const int N = 10000000;

		// create a profiler log
		ProfileLog log;

		// add some entries
		for(int i=0; i<N; ++i) {
			log << ProfileLogEntry::createTaskStartedEntry(WorkItemID(i));
		}

		// read the log
		int i = 0;
		for(auto& cur : log) {
			EXPECT_EQ(cur.getKind(), ProfileLogEntry::TaskStarted);
			EXPECT_EQ(i,cur.getWorkItem().getRootID());
			++i;
		}

		EXPECT_EQ(i,N);

	}

	TEST(ProfileLog, WriteStoreLoadRead) {

		// the number of entries to be tested
		const int N = 10000000;

		// create a in-memory stream
		std::stringstream buffer(std::ios_base::out | std::ios_base::in | std::ios_base::binary);

		{
			// create a profiler log
			ProfileLog log;

			// add some entries
			for(int i=0; i<N; ++i) {
				log << ProfileLogEntry::createTaskStartedEntry(WorkItemID(i));
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
				EXPECT_EQ(i,cur.getWorkItem().getRootID());
				++i;
			}

			EXPECT_EQ(i,N);

		}
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
