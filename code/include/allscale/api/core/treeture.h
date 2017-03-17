#pragma once

#include <utility>

/**
 * This header file formalizes the general, public interface of treetures, independent
 * of any actual implementation.
 *
 * TODO: extend on this here ...
 */

#include "allscale/api/core/impl/sequential/treeture.h"
#include "allscale/api/core/impl/reference/treeture.h"

namespace allscale {
namespace api {
namespace core {


	// --------------------------------------------------------------------------------------------
	//											Treetures
	// --------------------------------------------------------------------------------------------


	/**
	 * The actual treeture, referencing the computation of a value.
	 */
	template<typename T>
	using treeture = impl::reference::treeture<T>;

	/**
	 * A reference to a sub-task, to create
	 */
	using task_reference = impl::reference::task_reference;


	// ---------------------------------------------------------------------------------------------
	//						               Auxiliary Construct
	// ---------------------------------------------------------------------------------------------


	namespace detail {

		template<typename T>
		struct completed_task {

			using value_type = T;

			T value;

			operator impl::sequential::unreleased_treeture<T>() {
				return impl::sequential::done(value);
			}

			operator impl::reference::unreleased_treeture<T>() {
				return impl::reference::done(value);
			}

			operator impl::sequential::treeture<T>() {
				return impl::sequential::done(value);
			}

			operator impl::reference::treeture<T>() {
				return impl::reference::done(value);
			}

			T get() {
				return value;
			}

		};

		template<>
		struct completed_task<void> {

			using value_type = void;

			operator impl::sequential::unreleased_treeture<void>() {
				return impl::sequential::done();
			}

			operator impl::reference::unreleased_treeture<void>() {
				return impl::reference::done();
			}

			operator impl::sequential::treeture<void>() {
				return impl::sequential::done();
			}

			operator impl::reference::treeture<void>() {
				return impl::reference::done();
			}

			void get() {
			}

		};

	}


	// ---------------------------------------------------------------------------------------------
	//											Operators
	// ---------------------------------------------------------------------------------------------

	// --- dependencies ---

	/**
	 * A class aggregating task dependencies.
	 */
	class dependencies {

		impl::reference::dependencies par_deps;

	public:

		dependencies() {}

		dependencies(const impl::sequential::task_reference&) {}

		dependencies(const impl::reference::task_reference& dep) {
			par_deps.add(dep);
		}

		dependencies(dependencies&& other) = default;

		dependencies& operator=(dependencies&& other) = default;

		// -- mutators --

		dependencies& add(const impl::sequential::task_reference&) {
			// can be ignored, since sequential dependencies are always satisfied
			return *this;
		}

		dependencies& add(const impl::reference::task_reference& dep) {
			par_deps.add(dep);
			return *this;
		}

		// -- implicit and explicit converters --

		operator impl::sequential::dependencies() const {
			return impl::sequential::dependencies();
		}

		impl::sequential::dependencies toSequentialDependencies() const {
			return *this;
		}

		operator impl::reference::dependencies&&() && {
			return std::move(par_deps);
		}

		impl::reference::dependencies&& toReferenceDependencies() && {
			return std::move(par_deps);
		}

	};

	// -- no dependencies --

	inline dependencies after() {
		return dependencies();
	}


	// -- sequential --

	template<typename ... Rest>
	dependencies after(const impl::sequential::task_reference& dep, const Rest& ... rest) {
		return std::move(allscale::api::core::after(rest...).add(dep));
	}


	// -- reference --

	template<typename ... Rest>
	dependencies after(const impl::reference::task_reference& dep, const Rest& ... rest) {
		return std::move(allscale::api::core::after(rest...).add(dep));
	}


	// --- releasing tasks ---

	template<typename T>
	inline impl::sequential::treeture<T> run(impl::sequential::unreleased_treeture<T>&& treeture) {
		return std::move(treeture).release();
	}

	template<typename T, typename Gen>
	inline impl::sequential::treeture<T> run(impl::sequential::lazy_unreleased_treeture<T,Gen>&& treeture) {
		return std::move(treeture).release();
	}

