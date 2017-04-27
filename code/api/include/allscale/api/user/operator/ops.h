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
		[op](const range& r)->res_type {
			if (r.size() == 0) return res_type();

			return op(*r.begin(),res_type());
		},
		core::pick(
		[op](const range& r, const auto& nested) {
			// here we have the binary splitting
			auto fragments = r.split();
			auto left = fragments.first;
			auto right = fragments.second;

			return core::impl::reference::make_split_task(std::move(core::impl::reference::after()),
					std::move(nested(left)).toTask(), std::move(nested(right)).toTask(), op, true);
		},
		[op](const range& r, const auto&)->res_type {
			if (r.size() == 0) return res_type();

			auto tmp = res_type();

			auto opB = [op, &tmp](const auto& cur) {
				tmp = op(cur, tmp);
			};

			r.forEach(opB);
			return tmp;
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

// ----- map / reduce ------

template<
	typename Iter,
	typename MapOp,
	typename ReduceOp,
	typename InitLocalState,
	typename ReduceLocalState
>
typename utils::lambda_traits<ReduceOp>::result_type
preduce(
		const Iter& a,
		const Iter& b,
		const MapOp& map,
		const ReduceOp& reduce,
		const InitLocalState& init,
		const ReduceLocalState& exit
	) {


	using range = detail::range<Iter>;
	using res_type = typename utils::lambda_traits<ReduceOp>::result_type;

	auto handle = [](const InitLocalState& init, const MapOp& map, const ReduceLocalState& exit, const range& r)->res_type {
		auto res = init();
		auto mapB = [map,&res](const auto& cur) {
			return map(cur,res);
		};
		r.forEach(mapB);
		return exit(res);
	};

	return core::prec(
		[](const range& r) {
			return r.size() <= 1;
		},
		[init,map,exit,handle](const range& r)->res_type {
			return handle(init, map, exit, r);
		},
		core::pick(
		[reduce](const range& r, const auto& nested) {
			// here we have the binary splitting
			auto fragments = r.split();
			auto left = fragments.first;
			auto right = fragments.second;

			return core::impl::reference::make_split_task(std::move(core::impl::reference::after()),
					std::move(nested(left)).toTask(), std::move(nested(right)).toTask(), reduce, true);
		},
		[init,map,exit,handle](const range& r, const auto&)->res_type {
			return handle(init, map, exit, r);
		})
	)(range(a, b)).get();
}

template<
	typename Iter,
	typename MapOp,
	typename ReduceOp,
	typename InitLocalState
>
typename utils::lambda_traits<ReduceOp>::result_type
preduce(
		const Iter& a,
		const Iter& b,
		const MapOp& map,
		const ReduceOp& reduce,
		const InitLocalState& init
	) {

	return preduce(a, b, map, reduce, init, ([](typename utils::lambda_traits<ReduceOp>::result_type r) { return r; } ));
}

template<
	typename Container,
	typename MapOp,
	typename ReduceOp,
	typename InitLocalState,
	typename ReduceLocalState
>
typename utils::lambda_traits<ReduceOp>::result_type
preduce(
		const Container& c,
		const MapOp& map,
		const ReduceOp& reduce,
		const InitLocalState& init,
		const ReduceLocalState& exit
	) {

	return preduce(c.begin(), c.end(), map, reduce, init, exit);

}

template<
	typename Container,
	typename MapOp,
	typename ReduceOp,
	typename InitLocalState
>
typename utils::lambda_traits<ReduceOp>::result_type
preduce(
		const Container& c,
		const MapOp& map,
		const ReduceOp& reduce,
		const InitLocalState& init
	) {

	return preduce(c.begin(), c.end(), map, reduce, init);

}

} // end namespace user
} // end namespace api
} // end namespace allscale
