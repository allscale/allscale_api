
template<typename Region>
void checkParameters(const Region& a, const Region& b) {

	using allscale::api::core::is_region;
	using allscale::api::core::isSubRegion;
	
	// very first check: syntax
	static_assert(is_region<Region>::value, "Provided region does not fit region concept!");
	
	// check input parameters
	ASSERT_FALSE(a.empty()) << "Require first parameter to be non-empty: " << a;
	ASSERT_FALSE(b.empty()) << "Require second parameter to be non-empty: " << b;
	ASSERT_NE(a,b) << "Requires parameters to be not equivalent.";

	ASSERT_FALSE(isSubRegion(a,b));
	ASSERT_FALSE(isSubRegion(b,a));
	
	// compute intersection
	Region c = Region::intersect(a,b);
	ASSERT_FALSE(c.empty());
}


template<typename Region>
void testRegion(const Region& a, const Region& b) {
	
	using allscale::api::core::isSubRegion;
	
	// check whether parameters are two distinct set with a non-empty common sub-set
	checkParameters(a,b);
	
	// compute intersection
	Region c = Region::intersect(a,b);
	Region d = Region::merge(a,b);
	
	// so, we have: 
	//		region a and b -- neither a subset of the other
	//		region c - a non-empty sub-set of region a and b
	//      region d - the union of a and b
	
			
	// test semantic
	Region e;
	EXPECT_TRUE(e.empty());

	
	// check sub-region relation
	
	EXPECT_TRUE(isSubRegion(e,e));
	EXPECT_TRUE(isSubRegion(a,a));
	EXPECT_TRUE(isSubRegion(b,b));
	EXPECT_TRUE(isSubRegion(c,c));
	EXPECT_TRUE(isSubRegion(d,d));
	
	EXPECT_TRUE(isSubRegion(e,a));
	EXPECT_TRUE(isSubRegion(e,b));
	EXPECT_TRUE(isSubRegion(e,c));
	EXPECT_TRUE(isSubRegion(e,d));
	
	EXPECT_FALSE(isSubRegion(a,e));
	EXPECT_FALSE(isSubRegion(b,e));
	EXPECT_FALSE(isSubRegion(c,e));
	EXPECT_FALSE(isSubRegion(d,e));
	
	EXPECT_FALSE(isSubRegion(a,c));
	EXPECT_FALSE(isSubRegion(b,c));
	EXPECT_FALSE(isSubRegion(d,c));
	
	EXPECT_FALSE(isSubRegion(d,a));
	EXPECT_FALSE(isSubRegion(d,b));
		
	EXPECT_TRUE(isSubRegion(c,a));
	EXPECT_TRUE(isSubRegion(c,b));
	EXPECT_TRUE(isSubRegion(a,d));
	EXPECT_TRUE(isSubRegion(b,d));
	
	
	// check equivalencis
	EXPECT_EQ(e, Region::difference(e,e));
	EXPECT_EQ(e, Region::difference(a,a));
	EXPECT_EQ(e, Region::difference(a,d));
	EXPECT_EQ(e, Region::difference(b,b));
	EXPECT_EQ(e, Region::difference(b,d));
	
	EXPECT_EQ(Region::difference(b,a), Region::difference(b,c)) << "a=" << a << "\nb=" << b << "\nc=" << c;
	EXPECT_EQ(Region::difference(a,b), Region::difference(a,c)) << "a=" << a << "\nb=" << b << "\nc=" << c;
		
}


template<typename Fragment>
void testFragment(const typename Fragment::shared_data_type& shared, const typename Fragment::region_type& a, const typename Fragment::region_type& b) {
	
	using Region = typename Fragment::region_type;
	
	// check whether parameters are two distinct set with a non-empty common sub-set
	checkParameters(a,b);
	
	// compute empty region, intersection, and union of a and b
	Region e;	
	Region c = Region::intersect(a,b);
	Region d = Region::merge(a,b);
	
	
	// create a few fragments
	Fragment empty(shared,e);
	EXPECT_EQ(e, empty.getCoveredRegion());
	
	Fragment fA(shared,a);
	EXPECT_EQ(a, fA.getCoveredRegion());
	
	Fragment fB(shared,b);
	EXPECT_EQ(b, fB.getCoveredRegion());
	
	// create an empty fragment
	Fragment tmp(shared,e);
	EXPECT_EQ(e, tmp.getCoveredRegion());
	
	// resize fragment to size of c
	tmp.resize(c);
	EXPECT_EQ(c, tmp.getCoveredRegion());
	
	// load c-share for fA into tmp
	tmp.insertRegion(fA,c);	
	EXPECT_EQ(c, tmp.getCoveredRegion());
	
	// resize tmp to d
	tmp.resize(d);
	EXPECT_EQ(d, tmp.getCoveredRegion());
	
	// load fA and fB into tmp
	tmp.insertRegion(fA, Region::difference(a,c));
	tmp.insertRegion(fB, Region::difference(b,a));
	
	auto facada = fA.mask();
}


template<typename Fragment>
void testFragment(const typename Fragment::region_type& a, const typename Fragment::region_type& b) {
	testFragment<Fragment>(typename Fragment::shared_data_type(), a, b);
}


template<typename Fragment, typename Region>
utils::Archive extract(const Fragment& fragment, const Region& region) {
	utils::ArchiveWriter writer;
	fragment.extract(writer,region);
	return std::move(writer).toArchive();
}

template<typename Fragment>
void insert(Fragment& fragment, const utils::Archive& archive) {
	utils::ArchiveReader reader(archive);
	fragment.insert(reader);
}