	template<typename T>
	inline impl::reference::treeture<T> run(impl::reference::unreleased_treeture<T>&& treeture) {
		return std::move(treeture).release();
	}


	// --- completed tasks ---

	inline detail::completed_task<void> done() {
		return detail::completed_task<void>();
	}

	template<typename T>
	detail::completed_task<T> done(const T& value) {
		return detail::completed_task<T>{value};
	}


	// --- control flow ---


	namespace detail {

		/**
		 * Different implementations utilized by this reference implementation.
		 */

		struct DoneImpl {

			template<typename T>
			auto convertParameter(completed_task<T>&& a) const {
				return std::move(a);
			}

			template<typename A, typename B>
			auto sequential(completed_task<A>&&,completed_task<B>&&) {
				return done();
			}

			template<typename D, typename A, typename B>
			auto sequential(const D&, completed_task<A>&&,completed_task<B>&&) {
				return done();
			}

			template<typename A, typename B>
			auto parallel(completed_task<A>&&,completed_task<B>&&) {
				return done();
			}

			template<typename D, typename A, typename B>
			auto parallel(const D&, completed_task<A>&&,completed_task<B>&&) {
				return done();
			}

			template<typename A, typename B, typename M>
			auto combine(completed_task<A>&& a, completed_task<B>&& b, M&& m, bool) {
				return done(m(a.get(),b.get()));
			}

			template<typename D, typename A, typename B, typename M>
			auto combine(const D&, completed_task<A>&& a, completed_task<B>&& b, M&& m, bool) {
				return done(m(a.get(),b.get()));
			}

		};

		struct SequentialImpl {

			template<typename T>
			auto convertParameter(completed_task<T>&& a) const {
				return impl::sequential::done(a.get());
			}

			template<typename T, typename FT>
			auto convertParameter(impl::sequential::lazy_unreleased_treeture<T,FT>&& a) const {
				return std::move(a);
			}

			template<typename A, typename FA, typename B, typename FB>
			auto sequential(impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b) {
				return impl::sequential::sequential(std::move(a),std::move(b));
			}

			template<typename A, typename FA, typename B, typename FB>
			auto sequential(impl::sequential::dependencies&& deps, impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b) {
				return impl::sequential::sequential(std::move(deps),std::move(a),std::move(b));
			}

			template<typename A, typename FA, typename B, typename FB>
			auto parallel(impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b) {
				return impl::sequential::parallel(std::move(a),std::move(b));
			}

			template<typename A, typename FA, typename B, typename FB>
			auto parallel(impl::sequential::dependencies&& deps, impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b) {
				return impl::sequential::parallel(std::move(deps),std::move(a),std::move(b));
			}

			template<typename A, typename FA, typename B, typename FB, typename M>
			auto combine(impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b, M&& m, bool parallel) {
				return impl::sequential::combine(std::move(a),std::move(b),std::move(m), parallel);
			}

			template<typename A, typename FA, typename B, typename FB, typename M>
			auto combine(impl::sequential::dependencies&& deps, impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b, M&& m, bool parallel) {
				return impl::sequential::combine(std::move(deps),std::move(a),std::move(b),std::move(m), parallel);
			}
		};

		struct ReferenceImpl {

			template<typename T>
			auto convertParameter(completed_task<T>&& a) const {
				return impl::reference::done(a.get());
			}

			template<typename T>
			auto convertParameter(impl::reference::unreleased_treeture<T>&& a) const {
				return std::move(a);
			}

			template<typename A, typename B>
			auto sequential(impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b) {
				return impl::reference::sequential(std::move(a),std::move(b));
			}

			template<typename A, typename B>
			auto sequential(impl::reference::dependencies&& deps, impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b) {
				return impl::reference::sequential(std::move(deps),std::move(a),std::move(b));
			}

			template<typename A, typename B>
			auto parallel(impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b) {
				return impl::reference::parallel(std::move(a),std::move(b));
			}

