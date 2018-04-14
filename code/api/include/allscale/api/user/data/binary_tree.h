#pragma once

#include <cstring>
#include <memory>
#include <bitset>

#include "allscale/utils/serializer.h"
#include "allscale/utils/serializer/vectors.h"

#include "allscale/api/core/data.h"


namespace allscale {
namespace api {
namespace user {
namespace data {


	// ---------------------------------------------------------------------------------
	//								 Declarations
	// ---------------------------------------------------------------------------------


	// Static Balanced Binary Tree Region
	template<std::size_t depth>
	class StaticBalancedBinaryTreeRegion;

	// Static Balanced Binary Tree Element Address
	template<std::size_t depth>
	class StaticBalancedBinaryTreeElementAddress;

	// Static Balanced Binary Tree Fragment
	template<typename T, std::size_t depth>
	class StaticBalancedBinaryTreeFragment;

	// Static Balanced Binary Tree facade
	template<typename T, std::size_t depth>
	class StaticBalancedBinaryTree;



	// ---------------------------------------------------------------------------------
	//								  Definitions
	// ---------------------------------------------------------------------------------


	/**
	 * A region description for a sub-set of binary tree elements.
	 */
	template<std::size_t depth>
	class StaticBalancedBinaryTreeRegion : public utils::trivially_serializable {

	public:

		// the depth of the root tree -- at most 10, to produce 1K leaf trees
		constexpr static std::size_t root_tree_depth = std::min<std::size_t>(depth/2,10);

		// the number of leaf trees
		constexpr static std::size_t num_leaf_trees = 1 << root_tree_depth;

	private:

		// the type for the sub-tree mask
		using mask_t = std::bitset<num_leaf_trees+1>;

		// a mask for leaf sub-trees (up to mask_length sub-trees) + root tree (last bit)
		mask_t mask;

		StaticBalancedBinaryTreeRegion(const mask_t& mask) : mask(mask) {}

	public:

		// -- Data Item Region Interface --

		StaticBalancedBinaryTreeRegion() {}

		bool operator==(const StaticBalancedBinaryTreeRegion& other) const {
			return mask == other.mask;
		}

		bool operator!=(const StaticBalancedBinaryTreeRegion& other) const {
			return !(*this == other);
		}

		bool empty() const {
			return mask.none();
		}

		static StaticBalancedBinaryTreeRegion merge(const StaticBalancedBinaryTreeRegion& a, const StaticBalancedBinaryTreeRegion& b) {
			return a.mask | b.mask;
		}

		static StaticBalancedBinaryTreeRegion intersect(const StaticBalancedBinaryTreeRegion& a, const StaticBalancedBinaryTreeRegion& b) {
			return a.mask & b.mask;
		}

		static StaticBalancedBinaryTreeRegion difference(const StaticBalancedBinaryTreeRegion& a, const StaticBalancedBinaryTreeRegion& b) {
			return a.mask & (~b.mask);
		}

		static StaticBalancedBinaryTreeRegion span(const StaticBalancedBinaryTreeRegion&, const StaticBalancedBinaryTreeRegion&) {
			assert_fail() << "Invalid operation!";
			return {};
		}

		static StaticBalancedBinaryTreeRegion closure(const StaticBalancedBinaryTreeRegion& r) {
			if (r.containsRootTree()) return full();
			return r;
		}

		static StaticBalancedBinaryTreeRegion closure(int i) {
			if (i == num_leaf_trees) return full();
			return subtree(i);
		}

		static StaticBalancedBinaryTreeRegion full() {
			StaticBalancedBinaryTreeRegion res;
			res.mask.flip();
			return res;
		}

		// -- Region Specific Interface --

		static StaticBalancedBinaryTreeRegion root() {
			mask_t res;
			res.set(num_leaf_trees);
			return res;
		}

		static StaticBalancedBinaryTreeRegion subtree(int i) {
			assert_le(0,i);
			assert_le(i,int(num_leaf_trees));
			mask_t res;
			res.set(i);
			return res;
		}

		bool containsRootTree() const {
			return mask.test(num_leaf_trees);
		}

		bool containsSubTree(int i) const {
			return 0 <= i && i < num_leaf_trees && mask.test(i);
		}

		bool contains(const StaticBalancedBinaryTreeElementAddress<depth>& addr) const {
			return mask.test(addr.getSubtreeIndex());
		}

		template<typename Lambda>
		void forEachSubTree(const Lambda& op) const {
			for(std::size_t i=0; i<num_leaf_trees; i++) {
				if (mask.test(i)) op(i);
			}
		}

		friend std::ostream& operator<<(std::ostream& out, const StaticBalancedBinaryTreeRegion& region) {
			out << "{";
			if (region.containsRootTree()) out << " R";
			region.forEachSubTree([&](std::size_t i){
				out << ' ' << i;
			});
			return out << " }";
		}

	};

	/**
	 * The type used to address elements in a static balanced binary tree.
	 */
	template<std::size_t depth>
	class StaticBalancedBinaryTreeElementAddress : public utils::trivially_serializable {

		constexpr static int num_leaf_trees = StaticBalancedBinaryTreeRegion<depth>::num_leaf_trees;
		constexpr static int root_tree_depth = StaticBalancedBinaryTreeRegion<depth>::root_tree_depth;
		constexpr static int root_tree_index = num_leaf_trees;

		// the sub-tree it belongs to
		int subtree;

		// the index within the selected sub-tree
		int index;

		// the nesting level
		int level;

		StaticBalancedBinaryTreeElementAddress(int s, int i, int l)
			: subtree(s), index(i), level(l) {}

	public:

		// creates an address pointing to the root node
		StaticBalancedBinaryTreeElementAddress() : subtree(root_tree_index), index(1), level(0) {}

		// tests whether the addressed node is a leaf node
		bool isLeaf() const {
			return level == depth-1;
		}

		// obtains the level addressed by this node
		int getLevel() const {
			return level;
		}

		// obtains the address of the left child
		StaticBalancedBinaryTreeElementAddress getLeftChild() const {
			if (level == root_tree_depth-1) {
				return { (2*index) % num_leaf_trees, 1, level+1 };
			}
			return { subtree, 2*index, level+1 };
		}

		StaticBalancedBinaryTreeElementAddress getRightChild() const {
			if (level == root_tree_depth-1) {
				return { (2*index+1) % num_leaf_trees, 1, level+1 };
			}
			return { subtree, 2*index+1, level+1 };
		}

		int getSubtreeIndex() const {
			return subtree;
		}

		int getIndexInSubtree() const {
			return index;
		}

		friend std::ostream& operator<<(std::ostream& out, const StaticBalancedBinaryTreeElementAddress& address) {
			if (address.subtree == root_tree_index) {
				out << 'R';
			} else {
				out << address.subtree;
			}
			out << '/' << address.index;
			out << "(l=" << address.level << ")";
			return out;
		}

	};

	namespace detail {

		template<typename T, std::size_t depth>
		class StaticBalancedBinarySubTree {

		public:

			// the number of elements stored in this tree
			constexpr static std::size_t num_elements = (std::size_t(1) << depth) - 1;

		private:

			using data_t = std::vector<T>;

			// the flattened tree
			data_t data;

			StaticBalancedBinarySubTree(data_t&& data) : data(std::move(data)) {}

		public:

			StaticBalancedBinarySubTree() : data(num_elements) {}

			// -- serialization support --

			static StaticBalancedBinarySubTree load(utils::ArchiveReader& reader) {
				return reader.read<data_t>();
			}

			void store(utils::ArchiveWriter& writer) const {
				writer.write(data);
			}

			const T& get(std::size_t i) const {
				assert_lt(0,i);
				assert_lt(i,num_elements+1);
				return data[i-1];
			}

			T& get(std::size_t i) {
				assert_lt(0,i);
				assert_lt(i,num_elements+1);
				return data[i-1];
			}
		};

	}

	/**
	 * A fragment capable of storing a sub-set of a static balanced binary tree.
	 */
	template<typename T, std::size_t depth>
	class StaticBalancedBinaryTreeFragment {
	public:

		using address_t = StaticBalancedBinaryTreeElementAddress<depth>;
		using region_type = StaticBalancedBinaryTreeRegion<depth>;
		using facade_type = StaticBalancedBinaryTree<T,depth>;
		using shared_data_type = core::no_shared_data;

	private:

		using root_tree_type = detail::StaticBalancedBinarySubTree<T,region_type::root_tree_depth>;
		using leaf_tree_type = detail::StaticBalancedBinarySubTree<T,depth - region_type::root_tree_depth>;

		using root_tree_ptr = std::unique_ptr<root_tree_type>;
		using leaf_tree_ptr = std::unique_ptr<leaf_tree_type>;

		// the covered tree region
		region_type covered;

		// the active fragments
		root_tree_ptr root;
		std::array<leaf_tree_ptr,region_type::num_leaf_trees> leafs;

	public:

		StaticBalancedBinaryTreeFragment(const shared_data_type&, const region_type& region = region_type()) : covered() {
			// create covered data
			resize(region);
		}

		const region_type& getCoveredRegion() const {
			return covered;
		}

		void resize(const region_type& newSize) {

			// compute sets to be added and removed
			auto remove = region_type::difference(covered,newSize);
			auto add = region_type::difference(newSize,covered);

			// remove no longer needed regions first
			if (remove.containsRootTree()) root.reset();
			remove.forEachSubTree([&](std::size_t i){
				leafs[i].reset();
			});

			// allocate new regions
			if (add.containsRootTree()) root = std::make_unique<root_tree_type>();
			add.forEachSubTree([&](std::size_t i) {
				leafs[i] = std::make_unique<leaf_tree_type>();
			});

			// update covered size
			covered = newSize;
		}

		void insert(const StaticBalancedBinaryTreeFragment& src, const region_type& region) {
			assert_true(core::isSubRegion(region,src.covered)) << "Can't import non-covered region!";
			assert_true(core::isSubRegion(region,covered))     << "Can't import non-covered region!";

			// copy data from source fragment
			if (region.containsRootTree()) *root = *src.root;
			region.forEachSubTree([&](std::size_t i){
				*leafs[i] = *src.leafs[i];
			});
		}

		void extract(utils::ArchiveWriter& writer, const region_type& region) const {
			assert_true(core::isSubRegion(region,covered)) << "Can't extract non-covered region!";

			// write region first
			writer.write(region);

			// write selected regions to stream
			if (region.containsRootTree()) writer.write(*root);
			region.forEachSubTree([&](std::size_t i){
				writer.write(*leafs[i]);
			});
		}

		void insert(utils::ArchiveReader& reader) {
			// read region to import first
			auto region = reader.read<region_type>();

			// check validity
			assert_true(core::isSubRegion(region,covered)) << "Can't insert non-covered region!";

			// insert selected regions from stream
			if (region.containsRootTree()) *root = reader.read<root_tree_type>();
			region.forEachSubTree([&](std::size_t i){
				*leafs[i] = reader.read<leaf_tree_type>();
			});
		}

		facade_type mask() {
			return *this;
		}

		// -- Tree Specific Interface --

		const T& operator[](const address_t& addr) const {
			return const_cast<StaticBalancedBinaryTree<T,depth>&>(*this)[addr];
		}

		T& operator[](const address_t& addr) {
			// dispatch to root ..
			if (addr.getSubtreeIndex() == region_type::num_leaf_trees) {
				return root->get(addr.getIndexInSubtree());
			}
			// .. otherwise access leaf tree data
			return leafs[addr.getSubtreeIndex()]->get(addr.getIndexInSubtree());
		}
	};

	/**
	 * A static balanced binary tree
	 */
	template<typename T, std::size_t depth>
	class StaticBalancedBinaryTree : public core::data_item<StaticBalancedBinaryTreeFragment<T,depth>> {

		using region_t = StaticBalancedBinaryTreeRegion<depth>;
		using fragment_t = StaticBalancedBinaryTreeFragment<T,depth>;

		/**
		 * A pointer to an underlying fragment owned if used in an unmanaged state.
		 */
		std::unique_ptr<fragment_t> owned;

		/**
		 * A reference to the fragment instance operating on, referencing the owned fragment or an externally managed one.
		 */
		fragment_t& base;

		/**
		 * Enables fragments to use the private constructor below.
		 */
		friend class StaticBalancedBinaryTreeFragment<T,depth>;

		/**
		 * The constructor to be utilized by the fragment to create a facade for an existing fragment.
		 */
		StaticBalancedBinaryTree(StaticBalancedBinaryTreeFragment<T,depth>& base) : base(base) {}

	public:

		// the type used to address elements within the tree
		using address_t = StaticBalancedBinaryTreeElementAddress<depth>;

		/**
		 * The type of element stored in this tree.
		 */
		using element_type = T;

		/**
		 * Creates a new tree, all elements default initialized.
		 */
		StaticBalancedBinaryTree() : owned(std::make_unique<fragment_t>(core::no_shared_data(), region_t::full())), base(*owned) {}

		// -- tree element access --

		/**
		 * Provide constant element access.
		 */
		const T& operator[](const address_t& addr) const {
			return data_item_element_access(*this,region_t::subtree(addr.getSubtreeIndex()),base[addr]);
		}

		/**
		 * Provide immutable element access.
		 */
		T& operator[](const address_t& addr) {
			return data_item_element_access(*this,region_t::subtree(addr.getSubtreeIndex()),base[addr]);
		}
	};


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale

