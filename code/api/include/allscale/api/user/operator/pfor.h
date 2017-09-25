#pragma once

#include <utility>

#include "allscale/utils/assert.h"

#include "allscale/api/core/prec.h"

#include "allscale/utils/vector.h"

namespace allscale {
namespace api {
namespace user {

	// ----- forward declarations ------

	namespace detail {

		/**
		 * The object representing the iterator range of a (parallel) loop.
		 */
		template<typename Iter>
		class range;


		// -- Adaptive Loop Dependencies --

		/**
		 * The token produced by the pfor operator to reference the execution
		 * of a parallel loop.
		 */
		template<typename Iter>
		class loop_reference;

		/**
		 * A marker type for loop dependencies.
		 */
		struct loop_dependency {};

		/**
		 * A test for loop dependencies.
		 */
		template<typename T>
		struct is_loop_dependency : public std::is_base_of<loop_dependency,T> {};

		/**
		 * A small container for splitting dependencies.
		 */
		template<typename Dependency>
		struct SubDependencies {
			Dependency left;
			Dependency right;
		};

	} // end namespace detail

	/**
	 * The dependency to be used if no dependencies are required.
	 */
	struct no_dependencies : public detail::loop_dependency {

		detail::SubDependencies<no_dependencies> split() const {
			return detail::SubDependencies<no_dependencies>();
		}

	};

	// ---------------------------------------------------------------------------------------------
	//									Basic Generic pfor Operators
	// ---------------------------------------------------------------------------------------------

	/**
	 * The generic version of all parallel loops with synchronization dependencies.
	 *
	 * @tparam Iter the type of the iterator to pass over
	 * @tparam Body the type of the body operation, thus the operation to be applied on each element in the given range
	 * @tparam Dependency the type of the dependencies to be enforced
	 *
	 * @param r the range to iterate over
	 * @param body the operation to be applied on each element of the given range
	 * @param dependency the dependencies to be obeyed when scheduling the iterations of this parallel loop
	 *
	 * @return a reference to the iterations of the processed parallel loop to be utilized for forming dependencies
	 */
	template<typename Iter, typename Body, typename Dependency>
	detail::loop_reference<Iter> pfor(const detail::range<Iter>& r, const Body& body, const Dependency& dependency);

	/**
	 * The generic version of all parallel loops without synchronization dependencies.
	 *
	 * @tparam Iter the type of the iterator to pass over
	 * @tparam Body the type of the body operation, thus the operation to be applied on each element in the given range
	 *
	 * @param r the range to iterate over
	 * @param body the operation to be applied on each element of the given range
	 *
	 * @return a reference to the iterations of the processed parallel loop to be utilized for forming dependencies
	 */
	template<typename Iter, typename Body>
	detail::loop_reference<Iter> pfor(const detail::range<Iter>& r, const Body& body, const no_dependencies& = no_dependencies());


	// ---------------------------------------------------------------------------------------------
	//									pfor Operators with Boundaries
	// ---------------------------------------------------------------------------------------------

	/**
	 * The generic version of all parallel loops with synchronization dependencies.
	 *
	 * @tparam Iter the type of the iterator to pass over
	 * @tparam InnerBody the type of the inner body operation, thus the operation to be applied on each element in the given range that is not on the surface
	 * @tparam BoundaryBody the type of the boundary body operation, thus the operation to be applied on each element in the given range that is on the surface
	 * @tparam Dependency the type of the dependencies to be enforced
	 *
	 * @param r the range to iterate over
	 * @param innerBody the operation to be applied on each element of the given range that is not on the surface
	 * @param boundaryBody the operation to be applied on each element of the given range that is on the surface
	 * @param dependency the dependencies to be obeyed when scheduling the iterations of this parallel loop
	 *
	 * @return a reference to the iterations of the processed parallel loop to be utilized for forming dependencies
	 */
	template<typename Iter, typename InnerBody, typename BoundaryBody, typename Dependency>
	detail::loop_reference<Iter> pforWithBoundary(const detail::range<Iter>& r, const InnerBody& innerBody, const BoundaryBody& boundaryBody, const Dependency& dependency);

	/**
	 * The generic version of all parallel loops without synchronization dependencies.
	 *
	 * @tparam Iter the type of the iterator to pass over
	 * @tparam InnerBody the type of the inner body operation, thus the operation to be applied on each element in the given range that is not on the surface
	 * @tparam BoundaryBody the type of the boundary body operation, thus the operation to be applied on each element in the given range that is on the surface
	 *
	 * @param r the range to iterate over
	 * @param innerBody the operation to be applied on each element of the given range that is not on the surface
	 * @param boundaryBody the operation to be applied on each element of the given range that is on the surface
	 *
	 * @return a reference to the iterations of the processed parallel loop to be utilized for forming dependencies
	 */
	template<typename Iter, typename InnerBody, typename BoundaryBody>
	detail::loop_reference<Iter> pforWithBoundary(const detail::range<Iter>& r, const InnerBody& innerBody, const BoundaryBody& boundaryBody, const no_dependencies& = no_dependencies());


