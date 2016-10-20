#include <gtest/gtest.h>

#include <vector>

#define OMP_CILK_IMPL
#include "allscale/api/core/treeture.h"

namespace allscale {
namespace api {
namespace core {

	TEST(Parec,ImplCheck) {
		EXPECT_EQ("OpenMP/Cilk", impl::getImplementationName());
	}

	TEST(Runtime, DefaultFuture) {
		treeture<int> treeture(0);
		EXPECT_EQ(0,treeture.get());
	}

	TEST(Runtime, SimpleTask) {
		treeture<int> future = spawn([]{ return 12; });
		EXPECT_EQ(12,future.get());
	}

	TEST(Runtime, Spawn) {

		// build a completed task
		treeture<int> d = done(10);
		EXPECT_EQ(10, d.get());

		// build a simple task
		auto f = spawn([](){ return 12; });

		// compute with futures
		EXPECT_EQ(5, add(done(2),done(3)).get());

		// build a splitable task
		auto g = spawn(
				[](){ return 6 + 8; },
				[](){ return add(
						spawn([](){ return 8; }),
						done(6)
					);
				}
		);

		// build an aggregate node
		auto h = add(std::move(f), std::move(g));

		EXPECT_EQ(26, h.get());

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
