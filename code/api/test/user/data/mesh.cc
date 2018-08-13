#include <gtest/gtest.h>

#include "allscale/api/core/data.h"
#include "allscale/api/user/data/mesh.h"
#include "allscale/utils/string_utils.h"


namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"
	#include "bar_mesh.inl"

	template<unsigned depth = 4>
	std::vector<detail::SubTreeRef> createListOfSubTreeRefs() {

		using namespace detail;

		std::vector<SubTreeRef> list;

		// insert all references of up to length 4
		SubTreeRef::root().enumerate<depth,true>([&](const SubTreeRef& ref) {
			list.push_back(ref);
		});

		return list;
	}

	template<unsigned depth = 4>
	std::vector<detail::SubMeshRef> createListOfSubMeshRefs() {

		using namespace detail;

		std::vector<SubMeshRef> list;

		// insert all references of up to length 4
		SubTreeRef::root().enumerate<depth,true>([&](const SubTreeRef& ref) {
			list.push_back(ref);
		});


		// check also with wild-cards
		SubTreeRef::root().enumerate<depth,true>([&](const SubTreeRef& ref) {

			auto curDepth = ref.getDepth();
			for(int c = 0; c < (1 << curDepth); ++c) {

				SubMeshRef cur = ref;
				for(unsigned i = 0; i<curDepth; ++i) {
					if ((c >> i) % 2) cur = cur.getMasked(i);
				}
				list.push_back(cur);

			}
		});

		// eliminate duplicates
		std::sort(list.begin(),list.end());
		list.erase(std::unique(list.begin(),list.end()),list.end());

		return list;
	}

	template<unsigned depth = 3>
	std::vector<detail::MeshRegion> createRegionList() {

		using namespace detail;

		const unsigned num_entries = (1 << depth);
		const unsigned num_subsets = (1 << num_entries);

		std::vector<MeshRegion> regions;
		for(unsigned i=0; i<num_subsets; ++i) {

			// use i as a bit mask to add elements
			std::vector<SubMeshRef> refs;
			for(unsigned j=0; j<num_entries; ++j) {
				// only if bit j is set in i
				if (!((i >> j) % 2)) continue;

				int mask = j;
				SubMeshRef cur = SubMeshRef::root();
				for(unsigned k=0; k<depth; k++) {
					if (mask & 0x1) {
						cur = cur.getRightChild();
					} else {
						cur = cur.getLeftChild();
					}
					mask >>= 1;
				}
				refs.push_back(cur);
			}
			regions.push_back(refs);
		}

		return regions;
	}

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


		using ptree = detail::PartitionTree<nodes<int>,edges<>>;
		using data = MeshData<int,int,1,ptree>;

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


		// check that it qualifies as a region
		EXPECT_TRUE(allscale::api::core::is_data_item<data>::value);

	}

	TEST(DISABLED_MeshData, SharedUsage) {

		using namespace detail;

		auto mesh = createBarMesh(100);

		using facade = decltype(mesh.createNodeData<Vertex,int>());
		using fragment = typename facade::fragment_type;
		using region = typename fragment::region_type;
		using shared_data = typename fragment::shared_data_type;

		// get shared data
		const shared_data& shared = mesh.getPartitionTree();

		// create two regions of nodes
		SubMeshRef r = SubMeshRef::root();

		MeshRegion partA = r.getLeftChild();
		MeshRegion partB = r.getRightChild();

		// create fragments
		fragment fA(shared,partA);
		fragment fB(shared,partB);

		// fill the data buffer
		auto a = fA.mask();
		partA.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
			a[node] = 0;
		});

		auto b = fB.mask();
		partB.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
			b[node] = 0;
		});

		for(int t = 0; t<10; t++) {

			partA.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
				EXPECT_EQ(t,a[node]);
				a[node]++;
			});

			partB.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
				EXPECT_EQ(t,b[node]);
				b[node]++;
			});

		}


		// --- alter data distribution ---

		MeshRegion newPartA = r.getLeftChild().getLeftChild();
		MeshRegion newPartB = r.getRightChild().getRightChild();
		MeshRegion newPartC {
			r.getLeftChild().getRightChild(),
			r.getRightChild().getLeftChild()
		};

		fragment fC(shared,newPartC);

