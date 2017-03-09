#pragma once

#include <algorithm>
#include <iterator>
#include <ostream>

#include <bitset>
#include <cstring>

#include "allscale/utils/assert.h"
#include "allscale/utils/io_utils.h"
#include "allscale/utils/range.h"
#include "allscale/utils/raw_buffer.h"
#include "allscale/utils/static_map.h"
#include "allscale/utils/table.h"

#include "allscale/utils/printer/vectors.h"

#include "allscale/api/core/prec.h"

namespace allscale {
namespace api {
namespace user {
namespace data {


	// --------------------------------------------------------------------
	//							  Declarations
	// --------------------------------------------------------------------



	// --- mesh type parameter constructs ---

	/**
	 * The base type of edges connecting nodes of kind A with nodes of kind B
	 * on the same level.
	 */
	template<typename A, typename B>
	struct edge {
		using src_node_kind = A;
		using trg_node_kind = B;
	};


	/**
	 * The base type of edges connecting nodes of kind A with nodes of kind B
	 * on adjacent levels.
	 */
	template<typename A, typename B>
	struct hierarchy {
		using parent_node_kind = A;
		using child_node_kind = B;
	};

	/**
	 * The constructor for the list of node kinds to be included in a mesh structure.
	 */
	template<typename ... Nodes>
	struct nodes {
		enum { size = sizeof...(Nodes) };
	};

	/**
	 * The constructor for the list of edge kinds to be included in a mesh structure.
	 */
	template<typename ... Edges>
	struct edges {
		enum { size = sizeof...(Edges) };
	};

	/**
	 * The constructor for the list of hierarchies to be included in a mesh structure.
	 */
	template<typename ... Hierarchies>
	struct hierarchies {
		enum { size = sizeof...(Hierarchies) };
	};


	// --- mesh type parameter constructs ---


	/**
	 * The type used for addressing nodes within meshes.
	 */
	template<typename Kind, unsigned Level = 0>
	struct NodeRef;

	/**
	 * The type used for iterating over lists of nodes, e.g. a list of adjacent nodes.
	 */
	template<typename Kind,unsigned Level>
	using NodeList = utils::range<const NodeRef<Kind,Level>*>;


	/**
	 * The type for representing the topological information of a hierarchical mesh.
	 */
	template<
		typename NodeKinds,						// < list of node types in each level
		typename EdgeKinds,						// < list of edge types connecting nodes within levels
		typename Hierarchies = hierarchies<>,	// < list of edge types connecting nodes between adjacent levels
		unsigned Levels = 1,					// < number of levels in the hierarchy
		unsigned PartitionDepth = 0				// < number of partitioning level
	>
	class Mesh;


	/**
	 * The type for associating (dynamic) information to nodes within a mesh.
	 */
	template<
		typename NodeKind,				// < the type of node to be annotated
		typename ElementType,			// < the type of value to be associated to each node on the given level
		unsigned Level,					// < the level of the mesh to be annotated
		typename PartitionTree			// < the type of the partition tree indexing the associated mesh
	>
	class MeshData;


	/**
	 * A utility to construct meshes.
	 */
	template<
		typename NodeKinds,						// < list of node types in each level
		typename EdgeKinds,						// < list of edge types connecting nodes within levels
		typename Hierarchies = hierarchies<>,	// < list of edge types connecting nodes between adjacent levels
		unsigned Levels = 1						// < number of levels in the hierarchy
	>
	class MeshBuilder;



	// --------------------------------------------------------------------
	//							  Definitions
	// --------------------------------------------------------------------

	// The type used for identifying nodes within meshes.
	using NodeID = uint32_t;

	/**
	 * The type used for addressing nodes within meshes.
	 */
	template<typename Kind,unsigned Level>
	struct NodeRef {


		using node_type = Kind;
		enum { level = Level };

		NodeID id;

		NodeRef() = default;

		constexpr NodeRef(std::size_t id) : id(id) {}

		NodeID getOrdinal() const {
			return id;
		}

		bool operator==(const NodeRef& other) const {
			return id == other.id;
		}

		bool operator!=(const NodeRef& other) const {
			return id != other.id;
		}

		bool operator<(const NodeRef& other) const {
			return id < other.id;
		}

		friend std::ostream& operator<<(std::ostream& out, const NodeRef& ref) {
			return out << "n" << ref.id;
		}
	};


	template<typename Kind, unsigned Level>
	class NodeRange {

		NodeRef<Kind,Level> _begin;

		NodeRef<Kind,Level> _end;

	public:

		NodeRange(const NodeRef<Kind,Level>& a, const NodeRef<Kind,Level>& b) : _begin(a), _end(b) {
			assert_le(_begin.id,_end.id);
		}

		NodeRef<Kind,Level> getBegin() const {
			return _begin;
		}

		NodeRef<Kind,Level> getEnd() const {
			return _end;
		}

		NodeRef<Kind,Level> operator[](std::size_t index) const {
			return NodeRef<Kind,Level> { _begin.id + index };
		}

		std::size_t size() const {
			return _end.id - _begin.id;
		}


		class const_iterator : public std::iterator<std::random_access_iterator_tag, NodeRef<Kind,Level>> {

			std::size_t cur;

		public:

			const_iterator(std::size_t pos) : cur(pos) {};

			bool operator==(const const_iterator& other) const {
				return cur == other.cur;
			}

			bool operator!=(const const_iterator& other) const {
				return !(*this == other);
			}

			bool operator<(const const_iterator& other) const {
				return cur < other.cur;
			}

			bool operator<=(const const_iterator& other) const {
				return cur <= other.cur;
			}

			bool operator>=(const const_iterator& other) const {
				return cur >= other.cur;
			}

			bool operator>(const const_iterator& other) const {
				return cur > other.cur;
			}

			NodeRef<Kind,Level> operator*() const {
				return NodeRef<Kind,Level>{cur};
			}

			const_iterator& operator++() {
				++cur;
				return *this;
			}

			const_iterator operator++(int) {
				const_iterator res = *this;
				++cur;
				return res;
			}

			const_iterator& operator--() {
				--cur;
				return *this;
			}

			const_iterator operator--(int) {
				const_iterator res = *this;
				--cur;
				return res;
			}

			const_iterator& operator+=(std::ptrdiff_t n) {
				cur += n;
				return *this;
			}

			const_iterator& operator-=(std::ptrdiff_t n) {
				cur -= n;
				return *this;
			}

			friend const_iterator operator+(const_iterator& iter, std::ptrdiff_t n) {
				const_iterator res = iter;
				res.cur += n;
				return res;

			}

			friend const_iterator& operator+(std::ptrdiff_t n, const_iterator& iter) {
				const_iterator res = iter;
				res.cur += n;
				return res;
			}

			const_iterator operator-(std::ptrdiff_t n) {
				const_iterator res = *this;
				res.cur -= n;
				return res;
			}

			std::ptrdiff_t operator-(const_iterator& other) const {
				return std::ptrdiff_t(cur - other.cur);
			}

			NodeRef<Kind,Level> operator[](std::ptrdiff_t n) const {
				return *(*this + n);
			}

		};

		const_iterator begin() const {
			return const_iterator(_begin.id);
		}

		const_iterator end() const {
			return const_iterator(_end.id);
		}

		template<typename Body>
		void forAll(const Body& body) {
			for(const auto& cur : *this) {
				body(cur);
			}
		}

		friend std::ostream& operator<<(std::ostream& out, const NodeRange& range) {
			return out << "[" << range._begin.id << "," << range._end.id << ")";
		}

	};


	namespace detail {

		template<typename List>
		struct is_nodes : public std::false_type {};

		template<typename ... Kinds>
		struct is_nodes<nodes<Kinds...>> : public std::true_type {};

		template<typename List>
		struct is_edges : public std::false_type {};

		template<typename ... Kinds>
		struct is_edges<edges<Kinds...>> : public std::true_type {};

		template<typename List>
		struct is_hierarchies : public std::false_type {};

		template<typename ... Kinds>
		struct is_hierarchies<hierarchies<Kinds...>> : public std::true_type {};

		template<unsigned Level>
		struct level {
			enum { value = Level };
		};


		template<typename T>
		struct get_level;

		template<unsigned L>
		struct get_level<level<L>> {
			enum { value = L };
		};

		template<typename T>
		struct get_level<T&> : public get_level<T> {};
		template<typename T>
		struct get_level<const T> : public get_level<T> {};
		template<typename T>
		struct get_level<volatile T> : public get_level<T> {};

		template<typename T>
		using plain_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;


		template<typename Element>
		void sumPrefixes(utils::Table<Element>& list) {
			Element counter = 0;
			for(auto& cur : list) {
				auto tmp = cur;
				cur = counter;
				counter += tmp;
			}
		}


		template<unsigned Level, typename ... NodeTypes>
		struct NodeSet;

		template<unsigned Level, typename First, typename ... Rest>
		struct NodeSet<Level,First,Rest...> {

			std::size_t node_counter;

			NodeSet<Level,Rest...> nested;

			NodeSet() : node_counter(0) {}
			NodeSet(const NodeSet&) = default;
			NodeSet(NodeSet&& other) = default;

			NodeSet& operator=(const NodeSet&) =default;
			NodeSet& operator=(NodeSet&&) =default;

			template<typename T>
			typename std::enable_if<std::is_same<First,T>::value,NodeSet&>::type get() {
				return *this;
			}

			template<typename T>
			typename std::enable_if<std::is_same<First,T>::value,const NodeSet&>::type get() const {
				return *this;
			}

			template<typename T>
			auto get() -> typename std::enable_if<!std::is_same<First,T>::value,decltype(nested.template get<T>())>::type {
				return nested.template get<T>();
			}

			template<typename T>
			auto get() const -> typename std::enable_if<!std::is_same<First,T>::value,decltype(nested.template get<T>())>::type {
				return nested.template get<T>();
			}

			NodeRef<First,Level> create() {
				return { node_counter++ };
			}

			NodeRange<First,Level> create(unsigned num) {
				NodeRef<First,Level> begin(node_counter);
				node_counter += num;
				NodeRef<First,Level> end(node_counter);
				return { begin, end };
			}

			std::size_t getNumNodes() const {
				return node_counter;
			}

			bool operator==(const NodeSet& other) const {
				return node_counter == other.node_counter && nested == other.nested;
			}

			template<typename Body>
			void forAll(const Body& body) const {
				// iterate over nodes of this kind
				for(std::size_t i = 0; i<node_counter; i++) {
					body(NodeRef<First,Level>(i));
				}
				// and on remaining kinds
				nested.forAll(body);
			}

