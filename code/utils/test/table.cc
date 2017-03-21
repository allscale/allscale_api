#include <gtest/gtest.h>

#include <type_traits>

#include "allscale/utils/table.h"
#include "allscale/utils/string_utils.h"
#include "allscale/utils/printer/vectors.h"

namespace allscale {
namespace utils {


	TEST(Table,TypeProperties) {

		using table_t = Table<std::vector<int>>;

		EXPECT_TRUE(std::is_copy_constructible<table_t>::value);
		EXPECT_TRUE(std::is_move_constructible<table_t>::value);

		EXPECT_TRUE(std::is_copy_assignable<table_t>::value);
		EXPECT_TRUE(std::is_move_assignable<table_t>::value);

		EXPECT_TRUE(std::is_destructible<table_t>::value);

	}

	TEST(Table,BasicCtors) {

		using table_t = Table<std::vector<int>>;

		table_t table(5);

		EXPECT_FALSE(table.empty());
		EXPECT_EQ(5,table.size());
		EXPECT_TRUE(table.isOwner());
		EXPECT_EQ("[[],[],[],[],[]]",toString(table));

		for(auto& cur : table) {
			cur.push_back(12);
		}

		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(table));

		Table<std::vector<int>> copy(table);
		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(copy));

		EXPECT_TRUE(table.isOwner());
		EXPECT_TRUE(copy.isOwner());

		Table<std::vector<int>> move(std::move(table));

		EXPECT_FALSE(table.isOwner());
		EXPECT_TRUE(copy.isOwner());
		EXPECT_TRUE(move.isOwner());

		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(table));
		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(copy));
		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(move));

	}

	TEST(Table,BasicAssignment) {

		using table_t = Table<std::vector<int>>;

		table_t table(5);

		EXPECT_FALSE(table.empty());
		EXPECT_EQ(5,table.size());
		EXPECT_TRUE(table.isOwner());
		EXPECT_EQ("[[],[],[],[],[]]",toString(table));

		for(auto& cur : table) {
			cur.push_back(12);
		}

		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(table));

		Table<std::vector<int>> copy;
		copy = table;
		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(copy));

		EXPECT_TRUE(table.isOwner());
		EXPECT_TRUE(copy.isOwner());

		Table<std::vector<int>> move;
		move = std::move(table);

		EXPECT_FALSE(table.isOwner());
		EXPECT_TRUE(copy.isOwner());
		EXPECT_TRUE(move.isOwner());

		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(table));
		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(copy));
		EXPECT_EQ("[[12],[12],[12],[12],[12]]",toString(move));

	}


	TEST(Table,InitCtor) {

		Table<int> table(5,5);
		EXPECT_EQ("[5,5,5,5,5]",toString(table));

	}

	TEST(Table,Empty) {

		Table<int> table;

		EXPECT_EQ(0,table.size());
		EXPECT_TRUE(table.empty());

		EXPECT_EQ(table.begin(),table.end());

	}


	TEST(Table,ExternOwn) {

		const int N = 4;

		int data[N] = { 1, 2, 3, 4 };

		Table<int> table(data,data+N);

		EXPECT_FALSE(table.empty());
		EXPECT_EQ(N,table.size());
		EXPECT_FALSE(table.isOwner());

		EXPECT_EQ("[1,2,3,4]",toString(table));
	}

} // end namespace utils
} // end namespace allscale
