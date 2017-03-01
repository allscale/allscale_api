#pragma once

#include <algorithm>
#include <iterator>
#include <ostream>

#include "allscale/utils/assert.h"
#include "allscale/utils/range.h"
#include "allscale/utils/static_map.h"

#include "allscale/utils/printer/vectors.h"

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
		unsigned Levels = 1						// < number of levels in the hierarchy
	>
	class Mesh;


	/**
	 * The type for associating (dynamic) information to nodes within a mesh.
	 */
	template<
		typename NodeKind,			// < the type of node to be annotated
		typename ElementType,		// < the type of value to be associated to each node on the given level
		unsigned Level = 0			// < the level of the mesh to be annotated
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


	template<
		typename NodeKind,
		typename ElementType,
		unsigned Level
	>
	class MeshData {

		template<typename NodeKinds,typename EdgeKinds,typename Hierarchies,unsigned Levels>
		friend class Mesh;

		std::vector<ElementType> data;

		MeshData(std::size_t size) : data(size) {}

	public:

		MeshData() = delete;

		MeshData(const MeshData&) = delete;
		MeshData(MeshData&&) = default;

		MeshData& operator=(const MeshData&) = delete;
		MeshData& operator=(MeshData&&) = default;

		const ElementType& operator[](const NodeRef<NodeKind,Level>& id) const {
			return data[id.getOrdinal()];
		}

		ElementType& operator[](const NodeRef<NodeKind,Level>& id) {
			return data[id.getOrdinal()];
		}

		std::size_t size() const {
			return data.size();
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

		template<typename Element>
		void sumPrefixes(std::vector<Element>& list) {
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

			std::size_t numNodes() const {
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
			void forAllTypes(const Body& body) const {
				// call for this type
				body(First(), level<Level>());
				// and the nested types
				nested.forAllTypes(body);
			}
/*
			void store(std::ostream& out) const {
				// store the number of nodes
				write<std::size_t>(out, node_counter);

				// store the nested hierarchy
				nested.store(out);
			}

			static NodeSet load(std::istream& in) {

				// restore the number of nodes
				std::size_t node_counter = read<std::size_t>(in);

				// load nested
				auto nested = NodeSet<Level,Rest...>::load(in);

				// done
				return NodeSet(node_counter,std::move(nested));
			}
*/
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
			void forAllTypes(const Body&) const {
				// nothing to do
			}
/*
			void store(std::ostream& out) const {
				// nothing
			}

			static NodeSet load(std::istream& in) {
				return NodeSet();
			}
*/
		};


		template<unsigned Level, typename ... EdgeTypes>
		struct EdgeSet;

		template<unsigned Level, typename First, typename ... Rest>
		struct EdgeSet<Level,First,Rest...> {

			using Src = typename First::src_node_kind;
			using Trg = typename First::trg_node_kind;

			std::vector<uint32_t> forward_offsets;
			std::vector<NodeRef<Trg,Level>> forward_targets;

			std::vector<uint32_t> backward_offsets;
			std::vector<NodeRef<Src,Level>> backward_targets;

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
				forward_offsets.resize(data.template numNodes<Src,Level>() + 1, 0);
				forward_targets.resize(edges.size());

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
				forward_pos.clear();

				// clear edges
				edges.clear();

				// also fill in backward edges
				restoreBackward(data.template numNodes<Trg,Level>() + 1);

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
			void forAllTypes(const Body& body) const {
				// visit all links for this type
				body(First(), level<Level>());
				// visit links of remaining hierarchies
				nested.forAllTypes(body);
			}
/*
			void store(std::ostream& out) const {
				// store this edge data (forward only)
				write<std::size_t>(out, forward_offsets.size());
				write(out,forward_offsets.begin(), forward_offsets.end());
				write<std::size_t>(out, forward_targets.size());
				write(out,forward_targets.begin(), forward_targets.end());

				// also store sizes of backward edges (simplicity)
				write<std::size_t>(out, backward_offsets.size());

				// store the nested hierarchy
				nested.store(out);
			}

			static EdgeSet load(std::istream& in) {

				// restore this edge set
				EdgeSet res;

				// load forward edges data
				std::size_t offsets = read<std::size_t>(in);
				res.forward_offsets.resize(offsets);
				read(in,res.forward_offsets.begin(), res.forward_offsets.end());

				std::size_t targets = read<std::size_t>(in);
				res.forward_targets.resize(targets);
				read(in,res.forward_targets.begin(), res.forward_targets.end());

				// load backward edges data
				offsets = read<std::size_t>(in);
				res.restoreBackward(offsets);

				// load nested
				res.nested = EdgeSet<Level,Rest...>::load(in);

				// done
				return res;
			}
*/
		private:

			void restoreBackward(std::size_t numTargetNodes) {

				// fix sizes of backward vectors
				backward_offsets.resize(numTargetNodes, 0);
				backward_targets.resize(forward_targets.size());		// the same length as forward

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
			void forAllTypes(const Body&) const {
				// nothing to do
			}

			template<typename MeshData>
			void close(const MeshData&) {
				// nothing to do
			}

			bool isClosed() const {
				return true;
			}

			void store(std::ostream& out) const {
				// nothing
			}

			static EdgeSet load(std::istream& in) {
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

			// -- efficient simulation structures --

			std::vector<NodeRef<Src,Level>> parents;

			std::vector<uint32_t> children_offsets;
			std::vector<NodeRef<Trg,Level-1>> children_targets;

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
				auto numParents = data.template numNodes<Src,Level>();
				children_offsets.resize(numParents + 1, 0);
				children_targets.resize(numParentChildLinks);

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

				// cut parents list down to actual length
				parents.resize(data.template numNodes<Trg,Level-1>(), unknownParent);

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
				assert_lt(child.id,parents.size());
				return parents[child.id];
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
			void forAllTypes(const Body& body) const {
				// visit all links for this type
				body(First(), level<Level>());
				// visit links of remaining hierarchies
				nested.forAllTypes(body);
			}
/*
			void store(std::ostream& out) const {
				// store this hierarchy data
				write<std::size_t>(out, children.size());
				for(const auto& cur : children) {
					write<std::size_t>(out, cur.size());
					for(const auto& n : cur) {
						write<std::size_t>(out, n.id);
					}
				}

				// store the nested hierarchy
				nested.store(out);
			}

			static HierarchySet load(std::istream& in) {
				// restore this hierarchy
				HierarchySet res;

				// load hierarchy data
				std::size_t size = read<std::size_t>(in);
				for(std::size_t i=0; i<size; i++) {
					NodeRef<Src,Level> parent{i};
					std::size_t cur_size = read<std::size_t>(in);
					for(std::size_t j=0; j<cur_size; j++) {
						NodeRef<Trg,Level-1> child{ read<std::size_t>(in) };
						res.addChild(parent,child);
					}
				}

				// load nested
				res.nested = HierarchySet<Level,Rest...>::load(in);

				// done
				return res;
			}
*/
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
			void forAllTypes(const Body&) const {
				// nothing to do
			}
/*
			void store(std::ostream& out) const {
				// nothing
			}

			static HierarchySet load(std::istream& in) {
				return HierarchySet();
			}
*/
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
			void forAllNodeTypes(const Body& body) const {
				nodes.forAllTypes(body);
				nested.forAllNodeTypes(body);
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {
				edges.forAll(body);
				nested.forAllEdges(body);
			}

			template<typename Body>
			void forAllEdgeTypes(const Body& body) const {
				edges.forAllTypes(body);
				nested.forAllEdgeTypes(body);
			}

			template<typename Body>
			void forAllHierarchies(const Body& body) const {
				hierarchies.forAll(body);
				nested.forAllHierarchies(body);
			}

			template<typename Body>
			void forAllHierarchyTypes(const Body& body) const {
				hierarchies.forAllTypes(body);
				nested.forAllHierarchyTypes(body);
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

/*
			void store(std::ostream& out) const {
				nested.store(out);
				nodes.store(out);
				edges.store(out);
				hierarchies.store(out);
			}

			static Levels load(std::istream& in) {
				auto nested = LevelData<Level-1>::load(in);
				auto nodes = NodeSet<Level,Nodes...>::load(in);
				auto edges = EdgeSet<Level,Edges...>::load(in);
				auto hierarchies = HierarchySet<Level,Hierarchies...>::load(in);
				return Levels ( std::move(nested), std::move(nodes), std::move(edges), std::move(hierarchies) );
			}
*/
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
			void forAllNodeTypes(const Body& body) const {
				nodes.forAllTypes(body);
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {
				edges.forAll(body);
			}

			template<typename Body>
			void forAllEdgeTypes(const Body& body) const {
				edges.forAllTypes(body);
			}

			template<typename Body>
			void forAllHierarchies(const Body&) const {
				// nothing to do here
			}

			template<typename Body>
			void forAllHierarchyTypes(const Body&) const {
				// nothing to do here
			}

			template<typename MeshData>
			void close(const MeshData& data) {
				edges.close(data);
			}

			bool isClosed() const {
				return edges.isClosed();
			}

/*
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
*/
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

			using data_store = detail::Levels<Levels,nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>>;

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
			void forAllNodeTypes(const Body& body) const {
				data.forAllNodeTypes(body);
			}

			template<typename Body>
			void forAllEdges(const Body& body) const {
				data.forAllEdges(body);
			}

			template<typename Body>
			void forAllEdgeTypes(const Body& body) const {
				data.forAllEdgeTypes(body);
			}

			template<typename Body>
			void forAllHierarchies(const Body& body) const {
				data.forAllHierarchies(body);
			}

			template<typename Body>
			void forAllHierarchyTypes(const Body& body) const {
				data.forAllHierarchyTypes(body);
			}

			template<typename Kind,unsigned Level = 0>
			std::size_t numNodes() const {
				return getNodes<Level>().template get<Kind>().numNodes();
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
/*
			void store(std::ostream& out) const {
				// store nested data
				data.store(out);
			}

			static MeshTopologyData load(std::istream& in) {
				return MeshTopologyData { data_store::load(in) };
			}
*/
		};


		template<typename Kind, unsigned Level>
		class NodeRange {

			NodeRef<Kind,Level> begin;

			NodeRef<Kind,Level> end;

			template<typename Body>
			void forEach(const Body& body) {
				for(auto i = begin.id; i < end.id; ++i) {
					body(NodeRef<Kind,Level>(i));
				}
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

			value_t getDepth() const {
				if (PathRefBase::mask == 0) return 0;
				return sizeof(PathRefBase::mask) * 8 - __builtin_clz(PathRefBase::mask);
			}

			Derived getLeftChild() const {
				assert_lt(getDepth(),sizeof(PathRefBase::path)*8);
				Derived res = static_cast<const Derived&>(*this);
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
				auto commonMask = PathRefBase::mask & other.PathRefBase::mask;
				return (PathRefBase::path & commonMask) < (other.PathRefBase::path & commonMask);
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

			SubMeshRef mask(unsigned pos) const {
				assert_lt(pos,getDepth());
				SubMeshRef res = *this;
				res.super::mask = res.super::mask & ~(1<<pos);
				return res;
			}

			SubMeshRef unmask(unsigned pos) const {
				assert_lt(pos,getDepth());
				SubMeshRef res = *this;
				res.super::mask = res.super::mask | (1<<pos);
				return res;
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
				SubMeshRef copy = unmask(zeroPos);

				// set bit to 0
				copy.super::path = copy.super::path & ~( 1 << zeroPos );
				copy.scan(body);

				// set bit to 1
				copy.super::path = copy.super::path |  ( 1 << zeroPos );
				copy.scan(body);
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

			std::vector<SubMeshRef> refs;

		public:

			MeshRegion() {}

			MeshRegion(const SubMeshRef& ref) {
				refs.push_back(ref);
			}

			MeshRegion(std::initializer_list<SubMeshRef> meshRefs) : refs(meshRefs) {
				std::sort(refs.begin(),refs.end());
				auto newEnd = std::unique(refs.begin(),refs.end());
				refs.erase(newEnd,refs.end());
			}

			bool operator==(const MeshRegion& other) const {
				return refs == other.refs;
			}

			bool operator!=(const MeshRegion& other) const {
				return !(*this == other);
			}

			bool empty() const {
				return refs.empty();
			}

			static MeshRegion merge(const MeshRegion& a, const MeshRegion& b) {
				MeshRegion res;
				std::set_union(
					a.refs.begin(), a.refs.end(),
					b.refs.begin(), b.refs.end(),
					std::back_inserter(res.refs)
				);
				return res;
			}

			template<typename ... Rest>
			static MeshRegion merge(const MeshRegion& a, const MeshRegion& b, const Rest& ... rest) {
				return merge(merge(a,b),rest...);
			}

			static MeshRegion intersect(const MeshRegion& a, const MeshRegion& b) {
				MeshRegion res;
				std::set_intersection(
					a.refs.begin(), a.refs.end(),
					b.refs.begin(), b.refs.end(),
					std::back_inserter(res.refs)
				);
				return res;
			}

			static MeshRegion difference(const MeshRegion& a, const MeshRegion& b) {
				MeshRegion res;
				std::set_difference(
					a.refs.begin(), a.refs.end(),
					b.refs.begin(), b.refs.end(),
					std::back_inserter(res.refs)
				);
				return res;
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

			friend std::ostream& operator<<(std::ostream& out, const MeshRegion& reg) {
				return out << reg.refs;
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
			unsigned depth
		>
		class PartitionTree<nodes<Nodes...>,edges<Edges...>,hierarchies<Hierarchies...>,Levels,depth> {

			static_assert(Levels > 0, "There must be at least one level!");

			class LevelInfo {

				utils::StaticMap<utils::keys<Nodes...>,std::pair<NodeID,NodeID>> nodeRanges;

				utils::StaticMap<utils::keys<Edges...>,MeshRegion> forwardClosure;
				utils::StaticMap<utils::keys<Edges...>,MeshRegion> backwardClosure;

				utils::StaticMap<utils::keys<Hierarchies...>,MeshRegion> parentClosure;
				utils::StaticMap<utils::keys<Hierarchies...>,MeshRegion> childClosure;

			};

			class Node {

				std::array<LevelInfo,Levels> data;

			};

			enum { num_elements = 1ul << (depth + 1) };

			std::array<Node,num_elements> data;

		public:

			PartitionTree() {}

			// TODO: return references

			template<typename Kind, unsigned Level = 0>
			NodeRange<Kind,Level> getNodeRange(const SubTreeRef& ref) const {
				auto pair = data[ref.getIndex()].data[Level].nodeRanges.template get<Kind>();
				return {
					NodeRef<Kind,Level>{ pair.first },
					NodeRef<Kind,Level>{ pair.second }
				};
			}

			template<typename Kind, unsigned Level = 0>
			void setNodeRange(const SubTreeRef& ref, const NodeRange<Kind,Level>& range) {
				auto& pair = data[ref.getIndex()].data[Level].nodeRanges.template get<Kind>();
				pair.first = range.begin.id;
				pair.second = range.end.id;
			}

			template<typename EdgeKind, unsigned Level = 0>
			const MeshRegion& getForwardClosure(const SubTreeRef& ref) const {
				return data[ref.getIndex()].data[Level].forwardClosure.template get<EdgeKind>();
			}

			template<typename EdgeKind, unsigned Level = 0>
			void setForwardClosure(const SubTreeRef& ref, const MeshRegion& region) {
				data[ref.getIndex()].data[Level].forwardClosure.template get<EdgeKind>() = region;
			}

			template<typename EdgeKind, unsigned Level = 0>
			const MeshRegion& getBackwardClosure(const SubTreeRef& ref) const {
				return data[ref.getIndex()].data[Level].backwardClosure.template get<EdgeKind>();
			}

			template<typename EdgeKind, unsigned Level = 0>
			void setBackwardClosure(const SubTreeRef& ref, const MeshRegion& region) {
				data[ref.getIndex()].data[Level].backwardClosure.template get<EdgeKind>() = region;
			}

			template<typename HierarchyKind, unsigned Level = 0>
			const MeshRegion& getParentClosure(const SubTreeRef& ref) const {
				return data[ref.getIndex()].data[Level].parentClosure.template get<HierarchyKind>();
			}

			template<typename HierarchyKind, unsigned Level = 0>
			void setParentClosure(const SubTreeRef& ref, const MeshRegion& region) {
				data[ref.getIndex()].data[Level].parentClosure.template get<HierarchyKind>() = region;
			}


			template<typename HierarchyKind, unsigned Level = 1>
			const MeshRegion& getChildClosure(const SubTreeRef& ref) const {
				return data[ref.getIndex()].data[Level].childClosure.template get<HierarchyKind>();
			}

			template<typename HierarchyKind, unsigned Level = 1>
			void setChildClosure(const SubTreeRef& ref, const MeshRegion& region) {
				data[ref.getIndex()].data[Level].childClosure.template get<HierarchyKind>() = region;
			}
		};



	} // end namespace detail


	/**
	 * The default implementation of a mesh is capturing all ill-formed parameterizations
	 * of the mesh type to provide cleaner compiler errors.
	 */
	template<
		typename Nodes,
		typename Edges,
		typename Hierarchies,
		unsigned Levels
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
		unsigned Levels
	>
	class Mesh<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels> {

		static_assert(Levels > 0, "There must be at least one level!");

	public:

		using topology_type = detail::MeshTopologyData<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels>;

		using builder_type = MeshBuilder<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels>;

		friend builder_type;

		enum { levels = Levels };

	private:

		topology_type data;

		Mesh(topology_type&& data) : data(std::move(data)) {
			assert_true(data.isClosed());
		}

	public:

		// -- ctors / dtors / assignments --

		Mesh(const Mesh&) = delete;
		Mesh(Mesh&&) = default;

		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&) = default;


		// -- mesh querying --

		template<typename Kind,unsigned Level = 0>
		std::size_t numNodes() const {
			return data.template numNodes<Kind,Level>();
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

/*
		template<typename Kind, unsigned Level = 0, typename Body>
		void forAll(const Body& body) const {
			// run a loop over all nodes of the requested kind sequentially
			for(const auto& cur : node_set_kind<Kind,Level>::range(NodeRef<Kind,Level>{0},NodeRef<Kind,Level>{numNodes<Kind,Level>()})) {
				body(cur);
			}
		}

		template<typename Kind, unsigned Level = 0, typename Body>
		void pforAll(const Body& body) const {
			// run a loop over all nodes of the requested kind in parallel
			parec::pfor(node_set_type<Kind,Level>::range(NodeRef<Kind,Level>{0},NodeRef<Kind,Level>{numNodes<Kind,Level>()}), body);
		}

		template<typename Body>
		void forAllNodes(const Body& body) const {
			data.forAllNodes(body);
		}

		template<typename Body>
		void forAllNodeTypes(const Body& body) const {
			data.forAllNodeTypes(body);
		}

		template<typename Body>
		void forAllEdges(const Body& body) const {
			data.forAllEdges(body);
		}

		template<typename Body>
		void forAllEdgeTypes(const Body& body) const {
			data.forAllEdgeTypes(body);
		}

		template<typename Body>
		void forAllHierarchies(const Body& body) const {
			data.forAllHierarchies(body);
		}

		template<typename Body>
		void forAllHierarchyTypes(const Body& body) const {
			data.forAllHierarchyTypes(body);
		}

		template<typename Body>
		void forAllLinks(const Body& body) const {
			forAllEdges(body);
			forAllHierarchies(body);
		}

		template<typename Body>
		void forAllLinkedTypes(const Body& body) const {
			forAllEdgeTypes(body);
			forAllHierarchyTypes(body);
		}
*/
		// -- graph data --

		template<typename NodeKind, typename T, unsigned Level = 0>
		MeshData<NodeKind,T,Level> createNodeData() const {
			return MeshData<NodeKind,T,Level>(numNodes<NodeKind,Level>());
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

		using mesh_type = Mesh<nodes<NodeKinds...>,edges<EdgeKinds...>,hierarchies<Hierarchies...>,Levels>;

		using topology_type = typename mesh_type::topology_type;

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

//		template<typename Kind,unsigned Level = 0>
//		node_list_type<Kind,Level> create(unsigned num) {
//			return impl.template create<Kind,Level>(num);
//		}

		template<typename EdgeKind, typename NodeKindA, typename NodeKindB, unsigned Level>
		void link(const NodeRef<NodeKindA,Level>& a, const NodeRef<NodeKindB,Level>& b) {
			// TODO: check that EdgeKind is a valid edge kind
			static_assert(Level < Levels, "Trying to create an edge on invalid level.");
			static_assert(std::is_same<NodeKindA,typename EdgeKind::src_node_kind>::value, "Invalid source node type");
			static_assert(std::is_same<NodeKindB,typename EdgeKind::trg_node_kind>::value, "Invalid target node type");
			return data.template getEdges<Level>().template get<EdgeKind>().addEdge(a,b);
//			return impl.template link<EdgeKind>(a, b);
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

		mesh_type build() const & {
			topology_type copy = data;
			copy.close();
			return std::move(copy);
		}

		mesh_type build() && {
			data.close();
			return std::move(data);
		}
	};

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