			template<typename Body>
			void forAllKinds(const Body& body) const {
				// call for this type
				body(First(), level<Level>());
				// and the nested types
				nested.forAllKinds(body);
			}

			// -- IO support --

			void store(std::ostream& out) const {
				// store the number of nodes
				utils::write<std::size_t>(out, node_counter);

				// store the nested hierarchy
				nested.store(out);
			}

			static NodeSet load(std::istream& in) {

				// produce result
				NodeSet res;

				// restore the number of nodes
				res.node_counter = utils::read<std::size_t>(in);

				// load nested
				res.nested = NodeSet<Level,Rest...>::load(in);

				// done
				return res;
			}

			static NodeSet interpret(utils::RawBuffer& buffer) {

				// produce result
				NodeSet res;

				// restore the number of nodes
				res.node_counter = buffer.consume<std::size_t>();

				// load nested
				res.nested = NodeSet<Level,Rest...>::interpret(buffer);

				// done
				return res;
			}
		};

		template<unsigned Level>
		struct NodeSet<Level> {

			NodeSet() = default;
			NodeSet(const NodeSet&) = default;
			NodeSet(NodeSet&&) {}

			NodeSet& operator=(NodeSet&&) =default;

			template<typename T>
			NodeSet& get() {
				assert(false && "Invalid type accessed!");
				return *this;
			}

			template<typename T>
			const NodeSet& get() const {
				assert(false && "Invalid type accessed!");
				return *this;
			}

			bool operator==(const NodeSet& other) const {
				return true;
			}

			template<typename Body>
			void forAll(const Body&) const {
				// no more nodes
			}

			template<typename Body>
			void forAllKinds(const Body&) const {
				// nothing to do
			}

			// -- IO support --

			void store(std::ostream&) const {
				// nothing
			}

			static NodeSet load(std::istream&) {
				return NodeSet();
			}

			static NodeSet interpret(utils::RawBuffer&) {
				return NodeSet();
			}

		};


		template<unsigned Level, typename ... EdgeTypes>
		struct EdgeSet;

		template<unsigned Level, typename First, typename ... Rest>
		struct EdgeSet<Level,First,Rest...> {

			using Src = typename First::src_node_kind;
			using Trg = typename First::trg_node_kind;

			utils::Table<uint32_t> forward_offsets;
			utils::Table<NodeRef<Trg,Level>> forward_targets;

			utils::Table<uint32_t> backward_offsets;
			utils::Table<NodeRef<Src,Level>> backward_targets;

			std::vector<std::pair<NodeRef<Src,Level>,NodeRef<Trg,Level>>> edges;


			EdgeSet<Level,Rest...> nested;

			EdgeSet() = default;
			EdgeSet(const EdgeSet&) = default;
			EdgeSet(EdgeSet&& other)
				: forward_offsets(std::move(other.forward_offsets)),
				  forward_targets(std::move(other.forward_targets)),
				  backward_offsets(std::move(other.backward_offsets)),
				  backward_targets(std::move(other.backward_targets)),
				  edges(std::move(other.edges)),
				  nested(std::move(other.nested))
			{}

			EdgeSet& operator=(EdgeSet&&) =default;

			template<typename T>
			typename std::enable_if<std::is_same<First,T>::value,EdgeSet&>::type get() {
				return *this;
			}

			template<typename T>
			typename std::enable_if<std::is_same<First,T>::value,const EdgeSet&>::type get() const {
				return *this;
			}

			template<typename T>
			auto get() -> typename std::enable_if<!std::is_same<First,T>::value,decltype(nested.template get<T>())>::type {
				return nested.template get<T>();
			}

			template<typename T>
			auto get() const -> typename std::enable_if<!std::is_same<First,T>::value,decltype(nested.template get<T>())>::type {
				return nested.template get<T>();
			}

		public:

			void addEdge(const NodeRef<Src,Level>& src, const NodeRef<Trg,Level>& trg) {
				edges.push_back({src,trg});
			}

			template<typename MeshData>
			void close(const MeshData& data) {

				// init forward / backward vectors
				forward_offsets = utils::Table<uint32_t>(data.template getNumNodes<Src,Level>() + 1, 0);
				forward_targets = utils::Table<NodeRef<Trg,Level>>(edges.size());

				// count number of sources / sinks
				for(const auto& cur : edges) {
					forward_offsets[cur.first.id]++;
				}

				// compute prefix sums
				sumPrefixes(forward_offsets);

				// fill in targets
				auto forward_pos = forward_offsets;
				for(const auto& cur : edges) {
					forward_targets[forward_pos[cur.first.id]++] = cur.second;
				}

				// clear edges
				edges.clear();

				// also fill in backward edges
				restoreBackward(data.template getNumNodes<Trg,Level>() + 1);

				// close nested edges
				nested.close(data);
			}

			bool isClosed() const {
				return edges.empty() && nested.isClosed();
			}

			NodeList<Trg,Level> getSinks(const NodeRef<Src,Level>& src) const {
				assert_true(edges.empty()) << "Accessing non-closed edge set!";
				if (src.id >= forward_offsets.size()-1 || forward_targets.empty()) return NodeList<Trg,Level>{nullptr,nullptr};
				return NodeList<Trg,Level>{&forward_targets[forward_offsets[src.id]], &forward_targets[forward_offsets[src.id+1]]};
			}

			NodeList<Src,Level> getSources(const NodeRef<Trg,Level>& src) const {
				assert_true(edges.empty()) << "Accessing non-closed edge set!";
				if (src.id >= backward_offsets.size()-1 || backward_targets.empty()) return NodeList<Src,Level>{nullptr,nullptr};
				return NodeList<Src,Level>{&backward_targets[backward_offsets[src.id]], &backward_targets[backward_offsets[src.id+1]]};
			}

			bool operator==(const EdgeSet& other) const {
				return forward_offsets == other.forward_offsets &&
						forward_targets == other.forward_targets &&
						backward_offsets == other.backward_offsets &&
						backward_targets == other.backward_targets &&
						edges == other.edges && nested == other.nested;
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {

				// visit all edges
				for(const auto& cur : edges) {
					body(First(),cur.first,cur.second);
				}

				// visit all forward edges
				for(std::size_t i = 0 ; i < forward_offsets.size()-1; ++i) {
					NodeRef<Src,Level> src {i};
					for(const auto& trg : getSinks(src)) {
						body(First(),src,trg);
					}
				}

			}

			template<typename Body>
			void forAll(const Body& body) const {

				// visit all local edges
				forAllEdges(body);

				// visit links of remaining hierarchies
				nested.forAll(body);
			}

			template<typename Body>
			void forAllKinds(const Body& body) const {
				// visit all links for this type
				body(First(), level<Level>());
				// visit links of remaining hierarchies
				nested.forAllKinds(body);
			}


			// -- IO support --

			void store(std::ostream& out) const {
				// only allow closed sets to be stored
				assert_true(isClosed());

				// write forward edge data
				forward_offsets.store(out);
				forward_targets.store(out);

				// write backward edge data
				backward_offsets.store(out);
				backward_targets.store(out);

				// store the nested hierarchy
				nested.store(out);
			}

			static EdgeSet load(std::istream& in) {

				EdgeSet res;

				// restore edge data
				res.forward_offsets = utils::Table<uint32_t>::load(in);
				res.forward_targets = utils::Table<NodeRef<Trg,Level>>::load(in);

				res.backward_offsets = utils::Table<uint32_t>::load(in);
				res.backward_targets = utils::Table<NodeRef<Src,Level>>::load(in);

				// restore nested
				res.nested.load(in);

				// done
				return res;
			}

			static EdgeSet interpret(utils::RawBuffer& buffer) {

				EdgeSet res;

				// restore edge data
				res.forward_offsets = utils::Table<uint32_t>::interpret(buffer);
				res.forward_targets = utils::Table<NodeRef<Trg,Level>>::interpret(buffer);

				res.backward_offsets = utils::Table<uint32_t>::interpret(buffer);
				res.backward_targets = utils::Table<NodeRef<Src,Level>>::interpret(buffer);

				// restore nested
				res.nested.interpret(buffer);

				// done
				return res;
			}

		private:

			void restoreBackward(std::size_t numTargetNodes) {

				// fix sizes of backward vectors
				backward_offsets = utils::Table<uint32_t>(numTargetNodes,0);
				backward_targets = utils::Table<NodeRef<Src,Level>>(forward_targets.size()); // the same length as forward

				// count number of sources
				forAllEdges([&](const auto&, const auto&, const auto& trg){
					++backward_offsets[trg.id];
				});

				// compute prefix sums
				sumPrefixes(backward_offsets);

				// fill in sources
				auto backward_pos = backward_offsets;
				forAllEdges([&](const auto&, const auto& src, const auto& trg){
					backward_targets[backward_pos[trg.id]++] = src;
				});

			}
		};

		template<unsigned Level>
		struct EdgeSet<Level> {

			EdgeSet() = default;
			EdgeSet(const EdgeSet&) = default;
			EdgeSet(EdgeSet&&) {}

			EdgeSet& operator=(EdgeSet&&) =default;

			template<typename T>
			EdgeSet& get() {
				assert(false && "Invalid type accessed!");
				return *this;
			}

			template<typename T>
			const EdgeSet& get() const {
				assert(false && "Invalid type accessed!");
				return *this;
			}

			bool operator==(const EdgeSet& other) const {
				return true;
			}

			template<typename Body>
			void forAll(const Body&) const {
				// nothing to do
			}

			template<typename Body>
			void forAllKinds(const Body&) const {
				// nothing to do
			}

			template<typename MeshData>
			void close(const MeshData&) {
				// nothing to do
			}

			bool isClosed() const {
				return true;
			}

			// -- IO support --

			void store(std::ostream&) const {
				// nothing to do her
			}

			static EdgeSet load(std::istream&) {
				return EdgeSet();
			}

			static EdgeSet interpret(utils::RawBuffer&) {
				return EdgeSet();
			}

		};

		template<unsigned Level, typename ... HierachyTypes>
		struct HierarchySet;

		template<unsigned Level, typename First, typename ... Rest>
		struct HierarchySet<Level,First,Rest...> {

			using Src = typename First::parent_node_kind;
			using Trg = typename First::child_node_kind;

			// -- inefficient build structures --

			std::vector<std::vector<NodeRef<Trg,Level-1>>> children;

