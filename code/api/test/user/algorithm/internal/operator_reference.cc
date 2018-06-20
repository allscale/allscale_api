#include <gtest/gtest.h>

#include <atomic>
#include <type_traits>

#include "allscale/api/user/algorithm/internal/operation_reference.h"

#include "allscale/api/core/prec.h"
#include "allscale/api/user/algorithm/async.h"

namespace allscale {
namespace api {
namespace user {
namespace algorithm {
namespace internal {

	TEST(OperatorReference,TypeTraits) {

		EXPECT_TRUE(std::is_default_constructible<operation_reference>::value);

		EXPECT_FALSE(std::is_copy_constructible<operation_reference>::value);
		EXPECT_TRUE(std::is_move_constructible<operation_reference>::value);

		EXPECT_FALSE(std::is_copy_assignable<operation_reference>::value);
		EXPECT_TRUE(std::is_move_assignable<operation_reference>::value);

		EXPECT_TRUE(std::is_destructible<operation_reference>::value);

	}

	TEST(OperatorReference,NoTask) {

		operation_reference empty;

		EXPECT_TRUE(empty.isDone());
		EXPECT_FALSE(empty.isValid());

	}


	TEST(OperatorReference,SimpleTask) {

		std::atomic<int> counter(0);

		operation_reference task = algorithm::async([&]{
				counter = 1;
		});

		EXPECT_TRUE(task.isValid());

		// wait for the task to complete
		task.wait();

		// check whether side-effects took place
		EXPECT_EQ(1,counter.load());
		EXPECT_TRUE(task.isDone());

	}


	TEST(OperatorReference,Move) {

		std::atomic<int> counter(0);

		operation_reference task = algorithm::async([&]{
				counter = 1;
		});

		EXPECT_TRUE(task.isValid());

		task.wait();
		EXPECT_TRUE(task.isDone());

		// -- move task through constructor --
		operation_reference moveCtr = std::move(task);

		// check validity
		EXPECT_FALSE(task.isValid());
		EXPECT_TRUE(moveCtr.isValid());

		EXPECT_TRUE(task.isDone());

		// -- move through assignment --
		operation_reference moveAss;
		moveAss = std::move(moveCtr);

		EXPECT_FALSE(task.isValid());
		EXPECT_FALSE(moveCtr.isValid());
		EXPECT_TRUE(moveAss.isValid());

		EXPECT_TRUE(task.isDone());
		EXPECT_TRUE(moveCtr.isDone());

		// wait for the completion
		moveAss.wait();
		EXPECT_EQ(1,counter.load());

		EXPECT_TRUE(task.isDone());
		EXPECT_TRUE(moveCtr.isDone());
		EXPECT_TRUE(moveAss.isDone());

	}


	TEST(OperatorReference,Scoping) {

		std::atomic<int> counter(0);

		operation_reference task = algorithm::async([&]{
				counter = 1;
		});

		{
			// move the task to a copy
			operation_reference cpy = std::move(task);
			// here there should be an implicit wait
		}

		// the counter should be 1 now
		EXPECT_EQ(1,counter.load());

	}


	TEST(OperatorReference,Detach) {

		std::atomic<int> counter(0);

		operation_reference task = algorithm::async([&]{
				counter = 1;
		});

		EXPECT_TRUE(task.isValid());

		auto job = task.detach();

		EXPECT_FALSE(task.isValid());
		EXPECT_TRUE(task.isDone());

		job.wait();

		EXPECT_EQ(1,counter.load());
		EXPECT_TRUE(job.isDone());

	}


} // end namespace internal
} // end namespace algorithm
} // end namespace user
} // end namespace api
} // end namespace allscale
