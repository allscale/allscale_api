#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include "allscale/api/core/impl/reference/runtime_predictor.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	using duration = RuntimePredictor::duration;


	TEST(RuntimePredictor, Basic) {

		RuntimePredictor predictor;

		// the highest layer should run infinitely (initially)
		EXPECT_EQ(duration::max(),predictor.predictTime(0));

		// the lowest layer should run in zero time (initially)
		EXPECT_EQ(duration::zero(), predictor.predictTime(RuntimePredictor::MAX_LEVELS-1));
	}

	TEST(RuntimePredictor, Estimate) {

		RuntimePredictor predictor;

		// register a time
		predictor.registerTime(5,duration(64000));

		EXPECT_EQ(duration(256000),predictor.predictTime(3));
		EXPECT_EQ(duration(128000),predictor.predictTime(4));
		EXPECT_EQ(duration(64000),predictor.predictTime(5));
		EXPECT_EQ(duration(32000),predictor.predictTime(6));
		EXPECT_EQ(duration(16000),predictor.predictTime(7));

		predictor.registerTime(5,duration(64000));

		EXPECT_EQ(duration(256000),predictor.predictTime(3));
		EXPECT_EQ(duration(128000),predictor.predictTime(4));
		EXPECT_EQ(duration(64000),predictor.predictTime(5));
		EXPECT_EQ(duration(32000),predictor.predictTime(6));
		EXPECT_EQ(duration(16000),predictor.predictTime(7));

		predictor.registerTime(6,duration(64000));

		EXPECT_LT(duration(256000),predictor.predictTime(3));
		EXPECT_LT(duration(128000),predictor.predictTime(4));
		EXPECT_LT(duration(64000),predictor.predictTime(5));
		EXPECT_LT(duration(32000),predictor.predictTime(6));
		EXPECT_LT(duration(16000),predictor.predictTime(7));

	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
