#pragma once

#include <cstring>
#include <memory>

#include "allscale/api/core/data.h"

#include "allscale/api/user/data/grid.h"
#include "allscale/api/user/operator/pfor.h"

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/join.h"
#include "allscale/utils/serializer.h"
#include "allscale/utils/static_grid.h"

namespace allscale {
namespace api {
namespace user {
namespace data {


	// ---------------------------------------------------------------------------------
	//								 Declarations
	// ---------------------------------------------------------------------------------

	template<std::size_t Dims = 2>
	using AdaptiveGridSharedData = GridSharedData<Dims>;

	template<std::size_t Dims = 2>
	using AdaptiveGridPoint = GridPoint<Dims>;

	template<std::size_t Dims = 2>
	using AdaptiveGridBox = GridBox<Dims>;

	template<std::size_t Dims = 2>
	using AdaptiveGridRegion = GridRegion<Dims>;


	// ---------------------------------------------------------------------------------
	//								  Definitions
	// ---------------------------------------------------------------------------------

	template<unsigned ... sizes>
	struct layer;

	template<typename ... Layers>
	struct layers {
		enum {
			num_layers = sizeof...(Layers)
		};
	};

	template<unsigned ... sizes>
	struct size;

	// structures for each Cell configuration and number of layers for nesting
	template<typename Layers>
	struct CellConfig {
		enum {
			num_layers = Layers::num_layers
		};
	};

	enum Direction {
		Up, Down, Left, Right
	};

	namespace detail {

		template<typename T, std::size_t... Sizes>
		std::vector<T> getBoundary(const Direction& dir, const utils::StaticGrid<T,Sizes...>& data) { // returns vector of boundary data in each direction
			int size[] = { Sizes... };
			int xSize = size[0];
			int ySize = size[1];
			switch(dir) {
				case Up: { // returns data from top strip of domain to neighbor
					std::vector<T> res(xSize);
					for(int i = 0; i < xSize; i++)
						res[i] = data[{ i, ySize - 1}];
					return res;
				}
				case Down: { // returns data from bottom strip of domain to neighbor
					std::vector<T> res(xSize);
					for(int i = 0; i < xSize; i++)
						res[i] = data[{ i, 0 }];
					return res;
				}
				case Left: {
					std::vector<T> res(ySize);
					for(int i = 0; i < ySize; i++) {
						res[i] = data[{ 0, i }];
					}
					return res;
				}
				case Right: {
					std::vector<T> res(ySize);
					for(int i = 0; i < ySize; i++)
						res[i] = data[{ xSize - 1, i }];
					return res;
				}
			}
			return std::vector<T>();
		}

		template<typename T, std::size_t... Sizes>
		void setBoundary(const Direction& dir, utils::StaticGrid<T, Sizes...>& data, const std::vector<T>& boundary) {
			int size[] = { Sizes... };
			int xSize = size[0];
			int ySize = size[1];

			switch(dir) {
				case Up: {
					assert_eq(boundary.size(), (size_t)xSize);
					for(int i = 0; i < xSize; i++)
						data[{ i, ySize - 1 }] = boundary[i];
					return;
				}
				case Down: {
					assert_eq(boundary.size(), (size_t)xSize);
					for(int i = 0; i < xSize; i++)
						data[{ i, 0 }] = boundary[i];
					return;
				}
				case Left: {
					assert_eq(boundary.size(), (size_t)ySize);
					for(int i = 0; i < ySize; i++)
						data[{ 0, i }] = boundary[i];
					return;
				}
				case Right: {
					assert_eq(boundary.size(), (size_t)ySize);
					for(int i = 0; i < ySize; i++)
						data[{ xSize - 1, i }] = boundary[i];
					return;
				}
			}
		}


	} // end namespace detail


	template <typename T, typename Size, typename Layers>
	struct GridLayerData;

	template <typename T, unsigned... Sizes, unsigned... Dims, typename... Rest>
	struct GridLayerData<T, size<Sizes...>, layers<layer<Dims...>, Rest...>> {
		using data_type = utils::StaticGrid<T, Sizes...>;
		using nested_type = GridLayerData<T, size<Sizes * Dims...>, layers<Rest...>>;

		enum { layer_number = sizeof...(Rest) + 1 };

		// the values to be stored on this layer
		data_type data;

		// the nested layers
		nested_type nested;

		unsigned getLayerNumber() const { return layer_number; }

		template <unsigned Layer>
		typename std::enable_if<Layer == layer_number, data_type&>::type getLayer() {
			return data;
		}

		template <unsigned Layer>
		typename std::enable_if<Layer == layer_number, const data_type&>::type getLayer() const {
			return data;
		}

		template <unsigned Layer>
			typename std::enable_if < Layer<layer_number, decltype(nested.template getLayer<Layer>())>::type getLayer() {
			return nested.template getLayer<Layer>();
		}

		template <unsigned Layer>
			typename std::enable_if < Layer<layer_number, decltype(static_cast<const nested_type&>(nested).template getLayer<Layer>())>::type getLayer() const {
			return nested.template getLayer<Layer>();
		}

