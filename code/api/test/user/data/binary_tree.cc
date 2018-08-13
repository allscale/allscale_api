#include <gtest/gtest.h>

#include "allscale/api/user/data/binary_tree.h"
#include "allscale/utils/string_utils.h"
#include "allscale/utils/serializer/strings.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(StaticBalancedBinaryTreeBlockedRegion,Traits) {

		EXPECT_TRUE(std::is_default_constructible<StaticBalancedBinaryTreeBlockedRegion<0>>::value);
		EXPECT_TRUE(std::is_copy_constructible<StaticBalancedBinaryTreeBlockedRegion<0>>::value);
		EXPECT_TRUE(std::is_copy_assignable<StaticBalancedBinaryTreeBlockedRegion<0>>::value);
		EXPECT_TRUE(std::is_move_assignable<StaticBalancedBinaryTreeBlockedRegion<0>>::value);

		EXPECT_TRUE(allscale::utils::is_value<StaticBalancedBinaryTreeBlockedRegion<0>>::value);
		EXPECT_TRUE(allscale::utils::is_serializable<StaticBalancedBinaryTreeBlockedRegion<0>>::value);
		EXPECT_TRUE(allscale::utils::is_trivially_serializable<StaticBalancedBinaryTreeBlockedRegion<0>>::value);

		EXPECT_TRUE(allscale::utils::is_value<StaticBalancedBinaryTreeBlockedRegion<1>>::value);
		EXPECT_TRUE(allscale::utils::is_serializable<StaticBalancedBinaryTreeBlockedRegion<1>>::value);
		EXPECT_TRUE(allscale::utils::is_trivially_serializable<StaticBalancedBinaryTreeBlockedRegion<1>>::value);

		EXPECT_TRUE(allscale::utils::is_value<StaticBalancedBinaryTreeBlockedRegion<32>>::value);
		EXPECT_TRUE(allscale::utils::is_serializable<StaticBalancedBinaryTreeBlockedRegion<32>>::value);
		EXPECT_TRUE(allscale::utils::is_trivially_serializable<StaticBalancedBinaryTreeBlockedRegion<32>>::value);

		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeBlockedRegion<0>>::value);
		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeBlockedRegion<1>>::value);
		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeBlockedRegion<2>>::value);
		EXPECT_TRUE(allscale::api::core::is_region<StaticBalancedBinaryTreeBlockedRegion<32>>::value);

	}

	TEST(StaticBalancedBinaryTreeBlockedRegion, Semantic) {

		using region = StaticBalancedBinaryTreeBlockedRegion<8>;

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

	TEST(StaticBalancedBinaryTreeBlockedRegion, Closure) {

		using region = StaticBalancedBinaryTreeBlockedRegion<3>;

		EXPECT_EQ("{ }",toString(region()));
		EXPECT_EQ("{ R }",toString(region::root()));
		EXPECT_EQ("{ 0 }",toString(region::subtree(0)));
		EXPECT_EQ("{ 1 }",toString(region::subtree(1)));
		EXPECT_EQ("{ R }",toString(region::subtree(2)));

		EXPECT_EQ("{ }",toString(region::closure(region())));
		EXPECT_EQ("{ R 0 1 }",toString(region::closure(region::root())));
		EXPECT_EQ("{ 0 }",toString(region::closure(region::subtree(0))));
		EXPECT_EQ("{ 1 }",toString(region::closure(region::subtree(1))));
		EXPECT_EQ("{ R 0 1 }",toString(region::closure(region::subtree(2))));

	}

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

		EXPECT_EQ("{ N1 S3 }",toString(a));
		EXPECT_EQ("{ N1 S7 }",toString(b));
		EXPECT_EQ("{ S3 S7 }",toString(c));

		testRegion(a,b);
		testRegion(a,c);
		testRegion(b,c);

	}

	TEST(StaticBalancedBinaryTreeRegion, Contains) {

		using region = StaticBalancedBinaryTreeRegion<8>;

		StaticBalancedBinaryTreeElementAddress<region> root;

		region a = core::merge(region::root(), region::node(root.getLeftChild().getRightChild()), region::subtree(3));
		region b = core::merge(region::root(), region::node(root.getRightChild().getLeftChild()), region::subtree(7));
		region c = core::merge(region::subtree(2), region::subtree(8));

		EXPECT_EQ("{ N1 N5 S3 }",toString(a));
		EXPECT_EQ("{ N1 N6 S7 }",toString(b));
		EXPECT_EQ("{ S2 S8 }",toString(c));

		// test contains function

		EXPECT_TRUE(a.contains(root));
		EXPECT_TRUE(b.contains(root));
		EXPECT_FALSE(c.contains(root));

		EXPECT_FALSE(a.contains(root.getLeftChild()));
		EXPECT_FALSE(b.contains(root.getLeftChild()));
		EXPECT_FALSE(c.contains(root.getLeftChild()));

		EXPECT_TRUE(a.contains(root.getLeftChild().getRightChild()));
		EXPECT_FALSE(b.contains(root.getLeftChild().getRightChild()));
		EXPECT_FALSE(c.contains(root.getLeftChild().getRightChild()));

		EXPECT_FALSE(a.contains(root.getRightChild().getLeftChild()));
		EXPECT_TRUE(b.contains(root.getRightChild().getLeftChild()));
		EXPECT_FALSE(c.contains(root.getRightChild().getLeftChild()));


		auto s0root = root.getLeftChild().getLeftChild().getLeftChild().getLeftChild();
		auto s2root = root.getLeftChild().getLeftChild().getRightChild().getLeftChild();
		auto s3root = root.getLeftChild().getLeftChild().getRightChild().getRightChild();
		auto s7root = root.getLeftChild().getRightChild().getRightChild().getRightChild();
		auto s8root = root.getRightChild().getLeftChild().getLeftChild().getLeftChild();

		EXPECT_EQ("0/1(l=4)",toString(s0root));
		EXPECT_EQ("2/1(l=4)",toString(s2root));
		EXPECT_EQ("3/1(l=4)",toString(s3root));
		EXPECT_EQ("7/1(l=4)",toString(s7root));
		EXPECT_EQ("8/1(l=4)",toString(s8root));


		EXPECT_FALSE(a.contains(s0root));
		EXPECT_FALSE(b.contains(s0root));
		EXPECT_FALSE(c.contains(s0root));

		EXPECT_FALSE(a.contains(s2root));
		EXPECT_FALSE(b.contains(s2root));
		EXPECT_TRUE(c.contains(s2root));

		EXPECT_TRUE(a.contains(s3root));
		EXPECT_FALSE(b.contains(s3root));
		EXPECT_FALSE(c.contains(s3root));

		EXPECT_FALSE(a.contains(s7root));
		EXPECT_TRUE(b.contains(s7root));
		EXPECT_FALSE(c.contains(s7root));

		EXPECT_FALSE(a.contains(s8root));
		EXPECT_FALSE(b.contains(s8root));
		EXPECT_TRUE(c.contains(s8root));

		// same holds for children of subtrees
		EXPECT_FALSE(a.contains(s0root.getLeftChild()));
		EXPECT_FALSE(b.contains(s0root.getLeftChild()));
		EXPECT_FALSE(c.contains(s0root.getLeftChild()));

		EXPECT_FALSE(a.contains(s2root.getLeftChild()));
		EXPECT_FALSE(b.contains(s2root.getLeftChild()));
		EXPECT_TRUE(c.contains(s2root.getLeftChild()));

		EXPECT_TRUE(a.contains(s3root.getLeftChild()));
		EXPECT_FALSE(b.contains(s3root.getLeftChild()));
		EXPECT_FALSE(c.contains(s3root.getLeftChild()));

		EXPECT_FALSE(a.contains(s7root.getLeftChild()));
		EXPECT_TRUE(b.contains(s7root.getLeftChild()));
		EXPECT_FALSE(c.contains(s7root.getLeftChild()));

		EXPECT_FALSE(a.contains(s8root.getLeftChild()));
		EXPECT_FALSE(b.contains(s8root.getLeftChild()));
		EXPECT_TRUE(c.contains(s8root.getLeftChild()));


	}


	TEST(StaticBalancedBinaryTreeRegion, Closure) {

		using region = StaticBalancedBinaryTreeRegion<3>;

		EXPECT_EQ("{ }",toString(region()));
		EXPECT_EQ("{ N1 }",toString(region::root()));
		EXPECT_EQ("{ S0 }",toString(region::subtree(0)));
		EXPECT_EQ("{ S1 }",toString(region::subtree(1)));

		EXPECT_EQ("{ }",toString(region::closure(region())));
		EXPECT_EQ("{ N1 S0 S1 }",toString(region::closure(region::root())));
		EXPECT_EQ("{ S0 }",toString(region::closure(region::subtree(0))));
		EXPECT_EQ("{ S1 }",toString(region::closure(region::subtree(1))));

	}

	TEST(StaticBalancedBinaryTreeRegion, Closure_2) {

		using region = StaticBalancedBinaryTreeRegion<8,3>;

		EXPECT_EQ("{ }",toString(region()));
		EXPECT_EQ("{ N1 }",toString(region::root()));
		EXPECT_EQ("{ S0 }",toString(region::subtree(0)));
		EXPECT_EQ("{ S1 }",toString(region::subtree(1)));

		EXPECT_EQ("{ }",toString(region::closure(region())));
		EXPECT_EQ("{ N1 N2 N3 N4 N5 N6 N7 S0 S1 S2 S3 S4 S5 S6 S7 }",toString(region::closure(region::root())));
		EXPECT_EQ("{ S0 }",toString(region::closure(region::subtree(0))));
		EXPECT_EQ("{ S1 }",toString(region::closure(region::subtree(1))));

		// more fragmented closures
		StaticBalancedBinaryTreeElementAddress<region> root;
		auto rl = root.getLeftChild();
		auto rr = root.getRightChild();

		auto rll = rl.getLeftChild();
		auto rlr = rl.getRightChild();

		auto rrl = rr.getLeftChild();
		auto rrr = rr.getRightChild();

		EXPECT_EQ("{ N1 }",toString(region::node(root)));
		EXPECT_EQ("{ N2 }",toString(region::node(rl)));
		EXPECT_EQ("{ N3 }",toString(region::node(rr)));
		EXPECT_EQ("{ N4 }",toString(region::node(rll)));
		EXPECT_EQ("{ N5 }",toString(region::node(rlr)));
		EXPECT_EQ("{ N6 }",toString(region::node(rrl)));
		EXPECT_EQ("{ N7 }",toString(region::node(rrr)));

		// below we reach the sub-tree level
		EXPECT_EQ("{ S0 }",toString(region::node(rll.getLeftChild())));
		EXPECT_EQ("{ S1 }",toString(region::node(rll.getRightChild())));

		EXPECT_EQ("{ S2 }",toString(region::node(rlr.getLeftChild())));
		EXPECT_EQ("{ S3 }",toString(region::node(rlr.getRightChild())));

		EXPECT_EQ("{ S4 }",toString(region::node(rrl.getLeftChild())));
		EXPECT_EQ("{ S5 }",toString(region::node(rrl.getRightChild())));

		EXPECT_EQ("{ S6 }",toString(region::node(rrr.getLeftChild())));
		EXPECT_EQ("{ S7 }",toString(region::node(rrr.getRightChild())));


		// - compute closure -

		EXPECT_EQ("{ N1 N2 N3 N4 N5 N6 N7 S0 S1 S2 S3 S4 S5 S6 S7 }", toString(region::closure(region::node(root))));
		EXPECT_EQ("{ N2 N4 N5 S0 S1 S2 S3 }", toString(region::closure(region::node(rl))));
		EXPECT_EQ("{ N3 N6 N7 S4 S5 S6 S7 }", toString(region::closure(region::node(rr))));

		EXPECT_EQ("{ N4 S0 S1 }", toString(region::closure(region::node(rll))));
		EXPECT_EQ("{ N5 S2 S3 }", toString(region::closure(region::node(rlr))));
		EXPECT_EQ("{ N6 S4 S5 }", toString(region::closure(region::node(rrl))));
		EXPECT_EQ("{ N7 S6 S7 }", toString(region::closure(region::node(rrr))));


		// - and closures of combinations -

		EXPECT_EQ("{ N4 N7 }", toString(region::merge(region::node(rll),region::node(rrr))));
		EXPECT_EQ("{ N4 N7 S0 S1 S6 S7 }", toString(region::closure(region::merge(region::node(rll),region::node(rrr)))));

		EXPECT_EQ("{ N2 N5 }", toString(region::merge(region::node(rl),region::node(rlr))));
		EXPECT_EQ("{ N2 N4 N5 S0 S1 S2 S3 }", toString(region::closure(region::merge(region::node(rl),region::node(rlr)))));

	}


	TEST(StaticBalancedBinarySubTree,Traits) {

		// test serializability
		EXPECT_TRUE((allscale::utils::is_serializable<detail::StaticBalancedBinarySubTree<int,5>>::value));
		EXPECT_TRUE((allscale::utils::is_serializable<detail::StaticBalancedBinarySubTree<int,20>>::value));

		EXPECT_TRUE((allscale::utils::is_trivially_serializable<std::array<int,20>>::value));
		EXPECT_TRUE((allscale::utils::is_trivially_serializable<std::array<int,1<<20>>::value));

	}

	TEST(StaticBalancedBinarySubTree,Serialization) {

		// test whether large sub-trees can be serialized

		{
			// test serializability
			using tree = detail::StaticBalancedBinarySubTree<int,24>;
			EXPECT_TRUE(allscale::utils::is_serializable<tree>::value);

			tree a;
			for(std::size_t i=1; i<tree::num_elements; i++) {
				a.get(i) = (int)i;
			}
			auto archive = allscale::utils::serialize(a);
			tree b = allscale::utils::deserialize<tree>(archive);
			for(std::size_t i=1; i<tree::num_elements; i++) {
				EXPECT_EQ(i,b.get(i));
			}
		}

		{
			// test serializability
			using tree = detail::StaticBalancedBinarySubTree<std::string,20>;
			EXPECT_TRUE(allscale::utils::is_serializable<tree>::value);

			tree a;
			for(std::size_t i=1; i<tree::num_elements; i++) {
				a.get(i) = toString(i);
			}
			auto archive = allscale::utils::serialize(a);
			tree b = allscale::utils::deserialize<tree>(archive);
			for(std::size_t i=1; i<tree::num_elements; i++) {
				EXPECT_EQ(toString(i),b.get(i));
			}
		}
	}


	TEST(StaticBalancedBinaryTreeFragment,Traits) {

		// test the fragment concept -- for blocked regions
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<int,StaticBalancedBinaryTreeBlockedRegion<0>>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<double,StaticBalancedBinaryTreeBlockedRegion<1>>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<char,StaticBalancedBinaryTreeBlockedRegion<2>>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<std::string,StaticBalancedBinaryTreeBlockedRegion<32>>>::value));

		// test the fragment concept -- for non-blocked regions
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<int,StaticBalancedBinaryTreeRegion<0>>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<double,StaticBalancedBinaryTreeRegion<1>>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<char,StaticBalancedBinaryTreeRegion<2>>>::value));
		EXPECT_TRUE((allscale::api::core::is_fragment<StaticBalancedBinaryTreeFragment<std::string,StaticBalancedBinaryTreeRegion<32>>>::value));

	}


	TEST(StaticBalancedBinaryTree,Traits) {

		// test the data item concept -- the default
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<int,0>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<double,1>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<char,2>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<std::string,32>>::value));


		// test the data item concept -- for blocked regions
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<int,0,StaticBalancedBinaryTreeBlockedRegion>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<double,1,StaticBalancedBinaryTreeBlockedRegion>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<char,2,StaticBalancedBinaryTreeBlockedRegion>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<std::string,32,StaticBalancedBinaryTreeBlockedRegion>>::value));

		// test the data item concept -- for none-blocked regions
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<int,0,StaticBalancedBinaryTreeRegion>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<double,1,StaticBalancedBinaryTreeRegion>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<char,2,StaticBalancedBinaryTreeRegion>>::value));
		EXPECT_TRUE((allscale::api::core::is_data_item<StaticBalancedBinaryTree<std::string,32,StaticBalancedBinaryTreeRegion>>::value));
	}

	TEST(StaticBalancedBinaryTreeElementAddress, Basic) {

		using region = StaticBalancedBinaryTreeBlockedRegion<8>;
		using addr_t = StaticBalancedBinaryTreeElementAddress<region>;

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

		template<typename Region, typename Operator>
		void forAllNodes(const Operator& op, const StaticBalancedBinaryTreeElementAddress<Region>& cur = StaticBalancedBinaryTreeElementAddress<Region>()) {
			op(cur);
			if (cur.isLeaf()) return;
			forAllNodes<Region>(op,cur.getLeftChild());
			forAllNodes<Region>(op,cur.getRightChild());
		}

	}

	TEST(StaticBalancedBinaryTree, BasicBlocked) {

		using tree_t = StaticBalancedBinaryTree<int,8,StaticBalancedBinaryTreeBlockedRegion>;
		using addr_t = typename tree_t::address_t;

		// create a tree
		tree_t tree;

		// get the root node
		addr_t root;

		tree[root] = 12;

	}

	TEST(StaticBalancedBinaryTree, BasicNonBlocked) {

		using tree_t = StaticBalancedBinaryTree<int,8,StaticBalancedBinaryTreeRegion>;
		using addr_t = typename tree_t::address_t;

		// create a tree
		tree_t tree;

		// get the root node
		addr_t root;

		tree[root] = 12;

	}


	namespace {

		template<std::size_t depth,template<std::size_t,std::size_t> class Region>
		void checkAddressing() {

			using tree_t = StaticBalancedBinaryTree<int,depth,Region>;
			using region = typename tree_t::region_type;

			// create a tree
			tree_t tree;

			// collect all addresses of addressable nodes
			std::set<int*> all;
			std::map<int*,typename tree_t::address_t> index;
			int count = 0;
			forAllNodes<region>([&](const auto& cur) {
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

	TEST(StaticBalancedBinaryTree, AddressingBlocked) {

		checkAddressing<4, StaticBalancedBinaryTreeBlockedRegion>();
		checkAddressing<5, StaticBalancedBinaryTreeBlockedRegion>();
		checkAddressing<6, StaticBalancedBinaryTreeBlockedRegion>();
		checkAddressing<7, StaticBalancedBinaryTreeBlockedRegion>();
		checkAddressing<8, StaticBalancedBinaryTreeBlockedRegion>();

		checkAddressing<20, StaticBalancedBinaryTreeBlockedRegion>();

	}

	TEST(StaticBalancedBinaryTree, AddressingNonBlocked) {

		checkAddressing<4, StaticBalancedBinaryTreeRegion>();
		checkAddressing<5, StaticBalancedBinaryTreeRegion>();
		checkAddressing<6, StaticBalancedBinaryTreeRegion>();
		checkAddressing<7, StaticBalancedBinaryTreeRegion>();
		checkAddressing<8, StaticBalancedBinaryTreeRegion>();

		checkAddressing<20, StaticBalancedBinaryTreeRegion>();

	}


	TEST(StaticBalancedBinaryTreeFragment, SemanticBlocked) {

		using region = StaticBalancedBinaryTreeBlockedRegion<8>;
		using fragment = StaticBalancedBinaryTreeFragment<int,region>;

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

	TEST(StaticBalancedBinaryTreeFragment, SemanticNonBlocked) {

		using region = StaticBalancedBinaryTreeRegion<8>;
		using fragment = StaticBalancedBinaryTreeFragment<int,region>;

		region a = region::merge(region::root(), region::subtree(3));
		region b = region::merge(region::root(), region::subtree(7));
		region c = region::merge(region::subtree(3), region::subtree(7));

		EXPECT_EQ("{ N1 S3 }",toString(a));
		EXPECT_EQ("{ N1 S7 }",toString(b));
		EXPECT_EQ("{ S3 S7 }",toString(c));

		testFragment<fragment>(a,b);
		testFragment<fragment>(a,c);
		testFragment<fragment>(b,c);

	}


	TEST(StaticBalancedBinaryTreeFragment, ManipulationTestBlocked) {

		using region = StaticBalancedBinaryTreeBlockedRegion<20>;
		using fragment = StaticBalancedBinaryTreeFragment<int,region>;

		region a = region::merge(region::root(), region::subtree(3));
		region b = region::merge(region::root(), region::subtree(7));

		// test that this version of tree properly works
		testFragment<fragment>(a,b);

		// -- simulate manipulation --

		// create some shared data
		core::no_shared_data shared;

		// create two fragments
		fragment fA(shared,a);
		fragment fB(shared,b);

		// mask and fill with data
		auto mA = fA.mask();
		auto mB = fB.mask();

		// insert some data
		auto reset = [&]() {
			int counterA = 1000000000;
			int counterB = 2000000000;
			forAllNodes<region>([&](const auto& cur) {
				if (a.contains(cur)) fA[cur] = (counterA++);
				if (b.contains(cur)) fB[cur] = (counterB++);
			});

			EXPECT_EQ(1000002046,counterA);
			EXPECT_EQ(2000002046,counterB);
		};
		reset();

		// check data
		int counterA = 1000000000;
		int counterB = 2000000000;
		forAllNodes<region>([&](const auto& cur) {
			if (a.contains(cur)) { EXPECT_EQ(fA[cur],counterA++); }
			if (b.contains(cur)) { EXPECT_EQ(fB[cur],counterB++); }
		});

		EXPECT_EQ(1000002046,counterA);
		EXPECT_EQ(2000002046,counterB);

		// transfer data from B to A (direct)
		fA.insertRegion(fB,region::root());

		counterA = 1000000000;
		counterB = 2000000000;
		forAllNodes<region>([&](const auto& cur) {
			if (a.contains(cur)) {
				if (b.contains(cur)) {
					EXPECT_EQ(fA[cur],fB[cur]);
				} else {
					EXPECT_EQ(fA[cur],counterA);
				}
				counterA++;
			}
			if (b.contains(cur))  { EXPECT_EQ(fB[cur],counterB++); }
		});

		EXPECT_EQ(1000002046,counterA);
		EXPECT_EQ(2000002046,counterB);

		// reset
		reset();

		// transfer data from A to B through serialization
		auto archive = extract(fA,region::root());
		insert(fB,archive);

		counterA = 1000000000;
		counterB = 2000000000;
		forAllNodes<region>([&](const auto& cur) {
			if (a.contains(cur)) { EXPECT_EQ(fA[cur],counterA++); }
			if (b.contains(cur)) {
				if (a.contains(cur)) {
					EXPECT_EQ(fB[cur],fA[cur]);
				} else {
					EXPECT_EQ(fB[cur],counterB);
				}
				counterB++;
			}
		});

		EXPECT_EQ(1000002046,counterA);
		EXPECT_EQ(2000002046,counterB);


	}


	TEST(StaticBalancedBinaryTreeFragment, ManipulationTestNonBlocked) {

		using region = StaticBalancedBinaryTreeRegion<20>;
		using fragment = StaticBalancedBinaryTreeFragment<int,region>;

		StaticBalancedBinaryTreeElementAddress<region> root;

		region a = core::merge(region::root(), region::node(root.getLeftChild().getRightChild()), region::subtree(3));
		region b = core::merge(region::root(), region::node(root.getRightChild().getLeftChild()), region::subtree(7));

		EXPECT_EQ("{ N1 N5 S3 }",toString(a));
		EXPECT_EQ("{ N1 N6 S7 }",toString(b));

		// test that this version of tree properly works
		testFragment<fragment>(a,b);

		// -- simulate manipulation --

		// create some shared data
		core::no_shared_data shared;

		// create two fragments
		fragment fA(shared,a);
		fragment fB(shared,b);

		// mask and fill with data
		auto mA = fA.mask();
		auto mB = fB.mask();

		// insert some data
		auto reset = [&]() {
			int counterA = 1000000000;
			int counterB = 2000000000;
			forAllNodes<region>([&](const auto& cur) {
				if (a.contains(cur)) fA[cur] = (counterA++);
				if (b.contains(cur)) fB[cur] = (counterB++);
			});

			EXPECT_EQ(1000001025,counterA);
			EXPECT_EQ(2000001025,counterB);
		};
		reset();

		// check data
		int counterA = 1000000000;
		int counterB = 2000000000;
		forAllNodes<region>([&](const auto& cur) {
			if (a.contains(cur)) { EXPECT_EQ(fA[cur],counterA++); }
			if (b.contains(cur)) { EXPECT_EQ(fB[cur],counterB++); }
		});

		EXPECT_EQ(1000001025,counterA);
		EXPECT_EQ(2000001025,counterB);

		// transfer data from B to A (direct)
		fA.insertRegion(fB,region::root());

		counterA = 1000000000;
		counterB = 2000000000;
		forAllNodes<region>([&](const auto& cur) {
			if (a.contains(cur)) {
				if (b.contains(cur)) {
					EXPECT_EQ(fA[cur],fB[cur]);
				} else {
					EXPECT_EQ(fA[cur],counterA);
				}
				counterA++;
			}
			if (b.contains(cur))  { EXPECT_EQ(fB[cur],counterB++); }
		});

		EXPECT_EQ(1000001025,counterA);
		EXPECT_EQ(2000001025,counterB);

		// reset
		reset();

		// transfer data from A to B through serialization
		auto archive = extract(fA,region::root());
		insert(fB,archive);

		counterA = 1000000000;
		counterB = 2000000000;
		forAllNodes<region>([&](const auto& cur) {
			if (a.contains(cur)) { EXPECT_EQ(fA[cur],counterA++); }
			if (b.contains(cur)) {
				if (a.contains(cur)) {
					EXPECT_EQ(fB[cur],fA[cur]);
				} else {
					EXPECT_EQ(fB[cur],counterB);
				}
				counterB++;
			}
		});

		EXPECT_EQ(1000001025,counterA);
		EXPECT_EQ(2000001025,counterB);

	}

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