			std::vector<NodeRef<Src,Level>> parents;

			// -- efficient simulation structures --

			utils::Table<NodeRef<Src,Level>> parent_targets;

			utils::Table<uint32_t> children_offsets;
			utils::Table<NodeRef<Trg,Level-1>> children_targets;

			// -- other hierarchies --

			HierarchySet<Level,Rest...> nested;

			HierarchySet() = default;
			HierarchySet(const HierarchySet&) = default;
			HierarchySet(HierarchySet&&) = default;

			HierarchySet& operator=(HierarchySet&&) =default;

			template<typename T>
			typename std::enable_if<std::is_same<First,T>::value,HierarchySet&>::type get() {
				return *this;
			}

			template<typename T>
			typename std::enable_if<std::is_same<First,T>::value,const HierarchySet&>::type get() const {
				return *this;
			}

			template<typename T>
			auto get() -> typename std::enable_if<!std::is_same<First,T>::value,decltype(nested.template get<T>())>::type {
				return nested.template get<T>();
			}

			template<typename T>
			auto get() const -> typename std::enable_if<!std::is_same<First,T>::value,decltype(nested.template get<T>())>::type {
				return nested.template get<T>();
			}

			void addChild(const NodeRef<Src,Level>& parent, const NodeRef<Trg,Level-1>& child) {
				// a constant for an unknown parent
				static const NodeRef<Src,Level> unknownParent(std::numeric_limits<NodeID>::max());

				assert_ne(parent,unknownParent) << "Unknown parent constant must not be used!";

				// register child as a child of parent
				while(parent.id >= children.size()) {
					children.resize(std::max<std::size_t>(10, children.size() * 2));
				}
				auto& list = children[parent.id];
				for(auto& cur : list) if (cur == child) return;
				list.push_back(child);


				// register parent of child
				while(child.id >= parents.size()) {
					parents.resize(std::max<std::size_t>(10, parents.size() * 2),unknownParent);
				}
				auto& trg = parents[child.id];
				assert_true(trg == unknownParent || trg == parent)
					<< "Double-assignment of parent for child " << child << " and parent " << parent;

				// update parent
				trg = parent;
			}

			template<typename MeshData>
			void close(const MeshData& data) {
				// a constant for an unknown parent
				static const NodeRef<Src,Level> unknownParent(std::numeric_limits<NodeID>::max());

				// compute total number of parent-child links
				std::size_t numParentChildLinks = 0;
				for(const auto& cur : children) {
					numParentChildLinks += cur.size();
				}

				// init forward / backward vectors
				auto numParents = data.template getNumNodes<Src,Level>();
				children_offsets = utils::Table<uint32_t>(numParents + 1, 0);
				children_targets = utils::Table<NodeRef<Trg,Level-1>>(numParentChildLinks);

				// init child offsets
				std::size_t idx = 0;
				std::size_t offset = 0;
				for(const auto& cur : children) {
					children_offsets[idx] = offset;
					offset += cur.size();
					idx++;
					if (idx >= numParents) break;
				}
				children_offsets[idx] = offset;

				// fill in targets
				idx = 0;
				for(const auto& cur : children) {
					for(const auto& child : cur) {
						children_targets[idx++] = child;
					}
				}

				// clear edges
				children.clear();

				// init parent target table
				parent_targets = utils::Table<NodeRef<Src,Level>>(data.template getNumNodes<Trg,Level-1>());
				for(std::size_t i=0; i<parent_targets.size(); ++i) {
					parent_targets[i] = (i < parents.size()) ? parents[i] : unknownParent;
				}

				// clear parents list
				parents.clear();

				// close nested edges
				nested.close(data);
			}

			bool isClosed() const {
				return children.empty();
			}

			NodeList<Trg,Level-1> getChildren(const NodeRef<Src,Level>& parent) const {
				assert_true(isClosed());
				if (parent.id >= children_offsets.size()-1 || children_targets.empty()) return NodeList<Trg,Level-1>{nullptr,nullptr};
				return NodeList<Trg,Level-1>{&children_targets[children_offsets[parent.id]], &children_targets[children_offsets[parent.id+1]]};
			}

			const NodeRef<Src,Level>& getParent(const NodeRef<Trg,Level-1>& child) const {
				assert_true(isClosed());
				assert_lt(child.id,parent_targets.size());
				return parent_targets[child.id];
			}

			bool operator==(const HierarchySet& other) const {
				return children == other.children && nested == other.nested;
			}

			template<typename Body>
			void forAll(const Body& body) const {
				// visit all links for this type
				std::size_t counter = 0;
				for(const auto& cur : children) {
					NodeRef<Src,Level> src {counter++};
					for(const auto& trg : cur) {
						body(First(),src,trg);
					}
				}
				// visit links of remaining hierarchies
				nested.forAll(body);
			}

			template<typename Body>
			void forAllKinds(const Body& body) const {
				// visit all links for this type
				body(First(), level<Level>());
				// visit links of remaining hierarchies
				nested.forAllKinds(body);
			}


			// -- IO support --

			void store(std::ostream& out) const {
				// only allow closed sets to be stored
				assert_true(isClosed());

				// write parents table
				parent_targets.store(out);

				// write child lists
				children_offsets.store(out);
				children_targets.store(out);

				// store the nested hierarchy
				nested.store(out);
			}

			static HierarchySet load(std::istream& in) {

				HierarchySet res;

				// restore parents
				res.parent_targets = utils::Table<NodeRef<Src,Level>>::load(in);

				res.children_offsets = utils::Table<uint32_t>::load(in);
				res.children_targets = utils::Table<NodeRef<Trg,Level-1>>::load(in);

				// restore nested
				res.nested.load(in);

				// done
				return res;
			}

			static HierarchySet interpret(utils::RawBuffer& buffer) {

				HierarchySet res;

				// restore parents
				res.parent_targets = utils::Table<NodeRef<Src,Level>>::interpret(buffer);

				res.children_offsets = utils::Table<uint32_t>::interpret(buffer);
				res.children_targets = utils::Table<NodeRef<Trg,Level-1>>::interpret(buffer);

				// restore nested
				res.nested.interpret(buffer);

				// done
				return res;
			}

		};

		template<unsigned Level>
		struct HierarchySet<Level> {

			HierarchySet() = default;
			HierarchySet(const HierarchySet&) = default;
			HierarchySet(HierarchySet&&) {}

			HierarchySet& operator=(HierarchySet&&) =default;

			template<typename T>
			HierarchySet& get() {
				assert(false && "Invalid type accessed!");
				return *this;
			}

			template<typename T>
			const HierarchySet& get() const {
				assert(false && "Invalid type accessed!");
				return *this;
			}

			bool operator==(const HierarchySet& other) const {
				return true;
			}

			template<typename MeshData>
			void close(const MeshData&) {
				// done
			}

			bool isClosed() const {
				return true;
			}

			template<typename Body>
			void forAll(const Body&) const {
				// nothing to do
			}

			template<typename Body>
			void forAllKinds(const Body&) const {
				// nothing to do
			}

			// -- IO support --

			void store(std::ostream&) const {
				// nothing to do
			}

			static HierarchySet load(std::istream&) {
				return HierarchySet();
			}

			static HierarchySet interpret(utils::RawBuffer&) {
				return HierarchySet();
			}

		};

		template<
			unsigned NumLevels,
			typename Nodes,
			typename Edges,
			typename Hierarchies
		>
		struct Levels;

