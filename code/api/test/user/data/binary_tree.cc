#include <gtest/gtest.h>

#include "allscale/api/user/data/binary_tree.h"
#include "allscale/utils/string_utils.h"
#include "allscale/utils/serializer/strings.h"

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

	TEST(StaticBalancedBinaryTreeRegion, Closure) {

		using region = StaticBalancedBinaryTreeRegion<3>;

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
				a.get(i) = i;
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


	TEST(StaticBalancedBinaryTreeFragment, ManipulationTest) {

		using region = StaticBalancedBinaryTreeRegion<20>;
		using fragment = StaticBalancedBinaryTreeFragment<int,20>;

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
			forAllNodes<20>([&](const auto& cur) {
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
		forAllNodes<20>([&](const auto& cur) {
			if (a.contains(cur)) { EXPECT_EQ(fA[cur],counterA++); }
			if (b.contains(cur)) { EXPECT_EQ(fB[cur],counterB++); }
		});

		EXPECT_EQ(1000002046,counterA);
		EXPECT_EQ(2000002046,counterB);

		// transfer data from B to A (direct)
		fA.insert(fB,region::root());

		counterA = 1000000000;
		counterB = 2000000000;
		forAllNodes<20>([&](const auto& cur) {
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
		forAllNodes<20>([&](const auto& cur) {
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


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