//		std::cout << "PartA:     " << partA << "\n";
//		std::cout << "newPartC:  " << newPartC << "\n";
//		std::cout << "intersect: " << region::intersect(newPartC,partA) << "\n";

		// move data from A and B to C
		fC.insertRegion(fA,region::intersect(newPartC,partA));
		fC.insertRegion(fB,region::intersect(newPartC,partB));


		// shrink A and B
		fA.resize(newPartA);
		fB.resize(newPartB);

		for(int t = 10; t<20; t++) {

			auto a = fA.mask();
			newPartA.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
				EXPECT_EQ(t,a[node]);
				a[node]++;
			});

			auto b = fB.mask();
			newPartB.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
				EXPECT_EQ(t,b[node]);
				b[node]++;
			});

			auto c = fC.mask();
			newPartC.scan<Vertex,0>(shared,[&](const NodeRef<Vertex,0>& node) {
				EXPECT_EQ(t,c[node]);
				c[node]++;
			});

		}

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


	TEST(SubTreeRef, Covers) {

		using namespace detail;

		SubTreeRef r = SubTreeRef::root();

		auto r0 = r.getLeftChild();
		auto r1 = r.getRightChild();

		auto covers = [](const auto& a, const auto& b) {
			return a.covers(b);
		};

		auto not_covers = [](const auto& a, const auto& b) {
			return !a.covers(b);
		};

		EXPECT_PRED2(covers,r,r0);
		EXPECT_PRED2(covers,r,r1);

		EXPECT_PRED2(not_covers,r0,r);
		EXPECT_PRED2(not_covers,r1,r);

		EXPECT_PRED2(not_covers,r0,r1);
		EXPECT_PRED2(not_covers,r1,r0);

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


		EXPECT_EQ(2, r.getRightChild().getLeftChild().getMasked(0).getDepth());
		EXPECT_EQ(2, r.getLeftChild().getRightChild().getMasked(0).getDepth());

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


		r = r.getMasked(1);
		EXPECT_EQ( "r.0.*.0",toString(r));
		EXPECT_EQ("[r.0.0.0,r.0.1.0]",toString(toList(r)));

		r = r.getMasked(0);
		EXPECT_EQ( "r.*.*.0",toString(r));
		EXPECT_EQ("[r.0.0.0,r.0.1.0,r.1.0.0,r.1.1.0]",toString(toList(r)));


		r = r.getMasked(2);
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

		EXPECT_EQ("r.*.0",toString(r.getRightChild().getLeftChild().getMasked(0)));
		EXPECT_EQ("r.*.1",toString(r.getLeftChild().getRightChild().getMasked(0)));

	}

	TEST(SubMeshRef, Order) {

		using namespace detail;

		auto list = createListOfSubMeshRefs<3>();

		// check each pair of references
		for(const auto& a : list) {
			for(const auto& b : list) {
				EXPECT_EQ(toString(a) < toString(b), a < b)
					<< "a = " << a << "\n"
					<< "b = " << b << "\n";

				EXPECT_EQ(toString(a) > toString(b), a > b)
					<< "a = " << a << "\n"
					<< "b = " << b << "\n";

				EXPECT_EQ(toString(a) <= toString(b), a <= b)
					<< "a = " << a << "\n"
					<< "b = " << b << "\n";

				EXPECT_EQ(toString(a) >= toString(b), a >= b)
					<< "a = " << a << "\n"
					<< "b = " << b << "\n";
			}
		}

	}

	TEST(SubMeshRef, Covers) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		auto r0 = r.getLeftChild();
		auto r1 = r.getRightChild();

		auto covers = [](const auto& a, const auto& b) {
			return a.covers(b);
		};

		auto not_covers = [](const auto& a, const auto& b) {
			return !a.covers(b);
		};

		EXPECT_PRED2(covers,r,r0);
		EXPECT_PRED2(covers,r,r1);

		EXPECT_PRED2(not_covers,r0,r);
		EXPECT_PRED2(not_covers,r1,r);

		EXPECT_PRED2(not_covers,r0,r1);
		EXPECT_PRED2(not_covers,r1,r0);



		auto a = r.getLeftChild().getLeftChild().getLeftChild().getLeftChild();
		auto b = r.getLeftChild().getLeftChild().getLeftChild();

		EXPECT_PRED2(covers,b,a);

		a = a.getMasked(2);
		EXPECT_PRED2(not_covers,b,a);

		b = b.getMasked(2);
		EXPECT_PRED2(covers,b,a);

	}

	TEST(SubMeshRef, EnclosingTree) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		EXPECT_EQ("r",toString(r.getEnclosingSubTree()));
		EXPECT_EQ("r.0",toString(r.getLeftChild().getEnclosingSubTree()));
		EXPECT_EQ("r.0.1",toString(r.getLeftChild().getRightChild().getEnclosingSubTree()));
		EXPECT_EQ("r.0.1.0",toString(r.getLeftChild().getRightChild().getLeftChild().getEnclosingSubTree()));

		EXPECT_EQ("r",toString(r.getLeftChild().getRightChild().getMasked(0).getEnclosingSubTree()));
		EXPECT_EQ("r.0",toString(r.getLeftChild().getRightChild().getLeftChild().getMasked(1).getEnclosingSubTree()));

	}

	TEST(SubMeshRef, TryIntersect) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();

		SubMeshRef r000 = r.getLeftChild().getLeftChild().getLeftChild();
		SubMeshRef r001 = r.getLeftChild().getLeftChild().getRightChild();
		SubMeshRef r010 = r.getLeftChild().getRightChild().getLeftChild();
		SubMeshRef r011 = r.getLeftChild().getRightChild().getRightChild();

		SubMeshRef r0s0 = r000.getMasked(1);
		SubMeshRef r0s1 = r001.getMasked(1);

		SubMeshRef rss1 = r0s1.getMasked(0).getMasked(1);


		EXPECT_FALSE(r000.tryIntersect(r001));
		EXPECT_TRUE (r001.tryIntersect(r001));
		EXPECT_FALSE(r010.tryIntersect(r001));
		EXPECT_FALSE(r011.tryIntersect(r001));

		SubMeshRef tmp = r;

		tmp = r;
		EXPECT_TRUE(tmp.tryIntersect(r0s0));
		EXPECT_EQ(r0s0,tmp);

		tmp = r0s0;
		EXPECT_TRUE(tmp.tryIntersect(r0s0));
		EXPECT_EQ(r0s0,tmp);

		tmp = r0s1;
		EXPECT_TRUE(tmp.tryIntersect(rss1));
		EXPECT_EQ(r0s1,tmp);


		tmp = r00;
		EXPECT_TRUE(tmp.tryIntersect(r0s1));
		EXPECT_EQ(r001,tmp);

	}

	TEST(SubMeshRef, Complement) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r0 = r.getLeftChild();
		SubMeshRef r1 = r.getRightChild();

		SubMeshRef r000 = r.getLeftChild().getLeftChild().getLeftChild();
		SubMeshRef r001 = r.getLeftChild().getLeftChild().getRightChild();
		SubMeshRef r010 = r.getLeftChild().getRightChild().getLeftChild();
		SubMeshRef r011 = r.getLeftChild().getRightChild().getRightChild();

		SubMeshRef r0s0 = r000.getMasked(1);
		SubMeshRef r0s1 = r001.getMasked(1);
		SubMeshRef rss1 = r0s1.getMasked(0);


		EXPECT_EQ("[]",toString(r.getComplement()));

		EXPECT_EQ("[r.1]",toString(r0.getComplement()));
		EXPECT_EQ("[r.0]",toString(r1.getComplement()));

		EXPECT_EQ("[r.1,r.0.1,r.0.0.1]",toString(r000.getComplement()));
		EXPECT_EQ("[r.1,r.0.1,r.0.0.0]",toString(r001.getComplement()));
		EXPECT_EQ("[r.1,r.0.0,r.0.1.1]",toString(r010.getComplement()));
		EXPECT_EQ("[r.1,r.0.0,r.0.1.0]",toString(r011.getComplement()));

		EXPECT_EQ("[r.1,r.0.0.1,r.0.1.1]",toString(r0s0.getComplement()));
		EXPECT_EQ("[r.1,r.0.0.0,r.0.1.0]",toString(r0s1.getComplement()));
		EXPECT_EQ("[r.0.0.0,r.0.1.0,r.1.0.0,r.1.1.0]",toString(rss1.getComplement()));

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

		MeshRegion sl = SubMeshRef::root().getLeftChild().getRightChild();
		EXPECT_EQ("[r.0.1]",toString(sl));

		MeshRegion sr = SubMeshRef::root().getRightChild().getLeftChild();
		EXPECT_EQ("[r.1.0]",toString(sr));

		MeshRegion s2 = MeshRegion::merge(sl,sr);
		EXPECT_EQ("[r.0.1,r.1.0]",toString(s2));
	}

	TEST(MeshRegion, SimpleSetOps) {

		using namespace detail;

		MeshRegion e;
		EXPECT_EQ("[]",toString(e));

		MeshRegion sl = SubMeshRef::root().getLeftChild();
		EXPECT_EQ("[r.0]",toString(sl));

		MeshRegion sr = SubMeshRef::root().getRightChild();
		EXPECT_EQ("[r.1]",toString(sr));

		MeshRegion s2 = MeshRegion::merge(sl,sr);
		EXPECT_EQ("[r]",toString(s2));

		// -- test union --
		EXPECT_EQ("[]",toString(MeshRegion::merge(e,e)));
		EXPECT_EQ("[r.0]",toString(MeshRegion::merge(e,sl)));
		EXPECT_EQ("[r.0]",toString(MeshRegion::merge(sl,e)));
		EXPECT_EQ("[r]",toString(MeshRegion::merge(e,s2)));
		EXPECT_EQ("[r]",toString(MeshRegion::merge(s2,e)));

		EXPECT_EQ("[r]",toString(MeshRegion::merge(sl,sr)));
		EXPECT_EQ("[r]",toString(MeshRegion::merge(s2,sr)));
		EXPECT_EQ("[r]",toString(MeshRegion::merge(sl,s2)));
		EXPECT_EQ("[r]",toString(MeshRegion::merge(s2,e)));
		EXPECT_EQ("[r]",toString(MeshRegion::merge(e,s2)));


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


	TEST(MeshRegion, Union) {

		using namespace detail;

		MeshRegion e;
		EXPECT_EQ("[]",toString(e));

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r0 = r.getLeftChild();
		SubMeshRef r1 = r.getRightChild();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();
		SubMeshRef r01 = r.getLeftChild().getRightChild();
		SubMeshRef r10 = r.getRightChild().getLeftChild();
		SubMeshRef r11 = r.getRightChild().getRightChild();

		SubMeshRef r000 = r.getLeftChild().getLeftChild().getLeftChild();
		SubMeshRef r001 = r.getLeftChild().getLeftChild().getRightChild();
		SubMeshRef r010 = r.getLeftChild().getRightChild().getLeftChild();
		SubMeshRef r011 = r.getLeftChild().getRightChild().getRightChild();
		SubMeshRef r100 = r.getRightChild().getLeftChild().getLeftChild();
		SubMeshRef r101 = r.getRightChild().getLeftChild().getRightChild();
		SubMeshRef r110 = r.getRightChild().getRightChild().getLeftChild();
		SubMeshRef r111 = r.getRightChild().getRightChild().getRightChild();

		SubMeshRef r0s1 = r001.getMasked(1);
		SubMeshRef rss1 = r001.getMasked(0).getMasked(1);


		// check cases where one reference is covering other
		EXPECT_EQ("[r]",toString(MeshRegion{ r, r00, r01 }));
		EXPECT_EQ("[r.0.*.1,r.0.0.0]",toString(MeshRegion{ r0s1, r000, r001, r011 }));
		EXPECT_EQ("[r.0.*.1,r.0.0.0]",toString(MeshRegion{ r011, r000, r0s1, r001 }));
		EXPECT_EQ("[r.*.*.1,r.0.0.0]",toString(MeshRegion{ r011, r000, r0s1, r001, rss1 }));


		// check situations where references may be merged
		EXPECT_EQ("[r]",toString(MeshRegion{ r01, r00, r1 }));

		// check sibling merges
		EXPECT_EQ("[r.0.*.0]",toString(MeshRegion{ r010, r000 }));
		EXPECT_EQ("[r.0.0]",toString(MeshRegion{ r000, r001 }));
		EXPECT_EQ("[r.0.0.1,r.0.1]",toString(MeshRegion{ r01, r001 }));

		EXPECT_EQ("[r]",toString(MeshRegion{
			r0,  r00,  r000,
			r1,  r01,  r001,
                 r10,  r010,
			     r11,  r011,
			           r100,
			           r101,
			           r110,
                       r111

		}));

		EXPECT_EQ("[r]",toString(MeshRegion{
			r00,
			r01,
			r10,
			r11
		}));

		EXPECT_EQ("[r]",toString(MeshRegion{
			r000,
			r001,
			r010,
			r011,
			r100,
			r101,
			r110,
			r111
		}));


		EXPECT_EQ("[r.0,r.1.1]",toString(MeshRegion{
			r00,
			r01,
			r11
		}));

		EXPECT_EQ("[r.0,r.1.*.0,r.1.1.1]",toString(MeshRegion{
			r000,
			r001,
			r010,
			r011,
			r100,
			r110,
			r111
		}));
	}

	TEST(MeshRegion, Intersection) {

		using namespace detail;

		MeshRegion e;
		EXPECT_EQ("[]",toString(e));

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();
		SubMeshRef r01 = r.getLeftChild().getRightChild();

		SubMeshRef r001 = r.getLeftChild().getLeftChild().getRightChild();
		SubMeshRef r011 = r.getLeftChild().getRightChild().getRightChild();
		SubMeshRef r111 = r.getRightChild().getRightChild().getRightChild();

		SubMeshRef r0s1 = r001.getMasked(1);
		SubMeshRef rss1 = r001.getMasked(0).getMasked(1);


		// check cases where one reference is covering other
		EXPECT_EQ("[r]",toString(MeshRegion::intersect(MeshRegion{ r, r00, r01 },MeshRegion{ r, r00, r01 })));

		EXPECT_EQ("[]",toString(MeshRegion::intersect(MeshRegion{ r0s1 },MeshRegion{ r111 })));
		EXPECT_EQ("[r.0.0.1]",toString(MeshRegion::intersect(MeshRegion{ r0s1 },MeshRegion{ r001 })));
		EXPECT_EQ("[r.*.1.1]",toString(MeshRegion::intersect(MeshRegion{ r0s1, rss1 },MeshRegion{ r011, r111 })));

		EXPECT_EQ("[r.0.0.1]",toString(MeshRegion::intersect(MeshRegion{ r00 },MeshRegion{ r0s1 })));

	}

	TEST(MeshRegion, Complement) {

		using namespace detail;


		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();

		SubMeshRef r000 = r.getLeftChild().getLeftChild().getLeftChild();
		SubMeshRef r011 = r.getLeftChild().getRightChild().getRightChild();

		SubMeshRef r0s0 = r000.getMasked(1);

		EXPECT_EQ("[r]",toString(MeshRegion{ }.complement()));
		EXPECT_EQ("[]", toString(MeshRegion{ r }.complement()));

		EXPECT_EQ("[r.0.1.0,r.1]", toString(MeshRegion{ r00, r011 }.complement()));
		EXPECT_EQ("[r.0.0.1,r.1]", toString(MeshRegion{ r0s0, r011 }.complement()));

	}

	TEST(MeshRegion, Difference) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();

		SubMeshRef r000 = r.getLeftChild().getLeftChild().getLeftChild();
		SubMeshRef r011 = r.getLeftChild().getRightChild().getRightChild();

		SubMeshRef r0s0 = r000.getMasked(1);

		EXPECT_EQ("[]", toString(MeshRegion::difference(MeshRegion{},MeshRegion{})));
		EXPECT_EQ("[r]",toString(MeshRegion::difference(MeshRegion{ r },MeshRegion{})));
		EXPECT_EQ("[]", toString(MeshRegion::difference(MeshRegion{ },MeshRegion{ r })));

		EXPECT_EQ("[r.0.1,r.1]", toString(MeshRegion::difference(MeshRegion{ r },MeshRegion{ r00 })));
		EXPECT_EQ("[r.0.0.1,r.0.1,r.1]", toString(MeshRegion::difference(MeshRegion{ r },MeshRegion{ r000 })));
		EXPECT_EQ("[r.0.0,r.0.1.0,r.1]", toString(MeshRegion::difference(MeshRegion{ r },MeshRegion{ r011 })));
		EXPECT_EQ("[r.0.*.1,r.1]", toString(MeshRegion::difference(MeshRegion{ r },MeshRegion{ r0s0 })));


		EXPECT_EQ("[r.0.1.1]", toString(MeshRegion::difference(MeshRegion{ r000, r011 },MeshRegion{ r0s0 })));
		EXPECT_EQ("[r.0.1]", toString(MeshRegion::difference(MeshRegion{ r0s0, r011 },MeshRegion{ r000 })));

	}

	TEST(MeshRegion, UnionExaustive) {

		using namespace detail;

		const int depth = 3;

		// create list of regions
		auto regions = createRegionList<depth>();

		// create list of references
		auto refs = createListOfSubTreeRefs<depth>();

		// check cross-product of all regions and references
		for(const auto& a : regions) {
			for(const auto& b : regions) {

				// compute union
				auto c = MeshRegion::merge(a,b);

				// check correctness
				for(const auto& cur : refs) {

					// only check full-depth entries
					if (cur.getDepth() < depth) continue;

					EXPECT_EQ((a.covers(cur) || b.covers(cur)),(c.covers(cur)))
						<< "\ta = " << a << "\n"
						<< "\tb = " << b << "\n"
						<< "\tc = " << c << "\n"
						<< "\tr = " << cur << "\n"
						<< "\tr in a : " << a.covers(cur) << "\n"
						<< "\tr in b : " << b.covers(cur) << "\n"
						<< "\tr in c : " << c.covers(cur) << "\n";

				}

			}
		}

	}


	TEST(MeshRegion, IntersectExaustive) {

		using namespace detail;

		const int depth = 3;

		// create list of regions
		auto regions = createRegionList<depth>();

		// create list of references
		auto refs = createListOfSubTreeRefs<depth>();

		// check cross-product of all regions and references
		for(const auto& a : regions) {
			for(const auto& b : regions) {

				// compute intersect
				auto c = MeshRegion::intersect(a,b);

				// check correctness
				for(const auto& cur : refs) {

					// only check full-depth entries
					if (cur.getDepth() < depth) continue;

					EXPECT_EQ((a.covers(cur) && b.covers(cur)),(c.covers(cur)))
						<< "\ta = " << a << "\n"
						<< "\tb = " << b << "\n"
						<< "\tc = " << c << "\n"
						<< "\tr = " << cur << "\n"
						<< "\tr in a : " << a.covers(cur) << "\n"
						<< "\tr in b : " << b.covers(cur) << "\n"
						<< "\tr in c : " << c.covers(cur) << "\n";

				}

			}
		}

	}

	TEST(MeshRegion, DifferenceExaustive) {

		using namespace detail;

		const int depth = 3;

		// create list of regions
		auto regions = createRegionList<depth>();

		// create list of references
		auto refs = createListOfSubTreeRefs<depth>();

		// check cross-product of all regions and references
		for(const auto& a : regions) {
			for(const auto& b : regions) {

				// compute difference
				auto c = MeshRegion::difference(a,b);

				// check correctness
				for(const auto& cur : refs) {

					// only check full-depth entries
					if (cur.getDepth() < depth) continue;

					EXPECT_EQ((a.covers(cur) && !b.covers(cur)),(c.covers(cur)))
						<< "\ta = " << a << "\n"
						<< "\tb = " << b << "\n"
						<< "\tc = " << c << "\n"
						<< "\tr = " << cur << "\n"
						<< "\tr in a : " << a.covers(cur) << "\n"
						<< "\tr in b : " << b.covers(cur) << "\n"
						<< "\tr in c : " << c.covers(cur) << "\n";

				}

			}
		}

	}

	TEST(MeshRegion, ComplementExaustive) {

		using namespace detail;

		const int depth = 3;

		// create list of regions
		auto regions = createRegionList<depth>();

		// create list of references
		auto refs = createListOfSubTreeRefs<depth>();

		// check cross-product of all regions and references
		for(const auto& a : regions) {

			// compute complement
			auto b = MeshRegion::complement(a);

			// check correctness
			for(const auto& cur : refs) {

				// only check full-depth entries
				if (cur.getDepth() < depth) continue;

				EXPECT_NE(a.covers(cur),b.covers(cur))
					<< "\ta = " << a << "\n"
					<< "\tb = " << b << "\n"
					<< "\tr = " << cur << "\n"
					<< "\tr in a : " << a.covers(cur) << "\n"
					<< "\tr in b : " << b.covers(cur) << "\n";
			}

		}

	}

	TEST(MeshRegion, AdvancedSetOps) {

		using namespace detail;

		MeshRegion e;
		EXPECT_EQ("[]",toString(e));

		MeshRegion a = SubMeshRef::root().getLeftChild();
		EXPECT_EQ("[r.0]",toString(a));

		MeshRegion b = SubMeshRef::root().getLeftChild().getRightChild();
		EXPECT_EQ("[r.0.1]",toString(b));

		// -- test union --
		EXPECT_EQ("[r.0]",toString(MeshRegion::merge(a,b)));

		// -- test intersect --
		EXPECT_EQ("[r.0.1]",toString(MeshRegion::intersect(a,b)));

		// -- test difference --
		EXPECT_EQ("[r.0.0]",toString(MeshRegion::difference(a,b)));
		EXPECT_EQ("[]",toString(MeshRegion::difference(b,a)));
	}


	TEST(MeshRegion, DataItemRegionConcept) {

		using namespace detail;

		SubMeshRef r = SubMeshRef::root();

		SubMeshRef r00 = r.getLeftChild().getLeftChild();
		SubMeshRef r01 = r.getLeftChild().getRightChild();
		SubMeshRef r11 = r.getRightChild().getRightChild();

		MeshRegion a { r00, r01 };
		MeshRegion b { r01, r11 };

		EXPECT_EQ("[r.0]",toString(a));
		EXPECT_EQ("[r.*.1]",toString(b));

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

		EXPECT_EQ("[r.0]",toString(toList(a)));
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

		EXPECT_FALSE(std::is_copy_constructible<Ptree>::value);
		EXPECT_TRUE(std::is_move_constructible<Ptree>::value);

		EXPECT_FALSE(std::is_trivially_copy_assignable<Ptree>::value);
		EXPECT_FALSE(std::is_trivially_move_assignable<Ptree>::value);

		EXPECT_FALSE(std::is_copy_assignable<Ptree>::value);
		EXPECT_TRUE(std::is_move_assignable<Ptree>::value);


	}

	template<typename PTree>
	void checkPtree(const PTree& ptree) {

		using namespace detail;

		SubTreeRef r = SubTreeRef::root();

		// root level
		EXPECT_EQ("[0,10)",toString(ptree.template getNodeRange<Vertex,0>(r)));

		// 1st level
		EXPECT_EQ("[0,5)",toString(ptree.template getNodeRange<Vertex,0>(r.getLeftChild())));
		EXPECT_EQ("[5,10)",toString(ptree.template getNodeRange<Vertex,0>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[0,2)",toString(ptree.template getNodeRange<Vertex,0>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[2,5)",toString(ptree.template getNodeRange<Vertex,0>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[5,7)",toString(ptree.template getNodeRange<Vertex,0>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[7,10)",toString(ptree.template getNodeRange<Vertex,0>(r.getRightChild().getRightChild())));


		// root level
		EXPECT_EQ("[0,5)",toString(ptree.template getNodeRange<Vertex,1>(r)));

		// 1st level
		EXPECT_EQ("[0,2)",toString(ptree.template getNodeRange<Vertex,1>(r.getLeftChild())));
		EXPECT_EQ("[2,5)",toString(ptree.template getNodeRange<Vertex,1>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[0,1)",toString(ptree.template getNodeRange<Vertex,1>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[1,2)",toString(ptree.template getNodeRange<Vertex,1>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[2,3)",toString(ptree.template getNodeRange<Vertex,1>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[3,5)",toString(ptree.template getNodeRange<Vertex,1>(r.getRightChild().getRightChild())));



		// --- check closures ---

		// root level
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r)));

		// 1st level
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r.getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getForwardClosure<Edge,0>(r.getRightChild().getRightChild())));


		// root level
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r)));

		// 1st level
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r.getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r.getRightChild())));


		// 2nd level
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getBackwardClosure<Edge,0>(r.getRightChild().getRightChild())));


		// --- check hierarchical closures ---

		// root level
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r)));

		// 1st level
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r.getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getParentClosure<Refine,0>(r.getRightChild().getRightChild())));


		// root level
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r)));

		// 1st level
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r.getLeftChild())));
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[]",toString(ptree.template getChildClosure<Refine,0>(r.getRightChild().getRightChild())));


		// - level 1 -

		// root level
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r)));

		// 1st level
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r.getLeftChild())));
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[]",toString(ptree.template getParentClosure<Refine,1>(r.getRightChild().getRightChild())));


		// root level
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r)));

		// 1st level
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r.getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r.getRightChild())));

		// 2nd level
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r.getLeftChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r.getLeftChild().getRightChild())));
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r.getRightChild().getLeftChild())));
		EXPECT_EQ("[r]",toString(ptree.template getChildClosure<Refine,1>(r.getRightChild().getRightChild())));
	}


	TEST(PartitionTree,Initialization) {

		using namespace detail;

		auto bar = createBarMesh<2,2>(5);
		auto& ptree = bar.getPartitionTree();

		// this ptree should be closed
		EXPECT_TRUE(ptree.isClosed());

		// check the content of the ptree
		checkPtree(ptree);

	}

	TEST(PartitionTree,IO) {

		// fix the type of the partition tree to be tested
		using PTree = detail::plain_type<decltype(createBarMesh<2,2>(5).getPartitionTree())>;

		std::stringstream buffer(std::ios_base::in | std::ios_base::out | std::ios_base::binary);

		{ // -- creation --

			// create a ptree
			auto bar = createBarMesh<2,2>(5);
			auto& ptree = bar.getPartitionTree();

			// this ptree should be closed
			EXPECT_TRUE(ptree.isClosed());

			// check the content of the ptree
			checkPtree(ptree);

			// convert to buffer
			ptree.store(buffer);

			// let ptree be destroyed
		}

		{ // -- reload mesh from a stream --

			// load from stream
			auto ptree = PTree::load(buffer);

			// this ptree should be closed
			EXPECT_TRUE(ptree.isClosed());

			// check the content of the ptree
			checkPtree(ptree);

		}

		{ // -- reload ptree using reinterpretation of in-memory buffer --

			// extract data buffer
			auto str = buffer.str();

			// interpret content
			utils::RawBuffer raw(const_cast<char*>(str.c_str()));
			auto ptree = PTree::interpret(raw);

			// this ptree should be closed
			EXPECT_TRUE(ptree.isClosed());

			// check the content of the ptree
			checkPtree(ptree);

		}

	}

	template<typename Mesh>
	void checkMesh(const Mesh& mesh) {

		using namespace detail;

		checkPtree(mesh.getPartitionTree());

		EXPECT_EQ(10,(mesh.template getNumNodes<Vertex,0>()));
		EXPECT_EQ( 5,(mesh.template getNumNodes<Vertex,1>()));

		for(std::size_t i = 1; i < 9 ; i++) {
			NodeRef<Vertex,0> cur(i);
			NodeRef<Vertex,0> left(i-1);
			NodeRef<Vertex,0> right(i+1);

			const auto& neighbors = mesh.template getSinks<Edge>(cur);

			EXPECT_EQ(2,neighbors.size());
			EXPECT_EQ(left,neighbors.front()) << "i=" << cur;
			EXPECT_EQ(right,neighbors.back()) << "i=" << cur;
		}

		for(std::size_t i = 1; i < 4 ; i++) {
			NodeRef<Vertex,1> cur(i);
			NodeRef<Vertex,1> left(i-1);
			NodeRef<Vertex,1> right(i+1);

			const auto& neighbors = mesh.template getSinks<Edge>(cur);

			EXPECT_EQ(2,neighbors.size());
			EXPECT_EQ(left,neighbors.front()) << "i=" << cur;
			EXPECT_EQ(right,neighbors.back()) << "i=" << cur;
		}

		for(std::size_t i = 0; i < 10 ; i++) {
			NodeRef<Vertex,0> cur(i);
			NodeRef<Vertex,1> parent(i/2);

			EXPECT_EQ(parent,mesh.template getParent<Refine>(cur)) << "i=" << cur;
		}

		for(std::size_t i = 0; i < 5 ; i++) {
			NodeRef<Vertex,1> cur(i);
			NodeRef<Vertex,0> left(2*i);
			NodeRef<Vertex,0> right(2*i+1);

			const auto& children = mesh.template getChildren<Refine>(cur);

			EXPECT_EQ(2,children.size()) << "Invalid number of children of parent " << cur;
			if (!children.empty()) {
				EXPECT_EQ(left,children.front()) << "i=" << cur;
				EXPECT_EQ(right,children.back()) << "i=" << cur;
			}
		}


	}


	TEST(Mesh,IO) {

		// fix the type of the partition tree to be tested
		using Mesh = detail::plain_type<decltype(createBarMesh<2,2>(5))>;

		std::stringstream buffer(std::ios_base::in | std::ios_base::out | std::ios_base::binary);

		{ // -- creation --

			// create a mesh
			auto bar = createBarMesh<2,2>(5);

			// check the content of the mesh
			checkMesh(bar);

			// convert to buffer
			bar.store(buffer);

			// let mesh be destroyed
		}

		{ // -- reload mesh from a stream --

			// load from stream
			auto bar = Mesh::load(buffer);

			// check the content of the mesh
			checkMesh(bar);

		}

		{ // -- reload mesh using reinterpretation of in-memory buffer --

			// extract data buffer
			auto str = buffer.str();

			// interpret content
			utils::RawBuffer raw(const_cast<char*>(str.c_str()));
			auto bar = Mesh::interpret(raw);

			// check the content of the mesh
			checkMesh(bar);

		}

	}

	TEST(Mesh,Scan) {

		auto bar = createBarMesh<2,2>(5);

		// check the first level
		std::atomic<int> counter(0);
		std::atomic<uint32_t> mask(0);
		bar.pforAll<Vertex>([&](auto& cur){
			counter++;
			mask.fetch_or(1<<cur.id);
		});
		EXPECT_EQ(10,counter);
		EXPECT_EQ(1023,mask);

		// check the second level
		counter.store(0);
		mask.store(0);
		bar.pforAll<Vertex,1>([&](auto& cur){
			counter++;
			mask.fetch_or(1<<cur.id);
		});
		EXPECT_EQ(5,counter);
		EXPECT_EQ(31,mask);
	}

	TEST(Mesh, Wait) {

		auto bar = createBarMesh<2, 2>(5);

		// check the first level
		std::atomic<int> counter(0);
		std::atomic<uint32_t> mask(0);
		auto handle = bar.pforAll<Vertex>([&](auto& cur) {
			counter++;
			mask.fetch_or(1 << cur.id);
		});

		handle.wait();

		EXPECT_EQ(10, counter);
		EXPECT_EQ(1023, mask);

		// check the second level
		counter.store(0);
		mask.store(0);
		auto ref = bar.pforAll<Vertex, 1>([&](auto& cur) {
			counter++;
			mask.fetch_or(1 << cur.id);
		});

		ref.wait();

		EXPECT_EQ(5, counter);
		EXPECT_EQ(31, mask);
	}

	TEST(Mesh,Preduce) {

		auto bar = createBarMesh<2,2>(5);

		// check the first level
		std::atomic<int> counter(0);
		unsigned sum = bar.preduce<Vertex>(
				[&](const auto& a, auto& b) {
					counter++;
					b += (unsigned)(a.id);
				},
				[](unsigned a, unsigned b) {
					return a + b;
				}
		);
		EXPECT_EQ(10,counter);
		EXPECT_EQ(45,sum);

		// check the second level
		counter.store(0);
		unsigned res = bar.preduce<Vertex,1>(
				[&](const auto& a, auto& b) {
					counter++;
					b |= (1<<a.id);
				},
				[](unsigned a, unsigned b) {
					return a | b;
				}
		);
		EXPECT_EQ(5,counter);
		EXPECT_EQ(31,res);
	}


	TEST(Mesh,MeshDataDemo) {

		auto bar = createBarMesh<2,2>(50);

		auto visitCounters = bar.createNodeData<Vertex,int,0>();

		// initialize counters
		bar.pforAll<Vertex>([&](const auto& node){
			visitCounters[node] = 0;
		});

		// check that all elements are now 0
		for(std::size_t i=0; i<bar.getNumNodes<Vertex>(); ++i) {
			EXPECT_EQ(0,visitCounters[NodeRef<Vertex>((node_index_t)i)]);
		}

		// increment all of those
		bar.pforAll<Vertex>([&](const auto& node){
			visitCounters[node]++;
		});

		for(std::size_t i=0; i<bar.getNumNodes<Vertex>(); ++i) {
			EXPECT_EQ(1,visitCounters[NodeRef<Vertex>((node_index_t)i)]);
		}

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
		const auto f = mb.create<Face>();

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
		EXPECT_EQ(bl, m.getNeighbor<BoundaryFace2Cell>(a));

		EXPECT_EQ(a, m.getSink<BoundaryFace2Cell>(bl));
		EXPECT_EQ(bl, m.getSource<BoundaryFace2Cell>(a));

		EXPECT_EQ(2, m.getNeighbors<Face2Cell>(f).size());
		EXPECT_EQ(2, m.getSinks<Face2Cell>(f).size());

		EXPECT_EQ(f, m.getNeighbor<Face2Cell>(a));
		EXPECT_EQ(f, m.getSource<Face2Cell>(a));

		EXPECT_EQ(b, m.getNeighbor<BoundaryFace2Cell>(br));
		EXPECT_EQ(br, m.getNeighbor<BoundaryFace2Cell>(b));

		EXPECT_EQ(b, m.getSink<BoundaryFace2Cell>(br));
		EXPECT_EQ(br, m.getSource<BoundaryFace2Cell>(b));


		// check node sets
		EXPECT_EQ(2,m.getNumNodes<Cell>());
		EXPECT_EQ(1,m.getNumNodes<Face>());
		EXPECT_EQ(2,m.getNumNodes<BoundaryFace>());

		EXPECT_EQ(2, (m.createNodeData<Cell,double>().size()));
		EXPECT_EQ(1, (m.createNodeData<Face,double>().size()));
		EXPECT_EQ(2, (m.createNodeData<BoundaryFace,double>().size()));

		m.createNodeDataArray<Cell, double, 2, 0>();

		std::vector<NodeRef<Cell, 0>> cells;
		m.getPartitionTree().template getNodeRange<Cell, 0>().forAll([&](const auto& c) { cells.push_back(c);});
		EXPECT_EQ(2, cells.size());

		// check forAll and pforAll support
		std::atomic<int> counter(0);
		m.pforAll<Cell>([&](auto) { counter++; });
		EXPECT_EQ(2,counter);

		m.pforAll<Cell,0>([&](const auto& cell){
			m.template getSources<Face2Cell>(cell);
		});

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
		EXPECT_EQ(1, m.getSinks<Cell2Cell>(l0a).size());
		EXPECT_EQ(2, m.getSinks<Cell2Cell>(l0b).size());
		EXPECT_EQ(2, m.getSinks<Cell2Cell>(l0c).size());
		EXPECT_EQ(1, m.getSinks<Cell2Cell>(l0d).size());

		EXPECT_EQ(l0b, m.getSink<Cell2Cell>(l0a));
		EXPECT_EQ(l0c, m.getSink<Cell2Cell>(l0d));

		EXPECT_EQ(1, m.getSinks<Cell2Cell>(l1a).size());
		EXPECT_EQ(1, m.getSinks<Cell2Cell>(l1b).size());

		EXPECT_EQ(l1b, m.getSink<Cell2Cell>(l1a));
		EXPECT_EQ(l1a, m.getSink<Cell2Cell>(l1b));


		// check reverse look-ups
		EXPECT_EQ(1, m.getSources<Cell2Cell>(l0a).size());
		EXPECT_EQ(2, m.getSources<Cell2Cell>(l0b).size());
		EXPECT_EQ(2, m.getSources<Cell2Cell>(l0c).size());
		EXPECT_EQ(1, m.getSources<Cell2Cell>(l0d).size());

		EXPECT_EQ(l0b, m.getSource<Cell2Cell>(l0a));
		EXPECT_EQ(l0c, m.getSource<Cell2Cell>(l0d));

		EXPECT_EQ(1, m.getSources<Cell2Cell>(l1a).size());
		EXPECT_EQ(1, m.getSources<Cell2Cell>(l1b).size());

		EXPECT_EQ(l1b, m.getSource<Cell2Cell>(l1a));
		EXPECT_EQ(l1a, m.getSource<Cell2Cell>(l1b));


		// check parent/child relations
		EXPECT_EQ(2, m.getChildren<Cell2Child>(l1a).size());
		EXPECT_EQ(2, m.getChildren<Cell2Child>(l1b).size());

		EXPECT_EQ(std::vector<decltype(l0a)>({ l0a,l0b }), m.getChildren<Cell2Child>(l1a));
		EXPECT_EQ(std::vector<decltype(l0a)>({ l0c,l0d }), m.getChildren<Cell2Child>(l1b));

		EXPECT_EQ(2, m.getChildren<Cell2Child>(l2a).size());
		EXPECT_EQ(std::vector<decltype(l1a)>({ l1a,l1b }), m.getChildren<Cell2Child>(l2a));

	}


	TEST(MeshData,IO) {

		std::stringstream buffer(std::ios_base::in | std::ios_base::out | std::ios_base::binary);

		// create a mesh
		auto bar = createBarMesh<2,2>(50);

		{ // -- creation --

			// create some mesh data
			auto data = bar.createNodeData<Vertex,int>();

			// fill in some property data
			int c = 0;
			bar.forAll<Vertex>([&](const NodeRef<Vertex>& node){
				data[node] = c;
				c++;
			});

			// convert to buffer
			data.store(buffer);

			// let mesh data be destroyed
		}

		{ // -- reload mesh data from a stream --

			// load from stream
			auto data = bar.loadNodeData<Vertex,int>(buffer);

			// check the content of the restored properties
			int c = 0;
			bar.forAll<Vertex>([&](const NodeRef<Vertex>& node){
				EXPECT_EQ(c,data[node]);
				c++;
			});

		}

		{ // -- reload mesh node data using reinterpretation of in-memory buffer --

			// extract data buffer
			auto str = buffer.str();

			// interpret content
			utils::RawBuffer raw(const_cast<char*>(str.c_str()));
			auto data = bar.interpretNodeData<Vertex,int>(raw);

			// check the content of the restored data field
			int c = 0;
			bar.forAll<Vertex>([&](const NodeRef<Vertex>& node){
				EXPECT_EQ(c,data[node]);
				c++;
			});

		}

	}