			template<typename A, typename B>
			auto parallel(impl::reference::dependencies&& deps, impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b) {
				return impl::reference::parallel(std::move(deps),std::move(a),std::move(b));
			}

			template<typename A, typename B, typename M>
			auto combine(impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b, M&& m, bool parallel) {
				return impl::reference::combine(std::move(a),std::move(b),std::move(m), parallel);
			}

			template<typename A, typename B, typename M>
			auto combine(impl::reference::dependencies&& deps, impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b, M&& m, bool parallel) {
				return impl::reference::combine(std::move(deps),std::move(a),std::move(b),std::move(m), parallel);
			}
		};


		/**
		 * A mapping of parameter combinations to implementations:
		 *
		 *   done, done -> done
		 *
		 *   seq , seq  -> seq
		 *   seq , done -> seq
		 *   done, seq  -> seq
		 *
		 *   ref , ref  -> ref
		 *   ref , done -> ref
		 *   done, ref  -> ref
		 *
		 * others are illegal
		 */

		template<typename A, typename B>
		struct implementation;

		template<typename A, typename B>
		struct implementation<completed_task<A>,completed_task<B>> : public DoneImpl {};

		template<typename A, typename FA, typename B, typename FB>
		struct implementation<impl::sequential::lazy_unreleased_treeture<A,FA>,impl::sequential::lazy_unreleased_treeture<B,FB>> : public SequentialImpl {};

		template<typename A, typename FA, typename B>
		struct implementation<impl::sequential::lazy_unreleased_treeture<A,FA>,completed_task<B>> : public SequentialImpl {};

		template<typename A, typename B, typename FB>
		struct implementation<completed_task<A>,impl::sequential::lazy_unreleased_treeture<B,FB>> : public SequentialImpl {};

		template<typename A, typename B>
		struct implementation<impl::reference::unreleased_treeture<A>,impl::reference::unreleased_treeture<B>> : public ReferenceImpl {};

		template<typename A, typename B>
		struct implementation<impl::reference::unreleased_treeture<A>,completed_task<B>> : public ReferenceImpl {};

		template<typename A, typename B>
		struct implementation<completed_task<A>,impl::reference::unreleased_treeture<B>> : public ReferenceImpl {};

	}


	// -- sequential --

	template<typename D, typename A, typename B>
	auto sequential(D&& deps, A&& a, B&& b) {
		detail::implementation<A,B> impl;
		return impl.sequential(std::move(deps),impl.convertParameter(std::move(a)),impl.convertParameter(std::move(b)));
	}

	template<typename A, typename B>
	auto sequential(A&& a, B&& b) {
		detail::implementation<A,B> impl;
		return impl.sequential(impl.convertParameter(std::move(a)),impl.convertParameter(std::move(b)));
	}


	// -- parallel --

	template<typename D, typename A, typename B>
	auto parallel(D&& deps, A&& a, B&& b) {
		detail::implementation<A,B> impl;
		return impl.parallel(std::move(deps),impl.convertParameter(std::move(a)),impl.convertParameter(std::move(b)));
	}

	template<typename A, typename B>
	auto parallel(A&& a, B&& b) {
		detail::implementation<A,B> impl;
		return impl.parallel(impl.convertParameter(std::move(a)),impl.convertParameter(std::move(b)));
	}


	// --- aggregation ---


	template<typename D, typename A, typename B, typename M>
	auto combine(D&& deps, A&& a, B&& b, M&& m, bool parallel = true) {
		detail::implementation<A,B> impl;
		return impl.combine(std::move(deps),impl.convertParameter(std::move(a)),impl.convertParameter(std::move(b)), std::move(m), parallel);
	}

	template<typename A, typename B, typename M>
	auto combine(A&& a, B&& b, M&& m, bool parallel = true) {
		detail::implementation<A,B> impl;
		return impl.combine(impl.convertParameter(std::move(a)),impl.convertParameter(std::move(b)), std::move(m), parallel);
	}


} // end namespace core
} // end namespace api
} // end namespace allscale

