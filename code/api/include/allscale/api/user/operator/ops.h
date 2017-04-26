#pragma once

#include <utility>

#include "allscale/api/core/prec.h"

#include "allscale/api/user/operator/pfor.h"

#include "allscale/utils/assert.h"
#include "allscale/utils/vector.h"

namespace allscale {
namespace api {
namespace user {

// ----- reduction ------

template<typename Iter, typename Op>
typename utils::lambda_traits<Op>::result_type
preduce(const Iter& a, const Iter& b, const Op& op) {
	typedef typename utils::lambda_traits<Op>::result_type res_type;

	using range = detail::range<Iter>;

	// implements a binary splitting policy for iterating over the given iterator range
	return  core::prec(
		[](const range& r) {
			return r.size() <= 1;
		},
		[&](const range& r)->res_type {
			if (r.size() == 0) return res_type();
			return op(*r.begin(),res_type());
		},
		core::pick(
		[&](const range& r, const auto& nested) {
			// here we have the binary splitting
			auto fragments = r.split();
			auto left = fragments.first;
			auto right = fragments.second;

//			core::impl::reference::Task<int>* leftTask = std::move(nested(left)).toTask();
//			core::impl::reference::Task<int>* rightTask = std::move(nested(right)).toTask();
			return core::impl::reference::make_split_task(std::move(core::impl::reference::after()),
					std::move(nested(left)).toTask(), std::move(nested(right)).toTask(), op, true);

			//return core::impl::reference::unreleased_treeture<int>(x);
//			return fun(left);
//			core::impl::reference::Task<int> task;
//			return core::impl::reference::unreleased_treeture<int>(&task);
		},
		[&](const range& r, const auto&)->res_type {
			if (r.size() == 0) return res_type();
			return op(*r.begin(),res_type());
		})
	)(range(a,b)).get();
}

/**
 * A parallel reduce implementation over the elements of the given container.
 */
template<typename Container, typename Op>
typename utils::lambda_traits<Op>::result_type
preduce(Container& c, Op& op) {
	return preduce(c.begin(), c.end(), op);
}

/**
 * A parallel reduce implementation over the elements of the given container.
 */
template<typename Container, typename Op>
typename utils::lambda_traits<Op>::result_type
preduce(const Container& c, const Op& op) {
	return preduce(c.begin(), c.end(), op);
}

} // end namespace user
} // end namespace api
} // end namespace allscale