		template <typename Op>
		void forAllOnLayer(unsigned layer, const Op& op) {
			if(layer == getLayerNumber()) {
				// apply it to this value
				data.forEach(op);
			} else {
				nested.forAllOnLayer(layer, op);
			}
		}

		template <typename Refiner>
		void refineFromLayer(unsigned layer, const Refiner& refiner) {
			if(layer == getLayerNumber()) {
				// iterate over cells on nested layer
				api::user::detail::forEach({0}, nested.data.size(), [&](const auto& index) {
					// using the index of a cell on nested layer, computes index covering cell on this layer
					auto newIndex = utils::elementwiseDivision(index, utils::elementwiseDivision(nested.data.size(), data.size()));
					// simply replicate data to cell on nested layer
					nested.data[index] = refiner(data[newIndex]);
				});
			} else {
				nested.refineFromLayer(layer, refiner);
			}
		}

		template <typename Refiner>
		void refineFromLayerGrid(unsigned layer, const Refiner& refiner) {
			if(layer == getLayerNumber()) {
				// using the index of a cell on this layer, computes index of first covered cell on nested layer
				auto indexer = [&](const auto& index) { return utils::elementwiseProduct(index, utils::elementwiseDivision(nested.data.size(), data.size())); };

				// iterate over cells on this layer
				api::user::detail::forEach({ 0 }, data.size(), [&](const auto& index) {
					const auto& res = refiner(data[index]);
					auto begin = indexer(index);
					auto end = indexer(index + decltype(index){1});
					api::user::detail::forEach(begin, end, [&](const auto& i) {
						nested.data[i] = res[i-indexer(index)];
					});
				});
				
			} else {
				nested.refineFromLayerGrid(layer, refiner);
			}
		}

		template <typename Coarsener>
		void coarsenToLayer(unsigned layer, const Coarsener& coarsener) {
			if(layer == getLayerNumber()) {
				// using the index of a cell on this layer, computes index of first covered cell on nested layer
				auto indexer = [&](const auto& index) { return utils::elementwiseProduct(index, utils::elementwiseDivision(nested.data.size(), data.size())); };

				// iterate over cells on this layer
				api::user::detail::forEach({ 0 }, data.size(), [&](const auto& index) {
					T sum = T();
					// iterate over subset of cells on nested layer, to be projected to the current cell pointed to by index
					auto begin = indexer(index);
					auto end = indexer(index + decltype(index){1});
					api::user::detail::forEach(begin, end, [&](const auto& i) {
						sum += coarsener(nested.data[i]);
					});
					// compute divisor for average
					unsigned result = 1;
					(void)std::initializer_list<unsigned>{ (result *= Dims, 0u)... };
					data[index] = sum / result;
				});
				
			} else {
				nested.coarsenToLayer(layer, coarsener);
			}
		}

		template <typename Coarsener>
		void coarsenToLayerGrid(unsigned layer, const Coarsener& coarsener) {
			if(layer == getLayerNumber()) {
				// using the index of a cell on this layer, computes index of first covered cell on nested layer
				auto indexer = [&](const auto& index) { return utils::elementwiseProduct(index, utils::elementwiseDivision(nested.data.size(), data.size())); };

				// iterate over cells on this layer
				utils::StaticGrid<T, Dims...> param;
				api::user::detail::forEach({ 0 }, data.size(), [&](const auto& index) {
					// iterate over subset of cells on nested layer, to be projected to the current cell pointed to by index
					auto begin = indexer(index);
					auto end = indexer(index + decltype(index){1});
					api::user::detail::forEach(begin, end, [&](const decltype(index)& i) {
						param[i - indexer(index)] = nested.data[i];
					});
					data[index] = coarsener(param);
				});

			} else {
				nested.coarsenToLayerGrid(layer, coarsener);
			}
		}

		std::vector<T> getBoundary(unsigned layer, Direction dir) const { // returns vector of boundary data in each direction
			if(layer == getLayerNumber()) {
				return detail::getBoundary(dir, data);
			}
			return nested.getBoundary(layer, dir);
		}

		void setBoundary(unsigned layer, Direction dir, const std::vector<T>& boundary) {
			if(layer == getLayerNumber()) {
				detail::setBoundary(dir, data, boundary);
			}
			nested.setBoundary(layer, dir, boundary);
		}

		void store(utils::ArchiveWriter& writer) const {
			writer.write(data);
			writer.write(nested);
		}

		static GridLayerData load(utils::ArchiveReader& reader) {
			auto data = std::move(reader.read<data_type>());
			auto nested = std::move(reader.read<nested_type>());
			return { data, nested };
		}

	};

	template <typename T, typename Size, typename Layers>
	struct GridLayerData;

	template <typename T, unsigned... Sizes>
	struct GridLayerData<T, size<Sizes...>, layers<>> {
		using data_type = utils::StaticGrid<T, Sizes...>;

		// the values to be stored on this last layer
		data_type data;

		unsigned getLayerNumber() const { return 0; }

		template <unsigned Layer>
		typename std::enable_if<Layer == 0, data_type&>::type getLayer() {
			return data;
		}

