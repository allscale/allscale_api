#pragma once

#include <utility>

#include "allscale/api/core/parec.h"
#include "allscale/api/user/data/vector.h"

namespace allscale {
namespace api {
namespace user {

	// ----- parallel loops ------

	namespace detail {

		template<typename Iter>
		size_t distance(const Iter& a, const Iter& b) {
			return std::distance(a,b);
		}

		size_t distance(int a, int b) {
			return b-a;
		}

		template<typename Iter>
		size_t distance(const std::pair<Iter,Iter>& r) {
			return distance(r.first,r.second);
		}

		template<typename Iter>
		auto access(const Iter& iter) -> decltype(*iter) {
			return *iter;
		}

		int access(int a) {
			return a;
		}


		template<typename Iter, size_t dims>
		size_t area(const std::array<std::pair<Iter,Iter>,dims>& range) {
			size_t res = 1;
			for(size_t i = 0; i<dims; i++) {
				res *= distance(range[i].first, range[i].second);
			}
			return res;
		}


		template<size_t idx>
		struct scanner {
			scanner<idx-1> nested;
			template<typename Iter, size_t dims, typename Op>
			void operator()(const std::array<std::pair<Iter,Iter>,dims>& range, std::array<Iter,dims>& cur, const Op& op) {
				auto& i = cur[dims-idx];
				for(i = range[dims-idx].first; i != range[dims-idx].second; ++i ) {
					nested(range, cur, op);
				}
			}
		};

		template<>
		struct scanner<0> {
			template<typename Iter, size_t dims, typename Op>
			void operator()(const std::array<std::pair<Iter,Iter>,dims>&, std::array<Iter,dims>& cur, const Op& op) {
				op(cur);
			}
		};

		template<typename Iter, size_t dims, typename Op>
		void for_each(const std::array<std::pair<Iter,Iter>,dims>& range, const Op& op) {

			// the current position
			std::array<Iter,dims> cur;

			// scan range
			scanner<dims>()(range, cur, op);

		}
	}


	template<typename Iter, size_t dims, typename Body>
	core::Future<void> pfor(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Body& body) {

		// process 0-dimensional case
		if (dims == 0) return core::done(); // no iterations required

		// implements a recursive splitting policy for iterating over the given iterator range
		using range = std::array<std::pair<Iter,Iter>,dims>;

		// create the full range
		range full;
		for(size_t i = 0; i<dims; i++) {
			full[i] = std::make_pair(a[i],b[i]);
		}

		// trigger parallel processing
		return core::parec(
			[](const range& r) {
				// if there is only one element left, we reached the base case
				return detail::area(r) <= 1;
			},
			[body](const range& r) {
				if (detail::area(r) < 1) return;
				detail::for_each(r,body);
			},
			[](const range& r, const typename core::parec_fun<void(range)>::type& nested) {
				// here we have the binary splitting

				// TODO: think about splitting all dimensions

				// get the longest dimension
				size_t maxDim = 0;
				size_t maxDist = detail::distance(r[0]);
				for(size_t i = 1; i<dims;++i) {
					size_t curDist = detail::distance(r[i]);
					if (curDist > maxDist) {
						maxDim = i;
						maxDist = curDist;
					}
				}

				// split the longest dimension
				range a = r;
				range b = r;

				auto mid = r[maxDim].first + (maxDist / 2);
				a[maxDim].second = mid;
				b[maxDim].first = mid;

				// process branches in parallel
				return par(
					nested(a),
					nested(b)
				);
			}
		)(full);
	}

	/**
	 * A parallel for-each implementation iterating over the given range of elements.
	 */
	template<typename Iter, typename Body>
	core::Future<void> pfor(const Iter& a, const Iter& b, const Body& body) {
		// implements a binary splitting policy for iterating over the given iterator range
		typedef std::pair<Iter,Iter> range;
		return core::parec(
			[](const range& r) {
				return detail::distance(r.first,r.second) <= 1;
			},
			[body](const range& r) {
				for(auto it = r.first; it != r.second; ++it) body(detail::access(it));
			},
			[](const range& r, const typename core::parec_fun<void(range)>::type& nested) {
				// here we have the binary splitting
				auto mid = r.first + (r.second - r.first)/2;
				return par(
						nested(range(r.first,mid)),
						nested(range(mid,r.second))
				);
			}
		)(range(a,b));
	}

	// ---- container support ----

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	core::Future<void> pfor(Container& c, const Op& op) {
		return pfor(c.begin(), c.end(), op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	core::Future<void> pfor(const Container& c, const Op& op) {
		return pfor(c.begin(), c.end(), op);
	}


	// ---- Vector support ----

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vectors.
	 */
	template<typename Elem, size_t Dims, typename Body>
	core::Future<void> pfor(const data::Vector<Elem,Dims>& a, const data::Vector<Elem,Dims>& b, const Body& body) {
		const std::array<Elem,Dims>& x = a;
		const std::array<Elem,Dims>& y = b;
		return pfor(x,y,[&](const std::array<Elem,Dims>& pos) {
			body(static_cast<const data::Vector<Elem,Dims>&>(pos));
		});
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vector.
	 */
	template<typename Elem, size_t Dims, typename Body>
	core::Future<void> pfor(const data::Vector<Elem,Dims>& a, const Body& body) {
		return pfor(data::Vector<Elem,Dims>(0),a,body);
	}


} // end namespace user
} // end namespace api
} // end namespace allscale
