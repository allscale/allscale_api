#include <gtest/gtest.h>

#include <vector>

//#include "allscale/api/core/impl/simple_old/runtime/runtime.h"

namespace allscale {
namespace api {
namespace core {

	TEST(Runtime, Basic) {

//		int i = 0;
//
//		spawn([&](){ i++; }).get();
//		EXPECT_EQ(1,i);
//
//		spawn([&](){ i++; }).get();
//		EXPECT_EQ(2,i);
//
//		auto f = spawn([&]() { return i; });
//		EXPECT_EQ(2, f.get());

	}

//	TEST(Runtime, Stress) {
//
//		std::atomic<int> c;
//		c = 0;
////		int N = 10000000;
//		int N = 1000;
//
//		std::vector<Future<void>> list;
//		for(int i=0; i<N; i++) {
//			list.push_back(spawn([&]() {
//				c++;
//			}));
//		}
//
//		for(const auto& cur : list) {
//			cur.get();
//		}
//
//		EXPECT_EQ(N,c);
//
//	}
//
//	TEST(TaskQueue, Basic) {
//
//		SimpleQueue<int,3> queue;
//
//		EXPECT_TRUE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		std::cout << queue << "\n";
//
//		EXPECT_TRUE(queue.push_front(12));
//		std::cout << queue << "\n";
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		EXPECT_EQ(12, queue.pop_front());
//		std::cout << queue << "\n";
//
//		EXPECT_TRUE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		EXPECT_TRUE(queue.push_front(12));
//
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		std::cout << queue << "\n";
//		EXPECT_EQ(12, queue.pop_back());
//		std::cout << queue << "\n";
//
//	}
//
//	TEST(TaskQueue, Order) {
//
//
//		SimpleQueue<int,3> queue;
//
//		// fill queue in the front
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_front(1)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_front(2)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_front(3)) << queue;
//		EXPECT_TRUE(queue.full());
//		EXPECT_FALSE(queue.push_front(4)) << queue;
//		EXPECT_TRUE(queue.full());
//
//		// pop in the back
//		EXPECT_FALSE(queue.empty());
//		EXPECT_TRUE(queue.full());
//		EXPECT_EQ(1,queue.pop_back());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(2,queue.pop_back());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(3,queue.pop_back());
//		EXPECT_TRUE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		// fill queue in the front again
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_front(1)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_front(2)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_front(3)) << queue;
//		EXPECT_TRUE(queue.full());
//		EXPECT_FALSE(queue.push_front(4)) << queue;
//		EXPECT_TRUE(queue.full());
//
//		// pop in the front
//		EXPECT_FALSE(queue.empty());
//		EXPECT_TRUE(queue.full());
//		EXPECT_EQ(3,queue.pop_front());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(2,queue.pop_front());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(1,queue.pop_front());
//		EXPECT_TRUE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		// fill queue in the back
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_back(1)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_back(2)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_back(3)) << queue;
//		EXPECT_TRUE(queue.full());
//		EXPECT_FALSE(queue.push_back(4)) << queue;
//		EXPECT_TRUE(queue.full());
//
//		// pop in the front
//		EXPECT_FALSE(queue.empty());
//		EXPECT_TRUE(queue.full());
//		EXPECT_EQ(1,queue.pop_front());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(2,queue.pop_front());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(3,queue.pop_front());
//		EXPECT_TRUE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//		// fill queue in the back again
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_back(1)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_back(2)) << queue;
//		EXPECT_FALSE(queue.full());
//		EXPECT_TRUE(queue.push_back(3)) << queue;
//		EXPECT_TRUE(queue.full());
//		EXPECT_FALSE(queue.push_back(4)) << queue;
//		EXPECT_TRUE(queue.full());
//
//		// pop in the back
//		EXPECT_FALSE(queue.empty());
//		EXPECT_TRUE(queue.full());
//		EXPECT_EQ(3,queue.pop_back());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(2,queue.pop_back());
//		EXPECT_FALSE(queue.empty());
//		EXPECT_FALSE(queue.full());
//		EXPECT_EQ(1,queue.pop_back());
//		EXPECT_TRUE(queue.empty());
//		EXPECT_FALSE(queue.full());
//
//	}


} // end namespace core
} // end namespace api
} // end namespace allscale