#ifndef _MSC_VER

	TEST(MeshProperties,Basic) {

		struct PropertyA : public mesh_property<Vertex,int> {};
		struct PropertyB : public mesh_property<Vertex,int> {};

		auto bar = createBarMesh<2,2>(50);

		auto props = bar.createProperties<PropertyA,PropertyB>();


		auto& propA0 = props.get<PropertyA,0>();
		auto& propA1 = props.get<PropertyA,1>();

		auto& propB0 = props.get<PropertyB,0>();
		auto& propB1 = props.get<PropertyB,1>();


		// check that they are different
		EXPECT_NE((void*)&propA0,(void*)&propA1);
		EXPECT_NE((void*)&propB0,(void*)&propB1);

		// test-usage of properties

		bar.pforAll<Vertex,0>([&](const auto& cur){
			propA0[cur] = 12;
			propB0[cur] = 18;
		});

		bar.pforAll<Vertex,1>([&](const auto& cur){
			propA1[cur] = 14;
			propB1[cur] = 16;
		});


		bar.pforAll<Vertex,0>([&](const auto& cur){
			EXPECT_EQ(12,propA0[cur]);
			EXPECT_EQ(18,propB0[cur]);
		});

		bar.pforAll<Vertex,1>([&](const auto& cur){
			EXPECT_EQ(14,propA1[cur]);
			EXPECT_EQ(16,propB1[cur]);
		});

	}

	TEST(MeshProperties,IO) {

		struct PropertyA : public mesh_property<Vertex,int> {};
		struct PropertyB : public mesh_property<Vertex,double> {};

		std::stringstream buffer(std::ios_base::in | std::ios_base::out | std::ios_base::binary);

		// create a mesh
		auto bar = createBarMesh<2,2>(50);

		{ // -- creation --

			// create some properties
			auto props = bar.createProperties<PropertyA,PropertyB>();

			// fill in some property data
			int c = 0;
			bar.forAll<Vertex>([&](const NodeRef<Vertex>& node){
				props.get<PropertyA>()[node] = c;
				props.get<PropertyB>()[node] = c + 0.5;
				c++;
			});

			// convert to buffer
			props.store(buffer);

			// let properties be destroyed
		}

		{ // -- reload properties from a stream --

			// load from stream
			auto props = bar.loadProperties<PropertyA,PropertyB>(buffer);

			// check the content of the restored properties
			int c = 0;
			bar.forAll<Vertex>([&](const NodeRef<Vertex>& node){
				EXPECT_EQ(c,props.get<PropertyA>()[node]);
				EXPECT_DOUBLE_EQ(c + 0.5,props.get<PropertyB>()[node]);
				c++;
			});

		}

		{ // -- reload mesh properties using reinterpretation of in-memory buffer --

			// extract data buffer
			auto str = buffer.str();

			// interpret content
			utils::RawBuffer raw(const_cast<char*>(str.c_str()));
			auto props = bar.interpretProperties<PropertyA,PropertyB>(raw);

			// check the content of the restored properties
			int c = 0;
			bar.forAll<Vertex>([&](const NodeRef<Vertex>& node){
				EXPECT_EQ(c,props.get<PropertyA>()[node]);
				EXPECT_DOUBLE_EQ(c + 0.5,props.get<PropertyB>()[node]);
				c++;
			});

		}

	}

#endif

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale

