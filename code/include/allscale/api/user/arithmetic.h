
#pragma once

#include <type_traits>

#include "allscale/api/core/treeture.h"


namespace allscale {
namespace api {
namespace user {

	// --- specific aggregators ---

	template<typename A, typename B, typename R = decltype(std::declval<typename A::value_type>() + std::declval<typename B::value_type>())>
	auto add(A&& a, B&& b) {
		return core::combine(std::move(a),std::move(b),[](const R& a, const R& b) { return a + b; });
	}

} // end namespace user
} // end namespace api
} // end namespace allscale
