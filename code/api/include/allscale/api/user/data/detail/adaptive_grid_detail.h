#pragma once

#include "allscale/api/user/algorithm/pfor.h"

#include "allscale/utils/static_grid.h"
#include "allscale/utils/vector.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	namespace detail {

		template<typename NestedType, typename DataType, typename RefinerType>
		void refine(NestedType& nested, const DataType& data, const RefinerType& refiner) {
			api::user::algorithm::detail::forEach({ 0 }, nested.data.size(), [&](const auto& index) -> void {
				// using the index of a cell on nested layer, computes index covering cell on this layer
				auto newIndex = utils::elementwiseDivision(index, utils::elementwiseDivision(nested.data.size(), data.size()));
				// simply replicate data to cell on nested layer
				nested.data[index] = refiner(data[newIndex]);
			});
		}

		template<typename NestedType, typename DataType, typename RefinerType>
		void refineGrid(NestedType& nested, const DataType& data, const RefinerType& refiner) {
			// using the index of a cell on this layer, computes index of first covered cell on nested layer
			auto indexer = [&](const auto& index) { return utils::elementwiseProduct(index, utils::elementwiseDivision(nested.data.size(), data.size())); };

			// iterate over cells on this layer
			api::user::algorithm::detail::forEach({ 0 }, data.size(), [&](const auto& index) -> void {
				const auto& res = refiner(data[index]);
				auto begin = indexer(index);
				auto end = indexer(index + decltype(index){1});
				api::user::algorithm::detail::forEach(begin, end, [&](const auto& i) {
					nested.data[i] = res[i - indexer(index)];
				});
			});
		}

		template<typename ElementType, unsigned... Dims, typename NestedType, typename DataType, typename CoarsenerType>
		void coarsen(const NestedType& nested, DataType& data, const CoarsenerType& coarsener) {
			// using the index of a cell on this layer, computes index of first covered cell on nested layer
			auto indexer = [&](const auto& index) { return utils::elementwiseProduct(index, utils::elementwiseDivision(nested.data.size(), data.size())); };

			// compute divisor for average
			unsigned result = 1;
			(void)std::initializer_list<unsigned>{ (result *= Dims, 0u)... };

			// iterate over cells on this layer
			api::user::algorithm::detail::forEach({ 0 }, data.size(), [&](const auto& index) -> void {
				ElementType sum = ElementType();
				// iterate over subset of cells on nested layer, to be projected to the current cell pointed to by index
				auto begin = indexer(index);
				auto end = indexer(index + decltype(index){1});
				api::user::algorithm::detail::forEach(begin, end, [&](const auto& i) -> void {
					sum += coarsener(nested.data[i]);
				});
				data[index] = sum / result;
			});
		}

		template<typename ElementType, unsigned... Dims, typename NestedType, typename DataType, typename CoarsenerType>
		void coarsenGrid(const NestedType& nested, DataType& data, const CoarsenerType& coarsener) {
			// using the index of a cell on this layer, computes index of first covered cell on nested layer
			auto indexer = [&](const auto& index) { return utils::elementwiseProduct(index, utils::elementwiseDivision(nested.data.size(), data.size())); };

			// iterate over cells on this layer
			utils::StaticGrid<ElementType, Dims...> param;
			api::user::algorithm::detail::forEach({ 0 }, data.size(), [&](const auto& index) -> void {
				// iterate over subset of cells on nested layer, to be projected to the current cell pointed to by index
				auto begin = indexer(index);
				auto end = indexer(index + decltype(index){1});
				api::user::algorithm::detail::forEach(begin, end, [&](const decltype(index)& i) -> void {
					param[i - indexer(index)] = nested.data[i];
				});
				data[index] = coarsener(param);
			});
		}

	} // end namespace detail

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
