#include <gtest/gtest.h>

#include "allscale/utils/static_grid.h"
#include "allscale/utils/serializer.h"

namespace allscale {
namespace utils {

	TEST(StaticGrid1D, TypeProperties) {

		using GridType = StaticGrid<int, 1>;

		EXPECT_TRUE(std::is_default_constructible<GridType>::value);
		EXPECT_TRUE(std::is_trivially_default_constructible<GridType>::value);

		EXPECT_TRUE(std::is_copy_constructible<GridType>::value);
		EXPECT_TRUE(std::is_move_constructible<GridType>::value);

		EXPECT_TRUE(std::is_trivially_copy_constructible<GridType>::value);
		EXPECT_TRUE(std::is_trivially_move_constructible<GridType>::value);

		EXPECT_TRUE(std::is_copy_assignable<GridType>::value);
		EXPECT_TRUE(std::is_move_assignable<GridType>::value);

		EXPECT_TRUE((utils::is_serializable<GridType>::value));

	}

	TEST(StaticGrid1D, Basic) {

		StaticGrid<int, 1> grid;

		EXPECT_EQ((Vector<size_t, 1>{1}), grid.size());

		grid[{0}] = 42;
		EXPECT_EQ(42, grid[{0}]);

	}

	TEST(StaticGrid2D, Basic) {

		StaticGrid<int, 3, 5> grid;

		EXPECT_EQ((Vector<size_t, 2>{3,5}), grid.size());

		grid.forEach([](int& element) {
			element = 42;
		});
		
		grid.forEach([](const int& element) {
			EXPECT_EQ(42, element);
		});

	}
	
	TEST(StaticGrid2D, Indexing) {

		StaticGrid<int, 3, 5> grid;

		EXPECT_EQ((Vector<size_t, 2>{3, 5}), grid.size());

		int count = 0;
		for(size_t i = 0; i < grid.size().x; ++i) {
			for(size_t j = 0; j < grid.size().y; ++j) {
				grid[{i, j}] = count++;
			}
		}
	
		count = 0;
		grid.forEach([&count](const int& element) {
			EXPECT_EQ(count++, element);
		});

	}

	TEST(StaticGrid2D, Addressing) {

		StaticGrid<int, 3, 5> grid;

		grid[{0, 0}] = 2;
		EXPECT_EQ(2, (grid[{0, 0}]));

		Vector<size_t, 2> index{ 0,0 };
		EXPECT_EQ(2, (grid[StaticGrid<int, 3, 5>::addr_type({ 0,0 })]));
	}

	TEST(StaticGrid2D, Serialization) {

		StaticGrid<int, 3, 5> grid;

		int count = 0;
		grid.forEach([&count](int& element) {
			element = count++;
		});

		utils::Archive archive = utils::serialize(grid);

		StaticGrid<int, 3, 5> newGrid = utils::deserialize<StaticGrid<int, 3, 5>>(archive);

		count = 0;
		newGrid.forEach([&count](const int& element) {
			EXPECT_EQ(count++, element);
		});

	}

	TEST(StaticGrid2D, NonTrivialElements) {

		struct A {
			int x;
			A() = default;
			A(const A& other) { x = other.x; }
		};

		EXPECT_FALSE(std::is_trivially_copyable<A>());

		StaticGrid<A, 2, 2> grid;

		grid.forEach([](A& a) {
			a.x = 2;
		});

		StaticGrid<A, 2, 2> newGrid = grid;

		int count = 0;
		newGrid.forEach([&count](const A& a) {
			count += a.x;
		});

		EXPECT_EQ(8, count);
	}

} // end namespace utils
} // end namespace allscale