		template<
			unsigned Level,
			typename ... Nodes,
			typename ... Edges,
			typename ... Hierarchies
		>
		struct Levels<
				Level,
				nodes<Nodes...>,
				edges<Edges...>,
				hierarchies<Hierarchies...>
			> {

			template<unsigned Lvl>
			using LevelData = Levels<Lvl,nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>>;

			// the data of the lower levels
			LevelData<Level-1> nested;

			// the set of nodes on this level
			NodeSet<Level,Nodes...> nodes;

			// the set of edges on this level
			EdgeSet<Level,Edges...> edges;

			// the set of hierarchies connecting this level to the sub-level
			HierarchySet<Level,Hierarchies...> hierarchies;

			Levels() = default;
			Levels(const Levels&) = default;
			Levels(Levels&& other)
				: nested(std::move(other.nested)),
				  nodes(std::move(other.nodes)),
				  edges(std::move(other.edges)),
				  hierarchies(std::move(other.hierarchies))
			{}

			Levels(LevelData<Level-1>&& nested, NodeSet<Level,Nodes...>&& nodes, EdgeSet<Level,Edges...>&& edges, HierarchySet<Level,Hierarchies...>&& hierarchies)
				: nested(std::move(nested)),
				  nodes(std::move(nodes)),
				  edges(std::move(edges)),
				  hierarchies(std::move(hierarchies))
			{}

			Levels& operator= (Levels&& l) = default;

			template<unsigned Lvl>
			typename std::enable_if<Lvl == Level, LevelData<Lvl>&>::type
			getLevel() {
				return *this;
			}

			template<unsigned Lvl>
			typename std::enable_if<Lvl != Level, LevelData<Lvl>&>::type
			getLevel() {
				return nested.template getLevel<Lvl>();
			}

			template<unsigned Lvl>
			typename std::enable_if<Lvl == Level, const LevelData<Lvl>&>::type
			getLevel() const {
				return *this;
			}

			template<unsigned Lvl>
			typename std::enable_if<Lvl != Level, const LevelData<Lvl>&>::type
			getLevel() const {
				return nested.template getLevel<Lvl>();
			}

			bool operator==(const Levels& other) const {
				return nodes == other.nodes && edges == other.edges && nested == other.nested && hierarchies == other.hierarchies;
			}

			template<typename Body>
			void forAllNodes(const Body& body) const {
				nodes.forAll(body);
				nested.forAllNodes(body);
			}

			template<typename Body>
			void forAllNodeKinds(const Body& body) const {
				nodes.forAllKinds(body);
				nested.forAllNodeKinds(body);
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {
				edges.forAll(body);
				nested.forAllEdges(body);
			}

			template<typename Body>
			void forAllEdgeKinds(const Body& body) const {
				edges.forAllKinds(body);
				nested.forAllEdgeKinds(body);
			}

			template<typename Body>
			void forAllHierarchies(const Body& body) const {
				hierarchies.forAll(body);
				nested.forAllHierarchies(body);
			}

			template<typename Body>
			void forAllHierarchyKinds(const Body& body) const {
				hierarchies.forAllKinds(body);
				nested.forAllHierarchyKinds(body);
			}

			template<typename MeshData>
			void close(const MeshData& data) {
				nested.close(data);
				edges.close(data);
				hierarchies.close(data);
			}

			bool isClosed() const {
				return nested.isClosed() && edges.isClosed() && hierarchies.isClosed();
			}

			// -- IO support --

			void store(std::ostream& out) const {
				nested.store(out);
				nodes.store(out);
				edges.store(out);
				hierarchies.store(out);
			}

			static Levels load(std::istream& in) {
				Levels res;
				res.nested      = LevelData<Level-1>::load(in);
				res.nodes       = NodeSet<Level,Nodes...>::load(in);
				res.edges       = EdgeSet<Level,Edges...>::load(in);
				res.hierarchies = HierarchySet<Level,Hierarchies...>::load(in);
				return res;
			}

			static Levels interpret(utils::RawBuffer& buffer) {
				Levels res;
				res.nested      = LevelData<Level-1>::interpret(buffer);
				res.nodes       = NodeSet<Level,Nodes...>::interpret(buffer);
				res.edges       = EdgeSet<Level,Edges...>::interpret(buffer);
				res.hierarchies = HierarchySet<Level,Hierarchies...>::interpret(buffer);
				return res;
			}

		};

		template<
			typename ... Nodes,
			typename ... Edges,
			typename ... Hierarchies
		>
		struct Levels<0,nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>> {

			template<unsigned Lvl>
			using LevelData = Levels<Lvl,nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>>;

			// the set of nodes on this level
			NodeSet<0,Nodes...> nodes;

			// the set of edges on this level
			EdgeSet<0,Edges...> edges;

			Levels() = default;
			Levels(const Levels&) = default;
			Levels(Levels&& other)
				: nodes(std::move(other.nodes)),
				  edges(std::move(other.edges))
			{}

			Levels& operator= (Levels&& l) = default;

			template<unsigned Lvl>
			typename std::enable_if<Lvl == 0, LevelData<Lvl>&>::type
			getLevel() {
				return *this;
			}

			template<unsigned Lvl>
			typename std::enable_if<Lvl == 0, const LevelData<Lvl>&>::type
			getLevel() const {
				return *this;
			}

			bool operator==(const Levels& other) const {
				return nodes == other.nodes && edges == other.edges;
			}

			template<typename Body>
			void forAllNodes(const Body& body) const {
				nodes.forAll(body);
			}

			template<typename Body>
			void forAllNodeKinds(const Body& body) const {
				nodes.forAllKinds(body);
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {
				edges.forAll(body);
			}

			template<typename Body>
			void forAllEdgeKinds(const Body& body) const {
				edges.forAllKinds(body);
			}

			template<typename Body>
			void forAllHierarchies(const Body&) const {
				// nothing to do here
			}

			template<typename Body>
			void forAllHierarchyKinds(const Body&) const {
				// nothing to do here
			}

			template<typename MeshData>
			void close(const MeshData& data) {
				edges.close(data);
			}

			bool isClosed() const {
				return edges.isClosed();
			}

			// -- IO support --

			void store(std::ostream& out) const {
				nodes.store(out);
				edges.store(out);
			}

			static Levels load(std::istream& in) {
				Levels res;
				res.nodes = NodeSet<0,Nodes...>::load(in);
				res.edges = EdgeSet<0,Edges...>::load(in);
				return res;
			}

			static Levels interpret(utils::RawBuffer& buffer) {
				Levels res;
				res.nodes = NodeSet<0,Nodes...>::interpret(buffer);
				res.edges = EdgeSet<0,Edges...>::interpret(buffer);
				return res;
			}

		};


		template<
			typename Nodes,
			typename Edges,
			typename Hierarchies,
			unsigned Levels
		>
		struct MeshTopologyData;

		template<
			typename ... Nodes,
			typename ... Edges,
			typename ... Hierarchies,
			unsigned Levels
		>
		struct MeshTopologyData<nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>,Levels> {

			using data_store = detail::Levels<Levels-1,nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>>;

			// all the topological data of all the nodes, edges and hierarchy relations on all levels
			data_store data;

			MeshTopologyData() = default;
			MeshTopologyData(const MeshTopologyData&) = default;
			MeshTopologyData(MeshTopologyData&& other) : data(std::move(other.data)) {}
			MeshTopologyData(data_store&& data) : data(std::move(data)) {}

			MeshTopologyData& operator= (MeshTopologyData&& m) = default;

			template<unsigned Level>
			detail::NodeSet<Level,Nodes...>& getNodes() {
				return data.template getLevel<Level>().nodes;
			}

			template<unsigned Level>
			const detail::NodeSet<Level,Nodes...>& getNodes() const {
				return data.template getLevel<Level>().nodes;
			}

			template<unsigned Level>
			detail::EdgeSet<Level,Edges...>& getEdges() {
				return data.template getLevel<Level>().edges;
			}

			template<unsigned Level>
			const detail::EdgeSet<Level,Edges...>& getEdges() const {
				return data.template getLevel<Level>().edges;
			}

			template<unsigned Level>
			detail::HierarchySet<Level,Hierarchies...>& getHierarchies() {
				return data.template getLevel<Level>().hierarchies;
			}

			template<unsigned Level>
			const detail::HierarchySet<Level,Hierarchies...>& getHierarchies() const {
				return data.template getLevel<Level>().hierarchies;
			}

			template<typename Body>
			void forAllNodes(const Body& body) const {
				data.forAllNodes(body);
			}

			template<typename Body>
			void forAllNodeKinds(const Body& body) const {
				data.forAllNodeKinds(body);
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {
				data.forAllEdges(body);
			}

			template<typename Body>
			void forAllEdgeKinds(const Body& body) const {
				data.forAllEdgeKinds(body);
			}

			template<typename Body>
			void forAllHierarchies(const Body& body) const {
				data.forAllHierarchies(body);
			}

			template<typename Body>
			void forAllHierarchyKinds(const Body& body) const {
				data.forAllHierarchyKinds(body);
			}

			template<typename Kind,unsigned Level = 0>
			std::size_t getNumNodes() const {
				return getNodes<Level>().template get<Kind>().getNumNodes();
			}

			bool operator==(const MeshTopologyData& other) const {
				return data == other.data;
			}

			void close() {
				data.close(*this);
			}

			bool isClosed() const {
				return data.isClosed();
			}

			// -- IO support --

			void store(std::ostream& out) const {
				assert_true(isClosed());
				data.store(out);
			}

			static MeshTopologyData load(std::istream& in) {
				return MeshTopologyData(data_store::load(in));
			}

			static MeshTopologyData interpret(utils::RawBuffer& buffer) {
				return MeshTopologyData(data_store::interpret(buffer));
			}

		};

		/**
		 * A common basis class for sub-tree and sub-graph references, which are both based on paths
		 * within a tree.
		 */
		template<typename Derived>
		class PathRefBase {

		protected:

			using value_t = uint32_t;

			value_t path;
			value_t mask;

			PathRefBase(value_t path, value_t mask)
				: path(path), mask(mask) {}

		public:

			static Derived root() {
				return { 0u , 0u };
			}

			value_t getPath() const {
				return path;
			}

			value_t getMask() const {
				return mask;
			}

			value_t getDepth() const {
				if (PathRefBase::mask == 0) return 0;
				return sizeof(PathRefBase::mask) * 8 - __builtin_clz(PathRefBase::mask);
			}

			bool isRoot() const {
				return PathRefBase::mask == 0;
			}

			bool isLeftChild() const {
				assert_false(isRoot());
				return !isRightChild();
			}

			bool isRightChild() const {
				assert_false(isRoot());
				return PathRefBase::path & (1 << (getDepth()-1));
			}

			Derived getLeftChild() const {
				assert_lt(getDepth(),sizeof(PathRefBase::path)*8);
				Derived res = asDerived();
				res.PathRefBase::mask = res.PathRefBase::mask | (1 << getDepth());
				return res;
			}

			Derived getRightChild() const {
				Derived res = getLeftChild();
				res.PathRefBase::path = res.PathRefBase::path | (1 << getDepth());
				return res;
			}

			bool operator==(const Derived& other) const {
				// same mask and same valid bit part
				return (PathRefBase::mask == other.PathRefBase::mask) &&
						((PathRefBase::path & PathRefBase::mask) == (other.PathRefBase::path & other.PathRefBase::mask));
			}

			bool operator!=(const Derived& other) const {
				return !(*this == other);
			}

			bool operator<(const Derived& other) const {

				auto thisMask = PathRefBase::mask;
				auto thatMask = other.PathRefBase::mask;

				auto thisPath = PathRefBase::path;
				auto thatPath = other.PathRefBase::path;

				while(true) {

					// if they are the same, we are done
					if (thisMask == thatMask && thisPath == thatPath) return false;

					// check last mask bit
					auto thisMbit = thisMask & 0x1;
					auto thatMbit = thatMask & 0x1;

					if (thisMbit < thatMbit) return true;
					if (thisMbit > thatMbit) return false;

					auto thisPbit = thisMbit & thisPath;
					auto thatPbit = thatMbit & thatPath;

					if (thisPbit < thatPbit) return true;
					if (thisPbit > thatPbit) return false;

					thisMask >>= 1;
					thatMask >>= 1;
					thisPath >>= 1;
					thatPath >>= 1;
				}
			}

			bool operator<=(const Derived& other) const {
				return *this == other || *this < other;
			}

			bool operator>=(const Derived& other) const {
				return !(asDerived() < other);
			}

			bool operator>(const Derived& other) const {
				return !(*this <= other);
			}

			bool covers(const Derived& other) const {
				if (getDepth() > other.getDepth()) return false;
				if (PathRefBase::mask != (PathRefBase::mask & other.PathRefBase::mask)) return false;
				return (PathRefBase::mask & PathRefBase::path) == (PathRefBase::mask & other.PathRefBase::path);
			}

			bool tryMerge(const Derived& other) {

				if (covers(other)) return true;

				if (other.covers(asDerived())) {
					*this = other;
					return true;
				}

				// the masks need to be identical
				auto thisMask = PathRefBase::mask;
				auto thatMask = other.PathRefBase::mask;
				if (thisMask != thatMask) return false;


				// the valid portion of the paths must only differe in one bit
				auto thisPath = PathRefBase::path;
				auto thatPath = other.PathRefBase::path;

				auto thisValid = thisPath & thisMask;
				auto thatValid = thatPath & thatMask;

				auto diff = thisValid ^ thatValid;

				// if there is more than 1 bit difference, there is nothing we can do
				if (__builtin_popcount(diff) != 1) return false;

				// ignore this one bit in the mask
				PathRefBase::mask = PathRefBase::mask & (~diff);

				// done
				return true;
			}

			/**
			 * @return true if the intersection is not empty;
			 * 			in this case this instance has been updated to represent the intersection
			 * 		   false if the intersection is empty, the object has not been altered
			 */
			bool tryIntersect(const Derived& other) {

				// if the other covers this, the intersection is empty
				if (other.covers(asDerived())) return true;

				// if this one is the larger one, this one gets reduced to the smaller one
				if (covers(other)) {
					*this = other;
					return true;
				}

				// make sure common constraints are identical
				auto filterMask = PathRefBase::mask & other.PathRefBase::mask;
				auto thisFilter = PathRefBase::path & filterMask;
				auto thatFilter = other.PathRefBase::path & filterMask;
				if (thisFilter != thatFilter) return false;

				// unite (disjunction!) the constraints of both sides
				PathRefBase::path = (PathRefBase::path & PathRefBase::mask) | (other.PathRefBase::path & other.PathRefBase::mask);
				PathRefBase::mask = PathRefBase::mask | other.PathRefBase::mask;
				return true;
			}



			template<typename Body>
			void visitComplement(const Body& body, unsigned depth = 0) const {

				// when we reached the depth of this reference, we are done
				if (getDepth() == depth) return;

				auto bitMask = (1 << depth);

				// if at this depth there is no wild card
				if (PathRefBase::mask & bitMask) {

					// invert bit at this position
					Derived cpy = asDerived();
					cpy.PathRefBase::path ^= bitMask;
					cpy.PathRefBase::mask = cpy.PathRefBase::mask & ((bitMask << 1) - 1);

					// this is an element of the complement
					body(cpy);

					// continue path
					visitComplement<Body>(body,depth+1);

					return;
				}

				// follow both paths, do nothing here
				Derived cpy = asDerived();
				cpy.PathRefBase::mask = PathRefBase::mask | bitMask;

				// follow the 0 path
				cpy.PathRefBase::path = PathRefBase::path & ~bitMask;
				cpy.template visitComplement<Body>(body,depth+1);

				// follow the 1 path
				cpy.PathRefBase::path = PathRefBase::path | bitMask;
				cpy.template visitComplement<Body>(body,depth+1);

			}

			std::vector<Derived> getComplement() const {
				std::vector<Derived> res;
				visitComplement([&](const Derived& cur){
					res.push_back(cur);
				});
				return res;
			}

		private:

			Derived& asDerived() {
				return static_cast<Derived&>(*this);
			}

			const Derived& asDerived() const {
				return static_cast<const Derived&>(*this);
			}

		};


		/**
		 * A utility to address nodes in the partition tree.
		 */
		class SubTreeRef : public PathRefBase<SubTreeRef> {

			using super = PathRefBase<SubTreeRef>;

			friend super;

			friend class SubMeshRef;

			SubTreeRef(value_t path, value_t mask)
				: super(path,mask) {}

		public:

			value_t getIndex() const {
				// this is reversing the path 000ZYX to 1XYZ to get the usual
				// order of nodes within a embedded tree
				auto res = 1;
				value_t cur = path;
				for(unsigned i = 0; i<getDepth(); ++i) {
					res <<= 1;
					res += cur % 2;
					cur >>= 1;
				}
				return res;
			}


			SubTreeRef getParent() const {
				assert_false(isRoot());
				SubTreeRef res = *this;
				res.PathRefBase::mask = res.PathRefBase::mask & ~(1 << (getDepth()-1));
				return res;
			}


			template<unsigned DepthLimit, bool preOrder, typename Body>
			void enumerate(const Body& body) {

				if (preOrder) body(*this);

				if (getDepth() < DepthLimit) {
					getLeftChild().enumerate<DepthLimit,preOrder>(body);
					getRightChild().enumerate<DepthLimit,preOrder>(body);
				}

				if (!preOrder) body(*this);

			}


			friend std::ostream& operator<<(std::ostream& out, const SubTreeRef& ref) {
				out << "r";
				auto depth = ref.getDepth();
				for(value_t i = 0; i<depth; ++i) {
					out << "." << ((ref.path >> i) % 2);
				}
				return out;
			}

		};


		/**
		 * A reference to a continuously stored part of a mesh.
		 */
		class SubMeshRef : public PathRefBase<SubMeshRef> {

			using super = PathRefBase<SubMeshRef>;

			using value_t = uint32_t;

			friend super;

			SubMeshRef(value_t path, value_t mask)
				: super(path,mask) {}

		public:

			SubMeshRef(const SubTreeRef& ref)
				: super(ref.path, ref.mask) {}

			SubMeshRef getMasked(unsigned pos) const {
				assert_lt(pos,getDepth());
				SubMeshRef res = *this;
				res.super::mask = res.super::mask & ~(1<<pos);
				return res;
			}

			SubMeshRef getUnmasked(unsigned pos) const {
				assert_lt(pos,getDepth());
				SubMeshRef res = *this;
				res.super::mask = res.super::mask | (1<<pos);
				return res;
			}

			SubTreeRef getEnclosingSubTree() const {
				return SubTreeRef(
					super::path,
					(1 << __builtin_ctz(~super::mask)) - 1
				);
			}

			template<typename Body>
			void scan(const Body& body) const {

				// look for last 0 in mask
				unsigned zeroPos = __builtin_ctz(~super::mask);
				if (zeroPos >= getDepth()) {
					body(SubTreeRef(super::path,super::mask));
					return;
				}

				// recursive
				SubMeshRef copy = getUnmasked(zeroPos);

				// set bit to 0
				copy.super::path = copy.super::path & ~( 1 << zeroPos );
				copy.scan(body);

				// set bit to 1
				copy.super::path = copy.super::path |  ( 1 << zeroPos );
				copy.scan(body);
			}


			template<typename NodeKind, unsigned Level, typename PartitionTree, typename Body>
			void scan(const PartitionTree& ptree, const Body& body) const {
				scan([&](const SubTreeRef& ref){
					ptree.template getNodeRange<NodeKind,Level>(ref).forAll(body);
				});
			}


			friend std::ostream& operator<<(std::ostream& out, const SubMeshRef& ref) {
				out << "r";
				auto depth = ref.getDepth();
				for(value_t i = 0; i<depth; ++i) {
					if (ref.super::mask & (1 << i)) {
						out << "." << ((ref.super::path >> i) % 2);
					} else {
						out << ".*";
					}
				}
				return out;
			}

		};

		/**
		 * A union of sub mesh references.
		 */
		class MeshRegion {

			template<
				typename Nodes,
				typename Edges,
				typename Hierarchies,
				unsigned Levels,
				unsigned PartitionDepth
			>
			friend class PartitionTree;

			std::vector<SubMeshRef> refs;

			MeshRegion(const SubMeshRef* begin, const SubMeshRef* end)
				: refs(begin,end) {}

		public:

			MeshRegion() {}

			MeshRegion(const SubMeshRef& ref) {
				refs.push_back(ref);
			}

			MeshRegion(std::initializer_list<SubMeshRef> meshRefs) : refs(meshRefs) {
				restoreSet();
				compress();
			}

			MeshRegion(const std::vector<SubMeshRef>& refs) : refs(refs) {
				restoreSet();
				compress();
			}

			bool operator==(const MeshRegion& other) const {
				return this == &other || refs == other.refs || (difference(*this,other).empty() && difference(other,*this).empty());
			}

			bool operator!=(const MeshRegion& other) const {
				return !(*this == other);
			}

			const std::vector<SubMeshRef>& getSubMeshReferences() const {
				return refs;
			}

			bool empty() const {
				return refs.empty();
			}

			bool covers(const SubMeshRef& ref) const {
				// cheap: one is covering the given reference
				// expensive: the union of this and the reference is the same as this
				return std::any_of(refs.begin(),refs.end(),[&](const SubMeshRef& a) {
					return a.covers(ref);
				}) || (merge(*this,MeshRegion(ref)) == *this);
			}

			bool operator<(const MeshRegion& other) const {
				return refs < other.refs;
			}

			static MeshRegion merge(const MeshRegion& a, const MeshRegion& b) {
				MeshRegion res;
				std::set_union(
					a.refs.begin(), a.refs.end(),
					b.refs.begin(), b.refs.end(),
					std::back_inserter(res.refs)
				);
				res.compress();
				return res;
			}

			template<typename ... Rest>
			static MeshRegion merge(const MeshRegion& a, const MeshRegion& b, const Rest& ... rest) {
				return merge(merge(a,b),rest...);
			}

			static MeshRegion intersect(const MeshRegion& a, const MeshRegion& b) {

				MeshRegion res;

				// compute pairwise intersections
				for(const auto& ra : a.refs) {
					for(const auto& rb : b.refs) {
						auto tmp = ra;
						if (tmp.tryIntersect(rb)) {
							res.refs.push_back(tmp);
						}
					}
				}

				// restore set invariant
				res.restoreSet();

				// compress the set representation
				res.compress();
				return res;
			}

			static MeshRegion difference(const MeshRegion& a, const MeshRegion& b) {
				return intersect(a,complement(b));
			}

			MeshRegion complement() const {

				MeshRegion res = SubMeshRef::root();

				// aggregate the complements of all entries
				for(const auto& cur : refs) {

					// compute the complement of the current entry
					MeshRegion tmp;
					cur.visitComplement([&](const SubMeshRef& ref) {
						tmp.refs.push_back(ref);
					});

					// restore invariant
					tmp.restoreSet();
					tmp.compress();

					// intersect current complement with running complement
					res = intersect(res,tmp);
				}

				// done
				return res;
			}

			static MeshRegion complement(const MeshRegion& region) {
				return region.complement();
			}

			/**
			 * An operator to load an instance of this region from the given archive.
			 */
			static MeshRegion load(utils::Archive&) {
				assert_not_implemented();
				return MeshRegion();
			}

			/**
			 * An operator to store an instance of this region into the given archive.
			 */
			void store(utils::Archive&) const {
				assert_not_implemented();
				// nothing so far
			}

			template<typename Body>
			void scan(const Body& body) const {
				for(const auto& cur : refs) {
					cur.scan(body);
				}
			}

			template<typename NodeKind, unsigned Level, typename PartitionTree, typename Body>
			void scan(const PartitionTree& ptree, const Body& body) const {
				for(const auto& cur : refs) {
					cur.scan<NodeKind,Level>(ptree,body);
				}
			}


			friend std::ostream& operator<<(std::ostream& out, const MeshRegion& reg) {
				return out << reg.refs;
			}

		private:

			void compress() {

				// check precondition
				assert_true(std::is_sorted(refs.begin(),refs.end()));

				// Phase 1:  remove redundant entries
				removeCovered();

				// Phase 2:  collapse adjacent entries (iteratively)
				while (collapseSiblings()) {}
			}


			bool removeCovered() {

				// see whether any change happend
				bool changed = false;
				for(std::size_t i = 0; i<refs.size(); ++i) {

					auto& cur = refs[i];
					auto closure = cur.getEnclosingSubTree();

					std::size_t j = i+1;
					while(j < refs.size() && closure.covers(refs[j].getEnclosingSubTree())) {
						if (cur.covers(refs[j])) {
							refs[j] = cur;
							changed = true;
						}
						++j;
					}

				}

				// restore set condition
				if (changed) restoreSet();

				// report whether the content has been changed
				return changed;
			}

			bool collapseSiblings() {

				// see whether any change happend
				bool changed = false;
				auto size = refs.size();
				for(std::size_t i = 0; i<size; ++i) {
					for(std::size_t j = i+1; j<size; ++j) {
						if (refs[i].tryMerge(refs[j])) {
							refs[j] = refs[i];
							changed = true;
						}
					}
				}

				// restore set condition
				if (changed) restoreSet();

				// report whether the content has been changed
				return changed;

			}

			void restoreSet() {
				// sort elements
				std::sort(refs.begin(),refs.end());
				// remove duplicates
				refs.erase(std::unique(refs.begin(),refs.end()),refs.end());
			}

		};


		// --------------------------------------------------------------
		//					Partition Tree
		// --------------------------------------------------------------


		template<
			typename Nodes,
			typename Edges,
			typename Hierarchies = hierarchies<>,
			unsigned Levels = 1,
			unsigned depth = 12
		>
		class PartitionTree;

		template<
			typename Nodes,
			typename Edges,
			typename Hierarchies,
			unsigned Levels,
			unsigned depth
		>
		class PartitionTree {

			static_assert(detail::is_nodes<Nodes>::value,
					"First template argument of PartitionTree must be of type nodes<...>");

			static_assert(detail::is_edges<Edges>::value,
					"Second template argument of PartitionTree must be of type edges<...>");

			static_assert(detail::is_hierarchies<Hierarchies>::value,
					"Third template argument of PartitionTree must be of type hierarchies<...>");

		};

		template<
			typename ... Nodes,
			typename ... Edges,
			typename ... Hierarchies,
			unsigned Levels,
			unsigned PartitionDepth
		>
		class PartitionTree<nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>,Levels,PartitionDepth> {

		public:

			enum { depth = PartitionDepth };

		private:

			// an internal construct to store node ranges
			struct RangeStore {
				NodeID begin;
				NodeID end;
			};

			// an internal construct to store regions in open and
			// closed structure
			//		- open:   the region pointer is referencing the stored region
			//		- closed: the begin and end indices reference and interval of an externally maintained
			//					list of regions
			struct RegionStore {

				// -- open --
				MeshRegion* region;			// the ownership is managed by the enclosing tree

				// -- closed --
				std::size_t offset;
				std::size_t length;

				RegionStore()
					: region(nullptr), offset(0), length(0) {}

				MeshRegion toRegion(const SubMeshRef* references) const {
					if (region) return *region;
					auto start = references + offset;
					auto end = start + length;
					return MeshRegion(start,end);
				}

				RegionStore& operator=(const MeshRegion& value) {
					if (!region) region = new MeshRegion();
					*region = value;
					return *this;
				}
			};


			static_assert(Levels > 0, "There must be at least one level!");

			struct LevelInfo {

				utils::StaticMap<utils::keys<Nodes...>,RangeStore> nodeRanges;

				utils::StaticMap<utils::keys<Edges...>,RegionStore> forwardClosure;
				utils::StaticMap<utils::keys<Edges...>,RegionStore> backwardClosure;

				utils::StaticMap<utils::keys<Hierarchies...>,RegionStore> parentClosure;
				utils::StaticMap<utils::keys<Hierarchies...>,RegionStore> childClosure;

			};

			struct Node {

				std::array<LevelInfo,Levels> data;

			};

			// some preconditions required for the implementation of this class to work
			static_assert(std::is_trivially_copyable<RangeStore>::value,  "RangeStore should be trivially copyable!");
			static_assert(std::is_trivially_copyable<RegionStore>::value, "RegionStore should be trivially copyable!");
			static_assert(std::is_trivially_copyable<LevelInfo>::value,   "LevelInfo should be trivially copyable!"  );
			static_assert(std::is_trivially_copyable<Node>::value,        "Nodes should be trivially copyable!"      );
			static_assert(std::is_trivially_copyable<SubMeshRef>::value,  "SubMeshRefs should be trivially copyable!");

			enum { num_elements = 1ul << (depth + 1) };

			bool owned;

			Node* data;

			std::size_t numReferences;

			SubMeshRef* references;

			PartitionTree(Node* data, std::size_t numReferences, SubMeshRef* references)
				: owned(false), data(data), numReferences(numReferences), references(references) {
				assert_true(data);
				assert_true(references);
			}

		public:

			PartitionTree() : owned(true), data(new Node[num_elements]), numReferences(0), references(nullptr) {}

			~PartitionTree() {
				if (owned) {
					delete [] data;
					free(references);
				}
			}

			PartitionTree(const PartitionTree&) = delete;

			PartitionTree(PartitionTree&& other)
				: owned(other.owned),
				  data(other.data),
				  numReferences(other.numReferences),
				  references(other.references) {

				// free other from ownership
				other.owned = false;
				other.data = nullptr;
				other.references = nullptr;
			}

			PartitionTree& operator=(const PartitionTree&) = delete;

			PartitionTree& operator=(PartitionTree&& other) {
				assert_ne(this,&other);

				// swap content and ownership
				std::swap(owned,other.owned);
				numReferences = other.numReferences;
				std::swap(data,other.data);
				std::swap(references,other.references);

				// done
				return *this;
			}

			bool isClosed() const {
				return references != nullptr;
			}

			void close() {
				// must not be closed for now
				assert_false(isClosed());

				// a utility to apply an operation on each mesh region
				auto forEachMeshRegion = [&](const auto& op) {
					for(std::size_t i=0; i<num_elements; ++i) {
						Node& cur = data[i];
						for(std::size_t l=0; l<Levels; ++l) {
							cur.data[l].forwardClosure .forEach(op);
							cur.data[l].backwardClosure.forEach(op);
							cur.data[l].parentClosure  .forEach(op);
							cur.data[l].childClosure   .forEach(op);
						}
					}
				};

				// count number of references required for all ranges
				numReferences = 0;
				forEachMeshRegion([&](const RegionStore& cur) {
					if (!cur.region) return;
					numReferences += cur.region->getSubMeshReferences().size();
				});

				// create reference buffer
				references = static_cast<SubMeshRef*>(malloc(sizeof(SubMeshRef) * numReferences));
				if (!references) {
					throw "Unable to allocate memory for managing references!";
				}

				// transfer ownership of SubMeshRefs to reference buffer
				std::size_t offset = 0;
				forEachMeshRegion([&](RegionStore& cur){

					// check whether there is a region
					if (!cur.region) {
						cur.offset = 0;
						cur.length = 0;
						return;
					}

					// close the region
					const auto& refs = cur.region->getSubMeshReferences();
					cur.offset = offset;
					cur.length = refs.size();
					for(auto& cur : refs) {
						// placement new for this reference
						new (&references[offset++]) SubMeshRef(cur);
					}

					// delete old region
					delete cur.region;
					cur.region = nullptr;
				});

				// make sure counting and transferring covered the same number of references
				assert_eq(numReferences, offset);
			}

			template<typename Kind, unsigned Level = 0>
			NodeRange<Kind,Level> getNodeRange(const SubTreeRef& ref) const {
				assert_lt(ref.getIndex(),num_elements);
				auto range = data[ref.getIndex()].data[Level].nodeRanges.template get<Kind>();
				return {
					NodeRef<Kind,Level>{ range.begin },
					NodeRef<Kind,Level>{ range.end }
				};
			}

			template<typename Kind, unsigned Level = 0>
			void setNodeRange(const SubTreeRef& ref, const NodeRange<Kind,Level>& range) {
				auto& locRange = getNode(ref).data[Level].nodeRanges.template get<Kind>();
				locRange.begin = range.getBegin().id;
				locRange.end = range.getEnd().id;
			}

			template<typename EdgeKind, unsigned Level = 0>
			MeshRegion getForwardClosure(const SubTreeRef& ref) const {
				return getNode(ref).data[Level].forwardClosure.template get<EdgeKind>().toRegion(references);
			}

			template<typename EdgeKind, unsigned Level = 0>
			void setForwardClosure(const SubTreeRef& ref, const MeshRegion& region) {
				getNode(ref).data[Level].forwardClosure.template get<EdgeKind>() = region;
			}

			template<typename EdgeKind, unsigned Level = 0>
			MeshRegion getBackwardClosure(const SubTreeRef& ref) const {
				return getNode(ref).data[Level].backwardClosure.template get<EdgeKind>().toRegion(references);
			}

			template<typename EdgeKind, unsigned Level = 0>
			void setBackwardClosure(const SubTreeRef& ref, const MeshRegion& region) {
				getNode(ref).data[Level].backwardClosure.template get<EdgeKind>() = region;
			}

			template<typename HierarchyKind, unsigned Level = 0>
			MeshRegion getParentClosure(const SubTreeRef& ref) const {
				return getNode(ref).data[Level].parentClosure.template get<HierarchyKind>().toRegion(references);
			}

			template<typename HierarchyKind, unsigned Level = 0>
			void setParentClosure(const SubTreeRef& ref, const MeshRegion& region) {
				getNode(ref).data[Level].parentClosure.template get<HierarchyKind>() = region;
			}


			template<typename HierarchyKind, unsigned Level = 1>
			MeshRegion getChildClosure(const SubTreeRef& ref) const {
				return getNode(ref).data[Level].childClosure.template get<HierarchyKind>().toRegion(references);
			}

			template<typename HierarchyKind, unsigned Level = 1>
			void setChildClosure(const SubTreeRef& ref, const MeshRegion& region) {
				getNode(ref).data[Level].childClosure.template get<HierarchyKind>() = region;
			}


			template<typename Body>
			void visitPreOrder(const Body& body) {
				SubTreeRef::root().enumerate<depth,true>(body);
			}

			template<typename Body>
			void visitPostOrder(const Body& body) {
				SubTreeRef::root().enumerate<depth,false>(body);
			}

			// -- serialization support for network transferes --

			void save(utils::Archive&) const {
				assert_not_implemented();
			}

			static PartitionTree load(utils::Archive&) {
				assert_not_implemented();
				return PartitionTree();
			}

			// -- load / store for files --

			void store(std::ostream& out) const {

				// start by writing out number of references
				out.write(reinterpret_cast<const char*>(&numReferences),sizeof(numReferences));

				// continue with node information
				out.write(reinterpret_cast<const char*>(data),sizeof(Node)*num_elements);

				// and end with references
				out.write(reinterpret_cast<const char*>(references),sizeof(SubMeshRef)*numReferences);

			}

			static PartitionTree load(std::istream& in) {

				// create the resulting tree (owning all its data)
				PartitionTree res;

				// read in number of references
				in.read(reinterpret_cast<char*>(&res.numReferences),sizeof(res.numReferences));

				// load nodes
				in.read(reinterpret_cast<char*>(res.data),sizeof(Node)*num_elements);

				// load references
				res.references = reinterpret_cast<SubMeshRef*>(malloc(sizeof(SubMeshRef)*res.numReferences));
				in.read(reinterpret_cast<char*>(res.references),sizeof(SubMeshRef)*res.numReferences);

				// done
				return res;
			}

			static PartitionTree interpret(utils::RawBuffer& raw) {

				// get size
				std::size_t numReferences = raw.consume<std::size_t>();

				// get nodes
				Node* nodes = raw.consumeArray<Node>(num_elements);

				// get references
				SubMeshRef* references = raw.consumeArray<SubMeshRef>(numReferences);

				// wrap up results
				return PartitionTree(nodes,numReferences,references);
			}


		private:

			const Node& getNode(const SubTreeRef& ref) const {
				assert_lt(ref.getIndex(),num_elements);
				return data[ref.getIndex()];
			}

			Node& getNode(const SubTreeRef& ref) {
				assert_lt(ref.getIndex(),num_elements);
				return data[ref.getIndex()];
			}

		};


		class NaiveMeshPartitioner {

		public:

			template<
				unsigned PartitionDepth,
				typename Nodes,
				typename Edges,
				typename Hierarchies,
				unsigned Levels
			>
			PartitionTree<Nodes,Edges,Hierarchies,Levels,PartitionDepth> partition(const MeshTopologyData<Nodes,Edges,Hierarchies,Levels>& data) const {

				// create empty partition tree
				PartitionTree<Nodes,Edges,Hierarchies,Levels,PartitionDepth> res;

				// set up node ranges for partitions
				data.forAllNodeKinds([&](const auto& nodeKind, const auto& level) {

						// get node kind and level
						using NodeKind = plain_type<decltype(nodeKind)>;
						const unsigned lvl = get_level<decltype(level)>::value;


						// set root node to cover the full range
						auto num_nodes = data.template getNumNodes<NodeKind,lvl>();
						res.template setNodeRange<NodeKind,lvl>(
								SubTreeRef::root(),
								NodeRange<NodeKind,lvl>(
									NodeRef<NodeKind,lvl>{ 0 },
									NodeRef<NodeKind,lvl>{ num_nodes }
								)
						);

						// recursively sub-divide ranges
						res.visitPreOrder([&](const SubTreeRef& ref) {

							if (ref.isRoot()) return;

							// get the range of the parent
							auto range = res.template getNodeRange<NodeKind,lvl>(ref.getParent());

							// extract begin / end
							auto begin = range.getBegin();
							auto end = range.getEnd();

							// compute mid
							auto mid = begin.id + (end.id - begin.id) / 2;

							// get range for this node
							if (ref.isLeftChild()) {
								range = NodeRange<NodeKind,lvl>(begin,mid);
							} else {
								range = NodeRange<NodeKind,lvl>(mid,end);
							}

							// update the range
							res.template setNodeRange<NodeKind,lvl>(ref,range);

						});

				});

				// set up closures for edges
				data.forAllEdgeKinds([&](const auto& edgeKind, const auto& level) {

					// get edge kind and level
					using EdgeKind = plain_type<decltype(edgeKind)>;
					const unsigned lvl = get_level<decltype(level)>::value;

					// the closure is everything for now
					MeshRegion closure = SubMeshRef::root();

					// initialize all the closured with the full region
					res.visitPreOrder([&](const SubTreeRef& ref) {
						// fix forward closure
						res.template setForwardClosure<EdgeKind,lvl>(ref,closure);

						// fix backward closure
						res.template setBackwardClosure<EdgeKind,lvl>(ref,closure);
					});

				});


				// set up closures for hierarchies
				data.forAllHierarchyKinds([&](const auto& hierarchyKind, const auto& level) {

					// get hierarchy kind and level
					using HierarchyKind = plain_type<decltype(hierarchyKind)>;
					const unsigned lvl = get_level<decltype(level)>::value;

					// make sure this is not called for level 0
					assert_gt(lvl,0) << "There should not be any hierarchies on level 0.";

					// the closure is everything for now
					MeshRegion closure = SubMeshRef::root();

					// initialize all the closured with the full region
					res.visitPreOrder([&](const SubTreeRef& ref) {

						// fix parent closure
						res.template setParentClosure<HierarchyKind,lvl-1>(ref,closure);

						// fix child closure
						res.template setChildClosure<HierarchyKind,lvl>(ref,closure);
					});

				});

				// close the data representation
				res.close();

				// done
				return res;
			}

		};


		template<
			typename NodeKind,
			typename ElementType,
			unsigned Level,
			typename PartitionTree
		>
		class MeshDataFragment {
		public:

			using facade_type = MeshData<NodeKind,ElementType,Level,PartitionTree>;
			using region_type = MeshRegion;
			using shared_data_type = PartitionTree;

		private:

			using partition_tree_type = PartitionTree;

			const partition_tree_type& partitionTree;

			region_type coveredRegion;

			std::vector<ElementType> data;

			friend facade_type;

		public:

			MeshDataFragment() = delete;

			MeshDataFragment(const partition_tree_type& ptree, const region_type& region)
				: partitionTree(ptree), coveredRegion(region) {

				// get upper boundary of covered node ranges
				std::size_t max = 0;
				region.scan([&](const SubTreeRef& cur){
					max = std::max<std::size_t>(max,ptree.template getNodeRange<NodeKind,Level>(cur).getEnd().id);
				});

				// resize data storage
				data.resize(max);

			}

			MeshDataFragment(const MeshDataFragment&) = delete;
			MeshDataFragment(MeshDataFragment&&) = default;

			MeshDataFragment& operator=(const MeshDataFragment&) = delete;
			MeshDataFragment& operator=(MeshDataFragment&&) = default;


			facade_type mask() {
				return facade_type(*this);
			}

			const region_type& getCoveredRegion() const {
				return coveredRegion;
			}

			const ElementType& operator[](const NodeRef<NodeKind,Level>& id) const {
				return data[id.getOrdinal()];
			}

			ElementType& operator[](const NodeRef<NodeKind,Level>& id) {
				return data[id.getOrdinal()];
			}

			std::size_t size() const {
				return data.size();
			}

			void resize(const region_type&) {

			}

			void insert(const MeshDataFragment& other, const region_type& area) {
				assert_true(core::isSubRegion(area,other.coveredRegion)) << "New data " << area << " not covered by source of size " << coveredRegion << "\n";
				assert_true(core::isSubRegion(area,coveredRegion))       << "New data " << area << " not covered by target of size " << coveredRegion << "\n";

				assert_not_implemented();
				std::cout << core::isSubRegion(area,other.coveredRegion);

//				// copy data line by line using memcpy
//				area.scanByLines([&](const point& a, const point& b){
//					auto start = flatten(a);
//					auto length = (flatten(b) - start) * sizeof(T);
//					std::memcpy(&data[start],&other.data[start],length);
//				});
			}

			void save(utils::Archive&, const region_type&) const {
				assert_not_implemented();
			}

			void load(utils::Archive&) {
				assert_not_implemented();
			}
		};


		/**
		 * An entity to reference the full range of a scan. This token
		 * can not be copied and will wait for the completion of the scan upon destruction.
		 */
		class scan_reference {

			core::treeture<void> handle;

		public:

			scan_reference(core::treeture<void>&& handle)
				: handle(std::move(handle)) {}

			scan_reference() {};
			scan_reference(const scan_reference&) = delete;
			scan_reference(scan_reference&&) = default;

			scan_reference& operator=(const scan_reference&) = delete;
			scan_reference& operator=(scan_reference&&) = default;

			~scan_reference() { handle.wait(); }

		};

	} // end namespace detail

	template<
		typename NodeKind,
		typename ElementType,
		unsigned Level,
		typename PartitionTree
	>
	class MeshData : public core::data_item<detail::MeshDataFragment<NodeKind,ElementType,Level,PartitionTree>> {

		template<typename NodeKinds,typename EdgeKinds,typename Hierarchies,unsigned Levels,unsigned PartitionDepth>
		friend class Mesh;

	public:

		using fragment_type = detail::MeshDataFragment<NodeKind,ElementType,Level,PartitionTree>;

	private:

		std::unique_ptr<fragment_type> owned;

		fragment_type* data;


		friend fragment_type;

		MeshData(fragment_type& data) : data(&data) {}

		MeshData(const PartitionTree& ptree, const detail::MeshRegion& region)
			: owned(std::make_unique<fragment_type>(ptree,region)), data(owned.get()) {}

	public:

		const ElementType& operator[](const NodeRef<NodeKind,Level>& id) const {
			return (*data)[id.getOrdinal()];
		}

		ElementType& operator[](const NodeRef<NodeKind,Level>& id) {
			return (*data)[id.getOrdinal()];
		}

		std::size_t size() const {
			return (*data).size();
		}

	};


	/**
	 * The default implementation of a mesh is capturing all ill-formed parameterizations
	 * of the mesh type to provide cleaner compiler errors.
	 */
	template<
		typename Nodes,
		typename Edges,
		typename Hierarchies,
		unsigned Levels,
		unsigned PartitionDepth
	>
	class Mesh {

		static_assert(detail::is_nodes<Nodes>::value,
				"First template argument of Mesh must be of type nodes<...>");

		static_assert(detail::is_edges<Edges>::value,
				"Second template argument of Mesh must be of type edges<...>");

		static_assert(detail::is_hierarchies<Hierarchies>::value,
				"Third template argument of Mesh must be of type hierarchies<...>");

	};


	/**
	 * The type for representing the topological information of a hierarchical mesh.
	 */
	template<
		typename ... NodeKinds,
		typename ... EdgeKinds,
		typename ... Hierarchies,
		unsigned Levels,
		unsigned PartitionDepth
	>
	class Mesh<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels,PartitionDepth> {

		static_assert(Levels > 0, "There must be at least one level!");

	public:

		using topology_type = detail::MeshTopologyData<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels>;

		using partition_tree_type = detail::PartitionTree<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels,PartitionDepth>;

		using builder_type = MeshBuilder<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels>;

		friend builder_type;

		enum { levels = Levels };

	private:

//		static_assert(std::is_trivially_move_assignable<topology_type>::value, "Topology should be trivially copyable!");

		partition_tree_type partitionTree;

		topology_type data;

		Mesh(topology_type&& data, partition_tree_type&& partitionTree)
			: partitionTree(std::move(partitionTree)), data(std::move(data)) {
			assert_true(data.isClosed());
		}

	public:

		// -- ctors / dtors / assignments --

		Mesh(const Mesh&) = delete;
		Mesh(Mesh&&) = default;

		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&) = default;


		// -- provide access to components --

		const topology_type& getTopologyData() const {
			return data;
		}

		const partition_tree_type& getPartitionTree() const {
			return partitionTree;
		}

		// -- mesh querying --

		template<typename Kind,unsigned Level = 0>
		std::size_t getNumNodes() const {
			return data.template getNumNodes<Kind,Level>();
		}

		// -- mesh interactions --

		template<
			typename EdgeKind,
			typename A,
			unsigned Level,
			typename B = typename EdgeKind::trg_node_kind
		>
		NodeRef<B,Level> getNeighbor(const NodeRef<A,Level>& a) const {
			const auto& set = getNeighbors<EdgeKind>(a);
			assert_eq(set.size(),1);
			return set.front();
		}

		template<
			typename EdgeKind,
			typename A,
			unsigned Level,
			typename B = typename EdgeKind::trg_node_kind
		>
		NodeList<B,Level> getNeighbors(const NodeRef<A,Level>& a) const {
			return getSinks<EdgeKind,A,Level,B>(a);
		}

		template<
			typename EdgeKind,
			typename A,
			unsigned Level,
			typename B = typename EdgeKind::trg_node_kind
		>
		NodeList<B,Level> getSinks(const NodeRef<A,Level>& a) const {
			return data.template getEdges<Level>().template get<EdgeKind>().getSinks(a);
		}

		template<
			typename EdgeKind,
			typename B,
			unsigned Level,
			typename A = typename EdgeKind::src_node_kind
		>
		NodeList<A,Level> getSources(const NodeRef<B,Level>& b) const {
			return data.template getEdges<Level>().template get<EdgeKind>().getSources(b);
		}

		template<
			typename Hierarchy,
			typename A, unsigned Level,
			typename B = typename Hierarchy::child_node_kind
		>
		NodeList<B,Level-1> getChildren(const NodeRef<A,Level>& a) const {
			return data.template getHierarchies<Level>().template get<Hierarchy>().getChildren(a);
		}

		template<
			typename Hierarchy,
			typename A, unsigned Level,
			typename B = typename Hierarchy::parent_node_kind
		>
		NodeRef<B,Level+1> getParent(const NodeRef<A,Level>& a) const {
			return data.template getHierarchies<Level+1>().template get<Hierarchy>().getParent(a);
		}

		template<typename Kind, unsigned Level = 0, typename Body>
		detail::scan_reference pforAll(const Body& body) const {

			using range = detail::SubTreeRef;

			return core::prec(
				// -- base case test --
				[](const range& a){
					// when we reached a leaf, we are at the bottom
					return a.getDepth() == PartitionDepth;
				},
				// -- base case --
				[&](const range& a){
					// apply the body to the elements of the current range
					for(const auto& cur : partitionTree.template getNodeRange<Kind,Level>(a)) {
						body(cur);
					}
				},
				// -- step case --
				core::pick(
					// -- split --
					[](const range& a, const auto& rec){
						return core::parallel(
							rec(a.getLeftChild()),
							rec(a.getRightChild())
						);
					},
					// -- serialized step case (optimization) --
					[&](const range& a, const auto&){
						// apply the body to the elements of the current range
						for(const auto& cur : partitionTree.template getNodeRange<Kind,Level>(a)) {
							body(cur);
						}
					}
				)
			)(detail::SubTreeRef::root());
		}

		// -- graph data --

		template<typename NodeKind, typename T, unsigned Level = 0>
		MeshData<NodeKind,T,Level,partition_tree_type> createNodeData() const {
			return MeshData<NodeKind,T,Level,partition_tree_type>(partitionTree,detail::SubMeshRef::root());
		}


		// -- load / store for files --

		void store(std::ostream& out) const {

			// write partition tree
			partitionTree.store(out);

			// write topological data
			data.store(out);

		}

		static Mesh load(std::istream& in) {

			// interpret the partition tree
			auto partitionTree = partition_tree_type::load(in);

			// load topological data
			auto topologyData = topology_type::load(in);

			// create result
			return Mesh(
				std::move(topologyData),
				std::move(partitionTree)
			);

		}

		static Mesh interpret(utils::RawBuffer& raw) {

			// interpret the partition tree
			auto partitionTree = partition_tree_type::interpret(raw);

			// load topological data
			auto topologyData = topology_type::interpret(raw);

			// create result
			return Mesh(
				std::move(topologyData),
				std::move(partitionTree)
			);

		}

	};



	/**
	 * The default implementation of a mesh build is capturing all ill-formed parameterizations
	 * of the mesh builder type to provide cleaner compiler errors.
	 */
	template<
		typename Nodes,
		typename Edges,
		typename Hierarchies,
		unsigned layers
	>
	class MeshBuilder {

		static_assert(detail::is_nodes<Nodes>::value,
				"First template argument of MeshBuilder must be of type nodes<...>");

		static_assert(detail::is_edges<Edges>::value,
				"Second template argument of MeshBuilder must be of type edges<...>");

		static_assert(detail::is_hierarchies<Hierarchies>::value,
				"Third template argument of MeshBuilder must be of type hierarchies<...>");

	};

	/**
	 * A utility to construct meshes.
	 */
	template<
		typename ... NodeKinds,
		typename ... EdgeKinds,
		typename ... Hierarchies,
		unsigned Levels
	>
	class MeshBuilder<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels> {

		static_assert(Levels > 0, "There must be at least one level!");

	public:

		template<unsigned PartitionDepth>
		using mesh_type = Mesh<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels,PartitionDepth>;

		using topology_type = detail::MeshTopologyData<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels>;

	private:

		topology_type data;

	public:

		// -- mesh modeling --

		template<typename Kind,unsigned Level = 0>
		NodeRef<Kind,Level> create() {
			// TODO: check that Kind is a valid node kind
			static_assert(Level < Levels, "Trying to create a node on invalid level.");
			return data.template getNodes<Level>().template get<Kind>().create();
		}

		template<typename Kind,unsigned Level = 0>
		NodeRange<Kind,Level> create(unsigned num) {
			// TODO: check that Kind is a valid node kind
			static_assert(Level < Levels, "Trying to create a node on invalid level.");
			return data.template getNodes<Level>().template get<Kind>().create(num);
		}

		template<typename EdgeKind, typename NodeKindA, typename NodeKindB, unsigned Level>
		void link(const NodeRef<NodeKindA,Level>& a, const NodeRef<NodeKindB,Level>& b) {
			// TODO: check that EdgeKind is a valid edge kind
			static_assert(Level < Levels, "Trying to create an edge on invalid level.");
			static_assert(std::is_same<NodeKindA,typename EdgeKind::src_node_kind>::value, "Invalid source node type");
			static_assert(std::is_same<NodeKindB,typename EdgeKind::trg_node_kind>::value, "Invalid target node type");
			return data.template getEdges<Level>().template get<EdgeKind>().addEdge(a,b);
		}

		template<typename HierarchyKind, typename NodeKindA, typename NodeKindB, unsigned LevelA, unsigned LevelB>
		void link(const NodeRef<NodeKindA,LevelA>& parent, const NodeRef<NodeKindB,LevelB>& child) {
			// TODO: check that HierarchyKind is a valid hierarchy kind
			static_assert(LevelA == LevelB+1, "Can not connect nodes of non-adjacent levels in hierarchies");
			static_assert(LevelA < Levels, "Trying to create a hierarchical edge to an invalid level.");
			static_assert(std::is_same<NodeKindA,typename HierarchyKind::parent_node_kind>::value, "Invalid source node type");
			static_assert(std::is_same<NodeKindB,typename HierarchyKind::child_node_kind>::value, "Invalid target node type");
			return data.template getHierarchies<LevelA>().template get<HierarchyKind>().addChild(parent,child);
		}

		// -- build mesh --

		template<typename Partitioner, unsigned PartitionDepth = 0>
		mesh_type<PartitionDepth> build(const Partitioner& partitioner) const & {

			// close the topological data
			topology_type meshData = data;
			meshData.close();

			// partition the mesh
			auto partitionTree = partitioner.template partition<PartitionDepth>(meshData);

			return mesh_type<PartitionDepth>(std::move(meshData), std::move(partitionTree));
		}

		template<unsigned PartitionDepth = 0>
		mesh_type<PartitionDepth> build() const & {
			return build<detail::NaiveMeshPartitioner,PartitionDepth>(detail::NaiveMeshPartitioner());
		}


		template<typename Partitioner, unsigned PartitionDepth = 0>
		mesh_type<PartitionDepth> build(const Partitioner& partitioner) && {

			// partition the mesh
			auto partitionTree = partitioner.template partition<PartitionDepth>(data);

			return mesh_type<PartitionDepth>(std::move(data), std::move(partitionTree));
		}

		template<unsigned PartitionDepth = 0>
		mesh_type<PartitionDepth> build() const && {
			return std::move(*this).template build<detail::NaiveMeshPartitioner,PartitionDepth>(detail::NaiveMeshPartitioner());
		}

	};


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
