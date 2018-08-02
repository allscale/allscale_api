#include <gtest/gtest.h>

#include <type_traits>

#include "allscale/utils/optional.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace utils {

	TEST(Optional,Basics) {

		using opt_type = optional<int>;

		EXPECT_TRUE(std::is_copy_constructible<opt_type>::value);
		EXPECT_TRUE(std::is_move_constructible<opt_type>::value);

		EXPECT_TRUE(std::is_copy_assignable<opt_type>::value);
		EXPECT_TRUE(std::is_move_assignable<opt_type>::value);

		EXPECT_TRUE(std::is_destructible<opt_type>::value);

	}

	TEST(Optional, Copy) {

		using opt_t = optional<int>;

		opt_t none;
		opt_t zero = 0;
		opt_t one = 1;

		EXPECT_FALSE(bool(none));
		EXPECT_TRUE(bool(zero));
		EXPECT_TRUE(bool(one));

		EXPECT_LT(none,zero);
		EXPECT_LT(zero,one);

		EXPECT_EQ(none,none);
		EXPECT_EQ(zero,zero);
		EXPECT_EQ(one,one);

		opt_t cpy = one;

		EXPECT_TRUE(bool(cpy));
		EXPECT_TRUE(bool(one));
		EXPECT_EQ(one,cpy);
	}

	TEST(Optional, Move) {

		using opt_t = optional<int>;

		opt_t one = 1;

		opt_t mov = std::move(one);

		EXPECT_TRUE(bool(mov));
		EXPECT_FALSE(bool(one));
		EXPECT_EQ(1,*mov);
	}


	TEST(Optional, Int) {

		using opt_t = optional<int>;

		opt_t none;
		opt_t zero = 0;
		opt_t one = 1;

		EXPECT_FALSE(bool(none));
		EXPECT_TRUE(bool(zero));
		EXPECT_TRUE(bool(one));

		EXPECT_LT(none,zero);
		EXPECT_LT(zero,one);

	}

	TEST(Optional, Print) {

		using opt_t = optional<int>;

		EXPECT_EQ("Nothing",toString(opt_t()));
		EXPECT_EQ("Just(1)",toString(opt_t(1)));

	}

} // end namespace utils
} // end namespace allscale
