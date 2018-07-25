#include <gtest/gtest.h>

#include "allscale/utils/finalize.h"

namespace allscale {
namespace utils {

	TEST(Finalize, Basic) {

		int x = 0;
		auto inc = [&]{ x += 1; };

		EXPECT_EQ(0,x);

		// used as intended
		{
			auto _ = run_finally(inc);
			EXPECT_EQ(0,x);
		}

		EXPECT_EQ(1,x);

		// incorrectly used
		{
			run_finally(inc);
			EXPECT_EQ(2,x);
		}

		EXPECT_EQ(2,x);


		// multiple uses
		{
			auto _1 = run_finally(inc);
			auto _2 = run_finally(inc);
			EXPECT_EQ(2,x);
		}

		EXPECT_EQ(4,x);

	}

} // end namespace utils
} // end namespace allscale