	// ---------------------------------------------------------------------------------------------
	//									The after Utility
	// ---------------------------------------------------------------------------------------------

	/**
	 * A generic utility for inserting a single action into a single a chain of dependencies. The given action will be triggered
	 * once the corresponding iteration in the given loop reference has been completed. The resulting loop reference can be utilized
	 * by consecutive operations to synchronize on the completion of the concatenation of the given loop reference and inserted action.
	 *
	 * @tparam Iter the type of iterator the preceding loop operated on
	 * @tparam Point the iterator value of the point this action shell be associated to
	 * @tparam Action the type of action to be performed
	 *
	 * @param loop preceding loop
	 * @param point the point to which this event shell be associated to
	 * @param action the action to be performed
	 * @return a customized loop reference to sync upon the concatenation of this
	 */
	template<typename Iter, typename Point, typename Action>
	detail::loop_reference<Iter> after(const detail::loop_reference<Iter>& loop, const Point& point, const Action& action);


	// ---------------------------------------------------------------------------------------------
	//									adapters for the pfor operator
	// ---------------------------------------------------------------------------------------------

	template<typename Iter, size_t dims, typename Body>
	detail::loop_reference<std::array<Iter,dims>> pfor(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Body& body) {
		return pfor(detail::range<std::array<Iter,dims>>(a,b),body);
	}

	template<typename Iter, size_t dims, typename Body, typename Dependency>
	detail::loop_reference<std::array<Iter,dims>> pfor(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Body& body, const Dependency& dependency) {
		return pfor(detail::range<std::array<Iter,dims>>(a,b),body,dependency);
	}

	template<typename Iter, size_t dims, typename InnerBody, typename BoundaryBody>
	detail::loop_reference<std::array<Iter,dims>> pforWithBoundary(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const InnerBody& innerBody, const BoundaryBody& boundaryBody) {
		return pforWithBoundary(detail::range<std::array<Iter,dims>>(a,b),innerBody,boundaryBody);
	}

	template<typename Iter, size_t dims, typename InnerBody, typename BoundaryBody, typename Dependency>
	detail::loop_reference<std::array<Iter,dims>> pforWithBoundary(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const InnerBody& innerBody, const BoundaryBody& boundaryBody, const Dependency& dependency) {
		return pforWithBoundary(detail::range<std::array<Iter,dims>>(a,b),innerBody,boundaryBody,dependency);
	}

	/**
	 * A parallel for-each implementation iterating over the given range of elements.
	 */
	template<typename Iter, typename Body, typename Dependency>
	detail::loop_reference<Iter> pfor(const Iter& a, const Iter& b, const Body& body, const Dependency& dependency) {
		return pfor(detail::range<Iter>(a,b),body,dependency);
	}

	template<typename Iter, typename Body>
	detail::loop_reference<Iter> pfor(const Iter& a, const Iter& b, const Body& body) {
		return pfor(a,b,body,no_dependencies());
	}

	template<typename Iter, typename InnerBody, typename BoundaryBody>
	detail::loop_reference<Iter> pforWithBoundary(const Iter& a, const Iter& b, const InnerBody& innerBody, const BoundaryBody& boundaryBody) {
		return pforWithBoundary(detail::range<Iter>(a,b),innerBody,boundaryBody);
	}

	template<typename Iter, typename InnerBody, typename BoundaryBody, typename Dependency>
	detail::loop_reference<Iter> pforWithBoundary(const Iter& a, const Iter& b, const InnerBody& innerBody, const BoundaryBody& boundaryBody, const Dependency& dependency) {
		return pforWithBoundary(detail::range<Iter>(a,b),innerBody,boundaryBody,dependency);
	}

	// ---- container support ----

