#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/user/operator/ops.h"

namespace allscale {
namespace api {
namespace user {

	TEST(Ops, Reduce) {
		auto plus = [](int a, int b) { return a + b; };

		std::vector<int> v = { 1, 2, 3, 4, 5 };
		EXPECT_EQ(15, preduce(v, plus));

		std::vector<int> e = { };
		EXPECT_EQ(0, preduce(e, plus));

		auto concat = [](std::string a, std::string b) { return a + b; };
		std::vector<std::string> s = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
				"o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};
		EXPECT_EQ("abcdefghijklmnopqrstuvwxyz", preduce(s, concat));
	}

} // end namespace user
} // end namespace api
} // end namespace allscale
