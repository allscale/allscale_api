#define REFERENCE_IMPL
#include "../../treeture.cc"

TEST(Basic,ImplCheck) {
	EXPECT_EQ("Reference SharedMemory", allscale::api::core::impl::getImplementationName());
}

namespace allscale {
namespace api {
namespace core {
namespace impl {

	TEST(BitQueue, Basic) {
		BitQueue queue;

		// check put and pop
		int x = 577;
		for(int i=0; i<32; i++) {
			queue.put(x & (1<<i));
		}

		int y = 0;
		for(int i=0; i<32; i++) {
			if (queue.pop()) {
				y = y | (1<<i);
			}
		}

		EXPECT_EQ(x,y);

		// check the get function
		for(int i=0; i<32; i++) {
			queue.put(x & (1<<i));
		}

		y = 0;
		for(int i=0; i<32; i++) {
			if (queue.get()) {
				y = y | (1<<i);
			}
			queue.pop();
		}

		EXPECT_EQ(x,y);
	}

} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
