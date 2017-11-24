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

		const int M = 3;
		const int N = 5;
		StaticGrid<int, M, N> grid;

		EXPECT_EQ((Vector<size_t, 2>{3, 5}), grid.size());

		int count = 0;
		for(int i = 0; i < M; ++i) {
			for(int j = 0; j < N; ++j) {
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

	TEST(StaticGrid0D, ForEachWithCoordinates) {

		using grid_t = StaticGrid<int>;
		using addr_t = typename grid_t::addr_type;
		grid_t grid;

		// fill the grid
		int i = 0;
		grid.forEach([&](int& cur) {
			cur = i; i++;
		});
		EXPECT_EQ(1,i);

		// check iteration order
		i = 0;
		grid.forEach([&](const int& cur){
			EXPECT_EQ(i,cur); i++;
		});
		EXPECT_EQ(1,i);

		i = 0;
		addr_t last = { };
		grid.forEach([&](const auto& pos, int& cur) {
			if (i == 0) {
				EXPECT_EQ(last,pos);
			} else {
				EXPECT_LT(last,pos);
			}
			EXPECT_EQ(i,cur);
			EXPECT_EQ(i,grid[pos]);
			last = pos;
			i++;
		});
		EXPECT_EQ(1,i);
		EXPECT_EQ(last,(addr_t{}));
	}


	TEST(StaticGrid1D, ForEachWithCoordinates) {

		using grid_t = StaticGrid<int,5>;
		using addr_t = typename grid_t::addr_type;
		grid_t grid;

		// fill the grid
		int i = 0;
		grid.forEach([&](int& cur) {
			cur = i; i++;
		});
		EXPECT_EQ(5,i);

		// check iteration order
		i = 0;
		grid.forEach([&](const int& cur){
			EXPECT_EQ(i,cur); i++;
		});
		EXPECT_EQ(5,i);

		i = 0;
		addr_t last = { 0 };
		grid.forEach([&](const auto& pos, int& cur) {
			if (i == 0) {
				EXPECT_EQ(last,pos);
			} else {
				EXPECT_LT(last,pos);
			}
			EXPECT_EQ(i,cur);
			EXPECT_EQ(i,grid[pos]);
			last = pos;
			i++;
		});
		EXPECT_EQ(5,i);
		EXPECT_EQ(last,(addr_t{4}));
	}

	TEST(StaticGrid2D, ForEachWithCoordinates) {

		using grid_t = StaticGrid<int,2,4>;
		using addr_t = typename grid_t::addr_type;
		grid_t grid;

		// fill the grid
		int i = 0;
		grid.forEach([&](int& cur) {
			cur = i; i++;
		});
		EXPECT_EQ(2*4,i);

		// check iteration order
		i = 0;
		grid.forEach([&](const int& cur){
			EXPECT_EQ(i,cur); i++;
		});
		EXPECT_EQ(2*4,i);

		i = 0;
		addr_t last = { 0, 0 };
		grid.forEach([&](const auto& pos, int& cur) {
			if (i == 0) {
				EXPECT_EQ(last,pos);
			} else {
				EXPECT_LT(last,pos);
			}
			EXPECT_EQ(i,cur);
			EXPECT_EQ(i,grid[pos]);
			last = pos;
			i++;
		});
		EXPECT_EQ(2*4,i);
		EXPECT_EQ(last,(addr_t{1,3}));
	}

	TEST(StaticGrid3D, ForEachWithCoordinates) {

		using grid_t = StaticGrid<int,2,4,8>;
		using addr_t = typename grid_t::addr_type;
		grid_t grid;

		// fill the grid
		int i = 0;
		grid.forEach([&](int& cur) {
			cur = i; i++;
		});
		EXPECT_EQ(2*4*8,i);

		// check iteration order
		i = 0;
		grid.forEach([&](const int& cur){
			EXPECT_EQ(i,cur); i++;
		});
		EXPECT_EQ(2*4*8,i);

		i = 0;
		addr_t last = { 0, 0, 0 };
		grid.forEach([&](const auto& pos, int& cur) {
			if (i == 0) {
				EXPECT_EQ(last,pos);
			} else {
				EXPECT_LT(last,pos);
			}
			EXPECT_EQ(i,cur);
			EXPECT_EQ(i,grid[pos]);
			last = pos;
			i++;
		});
		EXPECT_EQ(2*4*8,i);
		EXPECT_EQ(last,(addr_t{1,3,7}));
	}


} // end namespace utils
} // end namespace allscale