		template <unsigned Layer>
		typename std::enable_if<Layer == 0, const data_type&>::type getLayer() const {
			return data;
		}

		template <typename Op>
		void forAllOnLayer(unsigned layer, const Op& op) {
			assert_eq(layer, 0) << "Error: trying to access layer " << layer << " --no such layer!";
			data.forEach(op);
		}

		template <typename Refiner>
		void refineFromLayer(unsigned layer, const Refiner&) {
			assert_fail() << "Error: trying to access layer " << layer << " --no such layer!";
		}

		template <typename Refiner>
		void refineFromLayerGrid(unsigned layer, const Refiner&) {
			assert_fail() << "Error: trying to access layer " << layer << " --no such layer!";
		}

		template <typename Coarsener>
		void coarsenToLayer(unsigned layer, const Coarsener&) {
			assert_fail() << "Error: trying to access layer " << layer << " --no such layer!";
		}

		template <typename Coarsener>
		void coarsenToLayerGrid(unsigned layer, const Coarsener&) {
			assert_fail() << "Error: trying to access layer " << layer << " --no such layer!";
		}

		std::vector<T> getBoundary(unsigned layer, Direction dir) const {
			assert_eq(0, layer) << "No such layer";
			return detail::getBoundary(dir, data);
		}

		void setBoundary(unsigned layer, Direction dir, const std::vector<T>& boundary) {
			assert_eq(0, layer) << "No such layer";
			detail::setBoundary(dir, data, boundary);
		}

		void store(utils::ArchiveWriter& writer) const {
			writer.write(data);
		}

		static GridLayerData<T, size<Sizes...>, layers<>> load(utils::ArchiveReader& reader) {
			GridLayerData<T, size<Sizes...>, layers<>> grid;
			grid.data = std::move(reader.read<data_type>());

			return grid;
		}

	};


	template<typename T, typename CellConfig>
	struct AdaptiveGridCell;


	template<typename T, typename Layers>
	struct AdaptiveGridCell<T, CellConfig<Layers>> {

		AdaptiveGridCell() = default;
		AdaptiveGridCell(const AdaptiveGridCell& other) = delete;
		AdaptiveGridCell(AdaptiveGridCell&& other) = default;

		// determines the active layer of this grid cell
		unsigned active_layer = 0;

		// the data stored in
		GridLayerData<T, size<1, 1>, Layers> data;

		AdaptiveGridCell& operator=(const AdaptiveGridCell& other) {
			if(this == &other) return *this;
			active_layer = other.active_layer;
			data = other.data;
			return *this;
		}

		void setActiveLayer(unsigned level) {
			active_layer = level;
		}

		unsigned getActiveLayer() const {
			return active_layer;
		}

		template<unsigned Layer>
		auto getLayer() -> decltype(data.template getLayer<Layer>())& {
			return data.template getLayer<Layer>();
		}

		template<unsigned Layer>
		auto getLayer() const -> const decltype(data.template getLayer<Layer>())& {
			return data.template getLayer<Layer>();
		}

		template<typename Op>
		void forAllActiveNodes(const Op& op) {
			data.forAllOnLayer(active_layer, op);
		}

		template<typename Refiner>
		void refine(const Refiner& refiner) {
			assert_ge(active_layer, 0) << "Cannot refine any further";
			data.refineFromLayer(active_layer, refiner);
			active_layer--;
		}

		template<typename Refiner>
		void refineGrid(const Refiner& refiner) {
			assert_ge(active_layer, 0) << "Cannot refine any further";
			data.refineFromLayerGrid(active_layer, refiner);
			active_layer--;
		}

		template<typename Coarsener>
		void coarsen(const Coarsener& coarsener) {
			assert_gt(Layers::num_layers, active_layer) << "Cannot refine any further";
			active_layer++;
			data.coarsenToLayer(active_layer, coarsener);
		}

		template<typename Coarsener>
		void coarsenGrid(const Coarsener& coarsener) {
			assert_gt(Layers::num_layers, active_layer) << "Cannot refine any further";
			active_layer++;
			data.coarsenToLayerGrid(active_layer, coarsener);
		}

		std::vector<T> getBoundary(Direction dir) const {
			return data.getBoundary(active_layer, dir);
		}

		void setBoundary(Direction dir, const std::vector<T>& boundary) {
			data.setBoundary(active_layer, dir, boundary);
		}

		void store(utils::ArchiveWriter& writer) const {
			writer.write(active_layer);
			writer.write(data);
		}

		static AdaptiveGridCell load(utils::ArchiveReader& reader) {
			AdaptiveGridCell cell;
			cell.active_layer = std::move(reader.read<unsigned>());
			cell.data = reader.read<GridLayerData<T, size<1, 1>, Layers>>();
			return cell;
		}

	};

	template<typename T, typename CellConfig, std::size_t Dims = 2>
	using AdaptiveGridFragment = GridFragment<AdaptiveGridCell<T, CellConfig>, Dims>;

	template <typename T, typename CellConfig, std::size_t Dims>
	using AdaptiveGrid = Grid<AdaptiveGridCell<T, CellConfig>, Dims>;

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale