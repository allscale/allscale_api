#include <gtest/gtest.h>

#include "allscale/api/core/impl/reference/lock.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	TEST(OptimisticReadWriteLock, ReadOnly) {

		OptimisticReadWriteLock lock;

		// simulate a successful read operation
		for(int i=0; i<10000; ++i) {
			auto lease = lock.start_read();
			EXPECT_TRUE(lock.validate(lease));
		}

	}

	TEST(OptimisticReadWriteLock, ReadWrite) {

		OptimisticReadWriteLock lock;

		// simulate a successful a read and and upgrade to write operation
		for(int i=0; i<10000; ++i) {
			// start a read
			auto lease = lock.start_read();

			// upgrade to a write operation
			EXPECT_TRUE(lock.try_upgrade_to_write(lease));

			// end the write operation
			lock.end_write();
		}

	}

	TEST(OptimisticReadWriteLock, ReadWriteInterleaving) {

		OptimisticReadWriteLock lock;

		auto& lockA = lock;
		auto& lockB = lock;

		// simulate the interleaving

		auto leaseA = lockA.start_read();
		auto leaseB = lockB.start_read();

		// update one to write
		EXPECT_TRUE(lockA.try_upgrade_to_write(leaseA));
		EXPECT_FALSE(lockB.try_upgrade_to_write(leaseB));

		// finish writing
		lockA.end_write();

		EXPECT_FALSE(lockB.validate(leaseB));
		leaseB = lockB.start_read();

		EXPECT_TRUE(lockB.try_upgrade_to_write(leaseB));
		lockB.end_write();
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
