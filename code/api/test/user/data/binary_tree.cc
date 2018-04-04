#include <gtest/gtest.h>

#include "allscale/api/user/data/binary_tree.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(StaticBalancedBinaryTreeRegion,Traits) {

		EXPECT_TRUE(std::is_default_constructible<StaticBalancedBinaryTreeRegion<0>>::value);
		EXPECT_TRUE(std::is_copy_constructible<StaticBalancedBinaryTreeRegion<0>>::value);
		EXPECT_TRUE(std::is_copy_assignable<StaticBalancedBinaryTreeRegion<0>>::value);
		EXPECT_TRUE(std::is_move_assignable<StaticBalancedBinaryTreeRegion<0>>::value);

		EXPECT_TRUE(allscale::utils::is_value<StaticBalancedBinaryTreeRegion<0>>::value);
		EXPECT_TRUE(allscale::utils::is_serializable<StaticBalancedBinaryTreeRegion<0>>::value);
		EXPECT_TRUE(allscale::utils::is_trivially_serializable<StaticBalancedBinaryTreeRegion<0>>::value);

		EXPECT_TRUE(allscale::utils::is_value<StaticBalancedBinaryTreeRegion<1>>::value);
		EXPECT_TRUE(allscale::utils::is_serializable<StaticBalancedBinaryTreeRegion<1>>::value);
		EXPECT_TRUE(allscale::utils::is_trivially_serializable<StaticBalancedBinaryTreeRegion<1>>::value);

		EXPECT_TRUE(allscale::utils::is_value<StaticBalancedBinaryTreeRegion<32>>::value);
		EXPECT_TRUE(allscale::utils::is_serializable<StaticBalancedBinaryTreeRegion<32>>::value);
		EXPECT_TRUE(allscale::utils::is_trivially_serializable<StaticBalancedBinaryTreeRegion<32>>::value);

		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeRegion<0>>::value);
		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeRegion<1>>::value);
		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeRegion<2>>::value);
		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeRegion<32>>::value);

	}

	TEST(StaticBalancedBinaryTreeRegion, Semantic) {

		using region = StaticBalancedBinaryTreeRegion<8>;

		region a = region::merge(region::root(), region::subtree(3));
		region b = region::merge(region::root(), region::subtree(7));
		region c = region::merge(region::subtree(3), region::subtree(7));

		EXPECT_EQ("{ R 3 }",toString(a));
		EXPECT_EQ("{ R 7 }",toString(b));
		EXPECT_EQ("{ 3 7 }",toString(c));

		testRegion(a,b);
		testRegion(a,c);
		testRegion(b,c);

	}

	TEST(StaticBalancedBinaryTreeFragment,Traits) {

		// test the fragment concept
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<int,0>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<double,1>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<char,2>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<std::string,32>>::value));
	}


	TEST(StaticBalancedBinaryTree,Traits) {

		// test the fragment concept
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<int,0>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<double,1>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<char,2>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<std::string,32>>::value));
	}

	TEST(StaticBalancedBinaryTreeElementAddress, Basic) {

		using addr_t = StaticBalancedBinaryTreeElementAddress<8>;

		// this test assumes a root-tree depth of 4
		EXPECT_EQ(16,1 << 4);



		// create a root address
		addr_t r;

		EXPECT_EQ("R/1(l=0)",toString(r));

		// test child addresses
		EXPECT_EQ("R/2(l=1)",toString(r.getLeftChild()));
		EXPECT_EQ("R/3(l=1)",toString(r.getRightChild()));

		// test deeper addresses
		EXPECT_EQ("R/4(l=2)", toString(r.getLeftChild().getLeftChild()));
		EXPECT_EQ("R/8(l=3)", toString(r.getLeftChild().getLeftChild().getLeftChild()));
		EXPECT_EQ("R/9(l=3)", toString(r.getLeftChild().getLeftChild().getRightChild()));
		EXPECT_EQ("R/10(l=3)", toString(r.getLeftChild().getRightChild().getLeftChild()));
		EXPECT_EQ("R/11(l=3)", toString(r.getLeftChild().getRightChild().getRightChild()));

		// and here we should move to the next subtree
		EXPECT_EQ("0/1(l=4)",toString(r.getLeftChild().getLeftChild().getLeftChild().getLeftChild()));
		EXPECT_EQ("1/1(l=4)",toString(r.getLeftChild().getLeftChild().getLeftChild().getRightChild()));

		EXPECT_EQ("2/1(l=4)",toString(r.getLeftChild().getLeftChild().getRightChild().getLeftChild()));
		EXPECT_EQ("3/1(l=4)",toString(r.getLeftChild().getLeftChild().getRightChild().getRightChild()));
	}

	namespace {

		template<std::size_t depth, typename Operator>
		void forAllNodes(const Operator& op, const StaticBalancedBinaryTreeElementAddress<depth>& cur = StaticBalancedBinaryTreeElementAddress<depth>()) {
			op(cur);
			if (cur.isLeaf()) return;
			forAllNodes<depth>(op,cur.getLeftChild());
			forAllNodes<depth>(op,cur.getRightChild());
		}

	}

	TEST(StaticBalancedBinaryTree, Basic) {

		using tree_t = StaticBalancedBinaryTree<int,8>;
		using addr_t = typename tree_t::address_t;

		// create a tree
		tree_t tree;

		// get the root node
		addr_t root;

		tree[root] = 12;

	}

	namespace {

		template<std::size_t depth>
		void checkAddressing() {

			using tree_t = StaticBalancedBinaryTree<int,depth>;

			// create a tree
			tree_t tree;

			// collect all addresses of addressable nodes
			std::set<int*> all;
			std::map<int*,typename tree_t::address_t> index;
			int count = 0;
			forAllNodes<depth>([&](const auto& cur) {
				count++;
				all.insert(&tree[cur]);

				auto pos = index.find(&tree[cur]);
				if (pos != index.end()) {
					std::cout << "Collision: " << pos->second << " vs. " << cur << "\n";
				}

				index[&tree[cur]] = cur;
			});

			// check that this is the right number of elements
			EXPECT_EQ((1<<depth) - 1,count);
			EXPECT_EQ((1<<depth) - 1,all.size());
		}

	}

	TEST(StaticBalancedBinaryTree, Addressing) {

		checkAddressing<4>();
		checkAddressing<5>();
		checkAddressing<6>();
		checkAddressing<7>();
		checkAddressing<8>();

		checkAddressing<20>();

	}

	TEST(StaticBalancedBinaryTreeFragment, Semantic) {

		using region = StaticBalancedBinaryTreeRegion<8>;
		using fragment = StaticBalancedBinaryTreeFragment<int,8>;

		region a = region::merge(region::root(), region::subtree(3));
		region b = region::merge(region::root(), region::subtree(7));
		region c = region::merge(region::subtree(3), region::subtree(7));

		EXPECT_EQ("{ R 3 }",toString(a));
		EXPECT_EQ("{ R 7 }",toString(b));
		EXPECT_EQ("{ 3 7 }",toString(c));

		testFragment<fragment>(a,b);
		testFragment<fragment>(a,c);
		testFragment<fragment>(b,c);

	}


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
