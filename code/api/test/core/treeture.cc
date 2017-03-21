#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "allscale/api/core/treeture.h"

namespace allscale {
namespace api {
namespace core {

	TEST(Treeture, Immediates) {

		treeture<void> t1 = done();
		t1.wait();

		treeture<int> t2 = done(12);
		EXPECT_EQ(12, t2.get());

		treeture<std::string> t3 = done(std::string("Hello"));
		EXPECT_EQ("Hello",t3.get());


		// sequential implementation
		impl::sequential::treeture<int> t4 = done(14);
		EXPECT_EQ(14,t4.get());

		// reference implementation
		impl::reference::treeture<int> t5 = done(16);
		EXPECT_EQ(16,t5.get());

	}

} // end namespace core
} // end namespace api
} // end namespace allscale
