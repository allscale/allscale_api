#include <gtest/gtest.h>

#include "allscale/api/core/data.h"
#include "allscale/api/user/data/mesh.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	TEST(NodeRef, TypeProperties) {

		using node_type = NodeRef<int,4>;

		EXPECT_TRUE(std::is_default_constructible<node_type>::value);
		EXPECT_TRUE(std::is_trivially_default_constructible<node_type>::value);

		EXPECT_TRUE(std::is_copy_constructible<node_type>::value);
		EXPECT_TRUE(std::is_move_constructible<node_type>::value);

		EXPECT_TRUE(std::is_trivially_copy_constructible<node_type>::value);
		EXPECT_TRUE(std::is_trivially_move_constructible<node_type>::value);

		EXPECT_TRUE(std::is_trivially_copy_assignable<node_type>::value);
		EXPECT_TRUE(std::is_trivially_move_assignable<node_type>::value);

		EXPECT_TRUE(std::is_copy_assignable<node_type>::value);
		EXPECT_TRUE(std::is_move_assignable<node_type>::value);

		EXPECT_EQ(sizeof(node_type),sizeof(NodeID));
	}


	TEST(MeshData, TypeProperties) {

		using data = MeshData<int,int>;

		EXPECT_FALSE(std::is_default_constructible<data>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<data>::value);

		EXPECT_FALSE(std::is_trivially_copy_constructible<data>::value);
		EXPECT_FALSE(std::is_trivially_move_constructible<data>::value);

		EXPECT_FALSE(std::is_copy_constructible<data>::value);
		EXPECT_TRUE(std::is_move_constructible<data>::value);

		EXPECT_FALSE(std::is_trivially_copy_assignable<data>::value);
		EXPECT_FALSE(std::is_trivially_move_assignable<data>::value);

		EXPECT_FALSE(std::is_copy_assignable<data>::value);
		EXPECT_TRUE(std::is_move_assignable<data>::value);

	}

	TEST(Mesh, TypeProperties) {

		struct Cell {};
		struct Edge : public edge<Cell,Cell> {};

		using mesh_type = Mesh<nodes<Cell>,edges<Edge>>;


		EXPECT_FALSE(std::is_default_constructible<mesh_type>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<mesh_type>::value);

		EXPECT_FALSE(std::is_trivially_copy_constructible<mesh_type>::value);
		EXPECT_FALSE(std::is_trivially_move_constructible<mesh_type>::value);

		EXPECT_FALSE(std::is_copy_constructible<mesh_type>::value);
		EXPECT_TRUE(std::is_move_constructible<mesh_type>::value);

		EXPECT_FALSE(std::is_trivially_copy_assignable<mesh_type>::value);
		EXPECT_FALSE(std::is_trivially_move_assignable<mesh_type>::value);

		EXPECT_FALSE(std::is_copy_assignable<mesh_type>::value);
		EXPECT_TRUE(std::is_move_assignable<mesh_type>::value);

	}


	TEST(MeshBuilder, Basic) {

		struct Cell {};
		struct Edge : public edge<Cell,Cell> {};
		struct Tree : public hierarchy<Cell,Cell> {};

		MeshBuilder<nodes<Cell>,edges<Edge>,hierarchies<Tree>,2> builder;

		auto cell = builder.create<Cell>();
		builder.link<Edge>(cell,cell);

		auto root = builder.create<Cell,1>();
		builder.link<Tree>(root,cell);
	}


	TEST(MeshData, Basic) {

		struct Vertex {};
		struct Edge : public edge<Vertex,Vertex> {};

		MeshBuilder<nodes<Vertex>,edges<Edge>> builder;

		auto cell = builder.create<Vertex>();
		builder.link<Edge>(cell,cell);

		auto m = builder.build();

		auto store = m.createNodeData<Vertex,int>();

		EXPECT_EQ(1,store.size());

	}

	TEST(SubTreeRef, TypeProperties) {

		using namespace detail;

		EXPECT_FALSE(std::is_default_constructible<SubTreeRef>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<SubTreeRef>::value);

		EXPECT_TRUE(std::is_trivially_copy_constructible<SubTreeRef>::value);
		EXPECT_TRUE(std::is_trivially_move_constructible<SubTreeRef>::value);

		EXPECT_TRUE(std::is_copy_constructible<SubTreeRef>::value);
		EXPECT_TRUE(std::is_move_constructible<SubTreeRef>::value);

		EXPECT_TRUE(std::is_trivially_copy_assignable<SubTreeRef>::value);
		EXPECT_TRUE(std::is_trivially_move_assignable<SubTreeRef>::value);

		EXPECT_TRUE(std::is_copy_assignable<SubTreeRef>::value);
		EXPECT_TRUE(std::is_move_assignable<SubTreeRef>::value);

	}

	TEST(SubTreeRef, Depth) {

		using namespace detail;

		SubTreeRef r = SubTreeRef::root();

		EXPECT_EQ(0,r.getDepth());

		EXPECT_EQ(1,r.getLeftChild().getDepth());
		EXPECT_EQ(1,r.getRightChild().getDepth());

		EXPECT_EQ(2,r.getLeftChild().getLeftChild().getDepth());
		EXPECT_EQ(2,r.getRightChild().getRightChild().getDepth());
	}

	TEST(SubTreeRef, Index) {

		using namespace detail;

		SubTreeRef r = SubTreeRef::root();

		EXPECT_EQ(1,r.getIndex());

		EXPECT_EQ(2,r.getLeftChild().getIndex());
		EXPECT_EQ(3,r.getRightChild().getIndex());

		EXPECT_EQ(4,r.getLeftChild().getLeftChild().getIndex());
		EXPECT_EQ(5,r.getLeftChild().getRightChild().getIndex());
		EXPECT_EQ(6,r.getRightChild().getLeftChild().getIndex());
		EXPECT_EQ(7,r.getRightChild().getRightChild().getIndex());
	}


	TEST(SubTreeRef, Print) {

		using namespace detail;

		SubTreeRef r = SubTreeRef::root();

		EXPECT_EQ("r",toString(r));

		EXPECT_EQ("r.0",toString(r.getLeftChild()));
		EXPECT_EQ("r.1",toString(r.getRightChild()));

		EXPECT_EQ("r.1.0",toString(r.getRightChild().getLeftChild()));
		EXPECT_EQ("r.0.1",toString(r.getLeftChild().getRightChild()));

		EXPECT_EQ("r.1.0.1",toString(r.getRightChild().getLeftChild().getRightChild()));
		EXPECT_EQ("r.0.1.0",toString(r.getLeftChild().getRightChild().getLeftChild()));
	}



	TEST(SubMeshRef, TypeProperties) {

		using namespace detail;

		EXPECT_FALSE(std::is_default_constructible<SubMeshRef>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<SubMeshRef>::value);

		EXPECT_TRUE(std::is_trivially_copy_constructible<SubMeshRef>::value);
		EXPECT_TRUE(std::is_trivially_move_constructible<SubMeshRef>::value);

		EXPECT_TRUE(std::is_copy_constructible<SubMeshRef>::value);
		EXPECT_TRUE(std::is_move_constructible<SubMeshRef>::value);

		EXPECT_TRUE(std::is_trivially_copy_assignable<SubMeshRef>::value);
		EXPECT_TRUE(std::is_trivially_move_assignable<SubMeshRef>::value);

		EXPECT_TRUE(std::is_copy_assignable<SubMeshRef>::value);
		EXPECT_TRUE(std::is_move_assignable<SubMeshRef>::value);

	}

	TEST(SubMeshRef, Depth) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		EXPECT_EQ(0,r.getDepth());

		EXPECT_EQ(1,r.getLeftChild().getDepth());
		EXPECT_EQ(1,r.getRightChild().getDepth());

		EXPECT_EQ(2,r.getLeftChild().getLeftChild().getDepth());
		EXPECT_EQ(2,r.getRightChild().getRightChild().getDepth());


		EXPECT_EQ(2, r.getRightChild().getLeftChild().mask(0).getDepth());
		EXPECT_EQ(2, r.getLeftChild().getRightChild().mask(0).getDepth());

	}

	TEST(SubMeshRef, Scan) {

		using namespace detail;

		auto toList = [](const SubMeshRef& r) {
			std::vector<SubTreeRef> list;
			r.scan([&](const SubTreeRef& ref) {
				list.push_back(ref);
			});
			return list;
		};

		SubMeshRef r = SubMeshRef::root();
		EXPECT_EQ("[r]",toString(toList(r)));

		r = r.getLeftChild().getRightChild().getLeftChild();
		EXPECT_EQ( "r.0.1.0",toString(r));
		EXPECT_EQ("[r.0.1.0]",toString(toList(r)));


		r = r.mask(1);
		EXPECT_EQ( "r.0.*.0",toString(r));
		EXPECT_EQ("[r.0.0.0,r.0.1.0]",toString(toList(r)));

		r = r.mask(0);
		EXPECT_EQ( "r.*.*.0",toString(r));
		EXPECT_EQ("[r.0.0.0,r.0.1.0,r.1.0.0,r.1.1.0]",toString(toList(r)));


		r = r.mask(2);
		EXPECT_EQ( "r",toString(r));
		EXPECT_EQ("[r]",toString(toList(r)));

	}


	TEST(SubMeshRef, Print) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		EXPECT_EQ("r",toString(r));

		EXPECT_EQ("r.0",toString(r.getLeftChild()));
		EXPECT_EQ("r.1",toString(r.getRightChild()));

		EXPECT_EQ("r.1.0",toString(r.getRightChild().getLeftChild()));
		EXPECT_EQ("r.0.1",toString(r.getLeftChild().getRightChild()));

		EXPECT_EQ("r.1.0.1",toString(r.getRightChild().getLeftChild().getRightChild()));
		EXPECT_EQ("r.0.1.0",toString(r.getLeftChild().getRightChild().getLeftChild()));

		EXPECT_EQ("r.*.0",toString(r.getRightChild().getLeftChild().mask(0)));
		EXPECT_EQ("r.*.1",toString(r.getLeftChild().getRightChild().mask(0)));

	}




	TEST(MeshRegion, TypeProperties) {

		using namespace detail;

		EXPECT_TRUE(std::is_default_constructible<MeshRegion>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<MeshRegion>::value);

		EXPECT_FALSE(std::is_trivially_copy_constructible<MeshRegion>::value);
		EXPECT_FALSE(std::is_trivially_move_constructible<MeshRegion>::value);

		EXPECT_TRUE(std::is_copy_constructible<MeshRegion>::value);
		EXPECT_TRUE(std::is_move_constructible<MeshRegion>::value);

		EXPECT_FALSE(std::is_trivially_copy_assignable<MeshRegion>::value);
		EXPECT_FALSE(std::is_trivially_move_assignable<MeshRegion>::value);

		EXPECT_TRUE(std::is_copy_assignable<MeshRegion>::value);
		EXPECT_TRUE(std::is_move_assignable<MeshRegion>::value);

		// check that it qualifies as a region
		EXPECT_TRUE(allscale::api::core::is_region<MeshRegion>::value);

	}

	TEST(MeshRegion, Print) {

		using namespace detail;

		MeshRegion e;
		EXPECT_EQ("[]",toString(e));

		MeshRegion sl = SubMeshRef::root().getLeftChild();
		EXPECT_EQ("[r.0]",toString(sl));

		MeshRegion sr = SubMeshRef::root().getRightChild();
		EXPECT_EQ("[r.1]",toString(sr));

		MeshRegion s2 = MeshRegion::merge(sl,sr);
		EXPECT_EQ("[r.0,r.1]",toString(s2));
	}

	TEST(MeshRegion, SetOps) {

		using namespace detail;

		MeshRegion e;
		EXPECT_EQ("[]",toString(e));

		MeshRegion sl = SubMeshRef::root().getLeftChild();
		EXPECT_EQ("[r.0]",toString(sl));

		MeshRegion sr = SubMeshRef::root().getRightChild();
		EXPECT_EQ("[r.1]",toString(sr));

		MeshRegion s2 = MeshRegion::merge(sl,sr);
		EXPECT_EQ("[r.0,r.1]",toString(s2));

		// -- test union --
		EXPECT_EQ("[]",toString(MeshRegion::merge(e,e)));
		EXPECT_EQ("[r.0]",toString(MeshRegion::merge(e,sl)));
		EXPECT_EQ("[r.0]",toString(MeshRegion::merge(sl,e)));
		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(e,s2)));
		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(s2,e)));

		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(sl,sr)));
		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(s2,sr)));
		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(sl,s2)));
		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(s2,e)));
		EXPECT_EQ("[r.0,r.1]",toString(MeshRegion::merge(e,s2)));


		// -- test intersection --
		EXPECT_EQ("[]",toString(MeshRegion::intersect(e,e)));
		EXPECT_EQ("[]",toString(MeshRegion::intersect(e,sl)));
		EXPECT_EQ("[]",toString(MeshRegion::intersect(sl,e)));

		EXPECT_EQ("[]",toString(MeshRegion::intersect(sl,sr)));

		EXPECT_EQ("[r.0]",toString(MeshRegion::intersect(sl,s2)));
		EXPECT_EQ("[r.0]",toString(MeshRegion::intersect(s2,sl)));
		EXPECT_EQ("[r.1]",toString(MeshRegion::intersect(sr,s2)));
		EXPECT_EQ("[r.1]",toString(MeshRegion::intersect(s2,sr)));

	}

	TEST(MeshRegion, DataItemRegionConcept) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();
		SubMeshRef r01 = r.getLeftChild().getRightChild();
		SubMeshRef r11 = r.getRightChild().getRightChild();

		MeshRegion a { r00, r01 };
		MeshRegion b { r01, r11 };

		EXPECT_EQ("[r.0.0,r.0.1]",toString(a));
		EXPECT_EQ("[r.0.1,r.1.1]",toString(b));

		testRegion(a,b);
	}


	TEST(MeshRegion, Scan) {

		using namespace detail;

		auto toList = [](const MeshRegion& r) {
			std::vector<SubTreeRef> list;
			r.scan([&](const SubTreeRef& ref) {
				list.push_back(ref);
			});
			return list;
		};


		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();
		SubMeshRef r01 = r.getLeftChild().getRightChild();
		SubMeshRef r11 = r.getRightChild().getRightChild();

		MeshRegion a { r00, r01 };
		MeshRegion b { r01, r11 };

		EXPECT_EQ("[r.0.0,r.0.1]",toString(toList(a)));
		EXPECT_EQ("[r.0.1,r.1.1]",toString(toList(b)));

	}


	TEST(PartitionTree, Basic) {

		using namespace detail;

		struct Vertex {};
		struct Edge : public edge<Vertex,Vertex> {};

		using Ptree = PartitionTree<nodes<Vertex>,edges<Edge>>;


		EXPECT_TRUE(std::is_default_constructible<Ptree>::value);
		EXPECT_FALSE(std::is_trivially_default_constructible<Ptree>::value);

		EXPECT_FALSE(std::is_trivially_copy_constructible<Ptree>::value);
		EXPECT_FALSE(std::is_trivially_move_constructible<Ptree>::value);

		EXPECT_TRUE(std::is_copy_constructible<Ptree>::value);
		EXPECT_TRUE(std::is_move_constructible<Ptree>::value);

		EXPECT_FALSE(std::is_trivially_copy_assignable<Ptree>::value);
		EXPECT_FALSE(std::is_trivially_move_assignable<Ptree>::value);

		EXPECT_TRUE(std::is_copy_assignable<Ptree>::value);
		EXPECT_TRUE(std::is_move_assignable<Ptree>::value);


	}

	// --- combinations ---

	TEST(Mesh, BuildSingleLevel) {

		// define 'object' types
		struct Cell {};
		struct Face {};
		struct Node {};
		struct BoundaryFace {};

		// define 'relations'
		struct Face2Cell : public edge<Face,Cell> {};
		struct BoundaryFace2Cell : public edge<BoundaryFace,Cell> {};

		// create a mesh instance
		MeshBuilder<
			nodes<Cell,Face,Node,BoundaryFace>,
			edges<Face2Cell, BoundaryFace2Cell>
		> mb;

		// create two cells
		auto a = mb.create<Cell>();
		auto b = mb.create<Cell>();

		// create the face in-between
		auto f = mb.create<Face>();

		// create Boundary faces
		auto bl = mb.create<BoundaryFace>();
		auto br = mb.create<BoundaryFace>();

		// connect the elements
		mb.link<BoundaryFace2Cell>(bl,a);
		mb.link<Face2Cell>(f,a);
		mb.link<Face2Cell>(f,b);
		mb.link<BoundaryFace2Cell>(br,b);


		// build mesh
		auto m = mb.build();

		// test edges
		EXPECT_EQ(a, m.getNeighbor<BoundaryFace2Cell>(bl));

		// check node sets
		EXPECT_EQ(2,m.numNodes<Cell>());
		EXPECT_EQ(1,m.numNodes<Face>());
		EXPECT_EQ(2,m.numNodes<BoundaryFace>());

		EXPECT_EQ(2, (m.createNodeData<Cell,double>().size()));
		EXPECT_EQ(1, (m.createNodeData<Face,double>().size()));
		EXPECT_EQ(2, (m.createNodeData<BoundaryFace,double>().size()));

/*
		// check forAll and pforAll support
		std::atomic<int> counter(0);
		m.forAll<Cell>([&](auto) { counter++; });
		EXPECT_EQ(2,counter);

		counter.store(0);
		m.pforAll<Cell>([&](auto) { counter++; });
		EXPECT_EQ(2,counter);
*/
	}


	TEST(Mesh, BuildMultiLevel) {

		// define 'object' types
		struct Cell {};

		// define 'relations'
		struct Cell2Cell : public edge<Cell,Cell> {};

		// and 'hierarchies'
		struct Cell2Child : public hierarchy<Cell,Cell> {};

		// create a mesh instance
		MeshBuilder<
			nodes<Cell>,
			edges<Cell2Cell>,
			hierarchies<Cell2Child>,
			3				// 3 layers
		> mb;

		// create four cells on layer 0
		auto l0a = mb.create<Cell>();
		auto l0b = mb.create<Cell>();
		auto l0c = mb.create<Cell>();
		auto l0d = mb.create<Cell>();

		// create two cells on layer 1
		auto l1a = mb.create<Cell,1>();
		auto l1b = mb.create<Cell,1>();

		// create a final cell on layer 2
		auto l2a = mb.create<Cell,2>();

		// link first layer
		mb.link<Cell2Cell>(l0a,l0b);
		mb.link<Cell2Cell>(l0b,l0a);
		mb.link<Cell2Cell>(l0b,l0c);
		mb.link<Cell2Cell>(l0c,l0b);
		mb.link<Cell2Cell>(l0c,l0d);
		mb.link<Cell2Cell>(l0d,l0c);

		// link second layer
		mb.link<Cell2Cell>(l1a,l1b);
		mb.link<Cell2Cell>(l1b,l1a);

		// link hierarchies
		mb.link<Cell2Child>(l1a,l0a);
		mb.link<Cell2Child>(l1a,l0b);
		mb.link<Cell2Child>(l1b,l0c);
		mb.link<Cell2Child>(l1b,l0d);

		mb.link<Cell2Child>(l2a,l1a);
		mb.link<Cell2Child>(l2a,l1b);


		// build mesh
		auto m = mb.build();

		// check relations per level
		EXPECT_EQ(1, m.getNeighbors<Cell2Cell>(l0a).size());
		EXPECT_EQ(2, m.getNeighbors<Cell2Cell>(l0b).size());
		EXPECT_EQ(2, m.getNeighbors<Cell2Cell>(l0c).size());
		EXPECT_EQ(1, m.getNeighbors<Cell2Cell>(l0d).size());

		EXPECT_EQ(l0b, m.getNeighbor<Cell2Cell>(l0a));
		EXPECT_EQ(l0c, m.getNeighbor<Cell2Cell>(l0d));

		EXPECT_EQ(1, m.getNeighbors<Cell2Cell>(l1a).size());
		EXPECT_EQ(1, m.getNeighbors<Cell2Cell>(l1b).size());

		EXPECT_EQ(l1b, m.getNeighbor<Cell2Cell>(l1a));
		EXPECT_EQ(l1a, m.getNeighbor<Cell2Cell>(l1b));

		// check parent/child relations
		EXPECT_EQ(2, m.getChildren<Cell2Child>(l1a).size());
		EXPECT_EQ(2, m.getChildren<Cell2Child>(l1b).size());

		EXPECT_EQ(std::vector<decltype(l0a)>({ l0a,l0b }), m.getChildren<Cell2Child>(l1a));
		EXPECT_EQ(std::vector<decltype(l0a)>({ l0c,l0d }), m.getChildren<Cell2Child>(l1b));

		EXPECT_EQ(2, m.getChildren<Cell2Child>(l2a).size());
		EXPECT_EQ(std::vector<decltype(l1a)>({ l1a,l1b }), m.getChildren<Cell2Child>(l2a));

	}

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
