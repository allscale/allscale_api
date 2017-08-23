#include <gtest/gtest.h>

#include "allscale/api/user/data/adaptive_grid.h"

#include "allscale/utils/printer/vectors.h"
#include "allscale/utils/printer/pairs.h"
#include "allscale/utils/static_grid.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	#include "data_item_test.inl"

	// test config frequently used throughout this test
	using FourLayerCellConfig = CellConfig<layers<layer<5, 5>, layer<2, 2>, layer<3, 3>>>;

	TEST(AdaptiveGridCell, TypeProperties) {

		using CellType = AdaptiveGridCell<double, FourLayerCellConfig>;

		EXPECT_TRUE(std::is_default_constructible<CellType>::value);

		EXPECT_TRUE(std::is_move_constructible<CellType>::value);

		EXPECT_TRUE(std::is_move_assignable<CellType>::value);

		EXPECT_TRUE((utils::is_serializable<AdaptiveGridCell<int,FourLayerCellConfig>>::value));

	}

	TEST(AdaptiveGridCell, ActiveLayers) {

		AdaptiveGridCell<double, FourLayerCellConfig> cell;
		int cellCount;

		EXPECT_EQ(0, cell.getActiveLayer());
		cellCount = 0;
		cell.forAllActiveNodes([&cellCount](const double&) { cellCount++; });
		EXPECT_EQ(5*5*2*2*3*3, cellCount);

		cell.setActiveLayer(1);
		EXPECT_EQ(1, cell.getActiveLayer());
		cellCount = 0;
		cell.forAllActiveNodes([&cellCount](const double&) { cellCount++; });
		EXPECT_EQ(5*5*2*2, cellCount);

		cell.setActiveLayer(2);
		EXPECT_EQ(2, cell.getActiveLayer());
		cellCount = 0;
		cell.forAllActiveNodes([&cellCount](const double&) { cellCount++; });
		EXPECT_EQ(5*5, cellCount);

		cell.setActiveLayer(3);
		EXPECT_EQ(3, cell.getActiveLayer());
		cellCount = 0;
		cell.forAllActiveNodes([&cellCount](const double&) { cellCount++; });
		EXPECT_EQ(1, cellCount);


	}

	TEST(AdaptiveGridCell, RefinementCoarsening) {

		AdaptiveGridCell<int, FourLayerCellConfig> cell;

		cell.setActiveLayer(3);
		EXPECT_EQ(3, cell.getActiveLayer());
		cell.forAllActiveNodes([](int& element) { element = 3; });

		cell.refine([](const int& element) { return element * 5; });

		EXPECT_EQ(2, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(15, element); });

		cell.refine([](const int& element) { return element * 10; });
		EXPECT_EQ(1, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(150, element); });

		cell.refine([](const int& element) { return element / 2; });
		EXPECT_EQ(0, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(75, element); });

		cell.coarsen([](const int& element) { return element / 5; });
		EXPECT_EQ(1, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(15, element); });

		cell.coarsen([](const int& element) { return element / 3; });
		EXPECT_EQ(2, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(5, element); });

		cell.coarsen([](const int&) { return 0; });
		EXPECT_EQ(3, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(0, element); });

	}

	TEST(AdaptiveGridCell, RefinementCoarseningGrid) {

		AdaptiveGridCell<int, FourLayerCellConfig> cell;

		cell.setActiveLayer(3);
		EXPECT_EQ(3, cell.getActiveLayer());
		cell.forAllActiveNodes([](int& element) { element = 3; });

		cell.refineGrid([](const int& element) {
			utils::StaticGrid<int, 5, 5> newGrid;
			newGrid.forEach([element](auto& e) { e = element * 5; });
			return newGrid;
		});

		EXPECT_EQ(2, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(15, element); });

		cell.refineGrid([](const int& element) { 			
			utils::StaticGrid<int, 2, 2> newGrid;
			newGrid.forEach([element](auto& e) { e = element * 10; });
			return newGrid; 
		});
		EXPECT_EQ(1, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(150, element); });
		
		cell.refineGrid([](const int& element) {
			utils::StaticGrid<int, 3, 3> newGrid;
			newGrid.forEach([element](auto& e) { e = element / 2; });
			return newGrid;
		});
		EXPECT_EQ(0, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(75, element); });

		cell.coarsenGrid([](const auto& grid) {
			int res = 0;
			grid.forEach([&](const int& element) { res += element / 5; });
			return res / (int)(grid.size().x * grid.size().y);
		});
		EXPECT_EQ(1, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(15, element); });

		cell.coarsenGrid([](const auto& grid) {
			int res = 0;
			grid.forEach([&](const int& element) { res += element / 3; });
			return res / (int)(grid.size().x * grid.size().y);
		});		
		EXPECT_EQ(2, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(5, element); });

		cell.coarsenGrid([](__unused const auto& grid) {
			return 0;
		});		
		EXPECT_EQ(3, cell.getActiveLayer());
		cell.forAllActiveNodes([](const int& element) { EXPECT_EQ(0, element); });

	}

	TEST(AdaptiveGridCell, BoundaryExchange) {

		using SpecialLayerCellConfig = CellConfig<layers<layer<2, 3>, layer<2, 5>>>;

		AdaptiveGridCell<int, SpecialLayerCellConfig> cell;

		auto& cellGrid = cell.getLayer<0>();

		int count = 0;
		cellGrid.forEach([&count](auto& element) {
			element = count++;
		});
		cell.setActiveLayer(0);

		cell.forAllActiveNodes([](int& element) { element = 1; });
		const std::vector<int> xRef(2*2, 1);
		const std::vector<int> yRef(3*5, 1);

		auto leftBoundary = cell.getBoundary(Direction::Left);
		ASSERT_EQ(3*5, leftBoundary.size());
		EXPECT_EQ(yRef, leftBoundary);

		auto rightBoundary = cell.getBoundary(Direction::Right);
		ASSERT_EQ(3*5, rightBoundary.size());
		EXPECT_EQ(yRef, rightBoundary);

		auto upperBoundary = cell.getBoundary(Direction::Up);
		ASSERT_EQ(2*2, upperBoundary.size());
		EXPECT_EQ(xRef, upperBoundary);
		
		auto lowerBoundary = cell.getBoundary(Direction::Down);
		ASSERT_EQ(2*2, lowerBoundary.size());
		EXPECT_EQ(xRef, lowerBoundary);

		using rT = std::vector<int>;

		const std::vector<int> xUpdate(2*2, 5);
		const std::vector<int> yUpdate(3*5, 5);

		cell.setBoundary(Direction::Left, yUpdate);
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 5,1,1,1 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 5,1,1,1 }), cell.getBoundary(Direction::Down));

		cell.setBoundary(Direction::Right, yUpdate);
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 5,1,1,5 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 5,1,1,5 }), cell.getBoundary(Direction::Down));

		cell.setBoundary(Direction::Up, xUpdate);
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 5,5,5,5 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 5,1,1,5 }), cell.getBoundary(Direction::Down));

		cell.setBoundary(Direction::Down, xUpdate);
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 5,5,5,5 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 5,5,5,5 }), cell.getBoundary(Direction::Down));

		int sum = 0;
		cell.forAllActiveNodes([&sum](const int& element) { sum += element; });
		EXPECT_EQ(196, sum);

		// check different layer
		cell.setActiveLayer(1);

		cell.forAllActiveNodes([](int& element) { element = 0; });

		cell.setBoundary(Direction::Left, (rT{ 1, 2, 3 }));
		EXPECT_EQ((rT{ 1, 2, 3 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 0, 0, 0 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 3, 0 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 1, 0 }), cell.getBoundary(Direction::Down));

		cell.setBoundary(Direction::Right, (rT{ 4, 5, 6 }));
		EXPECT_EQ((rT{ 1, 2, 3 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 4, 5, 6 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 3, 6 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 1, 4 }), cell.getBoundary(Direction::Down));

		cell.setBoundary(Direction::Up, (rT{ 7, 8 }));
		EXPECT_EQ((rT{ 1, 2, 7 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 4, 5, 8 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 7, 8 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 1, 4 }), cell.getBoundary(Direction::Down));

		cell.setBoundary(Direction::Down, (rT{ 9, 10 }));
		EXPECT_EQ((rT{ 9, 2, 7 }), cell.getBoundary(Direction::Left));
		EXPECT_EQ((rT{ 10, 5, 8 }), cell.getBoundary(Direction::Right));
		EXPECT_EQ((rT{ 7, 8 }), cell.getBoundary(Direction::Up));
		EXPECT_EQ((rT{ 9, 10 }), cell.getBoundary(Direction::Down));
		
	}

	TEST(AdaptiveGridCell, LoadStore) {

		using TwoLayerCellConfig = CellConfig<layers<layer<2, 2>>>;
		using CellType = AdaptiveGridCell<double, TwoLayerCellConfig>;

		CellType aGrid;

		aGrid.setActiveLayer(0);
		aGrid.forAllActiveNodes([](double& element) {
			element = 2.0;
		});

		aGrid.setActiveLayer(1);
		aGrid.forAllActiveNodes([](double& element) {
			element = 3.0;
		});

		utils::Archive safe = utils::serialize(aGrid);

		CellType bGrid = utils::deserialize<CellType>(safe);

		EXPECT_EQ(1, bGrid.getActiveLayer());
		bGrid.forAllActiveNodes([](double& element) {
			EXPECT_EQ(3.0, element);
		});

		bGrid.setActiveLayer(0);
		bGrid.forAllActiveNodes([](double& element) {
			EXPECT_EQ(2.0, element);
		});
	}
	
	TEST(AdaptiveGridRegion, TypeProperties) {

		using namespace detail;

		EXPECT_TRUE(utils::is_value<AdaptiveGridRegion<1>>::value);
		EXPECT_TRUE(utils::is_value<AdaptiveGridRegion<2>>::value);
		EXPECT_TRUE(utils::is_value<AdaptiveGridRegion<3>>::value);

		EXPECT_TRUE(utils::is_serializable<AdaptiveGridRegion<1>>::value);
		EXPECT_TRUE(utils::is_serializable<AdaptiveGridRegion<2>>::value);
		EXPECT_TRUE(utils::is_serializable<AdaptiveGridRegion<3>>::value);

		EXPECT_TRUE(core::is_region<AdaptiveGridRegion<1>>::value);
		EXPECT_TRUE(core::is_region<AdaptiveGridRegion<2>>::value);
		EXPECT_TRUE(core::is_region<AdaptiveGridRegion<3>>::value);

	}

	TEST(AdaptiveGridRegion, LoadStore) {

		AdaptiveGridRegion<1> a(5, 10);
		AdaptiveGridRegion<1> b(8, 14);

		EXPECT_NE(a, b);

		// serialize
		auto aa = utils::serialize(a);
		auto ab = utils::serialize(b);

		// restore value
		auto a2 = utils::deserialize<AdaptiveGridRegion<1>>(aa);
		auto b2 = utils::deserialize<AdaptiveGridRegion<1>>(ab);

		EXPECT_EQ(a, a2);
		EXPECT_EQ(b, b2);

	}
	
	TEST(AdaptiveGridRegion1D, RegionTestBasic) {

		AdaptiveGridRegion<1> a(5, 10);
		AdaptiveGridRegion<1> b(8, 14);
		testRegion(a, b);

		a = AdaptiveGridRegion<1>(7, 10);
		b = AdaptiveGridRegion<1>(6, 8);
		testRegion(a, b);
	}

	TEST(AdaptiveGridRegion2D, RegionTestBasic) {

		AdaptiveGridRegion<2> a(5, 10);
		AdaptiveGridRegion<2> b(8, 14);
		testRegion(a, b);

		// mirrored
		a = AdaptiveGridRegion<2>(8, 14);
		b = AdaptiveGridRegion<2>(5, 10);
		testRegion(a, b);

		// rotated left
		a = AdaptiveGridRegion<2>(5, 10);
		b = AdaptiveGridRegion<2>({ 6,3 }, { 12,8 });
		testRegion(a, b);

		// rotated right
		a = AdaptiveGridRegion<2>({ 6,3 }, { 12,8 });
		b = AdaptiveGridRegion<2>(5, 10);
		testRegion(a, b);

		// the cross
		a = AdaptiveGridRegion<2>({ 4,2 }, { 10,12 });
		b = AdaptiveGridRegion<2>({ 2,4 }, { 12,10 });
		testRegion(a, b);

	}

	TEST(AdaptiveGridFragment, Basic) {

		EXPECT_TRUE((core::is_fragment<AdaptiveGridFragment<double, FourLayerCellConfig, 2>>::value));

		AdaptiveGridPoint<2> size = { 30,50 };

		AdaptiveGridRegion<2> region(20, 30);
		AdaptiveGridFragment<int, FourLayerCellConfig, 2> fA({size},region);

	}

	TEST(AdaptiveGridFragment, TypeProperties) {

		using namespace detail;

		EXPECT_TRUE((core::is_fragment<AdaptiveGridFragment<int, FourLayerCellConfig, 1>>::value));
		EXPECT_TRUE((core::is_fragment<AdaptiveGridFragment<int, FourLayerCellConfig, 2>>::value));
		EXPECT_TRUE((core::is_fragment<AdaptiveGridFragment<int, FourLayerCellConfig, 3>>::value));

	}

	TEST(AdaptiveGridFragment1D, FragmentTestBasic) {

		AdaptiveGridPoint<1> size = 50;

		AdaptiveGridRegion<1> a(5, 10);
		AdaptiveGridRegion<1> b(8, 14);

		testFragment<AdaptiveGridFragment<int, FourLayerCellConfig, 1>>({ size }, a, b);

	}

	TEST(AdaptiveGridFragment2D, FragmentTestBasic) {

		AdaptiveGridPoint<2> size = { 50,60 };

		AdaptiveGridRegion<2> a({ 5,6 }, { 10,12 });
		AdaptiveGridRegion<2> b({ 8,9 }, { 14,16 });

		testFragment<AdaptiveGridFragment<int, FourLayerCellConfig, 2>>({ size }, a, b);

	}

	TEST(AdaptiveGridFragment1D, ExtractInsert) {

		AdaptiveGridPoint<1> size = 50;

		AdaptiveGridRegion<1> full(0, 50);
		AdaptiveGridRegion<1> a(5, 10);
		AdaptiveGridRegion<1> b(8, 14);

		AdaptiveGridSharedData<1> shared{ size };
		AdaptiveGridFragment<int, FourLayerCellConfig, 1> src(shared);
		AdaptiveGridFragment<int, FourLayerCellConfig, 1> dst1(shared);
		AdaptiveGridFragment<int, FourLayerCellConfig, 1> dst2(shared);

		EXPECT_TRUE(src.getCoveredRegion().empty());
		EXPECT_TRUE(dst1.getCoveredRegion().empty());
		EXPECT_TRUE(dst2.getCoveredRegion().empty());

		// fix some sizes
		src.resize(full);
		dst1.resize(a);
		dst2.resize(b);

		EXPECT_EQ(src.getCoveredRegion(), full);
		EXPECT_EQ(dst1.getCoveredRegion(), a);
		EXPECT_EQ(dst2.getCoveredRegion(), b);

		// fill in some data
		auto dataSrc = src.mask();
		full.scan([&](const AdaptiveGridPoint<1>& p) {
			dataSrc[p].setActiveLayer(2);
			dataSrc[p].forAllActiveNodes([p](int& element) {
				element = (int)p[0] * 2;
			});
		});

		// now, extract data
		auto aa = extract(src, a);
		auto ab = extract(src, b);

		// insert data in destinations
		insert(dst1, aa);
		insert(dst2, ab);

		// check the content
		int count = 0;
		a.scan([&](const AdaptiveGridPoint<1>& p) {
			EXPECT_EQ(2, dst1[p].getActiveLayer());
			dst1[p].forAllActiveNodes([p](const int& element) {
				EXPECT_EQ(p[0] * 2, element);
			});
			count++;
		});
		EXPECT_EQ(a.area(), count);

		count = 0;
		b.scan([&](const AdaptiveGridPoint<1>& p) {
			EXPECT_EQ(2, dst2[p].getActiveLayer());
			dst2[p].forAllActiveNodes([p](const int& element) {
				EXPECT_EQ(p[0] * 2, element);
			});
			count++;
		});
		EXPECT_EQ(b.area(), count);

		// those insertions should fail, since area is not covered
		EXPECT_DEBUG_DEATH(insert(dst1, ab), ".*Targeted fragment does not cover data to be inserted!.*");
		EXPECT_DEBUG_DEATH(insert(dst2, aa), ".*Targeted fragment does not cover data to be inserted!.*");
	}


	TEST(AdaptiveGridFragment2D, ExtractInsert) {

		AdaptiveGridPoint<2> size = { 50,60 };

		AdaptiveGridRegion<2> full({ 0,0 }, { 50,60 });
		AdaptiveGridRegion<2> a({ 5,6 }, { 10,12 });
		AdaptiveGridRegion<2> b({ 8,9 }, { 14,16 });

		AdaptiveGridSharedData<2> shared{ size };
		AdaptiveGridFragment<int, FourLayerCellConfig, 2> src(shared);
		AdaptiveGridFragment<int, FourLayerCellConfig, 2> dst1(shared);
		AdaptiveGridFragment<int, FourLayerCellConfig, 2> dst2(shared);

		EXPECT_TRUE(src.getCoveredRegion().empty());
		EXPECT_TRUE(dst1.getCoveredRegion().empty());
		EXPECT_TRUE(dst2.getCoveredRegion().empty());

		// fix some sizes
		src.resize(full);
		dst1.resize(a);
		dst2.resize(b);

		EXPECT_EQ(src.getCoveredRegion(), full);
		EXPECT_EQ(dst1.getCoveredRegion(), a);
		EXPECT_EQ(dst2.getCoveredRegion(), b);

		// fill in some data
		auto dataSrc = src.mask();
		full.scan([&](const AdaptiveGridPoint<2>& p) {
			dataSrc[p].setActiveLayer(2);
			dataSrc[p].forAllActiveNodes([p](int& element) {
				element = (int)p[0] * 2;
			});
		});


		// now, extract data
		auto aa = extract(src, a);
		auto ab = extract(src, b);

		// insert data in destinations
		insert(dst1, aa);
		insert(dst2, ab);

		// check the content
		int count = 0;
		a.scan([&](const AdaptiveGridPoint<2>& p) {
			EXPECT_EQ(2, dst1[p].getActiveLayer());
			dst1[p].forAllActiveNodes([p](const int& element) {
				EXPECT_EQ(p[0] * 2, element);
			});
			count++;
		});
		EXPECT_EQ(a.area(), count);

		count = 0;
		b.scan([&](const AdaptiveGridPoint<2>& p) {
			EXPECT_EQ(2, dst2[p].getActiveLayer());
			dst2[p].forAllActiveNodes([p](const int& element) {
				EXPECT_EQ(p[0] * 2, element);
			});
			count++;
		});
		EXPECT_EQ(b.area(), count);

		// those insertions should fail, since area is not covered
		EXPECT_DEBUG_DEATH(insert(dst1, ab), ".*Targeted fragment does not cover data to be inserted!.*");
		EXPECT_DEBUG_DEATH(insert(dst2, aa), ".*Targeted fragment does not cover data to be inserted!.*");
	}

	TEST(AdaptiveGrid,TypeProperties) {

		EXPECT_TRUE((core::is_data_item<AdaptiveGrid<int,FourLayerCellConfig,1>>::value));
		EXPECT_TRUE((core::is_data_item<AdaptiveGrid<int,FourLayerCellConfig,2>>::value));
		EXPECT_TRUE((core::is_data_item<AdaptiveGrid<int,FourLayerCellConfig,3>>::value));

	}

	TEST(AdaptiveGrid, Size) {

		AdaptiveGrid<int,FourLayerCellConfig,2> grid({10,20});

		EXPECT_EQ("[10,20]",toString(grid.size()));


	}

	TEST(AdaptiveGrid, OneLayer) {

		using OneLayerCellConfig = CellConfig<layers<>>;

		AdaptiveGrid<int, OneLayerCellConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, OneLayerCellConfig>& cell) {

			EXPECT_EQ(0, cell.getActiveLayer());

			EXPECT_EQ((AdaptiveGridPoint<2>{1, 1}), cell.getLayer<0>().size());

		});

	}

	TEST(AdaptiveGrid, TwoLayers) {

		using TwoLayerCellConfig = CellConfig<layers<layer<2, 2>>>;

		AdaptiveGrid<int, TwoLayerCellConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, TwoLayerCellConfig>& cell) {

			EXPECT_EQ(0, cell.getActiveLayer());

			EXPECT_EQ((AdaptiveGridPoint<2>{2, 2}), cell.getLayer<0>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{1, 1}), cell.getLayer<1>().size());

			cell.setActiveLayer(1);
			EXPECT_EQ(1, cell.getActiveLayer());

		});

	}

	TEST(AdaptiveGrid, ThreeLayers) {

		using ThreeLayerCellConfig = CellConfig<layers<layer<2, 2>, layer<3, 3>>>;

		AdaptiveGrid<int, ThreeLayerCellConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, ThreeLayerCellConfig>& cell) {

			EXPECT_EQ(0, cell.getActiveLayer());

			EXPECT_EQ((AdaptiveGridPoint<2>{6, 6}), cell.getLayer<0>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{2, 2}), cell.getLayer<1>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{1, 1}), cell.getLayer<2>().size());

			cell.setActiveLayer(2);
			EXPECT_EQ(2, cell.getActiveLayer());

		});

	}

	TEST(AdaptiveGrid, FourLayers) {

		AdaptiveGrid<int, FourLayerCellConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, FourLayerCellConfig>& cell) {

			EXPECT_EQ(0, cell.getActiveLayer());

			EXPECT_EQ((AdaptiveGridPoint<2>{30, 30}), cell.getLayer<0>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{10, 10}), cell.getLayer<1>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{5, 5}), cell.getLayer<2>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{1, 1}), cell.getLayer<3>().size());

			cell.setActiveLayer(3);
			EXPECT_EQ(3, cell.getActiveLayer());

		});

	}

	TEST(AdaptiveGrid, FiveLayers) {

		using FiveLayerCellConfig = CellConfig<layers<layer<2, 2>, layer<3, 3>, layer<4, 4>, layer<5, 5>>>;

		AdaptiveGrid<int, FiveLayerCellConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, FiveLayerCellConfig>& cell) {

			EXPECT_EQ(0, cell.getActiveLayer());

			EXPECT_EQ((AdaptiveGridPoint<2>{120, 120}), cell.getLayer<0>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{24, 24}), cell.getLayer<1>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{6, 6}), cell.getLayer<2>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{2, 2}), cell.getLayer<3>().size());
			EXPECT_EQ((AdaptiveGridPoint<2>{1, 1}), cell.getLayer<4>().size());

			cell.setActiveLayer(4);
			EXPECT_EQ(4, cell.getActiveLayer());

		});

	}
	
 	TEST(AdaptiveGrid, Refinement) {
 
 		AdaptiveGrid<int, FourLayerCellConfig, 2> aGrid({ 2,2 });
 
 		aGrid.forEach([](AdaptiveGridCell<int, FourLayerCellConfig>& cell) {
 
 			cell.setActiveLayer(3);
 
 			cell.forAllActiveNodes([](int& element) { element = 5; });
 
 			cell.refine([&](const int& element) {
 				return element * 2;
 			});
 
 			ASSERT_EQ(2, cell.getActiveLayer());
 
 			cell.forAllActiveNodes([](const int& element) { ASSERT_EQ(10, element); });
 
 			cell.refine([&](const int& element) {
 				return element * 3;
 			});
 
 			ASSERT_EQ(1, cell.getActiveLayer());
 
 			cell.forAllActiveNodes([](const int& element) { ASSERT_EQ(30, element); });
 
 		});
 
 	}
 
 	TEST(AdaptiveGrid, Coarsening) {
 
 		AdaptiveGrid<int, FourLayerCellConfig, 2> aGrid({ 2,2 });
 
 		aGrid.forEach([](AdaptiveGridCell<int, FourLayerCellConfig>& cell) {
 
 			cell.setActiveLayer(1);
 
 			cell.forAllActiveNodes([](int& element) { element = 30; });
 
 			cell.coarsen([&](const int& element) {
 				return element / 2;
 			});
 
 			ASSERT_EQ(2, cell.getActiveLayer());
 
 			cell.forAllActiveNodes([](const int& element) { ASSERT_EQ(15, element); });
 
 			cell.coarsen([&](const int& element) {
 				return element / 3;
 			});
 
 			ASSERT_EQ(3, cell.getActiveLayer());
 
 			cell.forAllActiveNodes([](const int& element) { ASSERT_EQ(5, element); });
 
 		});

	}

	TEST(AdaptiveGrid, RefinementAssertions) {

		using TwoLayerConfig = CellConfig<layers<layer<2, 2>>>;

		AdaptiveGrid<int, TwoLayerConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, TwoLayerConfig>& cell) {

			ASSERT_EQ(0, cell.getActiveLayer());

			ASSERT_DEBUG_DEATH(cell.refine([&](const int& element) { return element; }), ".*Cannot refine.*");

		});

	}

	TEST(AdaptiveGrid, CoarseningAssertions) {

		using TwoLayerConfig = CellConfig<layers<layer<2, 2>>>;

		AdaptiveGrid<int, TwoLayerConfig, 2> aGrid({ 2,2 });

		aGrid.forEach([](AdaptiveGridCell<int, TwoLayerConfig>& cell) {

			ASSERT_EQ(0, cell.getActiveLayer());

			cell.coarsen([&](const int& element) {
				return element;
			});

			ASSERT_EQ(1, cell.getActiveLayer());

			ASSERT_DEBUG_DEATH(cell.coarsen([&](const int& element) { return element; }), ".*Cannot coarsen.*");

		});

	}

	TEST(AdaptiveGrid, CellRefinementAssertions) {

		AdaptiveGrid<int, CellConfig<layers<>>, 2> smallGrid({ 1,1 });

		auto& cell = smallGrid[{0, 0}].data;

		ASSERT_DEBUG_DEATH(cell.refineFromLayer(0, [&](const int& element) { return element; }), "Error.*no such layer.*");

	}

	TEST(AdaptiveGrid, CellCoarseningAssertions) {

		AdaptiveGrid<int, CellConfig<layers<>>, 2> smallGrid({ 1,1 });

		auto& cell = smallGrid[{0, 0}].data;

		ASSERT_DEBUG_DEATH(cell.coarsenToLayer(0, [&](const int& element) { return element; }), "Error.*no such layer.*");

	}

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
