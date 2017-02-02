#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/core/impl/reference/queue.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	TEST(TaskQueue, Basic) {

		BoundQueue<int,3> queue;

		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(0, queue.size());


		EXPECT_TRUE(queue.push_front(12));
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1, queue.size());

		EXPECT_EQ(12, queue.pop_front());

		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(0, queue.size());

		EXPECT_TRUE(queue.push_front(12));

		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1, queue.size());

		EXPECT_EQ(12, queue.pop_back());

	}

	TEST(TaskQueue, Size) {

		BoundQueue<int,3> queue;

		EXPECT_EQ(0, queue.size());
		queue.push_front(1);
		EXPECT_EQ(1, queue.size());
		queue.push_front(1);
		EXPECT_EQ(2, queue.size());
		queue.push_front(1);
		EXPECT_EQ(3, queue.size());

		for (int i =0; i<10; i++) {
			queue.pop_front();
			EXPECT_EQ(2, queue.size());
			queue.pop_front();
			EXPECT_EQ(1, queue.size());

			queue.push_front(1);
			EXPECT_EQ(2, queue.size());
			queue.push_front(1);
			EXPECT_EQ(3, queue.size());
		}

	}


	TEST(TaskQueue, Order) {


		BoundQueue<int,3> queue;

		// fill queue in the front
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_front(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the back
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(1,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(3,queue.pop_back());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

		// fill queue in the front again
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_front(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_front(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the front
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(3,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1,queue.pop_front());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

		// fill queue in the back
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_back(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the front
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(1,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_front());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(3,queue.pop_front());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

		// fill queue in the back again
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(1)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(2)) << queue;
		EXPECT_FALSE(queue.full());
		EXPECT_TRUE(queue.push_back(3)) << queue;
		EXPECT_TRUE(queue.full());
		EXPECT_FALSE(queue.push_back(4)) << queue;
		EXPECT_TRUE(queue.full());

		// pop in the back
		EXPECT_FALSE(queue.empty());
		EXPECT_TRUE(queue.full());
		EXPECT_EQ(3,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(2,queue.pop_back());
		EXPECT_FALSE(queue.empty());
		EXPECT_FALSE(queue.full());
		EXPECT_EQ(1,queue.pop_back());
		EXPECT_TRUE(queue.empty());
		EXPECT_FALSE(queue.full());

	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
