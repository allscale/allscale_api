#include <gtest/gtest.h>

#include <vector>

#include "allscale/api/core/impl/reference/task_id.h"

#include "allscale/utils/string_utils.h"

#include "allscale/utils/printer/vectors.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	bool isParentOf(const TaskID& a, const TaskID& b) {
		return a.isParentOf(b);
	}

	bool isNotParentOf(const TaskID& a, const TaskID& b) {
		return !a.isParentOf(b);
	}

	TEST(TaskID, TypeProperties) {

		EXPECT_TRUE(std::is_default_constructible<TaskID>::value);
		EXPECT_TRUE(std::is_trivially_constructible<TaskID>::value);

		EXPECT_TRUE(std::is_trivially_copy_constructible<TaskID>::value);
		EXPECT_TRUE(std::is_trivially_copy_assignable<TaskID>::value);

		EXPECT_TRUE(std::is_trivially_move_constructible<TaskID>::value);
		EXPECT_TRUE(std::is_trivially_move_assignable<TaskID>::value);

	}

	TEST(TaskID, Basic) {

		TaskID a = 12;
		EXPECT_EQ("T-12",toString(a));

		TaskID b = a;
		EXPECT_EQ(a,b);

		auto c = a.getLeftChild();
		auto d = a.getRightChild();

		EXPECT_NE(a,c);
		EXPECT_NE(a,d);
		EXPECT_NE(c,d);

		EXPECT_EQ("T-12.0",toString(c));
		EXPECT_EQ("T-12.1",toString(d));

		EXPECT_EQ("T-12.1.0.1",toString(d.getLeftChild().getRightChild()));

		EXPECT_PRED2(isNotParentOf,a,a);
		EXPECT_PRED2(isNotParentOf,a,b);

		EXPECT_PRED2(isParentOf,a,c);
		EXPECT_PRED2(isParentOf,a,d);

		EXPECT_PRED2(isParentOf,a,c.getLeftChild());
		EXPECT_PRED2(isParentOf,a,c.getRightChild());

	}


	TEST(TaskID, Order) {

		std::vector<TaskID> list {
			TaskID(12),
			TaskID(14),
			TaskID(12).getLeftChild(),
			TaskID(12).getRightChild().getRightChild(),
			TaskID(12).getRightChild(),
			TaskID(12).getLeftChild().getLeftChild()
		};

		std::sort(list.begin(),list.end());

		EXPECT_EQ("[T-12,T-12.0,T-12.0.0,T-12.1,T-12.1.1,T-14]",toString(list));

		for(auto itA = list.begin(); itA != list.end(); ++itA) {
			for(auto itB = list.begin(); itB != list.end(); ++itB) {

				auto a = *itA;
				auto b = *itB;

				if (itA < itB) {
					EXPECT_LT(a,b);
				} else if (itA == itB) {
					EXPECT_FALSE(a < b);
					EXPECT_EQ(a,b);
				} else {
					EXPECT_LT(b,a);
				}
			}
		}

	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
