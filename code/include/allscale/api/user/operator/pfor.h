#pragma once

#include <utility>

#include "allscale/api/core/prec.h"
#include "allscale/api/user/data/vector.h"

#include "allscale/utils/assert.h"

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

		// -- Adaptive Loop Dependencies --

		class iteration_reference {

			friend class iteration_dependency;

			core::treeture<void> handle;

		public:

			iteration_reference(core::treeture<void>&& handle) : handle(std::move(handle)) {}

			iteration_reference() {};
			iteration_reference(const iteration_reference&) = delete;
			iteration_reference(iteration_reference&&) = default;

			iteration_reference& operator=(const iteration_reference&) = delete;
			iteration_reference& operator=(iteration_reference&&) = default;

			~iteration_reference() { wait(); }

			void wait() {
				handle.get();
			}

		};

		class iteration_dependency {

			core::task_reference handle;

			iteration_dependency(core::task_reference&& handle)
				: handle(std::move(handle)) {}

		public:

			iteration_dependency() {}

			iteration_dependency(const iteration_reference& ref)
				: handle(ref.handle) {}


			void wait() const {
				handle.wait();
			}

			iteration_dependency getLeft() const {
				return handle.getLeft();
			}

			iteration_dependency getRight() const {
				return handle.getRight();
			}

			operator core::task_reference() const {
				return handle;
			}

		};

		class Dependencies {

			std::vector<iteration_dependency> dependencies;

		public:

			Dependencies() {}

			template<typename ... Refs>
			Dependencies(const Refs& ... refs)
				: dependencies({ refs... }) {}

			Dependencies(const Dependencies&) = default;
			Dependencies(Dependencies&&) = default;

			Dependencies& operator=(const Dependencies&) = default;
			Dependencies& operator=(Dependencies&&) = default;

			bool isSingle() const {
				return dependencies.size() == 1;
			}

			const iteration_dependency& getDependency() const {
				assert_true(isSingle());
				return dependencies[0];
			}

			const std::vector<iteration_dependency>& getDependencies() const {
				return dependencies;
			}

			operator std::vector<core::task_reference>() const {
				std::vector<core::task_reference> res;
				for(const auto& cur : dependencies) res.push_back(cur);
				return res;
			}

			void wait() const {
				for(const auto& cur : dependencies) {
					cur.wait();
				}
			}

		};

		struct SubDependencies {
			Dependencies left;
			Dependencies right;
		};

	}

	struct no_dependencies {

		const detail::Dependencies& getInitial() const {
			static const detail::Dependencies none = detail::Dependencies();
			return none;
		}

		static detail::SubDependencies split(const detail::Dependencies&) {
			return detail::SubDependencies();
		}

	};


	template<typename Iter, size_t dims, typename Body>
	detail::iteration_reference pfor(const std::array<Iter,dims>& a, const std::array<Iter,dims>& b, const Body& body) {

		// process 0-dimensional case
		if (dims == 0) return detail::iteration_reference(core::done()); // no iterations required

		// implements a recursive splitting policy for iterating over the given iterator range
		using range = std::array<std::pair<Iter,Iter>,dims>;

		// create the full range
		range full;
		for(size_t i = 0; i<dims; i++) {
			full[i] = std::make_pair(a[i],b[i]);
		}

		// trigger parallel processing
		return core::prec(
			[](const range& r) {
				// if there is only one element left, we reached the base case
				return detail::area(r) <= 1;
			},
			[body](const range& r) {
				if (detail::area(r) < 1) return;
				detail::for_each(r,body);
			},
			[](const range& r, const auto& nested) {
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
				return parallel(
					nested(a),
					nested(b)
				);
			}
		)(full);
	}

	/**
	 * A parallel for-each implementation iterating over the given range of elements.
	 */
	template<typename Iter, typename Body, typename Dependency>
	detail::iteration_reference pfor(const Iter& a, const Iter& b, const Body& body, const Dependency& dependency) {
		// implements a binary splitting policy for iterating over the given iterator range

		// the iterator range and the local dependency
		struct range {
			Iter begin;
			Iter end;
			detail::Dependencies dependencies;
		};

		// the parallel execution
		return core::prec(
			[](const range& r) {
				return detail::distance(r.begin,r.end) <= 1;
			},
			[body](const range& r) {
				for(auto it = r.begin; it != r.end; ++it) body(detail::access(it));
			},
			[](const range& r, const auto& nested) {
				// here we have the binary splitting
				auto mid = r.begin + (r.end - r.begin)/2;
				auto dep = Dependency::split(r.dependencies);
				return core::parallel(
						nested(core::after(dep.left), range{r.begin,mid,dep.left}),
						nested(core::after(dep.right),range{mid,r.end,dep.right})
				);
			}
		)(range{a,b,dependency.getInitial()});
	}

	template<typename Iter, typename Body>
	detail::iteration_reference pfor(const Iter& a, const Iter& b, const Body& body) {
		return pfor(a,b,body,no_dependencies());
	}

	// ---- container support ----

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	detail::iteration_reference pfor(Container& c, const Op& op) {
		return pfor(c.begin(), c.end(), op);
	}

	/**
	 * A parallel for-each implementation iterating over the elements of the given container.
	 */
	template<typename Container, typename Op>
	detail::iteration_reference pfor(const Container& c, const Op& op) {
		return pfor(c.begin(), c.end(), op);
	}


	// ---- Vector support ----

	/**
	 * A parallel for-each implementation iterating over the elements of the points covered by
	 * the hyper-box limited by the given vectors.
	 */
	template<typename Elem, size_t Dims, typename Body>
	detail::iteration_reference pfor(const data::Vector<Elem,Dims>& a, const data::Vector<Elem,Dims>& b, const Body& body) {
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
	detail::iteration_reference pfor(const data::Vector<Elem,Dims>& a, const Body& body) {
		return pfor(data::Vector<Elem,Dims>(0),a,body);
	}


	// -------------------------------------------------------------------------------------------
	//								Adaptive Synchronization
	// -------------------------------------------------------------------------------------------

	class dependency {

		detail::Dependencies initial;

	public:

		dependency(const detail::iteration_dependency& loop)
			: initial(loop) {}

		const detail::Dependencies& getInitial() const {
			return initial;
		}

	};

	struct one_on_one : public dependency {

		one_on_one(const detail::iteration_dependency& loop)
			: dependency(loop) {}

		static detail::SubDependencies split(const detail::Dependencies& dep) {

			// extract the full dependency
			assert(dep.isSingle());
			const auto& task = dep.getDependency();

			// split the dependencies
			return {
				task.getLeft(), task.getRight()
			};
		}

	};

	struct neighborhood_sync : public dependency {

		neighborhood_sync(const detail::iteration_reference& loop)
			: dependency(loop) {}

		static detail::SubDependencies split(const detail::Dependencies& dep) {
			using iter_dependency = detail::iteration_dependency;


			// check for the root case
			if (dep.isSingle()) {
				const iter_dependency& dependency = dep.getDependency();

				// split the dependency
				const iter_dependency& left = dependency.getLeft();
				const iter_dependency& right = dependency.getRight();

				// combine sub-dependencies
				iter_dependency done;
				return {
					detail::Dependencies( done,left,right ),
					detail::Dependencies( left,right,done )
				};
			}

			// split up input dependencies
			const auto& dependencies = dep.getDependencies();
			if (dependencies.size() == 3) {

				// split each of those
				auto a = dependencies[0].getRight();
				auto b = dependencies[1].getLeft();
				auto c = dependencies[1].getRight();
				auto d = dependencies[2].getLeft();

				// and pack accordingly
				return {
					detail::Dependencies( a,b,c ),
					detail::Dependencies( b,c,d )
				};

			}

			// this should not happen
			assert_fail() << "Unexpected input dependency!";

			// fall-back: no splitting
			return { dep, dep };
		}

	};


} // end namespace user
} // end namespace api
} // end namespace allscale