	/**
	 * A parallel for-each implementation iterating over the elements of the given, mutable container.
	 */
	template<typename Container, typename Op>
	detail::loop_reference<typename Container::iterator>
	pfor(Container& c, const Op& op) {
		return pfor(c.begin(), c.end(), op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given, mutable container.
	 */
	template<typename Container, typename Op, typename Dependency>
	std::enable_if_t<detail::is_loop_dependency<Dependency>::value,detail::loop_reference<typename Container::iterator>>
	pfor(Container& c, const Op& op, const Dependency& dependency) {
		return pfor(c.begin(), c.end(), op, dependency);
	}


	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	detail::loop_reference<typename Container::const_iterator>
	pfor(const Container& c, const Op& op) {
		return pfor(c.begin(), c.end(), op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op, typename Dependency>
	detail::loop_reference<typename Container::const_iterator>
	pfor(const Container& c, const Op& op, const Dependency& dependency) {
		return pfor(c.begin(), c.end(), op, dependency);
	}


	// ---- Vector support ----

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vectors.
	 */
	template<typename Elem, size_t dims, typename Body>
	detail::loop_reference<utils::Vector<Elem,dims>> pfor(const utils::Vector<Elem,dims>& a, const utils::Vector<Elem,dims>& b, const Body& body) {
		return pfor(detail::range<utils::Vector<Elem,dims>>(a,b),body);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vectors. Optional dependencies may be passed.
	 */
	template<typename Elem, size_t dims, typename Body, typename Dependencies>
	detail::loop_reference<utils::Vector<Elem,dims>> pfor(const utils::Vector<Elem,dims>& a, const utils::Vector<Elem,dims>& b, const Body& body, const Dependencies& dependencies) {
		return pfor(detail::range<utils::Vector<Elem,dims>>(a,b),body,dependencies);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vector.
	 */
	template<typename Elem, size_t Dims, typename Body>
	auto pfor(const utils::Vector<Elem,Dims>& a, const Body& body) {
		return pfor(utils::Vector<Elem,Dims>(0),a,body);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vector. Optional dependencies may be passed.
	 */
	template<typename Elem, size_t Dims, typename Body, typename Dependencies>
	auto pfor(const utils::Vector<Elem,Dims>& a, const Body& body, const Dependencies& dependencies) {
		return pfor(utils::Vector<Elem,Dims>(0),a,body,dependencies);
	}

	// -------------------------------------------------------------------------------------------
	//								Adaptive Synchronization
	// -------------------------------------------------------------------------------------------

	/**
	 * A dependency between loop iterations where iteration i of a new parallel loop may be executed
	 * as soon as iteration i of a given parallel loop has been completed.
	 *
	 * @param Iter the iterator type utilized to address iterations
	 */
	template<typename Iter>
	class one_on_one_dependency;


	/**
	 * A factory for one_on_one dependencies.
	 */
	template<typename Iter>
	one_on_one_dependency<Iter> one_on_one(const detail::loop_reference<Iter>& dep) {
		return one_on_one_dependency<Iter>(dep);
	}

	/**
	 * A dependency between loop iterations where iteration i of a new parallel loop may be executed
	 * as soon as iteration (i-1), (i), and (i+1) of a given parallel loop has been completed.
	 *
	 * @param Iter the iterator type utilized to address iterations
	 */
	template<typename Iter>
	class neighborhood_sync_dependency;

	/**
	 * A factory for one_on_one dependencies.
	 */
	template<typename Iter>
	neighborhood_sync_dependency<Iter> neighborhood_sync(const detail::loop_reference<Iter>& dep) {
		return neighborhood_sync_dependency<Iter>(dep);
	}



	// -------------------------------------------------------------------------------------------
	//								     Range Utils
	// -------------------------------------------------------------------------------------------


	namespace detail {


		// -- distances between begin and end of iterators --

		template<typename Iter,typename filter = bool>
		struct volume {
			size_t operator()(const Iter& a, const Iter& b) const {
				return std::distance(a,b);
			}
		};

		template<typename Int>
		struct volume<Int,std::enable_if_t<std::template is_integral<Int>::value,bool>> {
			size_t operator()(Int a, Int b) const {
				return (a < b) ? b-a : 0;
			}
		};

		template<typename Iter,size_t dims>
		struct volume<std::array<Iter,dims>> {
			size_t operator()(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b) const {
				volume<Iter> inner;
				size_t res = 1;
				for(size_t i = 0; i<dims; i++) {
					res *= inner(a[i],b[i]);
				}
				return res;
			}
		};

		template<typename Iter,size_t dims>
		struct volume<utils::Vector<Iter,dims>> {
			size_t operator()(const utils::Vector<Iter,dims>& a, const utils::Vector<Iter,dims>& b) const {
				return volume<std::array<Iter,dims>>()(a,b);
			}
		};


		// -- coverage --

		template<typename Iter>
		bool covers(const Iter& a_begin, const Iter& a_end, const Iter& b_begin, const Iter& b_end) {
			return b_begin >= b_end || (a_begin <= b_begin && b_end <= a_end);
		}

		template<typename Iter, size_t dims>
		bool covers(const utils::Vector<Iter,dims>& a_begin, const utils::Vector<Iter,dims>& a_end, const utils::Vector<Iter,dims>& b_begin, const utils::Vector<Iter,dims>& b_end) {
			// if the second is empty, it is covered
			for(size_t i=0; i<dims; ++i) {
				if (b_begin[i] >= b_end[i]) return true;
			}
			// check that a non-empty range is covered
			for(size_t i=0; i<dims; ++i) {
				if (!(a_begin[i] <= b_begin[i] && b_end[i] <= a_end[i])) return false;
			}
			return true;
		}


		template<typename Iter, typename Point>
		bool covers(const Iter& begin, const Iter& end, const Point& p) {
			return begin <= p && p < end;
		}

		template<typename Iter, typename Point,size_t dims>
		bool covers(const utils::Vector<Iter,dims>& begin, const utils::Vector<Iter,dims>& end, const utils::Vector<Point,dims>& point) {
			for(size_t i=0; i<dims; ++i) {
				if (point[i] < begin[i] || end[i] <= point[i]) return false;
			}
			return true;
		}

		// -- iterator access utility --

		template<typename Iter>
		auto access(const Iter& iter) -> decltype(*iter) {
			return *iter;
		}

		template<typename T>
		typename std::enable_if<std::is_integral<T>::value,T>::type access(T a) {
			return a;
		}


		// -- scan utility --

		template<typename Iter, typename InnerOp, typename BoundaryOp>
		void forEach(const Iter& fullBegin, const Iter& fullEnd, const Iter& a, const Iter& b, const InnerOp& inner, const BoundaryOp& boundary) {

			// cut off empty loop
			if (a == b) return;

			// get inner range
			Iter innerBegin = a;
			Iter innerEnd = b;

			// check for boundaries
			if (fullBegin == a) {
				boundary(access(a));
				innerBegin++;
			}

			// reduce inner range if b is the end
			if (fullEnd == b) {
				innerEnd--;
			}

			// process inner part
			for(auto it = innerBegin; it != innerEnd; ++it) {
				inner(access(it));
			}

			// process left boundary
			if(fullEnd == b) {
				boundary(access(b-1));
			}
		}


		template<typename Iter, typename InnerOp, typename BoundaryOp>
		void forEach(const Iter& a, const Iter& b, const InnerOp& inner, const BoundaryOp& boundary) {

			// cut off empty loop
			if (a == b) return;

			// process left boundary
			boundary(access(a));
			if (a + 1 == b) return;

			// process inner part
			for(auto it = a+1; it != b-1; ++it) {
				inner(access(it));
			}

			// process left boundary
			boundary(access(b-1));
		}

		template<typename Iter, typename Op>
		void forEach(const Iter& a, const Iter& b, const Op& op) {
			for(auto it = a; it != b; ++it) {
				op(access(it));
			}
		}

		template<size_t idx>
		struct scanner {
			scanner<idx-1> nested;
			template<template<typename T, size_t d> class Compound, typename Iter, size_t dims, typename Op>
			void operator()(const Compound<Iter,dims>& begin, const Compound<Iter,dims>& end, Compound<Iter,dims>& cur, const Op& op) {
				auto& i = cur[dims-idx];
				for(i = begin[dims-idx]; i != end[dims-idx]; ++i ) {
					nested(begin, end, cur, op);
				}
			}
			template<template<typename T, size_t d> class Compound, typename Iter, size_t dims, typename Inner, typename Boundary>
			void operator()(const Compound<Iter,dims>& begin, const Compound<Iter,dims>& end, Compound<Iter,dims>& cur, const Inner& inner, const Boundary& boundary) {
				auto& i = cur[dims-idx];

				// extract range
				const auto& a = begin[dims-idx];
				const auto& b = end[dims-idx];

				// check empty range
				if (a==b) return;

				// handle left boundary
				i = a; nested(begin,end,cur,boundary);

				// check whether this has been all
				if (a + 1 == b) return;

				// process inner part
				for(i = a+1; i!=b-1; ++i) {
					nested(begin,end,cur,inner,boundary);
				}

				// handle right boundary
				i = b-1;
				nested(begin,end,cur,boundary);
			}

			template<template<typename T, size_t d> class Compound, typename Iter, size_t dims, typename Inner, typename Boundary>
			void operator()(const Compound<Iter,dims>& fullBegin, const Compound<Iter,dims>& fullEnd, const Compound<Iter,dims>& begin, const Compound<Iter,dims>& end, Compound<Iter,dims>& cur, const Inner& inner, const Boundary& boundary) {
				auto& i = cur[dims-idx];

				// extract range
				const auto& fa = fullBegin[dims-idx];
				const auto& fb = fullEnd[dims-idx];

				const auto& a = begin[dims-idx];
				const auto& b = end[dims-idx];

				// check empty range
				if (a==b) return;

				// get inner range
				auto ia = a;
				auto ib = b;

				// handle left boundary
				if (fa == ia) {
					i = ia;
					nested(begin,end,cur,boundary);
					ia++;
				}

				if (fb == b) {
					ib--;
				}

				// process inner part
				for(i = ia; i!=ib; ++i) {
					nested(fullBegin,fullEnd,begin,end,cur,inner,boundary);
				}

				// handle right boundary
				if (fb == b) {
					i = b-1;
					nested(begin,end,cur,boundary);
				}
			}
		};

		template<>
		struct scanner<0> {
			template<template<typename T, size_t d> class Compound, typename Iter, size_t dims, typename Op>
			void operator()(const Compound<Iter,dims>&, const Compound<Iter,dims>&, Compound<Iter,dims>& cur, const Op& op) {
				op(cur);
			}
			template<template<typename T, size_t d> class Compound, typename Iter, size_t dims, typename Inner, typename Boundary>
			void operator()(const Compound<Iter,dims>&, const Compound<Iter,dims>&, Compound<Iter,dims>& cur, const Inner& inner, const Boundary&) {
				inner(cur);
			}
			template<template<typename T, size_t d> class Compound, typename Iter, size_t dims, typename Inner, typename Boundary>
			void operator()(const Compound<Iter,dims>&, const Compound<Iter,dims>&, const Compound<Iter,dims>&, const Compound<Iter,dims>&, Compound<Iter,dims>& cur, const Inner& inner, const Boundary&) {
				inner(cur);
			}
		};

		template<typename Iter, size_t dims, typename InnerOp, typename BoundaryOp>
		void forEach(const std::array<Iter,dims>& fullBegin, const std::array<Iter,dims>& fullEnd, const std::array<Iter,dims>& begin, const std::array<Iter,dims>& end, const InnerOp& inner, const BoundaryOp& boundary) {

			// the current position
			std::array<Iter,dims> cur;

			// scan range
			detail::scanner<dims>()(fullBegin, fullEnd, begin, end, cur, inner, boundary);
		}

		template<typename Iter, size_t dims, typename InnerOp, typename BoundaryOp>
		void forEach(const std::array<Iter,dims>& begin, const std::array<Iter,dims>& end, const InnerOp& inner, const BoundaryOp& boundary) {

			// the current position
			std::array<Iter,dims> cur;

			// scan range
			detail::scanner<dims>()(begin, end, cur, inner, boundary);
		}

		template<typename Iter, size_t dims, typename Op>
		void forEach(const std::array<Iter,dims>& begin, const std::array<Iter,dims>& end, const Op& op) {

			// the current position
			std::array<Iter,dims> cur;

			// scan range
			detail::scanner<dims>()(begin, end, cur, op);
		}

		template<typename Elem, size_t dims, typename InnerOp, typename BoundaryOp>
		void forEach(const utils::Vector<Elem,dims>& fullBegin, const utils::Vector<Elem,dims>& fullEnd, const utils::Vector<Elem,dims>& begin, const utils::Vector<Elem,dims>& end, const InnerOp& inner, const BoundaryOp& boundary) {

			// the current position
			utils::Vector<Elem,dims> cur;

			// scan range
			detail::scanner<dims>()(fullBegin, fullEnd, begin, end, cur, inner, boundary);
		}

		template<typename Elem, size_t dims, typename InnerOp, typename BoundaryOp>
		void forEach(const utils::Vector<Elem,dims>& begin, const utils::Vector<Elem,dims>& end, const InnerOp& inner, const BoundaryOp& boundary) {

			// the current position
			utils::Vector<Elem,dims> cur;

			// scan range
			detail::scanner<dims>()(begin, end, cur, inner, boundary);
		}


		template<typename Elem, size_t dims, typename Op>
		void forEach(const utils::Vector<Elem,dims>& begin, const utils::Vector<Elem,dims>& end, const Op& op) {

			// the current position
			utils::Vector<Elem,dims> cur;

			// scan range
			detail::scanner<dims>()(begin, end, cur, op);
		}


		template<typename Iter>
		Iter grow(const Iter& value, const Iter& limit, int steps) {
			return std::min(limit, value+steps);
		}

		template<typename Iter, size_t dims>
		std::array<Iter,dims> grow(const std::array<Iter,dims>& value, const std::array<Iter,dims>& limit, int steps) {
			std::array<Iter,dims> res;
			for(unsigned i=0; i<dims; i++) {
				res[i] = grow(value[i],limit[i],steps);
			}
			return res;
		}

		template<typename Iter, size_t dims>
		utils::Vector<Iter,dims> grow(const utils::Vector<Iter,dims>& value, const utils::Vector<Iter,dims>& limit, int steps) {
			utils::Vector<Iter,dims> res;
			for(unsigned i=0; i<dims; i++) {
				res[i] = grow(value[i],limit[i],steps);
			}
			return res;
		}


		template<typename Iter>
		Iter shrink(const Iter& value, const Iter& limit, int steps) {
			return std::max(limit, value-steps);
		}

		template<typename Iter, size_t dims>
		std::array<Iter,dims> shrink(const std::array<Iter,dims>& value, const std::array<Iter,dims>& limit, int steps) {
			std::array<Iter,dims> res;
			for(unsigned i=0; i<dims; i++) {
				res[i] = shrink(value[i],limit[i],steps);
			}
			return res;
		}

		template<typename Iter, size_t dims>
		utils::Vector<Iter,dims> shrink(const utils::Vector<Iter,dims>& value, const utils::Vector<Iter,dims>& limit, int steps) {
			utils::Vector<Iter,dims> res;
			for(unsigned i=0; i<dims; i++) {
				res[i] = shrink(value[i],limit[i],steps);
			}
			return res;
		}


		template<typename Iter>
		struct range_spliter;

		/**
		 * The object representing the iterator range of a (parallel) loop.
		 */
		template<typename Iter>
		class range {

			/**
			 * The begin of this range (inclusive).
			 */
			Iter _begin;

			/**
			 * The end of this range (exclusive).
			 */
			Iter _end;

		public:

			range() : _begin(), _end() {}

			range(const Iter& begin, const Iter& end)
				: _begin(begin), _end(end) {
				if (empty()) { _end = _begin; }
			}

			size_t size() const {
				return detail::volume<Iter>()(_begin,_end);
			}

			bool empty() const {
				return size() == 0;
			}

			const Iter& begin() const {
				return _begin;
			}

			const Iter& end() const {
				return _end;
			}

			bool covers(const range<Iter>& r) const {
				return detail::covers(_begin,_end,r._begin,r._end);
			}

			template<typename Point>
			bool covers(const Point& p) const {
				return detail::covers(_begin,_end,p);
			}

			range grow(const range<Iter>& limit, int steps = 1) const {
				return range(
						detail::shrink(_begin,limit.begin(),steps),
						detail::grow(_end,limit.end(),steps)
				);
			}

			range shrink(int steps = 1) const {
				return grow(*this, -steps);
			}

			std::pair<range<Iter>,range<Iter>> split() const {
				return range_spliter<Iter>::split(*this);
			}

			template<typename Op>
			void forEach(const Op& op) const {
				detail::forEach(_begin,_end,op);
			}

			template<typename InnerOp, typename BoundaryOp>
			void forEachWithBoundary(const range& full, const InnerOp& inner, const BoundaryOp& boundary) const {
				detail::forEach(full._begin,full._end,_begin,_end,inner,boundary);
			}

			friend std::ostream& operator<<(std::ostream& out, const range& r) {
				return out << "[" << r.begin() << "," << r.end() << ")";
			}

		};



		template<typename Iter>
		struct range_spliter {

			using rng = range<Iter>;

			static std::pair<rng,rng> split(const rng& r) {
				const auto& a = r.begin();
				const auto& b = r.end();
				auto m = a + (b - a)/2;
				return std::make_pair(rng(a,m),rng(m,b));
			}

		};

		template<
			template<typename I, size_t d> class Container,
			typename Iter, size_t dims
		>
		struct range_spliter<Container<Iter,dims>> {

			using rng = range<Container<Iter,dims>>;

			static std::pair<rng,rng> split(const rng& r) {

				__allscale_unused const auto volume = detail::volume<Container<Iter,dims>>();
				const auto distance = detail::volume<Iter>();

				const auto& begin = r.begin();
				const auto& end = r.end();

				// get the longest dimension
				size_t maxDim = 0;
				size_t maxDist = distance(begin[0],end[0]);
				for(size_t i = 1; i<dims;++i) {
					size_t curDist = distance(begin[i],end[i]);
					if (curDist > maxDist) {
						maxDim = i;
						maxDist = curDist;
					}
				}

				// split the longest dimension, keep the others as they are
				auto midA = end;
				auto midB = begin;
				midA[maxDim] = midB[maxDim] = range_spliter<Iter>::split(range<Iter>(begin[maxDim],end[maxDim])).first.end();

				// make sure no points got lost
				assert_eq(volume(begin,end), volume(begin,midA) + volume(midB,end));

				// create result
				return std::make_pair(rng(begin,midA),rng(midB,end));
			}

		};

	} // end namespace detail



	// -------------------------------------------------------------------------------------------
	//								 Synchronization Definitions
	// -------------------------------------------------------------------------------------------

	namespace detail {

		/**
		 * An entity to reference ranges of iterations of a loop.
		 */
		template<typename Iter>
		class iteration_reference {

			/**
			 * The range covered by the iterations referenced by this object.
			 */
			range<Iter> _range;

			/**
			 * The reference to the task processing the covered range.
			 */
			core::task_reference handle;

		public:

			iteration_reference(const range<Iter>& _range = range<Iter>()) : _range(_range) {}

			iteration_reference(const range<Iter>& range, const core::task_reference& handle)
				: _range(range), handle(handle) {}

			iteration_reference(const iteration_reference&) = default;
			iteration_reference(iteration_reference&&) = default;

			iteration_reference& operator=(const iteration_reference&) = default;
			iteration_reference& operator=(iteration_reference&&) = default;

			void wait() const {
				if (handle.valid()) handle.wait();
			}

			iteration_reference<Iter> getLeft() const {
				return { range_spliter<Iter>::split(_range).first, handle.getLeft() };
			}

			iteration_reference<Iter> getRight() const {
				return { range_spliter<Iter>::split(_range).second, handle.getRight() };
			}

			operator core::task_reference() const {
				return handle;
			}

			const range<Iter>& getRange() const {
				return _range;
			}

			const core::task_reference& getHandle() const {
				return handle;
			}
		};


		/**
		 * An entity to reference the full range of iterations of a loop. This token
		 * can not be copied and will wait for the completion of the loop upon destruction.
		 */
		template<typename Iter>
		class loop_reference : public iteration_reference<Iter> {

		public:

			loop_reference(const range<Iter>& range, core::treeture<void>&& handle)
				: iteration_reference<Iter>(range, std::move(handle)) {}

			loop_reference() {};
			loop_reference(const loop_reference&) = delete;
			loop_reference(loop_reference&&) = default;

			loop_reference& operator=(const loop_reference&) = delete;
			loop_reference& operator=(loop_reference&&) = default;

			~loop_reference() { this->wait(); }

		};

	} // end namespace detail



	// ---------------------------------------------------------------------------------------------
	//									Definitions
	// ---------------------------------------------------------------------------------------------


	template<typename Iter, typename Body, typename Dependency>
	detail::loop_reference<Iter> pfor(const detail::range<Iter>& r, const Body& body, const Dependency& dependency) {

		struct rangeHelper {
			detail::range<Iter> range;
			Dependency dependencies;
		};

		// trigger parallel processing
		return { r, core::prec(
			[](const rangeHelper& rg) {
				// if there is only one element left, we reached the base case
				return rg.range.size() <= 1;
			},
			[body](const rangeHelper& rg) {
				// apply the body operation to every element in the remaining range
				rg.range.forEach(body);
			},
			core::pick(
				[](const rangeHelper& rg, const auto& nested) {
					// in the step case we split the range and process sub-ranges recursively
					auto fragments = rg.range.split();
					auto& left = fragments.first;
					auto& right = fragments.second;
					auto dep = rg.dependencies.split(left,right);
					return parallel(
						nested(dep.left.toCoreDependencies(), rangeHelper{left, dep.left} ),
						nested(dep.right.toCoreDependencies(), rangeHelper{right,dep.right})
					);
				},
				[body](const rangeHelper& rg, const auto&) {
					// the alternative is processing the step sequentially
					rg.range.forEach(body);
				}
			)
		)(dependency.toCoreDependencies(),rangeHelper{r,dependency}) };
	}

	template<typename Iter, typename Body>
	detail::loop_reference<Iter> pfor(const detail::range<Iter>& r, const Body& body, const no_dependencies&) {
		using range = detail::range<Iter>;

		// trigger parallel processing
		return { r, core::prec(
			[](const range& r) {
				// if there is only one element left, we reached the base case
				return r.size() <= 1;
			},
			[body](const range& r) {
				// apply the body operation to every element in the remaining range
				r.forEach(body);
			},
			core::pick(
				[](const range& r, const auto& nested) {
					// in the step case we split the range and process sub-ranges recursively
					auto fragments = r.split();
					auto& left = fragments.first;
					auto& right = fragments.second;
					return parallel(
						nested(left),
						nested(right)
					);
				},
				[body](const range& r, const auto&) {
					// the alternative is processing the step sequentially
					r.forEach(body);
				}
			)
		)(r) };
	}



	template<typename Iter>
	class one_on_one_dependency : public detail::loop_dependency {

		detail::iteration_reference<Iter> loop;

	public:

		one_on_one_dependency(const detail::iteration_reference<Iter>& loop)
			: loop(loop) {}

		core::impl::reference::dependencies<core::impl::reference::fixed_sized<1>> toCoreDependencies() const {
			return core::after(loop.getHandle());
		}

		detail::SubDependencies<one_on_one_dependency<Iter>> split(const detail::range<Iter>& left, const detail::range<Iter>& right) const {

			// get left and right loop fragments
			auto loopLeft = loop.getLeft();
			auto loopRight = loop.getRight();

			// split dependencies, thereby checking range coverage
			return {
				// we take the sub-task if it covers the targeted range, otherwise we stick to the current range
				loopLeft.getRange().covers(left)   ? one_on_one_dependency<Iter>{loopLeft}  : *this,
				loopRight.getRange().covers(right) ? one_on_one_dependency<Iter>{loopRight} : *this
			};

		}

		friend std::ostream& operator<< (std::ostream& out, const one_on_one_dependency& dep) {
			return out << dep.loop.getRange();
		}

	};

	template<typename Iter>
	class neighborhood_sync_dependency : public detail::loop_dependency {

		using deps_list = std::array<detail::iteration_reference<Iter>,3>;

		deps_list deps;

		std::size_t size;

		neighborhood_sync_dependency(std::array<detail::iteration_reference<Iter>,3>&& deps)
			: deps(std::move(deps)), size(3) {}

	public:

		neighborhood_sync_dependency(const detail::iteration_reference<Iter>& loop)
			: deps({{ loop }}), size(1) {}

		core::impl::reference::dependencies<core::impl::reference::fixed_sized<3>> toCoreDependencies() const {
			return core::after(deps[0],deps[1],deps[2]);
		}

		detail::SubDependencies<neighborhood_sync_dependency<Iter>> split(const detail::range<Iter>& left, const detail::range<Iter>& right) const {
			using iter_dependency = detail::iteration_reference<Iter>;

			// check for the root case
			if (size == 1) {
				const auto& dependency = deps.front();

				// split the dependency
				const iter_dependency& left = dependency.getLeft();
				const iter_dependency& right = dependency.getRight();

				// combine sub-dependencies
				iter_dependency start ({{left.getRange().begin(),left.getRange().begin()}});
				iter_dependency finish({{right.getRange().end(), right.getRange().end() }});

				return {
					neighborhood_sync_dependency(deps_list{{ start, left,  right  }}),
					neighborhood_sync_dependency(deps_list{{ left,  right, finish }})
				};
			}

			// split up input dependencies
			assert(size == 3);

			// split each of those
			auto a = deps[0].getRight();
			auto b = deps[1].getLeft();
			auto c = deps[1].getRight();
			auto d = deps[2].getLeft();


			// get covered by left and right
			detail::range<Iter> full(deps[0].getRange().begin(),deps[2].getRange().end());
			detail::range<Iter> leftPart(a.getRange().begin(),c.getRange().end());
			detail::range<Iter> rightPart(b.getRange().begin(),d.getRange().end());


			// check some range consistencies
			assert_true(full.covers(left.grow(full)));
			assert_true(full.covers(right.grow(full)));

			// and pack accordingly
			return {
				leftPart.covers(left.grow(full))   ? neighborhood_sync_dependency(deps_list{{ a,b,c }}) : *this,
				rightPart.covers(right.grow(full)) ? neighborhood_sync_dependency(deps_list{{ b,c,d }}) : *this
			};

		}

		friend std::ostream& operator<< (std::ostream& out, const neighborhood_sync_dependency& dep) {
			return out << "[" << utils::join(",",dep.deps,[](std::ostream& out, const detail::iteration_reference<Iter>& cur) { out << cur.getRange(); }) << "]";
		}

	};


	template<typename Iter, typename InnerBody, typename BoundaryBody, typename Dependency>
	detail::loop_reference<Iter> pforWithBoundary(const detail::range<Iter>& r, const InnerBody& innerBody, const BoundaryBody& boundaryBody, const Dependency& dependency) {

		struct rangeHelper {
			detail::range<Iter> range;
			Dependency dependencies;
		};

		// keep a copy of the full range
		auto full = r;

		// trigger parallel processing
		return { r, core::prec(
			[](const rangeHelper& rg) {
				// if there is only one element left, we reached the base case
				return rg.range.size() <= 1;
			},
			[innerBody,boundaryBody,full](const rangeHelper& rg) {
				// apply the body operation to every element in the remaining range
				rg.range.forEachWithBoundary(full,innerBody,boundaryBody);
			},
			core::pick(
				[](const rangeHelper& rg, const auto& nested) {
					// in the step case we split the range and process sub-ranges recursively
					auto fragments = rg.range.split();
					auto& left = fragments.first;
					auto& right = fragments.second;
					auto dep = rg.dependencies.split(left,right);
					return parallel(
						nested(dep.left.toCoreDependencies(), rangeHelper{left, dep.left} ),
						nested(dep.right.toCoreDependencies(), rangeHelper{right,dep.right})
					);
				},
				[innerBody,boundaryBody,full](const rangeHelper& rg, const auto&) {
					// the alternative is processing the step sequentially
					rg.range.forEachWithBoundary(full,innerBody,boundaryBody);
				}
			)
		)(dependency.toCoreDependencies(),rangeHelper{r,dependency}) };
	}

	template<typename Iter, typename InnerBody, typename BoundaryBody>
	detail::loop_reference<Iter> pforWithBoundary(const detail::range<Iter>& r, const InnerBody& innerBody, const BoundaryBody& boundaryBody, const no_dependencies&) {
		using range = detail::range<Iter>;

		// keep a copy of the full range
		auto full = r;

		// trigger parallel processing
		return { r, core::prec(
			[](const range& r) {
				// if there is only one element left, we reached the base case
				return r.size() <= 1;
			},
			[innerBody,boundaryBody,full](const range& r) {
				// apply the body operation to every element in the remaining range
				r.forEachWithBoundary(full,innerBody,boundaryBody);
			},
			core::pick(
				[](const range& r, const auto& nested) {
					// in the step case we split the range and process sub-ranges recursively
					auto fragments = r.split();
					auto& left = fragments.first;
					auto& right = fragments.second;
					return parallel(
						nested(left),
						nested(right)
					);
				},
				[innerBody,boundaryBody,full](const range& r, const auto&) {
					// the alternative is processing the step sequentially
					r.forEachWithBoundary(full,innerBody,boundaryBody);
				}
			)
		)(r) };
	}




	template<typename Iter, typename Point, typename Action>
	detail::loop_reference<Iter> after(const detail::loop_reference<Iter>& loop, const Point& point, const Action& action) {

		// get the full range
		auto r = loop.getRange();

		struct rangeHelper {
			detail::range<Iter> range;
			one_on_one_dependency<Iter> dependencies;
		};

		// get the initial dependency
		auto dependency = one_on_one(loop);

		// trigger parallel processing
		return { r, core::prec(
			[point](const rangeHelper& rg) {
				// check whether the point of action is covered by the current range
				return !rg.range.covers(point);
			},
			[action,point](const rangeHelper& rg) {
				// trigger the action if the current range covers the point
				if (rg.range.covers(point)) action();

			},
			core::pick(
				[](const rangeHelper& rg, const auto& nested) {
					// in the step case we split the range and process sub-ranges recursively
					auto fragments = rg.range.split();
					auto& left = fragments.first;
					auto& right = fragments.second;
					auto dep = rg.dependencies.split(left,right);
					return parallel(
						nested(dep.left.toCoreDependencies(), rangeHelper{left, dep.left} ),
						nested(dep.right.toCoreDependencies(), rangeHelper{right,dep.right})
					);
				},
				[action,point](const rangeHelper& rg, const auto&) {
					// trigger the action if the current range covers the point
					if (rg.range.covers(point)) action();
				}
			)
		)(dependency.toCoreDependencies(),rangeHelper{r,dependency}) };
	}


} // end namespace user
} // end namespace api
} // end namespace allscale
