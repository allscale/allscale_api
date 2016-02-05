#pragma once


// This header starts by importing the actual future implementation. The
// implementation includes:
//		- the definintion of the future
//		- a minimal set of basic factory functions
//
#include "allscale/api/core/impl/naive/future.h"

namespace allscale {
namespace api {
namespace core {

	// ------------------------------------------------------------------------------
	//								General Factories
	// ------------------------------------------------------------------------------

	namespace _detail {

		template<typename V>
		V add(const std::vector<Future<V>>& values) {
			V res = 0;
			for(const auto& cur : values) {
				res += cur.get();
			}
			return res;
		}

		template<typename V>
		V mul(const std::vector<Future<V>>& values) {
			V res = 1;
			for(const auto& cur : values) {
				res *= cur.get();
			}
			return res;
		}

	}

	template<typename V, typename ... Rest>
	Future<V> add(Future<V>&& first, Rest&& ... rest) {

		// trigger template instantiation of aggregation function
		// NOTE: a bug in GCC prevents this from being within a sub-expression
		auto op = &_detail::add<V>;

		// create aggregation node
		return aggregate(
				op,
				std::move(first), std::move(rest)...
		);
	}

	template<typename V, typename ... Rest>
	Future<V> mul(Future<V>&& first, Rest&& ... rest) {

		// trigger template instantiation of aggregation function
		// NOTE: a bug in GCC prevents this from being within a sub-expression
		auto op = &_detail::mul<V>;

		// create aggregation node
		return aggregate(
				op,
				std::move(first), std::move(rest)...
		);
	}


} // end namespace core
} // end namespace api
} // end namespace allscale

