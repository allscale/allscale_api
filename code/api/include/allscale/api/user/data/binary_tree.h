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

	// a type trait defining the default root tree depth (where applicable)
	template<std::size_t tree_depth>
	struct default_root_tree_depth;

	// Static Balanced Binary Tree Region handling the root fragment as a block
	template<std::size_t tree_depth, std::size_t root_depth = default_root_tree_depth<tree_depth>::value>
	class StaticBalancedBinaryTreeBlockedRegion;

	// Static Balanced Binary Tree Region handling the root fragment in a fine grained fashion
	template<std::size_t tree_depth, std::size_t root_depth = default_root_tree_depth<tree_depth>::value>
	class StaticBalancedBinaryTreeRegion;

	// Static Balanced Binary Tree Element Address
	template<typename Region>
	class StaticBalancedBinaryTreeElementAddress;

	// Static Balanced Binary Tree Fragment
	template<typename T, typename Region>
	class StaticBalancedBinaryTreeFragment;

	// Static Balanced Binary Tree facade
	template<typename T, std::size_t depth, template<std::size_t,std::size_t> class RegionType = StaticBalancedBinaryTreeRegion>
	class StaticBalancedBinaryTree;



	// ---------------------------------------------------------------------------------
	//								  Definitions
	// ---------------------------------------------------------------------------------

	/**
	 * A definition of the relation between a tree's depth and the root tree depth
	 */
	template<std::size_t tree_depth>
	struct default_root_tree_depth {

		// by default, the root tree is half the height of the overall tree, but at most 10, producing 1K subtrees
		constexpr static std::size_t value = std::min<std::size_t>(tree_depth/2,10);

	};


	/**
	 * A region description for a sub-set of binary tree elements.
	 */
	template<std::size_t tree_depth, std::size_t root_depth>
	class StaticBalancedBinaryTreeBlockedRegion : public utils::trivially_serializable {

	public:

		// the depth of the tree describing a region
		constexpr static std::size_t depth = tree_depth;

		// the depth of the root tree
		constexpr static std::size_t root_tree_depth = root_depth;

		// the number of leaf trees
		constexpr static std::size_t num_leaf_trees = 1 << root_tree_depth;

	private:

		// the type for the sub-tree mask
		using mask_t = std::bitset<num_leaf_trees+1>;

		// a mask for leaf sub-trees (up to mask_length sub-trees) + root tree (last bit)
		mask_t mask;

		StaticBalancedBinaryTreeBlockedRegion(const mask_t& mask) : mask(mask) {}

	public:

		// -- Data Item Region Interface --

		StaticBalancedBinaryTreeBlockedRegion() {}

		bool operator==(const StaticBalancedBinaryTreeBlockedRegion& other) const {
			return mask == other.mask;
		}

		bool operator!=(const StaticBalancedBinaryTreeBlockedRegion& other) const {
			return !(*this == other);
		}

		bool empty() const {
			return mask.none();
		}

		static bool isSubRegion(const StaticBalancedBinaryTreeBlockedRegion& a, const StaticBalancedBinaryTreeBlockedRegion& b) {
			return difference(a,b).empty();
		}

		static StaticBalancedBinaryTreeBlockedRegion merge(const StaticBalancedBinaryTreeBlockedRegion& a, const StaticBalancedBinaryTreeBlockedRegion& b) {
			return a.mask | b.mask;
		}

		static StaticBalancedBinaryTreeBlockedRegion intersect(const StaticBalancedBinaryTreeBlockedRegion& a, const StaticBalancedBinaryTreeBlockedRegion& b) {
			return a.mask & b.mask;
		}

		static StaticBalancedBinaryTreeBlockedRegion difference(const StaticBalancedBinaryTreeBlockedRegion& a, const StaticBalancedBinaryTreeBlockedRegion& b) {
			return a.mask & (~b.mask);
		}

		static StaticBalancedBinaryTreeBlockedRegion span(const StaticBalancedBinaryTreeBlockedRegion&, const StaticBalancedBinaryTreeBlockedRegion&) {
			assert_fail() << "Invalid operation!";
			return {};
		}

		static StaticBalancedBinaryTreeBlockedRegion closure(const StaticBalancedBinaryTreeBlockedRegion& r) {
			if (r.containsRootTree()) return full();
			return r;
		}

		static StaticBalancedBinaryTreeBlockedRegion closure(const StaticBalancedBinaryTreeElementAddress<StaticBalancedBinaryTreeBlockedRegion>& element) {
			return closure(node(element));
		}

		static StaticBalancedBinaryTreeBlockedRegion full() {
			StaticBalancedBinaryTreeBlockedRegion res;
			res.mask.flip();
			return res;
		}

		// -- Region Specific Interface --

		static StaticBalancedBinaryTreeBlockedRegion root() {
			return fullRootTree();	// there is no smaller size
		}

		static StaticBalancedBinaryTreeBlockedRegion fullRootTree() {
			mask_t res;
			res.set(num_leaf_trees);
			return res;
		}

		static StaticBalancedBinaryTreeBlockedRegion node(const StaticBalancedBinaryTreeElementAddress<StaticBalancedBinaryTreeBlockedRegion>& element) {
			if (element.addressesRootTree()) {
				return root();
			}
			return subtree(element.getSubtreeIndex());
		}

		static StaticBalancedBinaryTreeBlockedRegion subtree(int i) {
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

		bool contains(const StaticBalancedBinaryTreeElementAddress<StaticBalancedBinaryTreeBlockedRegion>& addr) const {
			if (addr.getSubtreeIndex() < 0) return containsRootTree();
			return mask.test(addr.getSubtreeIndex());
		}

		template<typename Lambda>
		void forEachSubTree(const Lambda& op) const {
			for(std::size_t i=0; i<num_leaf_trees; i++) {
				if (mask.test(i)) op(i);
			}
		}

		friend std::ostream& operator<<(std::ostream& out, const StaticBalancedBinaryTreeBlockedRegion& region) {
			out << "{";
			if (region.containsRootTree()) out << " R";
			region.forEachSubTree([&](std::size_t i){
				out << ' ' << i;
			});
			return out << " }";
		}

	};



	/**
	 * A region description for a sub-set of binary tree elements.
	 */
	template<std::size_t tree_depth, std::size_t root_depth>
	class StaticBalancedBinaryTreeRegion : public utils::trivially_serializable {

	public:

		// the depth of the three describing a region of
		constexpr static std::size_t depth = tree_depth;

		// the depth of the root tree
		constexpr static std::size_t root_tree_depth = root_depth;

		// the number of leaf trees
		constexpr static std::size_t num_leaf_trees = 1 << root_tree_depth;

	private:

		// the sub-tree index used when actually referring to the root tree
		constexpr static std::size_t root_tree_index = num_leaf_trees;

		// the number of elements contained in the root tree
		constexpr static std::size_t num_root_tree_entries = (1 << root_tree_depth) - 1;

		// the type for the sub-tree mask
		using mask_t = std::bitset<num_root_tree_entries + num_leaf_trees>;

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

		static bool isSubRegion(const StaticBalancedBinaryTreeRegion& a, const StaticBalancedBinaryTreeRegion& b) {
			return difference(a,b).empty();
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

	private:

		static void addShadow(StaticBalancedBinaryTreeRegion& region, std::size_t bit) {

			// if beyond the limit => done
			if (bit >= num_root_tree_entries + num_leaf_trees) return;

			// check the current bit
			if (region.mask.test(bit)) return;		// already set => stop here

			// not set => set the bit and repeat
			region.mask.set(bit);

			// cast more shadow
			addShadow(region, 2*(bit+1)-1);
			addShadow(region, 2*(bit+1));
		}

	public:

		static StaticBalancedBinaryTreeRegion closure(const StaticBalancedBinaryTreeRegion& r) {
			StaticBalancedBinaryTreeRegion res = r;

			// for each root tree node in r ..
			r.forEachRootTreeNode([&](std::size_t i){
				addShadow(res,2*i-1);
				addShadow(res,2*i);
			});
			// done
			return res;
		}

		static StaticBalancedBinaryTreeRegion closure(const StaticBalancedBinaryTreeElementAddress<StaticBalancedBinaryTreeRegion>& element) {
			return closure(node(element));
		}

		static StaticBalancedBinaryTreeRegion full() {
			StaticBalancedBinaryTreeRegion res;
			res.mask.flip();
			return res;
		}

		// -- Region Specific Interface --

		static StaticBalancedBinaryTreeRegion root() {
			mask_t res;
			res.set(0);
			return res;
		}

		static StaticBalancedBinaryTreeRegion fullRootTree() {
			mask_t res;
			for(std::size_t i=0; i<num_root_tree_entries; i++) {
				res.set(i);
			}
			return res;
		}

		static StaticBalancedBinaryTreeRegion node(const StaticBalancedBinaryTreeElementAddress<StaticBalancedBinaryTreeRegion>& element) {
			mask_t res;
			// depending on the element position ...
			if (element.getSubtreeIndex() < 0) {
				// .. in the root tree, we use the index to create a region
				assert_lt(element.getIndexInSubtree() - 1, int(num_root_tree_entries));
				res.set(element.getIndexInSubtree() - 1);
			} else {
				// .. in a subtree, we record the full subtree
				res.set(num_root_tree_entries + element.getSubtreeIndex());
			}
			return res;
		}

		static StaticBalancedBinaryTreeRegion subtree(int i) {
			assert_le(0,i);
			assert_lt(i,int(num_leaf_trees));
			mask_t res;
			res.set(num_root_tree_entries + i);
			return res;
		}

		bool containsAnyRootTreeNode() const {
			for(std::size_t i=0; i<num_root_tree_entries; i++) {
				if (mask.test(i)) return true;
			}
			return false;
		}

		bool containsSubTree(int i) const {
			return 0 <= i && i < num_leaf_trees && mask.test(num_root_tree_entries + i);
		}

		bool contains(const StaticBalancedBinaryTreeElementAddress<StaticBalancedBinaryTreeRegion>& addr) const {
			// if the addressed node is in the root node ..
			if (addr.addressesRootTree()) {
				assert_le(addr.getIndexInSubtree(), int(num_root_tree_entries));
				return mask.test(addr.getIndexInSubtree()-1);
			}
			// if it is in a subtree => just check the corresponding subtree bit
			return mask.test(num_root_tree_entries + addr.getSubtreeIndex());
		}

		template<typename Lambda>
		void forEachRootTreeNode(const Lambda& op) const {
			for(std::size_t i=0; i<num_root_tree_entries; i++) {
				if (mask.test(i)) op(i+1);
			}
		}

		template<typename Lambda>
		void forEachSubTree(const Lambda& op) const {
			for(std::size_t i=0; i<num_leaf_trees; i++) {
				if (mask.test(num_root_tree_entries + i)) op(i);
			}
		}

		friend std::ostream& operator<<(std::ostream& out, const StaticBalancedBinaryTreeRegion& region) {
			out << "{";
			region.forEachRootTreeNode([&](std::size_t i) {
				out << " N" << i;
			});
			region.forEachSubTree([&](std::size_t i) {
				out << " S" << i;
			});
			return out << " }";
		}

	};


	/**
	 * The type used to address elements in a static balanced binary tree.
	 */
	template<typename Region>
	class StaticBalancedBinaryTreeElementAddress : public utils::trivially_serializable {

		constexpr static int depth = Region::depth;
		constexpr static int num_leaf_trees = Region::num_leaf_trees;
		constexpr static int root_tree_depth = Region::root_tree_depth;

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
		StaticBalancedBinaryTreeElementAddress() : subtree(-1), index(1), level(0) {}

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

		bool addressesRootTree() const {
			return subtree < 0;
		}

		int getSubtreeIndex() const {
			return subtree;
		}

		int getIndexInSubtree() const {
			return index;
		}

		friend std::ostream& operator<<(std::ostream& out, const StaticBalancedBinaryTreeElementAddress& address) {
			if (address.addressesRootTree()) {
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
	 * A fragment capable of storing a sub-set of a static balanced binary tree for the blocked region type.
	 */
	template<typename T, std::size_t _depth, std::size_t _root_depth>
	class StaticBalancedBinaryTreeFragment<T,StaticBalancedBinaryTreeBlockedRegion<_depth,_root_depth>> {

		constexpr static std::size_t depth = _depth;

	public:

		using region_type = StaticBalancedBinaryTreeBlockedRegion<_depth,_root_depth>;
		using facade_type = StaticBalancedBinaryTree<T,depth,StaticBalancedBinaryTreeBlockedRegion>;
		using shared_data_type = core::no_shared_data;

		using address_t = StaticBalancedBinaryTreeElementAddress<region_type>;

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

		void insertRegion(const StaticBalancedBinaryTreeFragment& src, const region_type& region) {
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
			if (addr.addressesRootTree()) {
				return root->get(addr.getIndexInSubtree());
			}
			// .. otherwise access leaf tree data
			return leafs[addr.getSubtreeIndex()]->get(addr.getIndexInSubtree());
		}
	};


	/**
	 * A fragment capable of storing a sub-set of a static balanced binary tree for the non-blocked region type.
	 */
	template<typename T, std::size_t _depth, std::size_t _root_depth>
	class StaticBalancedBinaryTreeFragment<T,StaticBalancedBinaryTreeRegion<_depth,_root_depth>> {

		constexpr static std::size_t depth = _depth;

	public:

		using region_type = StaticBalancedBinaryTreeRegion<_depth,_root_depth>;
		using facade_type = StaticBalancedBinaryTree<T,depth,StaticBalancedBinaryTreeRegion>;
		using shared_data_type = core::no_shared_data;

		using address_t = StaticBalancedBinaryTreeElementAddress<region_type>;

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
			if (root && !newSize.containsAnyRootTreeNode()) {
				// all root nodes are dropped => remove tree
				root.reset();
			}
			remove.forEachSubTree([&](std::size_t i){
				leafs[i].reset();
			});

			// allocate new regions
			if (!root && add.containsAnyRootTreeNode()) {
				root = std::make_unique<root_tree_type>();
			}
			add.forEachSubTree([&](std::size_t i) {
				leafs[i] = std::make_unique<leaf_tree_type>();
			});

			// update covered size
			covered = newSize;
		}

		void insertRegion(const StaticBalancedBinaryTreeFragment& src, const region_type& region) {
			assert_true(core::isSubRegion(region,src.covered)) << "Can't import non-covered region!";
			assert_true(core::isSubRegion(region,covered))     << "Can't import non-covered region!";

			// copy data from source fragment
			region.forEachRootTreeNode([&](std::size_t i){
				root->get(i) = src.root->get(i);	// here we copy element wise
			});
			region.forEachSubTree([&](std::size_t i){
				*leafs[i] = *src.leafs[i];	// here we copy entire trees
			});
		}

		void extract(utils::ArchiveWriter& writer, const region_type& region) const {
			assert_true(core::isSubRegion(region,covered)) << "Can't extract non-covered region!";

			// write region first
			writer.write(region);

			// write selected regions to stream
			region.forEachRootTreeNode([&](std::size_t i){
				writer.write(root->get(i));	// here we serialize elements
			});
			region.forEachSubTree([&](std::size_t i){
				writer.write(*leafs[i]);	// here we serialize entire trees
			});
		}

		void insert(utils::ArchiveReader& reader) {
			// read region to import first
			auto region = reader.read<region_type>();

			// check validity
			assert_true(core::isSubRegion(region,covered)) << "Can't insert non-covered region!";

			// insert selected regions from stream
			region.forEachRootTreeNode([&](std::size_t i){
				root->get(i) = reader.read<T>();				// here we load elements
			});
			region.forEachSubTree([&](std::size_t i){
				*leafs[i] = reader.read<leaf_tree_type>();	// here we load entire trees
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
			if (addr.addressesRootTree()) {
				return root->get(addr.getIndexInSubtree());
			}
			// .. otherwise access leaf tree data
			return leafs[addr.getSubtreeIndex()]->get(addr.getIndexInSubtree());
		}
	};

	/**
	 * A static balanced binary tree
	 */
	template<typename T, std::size_t depth, template<std::size_t,std::size_t> class RegionType>
	class StaticBalancedBinaryTree : public core::data_item<StaticBalancedBinaryTreeFragment<T,RegionType<depth,default_root_tree_depth<depth>::value>>> {

		using region_t = RegionType<depth,default_root_tree_depth<depth>::value>;
		using fragment_t = StaticBalancedBinaryTreeFragment<T,region_t>;

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
		friend class StaticBalancedBinaryTreeFragment<T,region_t>;

		/**
		 * The constructor to be utilized by the fragment to create a facade for an existing fragment.
		 */
		StaticBalancedBinaryTree(fragment_t& base) : base(base) {}

	public:

		// the type used to address elements within the tree
		using address_t = StaticBalancedBinaryTreeElementAddress<region_t>;

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
			return data_item_element_access(*this,region_t::node(addr),base[addr]);
		}

		/**
		 * Provide immutable element access.
		 */
		T& operator[](const address_t& addr) {
			return data_item_element_access(*this,region_t::node(addr),base[addr]);
		}
	};


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale

