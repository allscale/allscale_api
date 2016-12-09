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

	// -- sequential --

	template<typename ... Rest>
	impl::sequential::dependencies after(const impl::sequential::task_reference& f, Rest ... rest) {
		return impl::sequential::after(std::move(f),std::move(rest)...);
	}

	inline impl::sequential::dependencies after(const std::vector<impl::sequential::task_reference>& deps) {
		return impl::sequential::after(deps);
	}



	// -- reference --

	template<typename ... Rest>
	impl::reference::dependencies after(const impl::reference::task_reference& f, Rest ... rest) {
		return impl::reference::after(std::move(f),std::move(rest)...);
	}

	inline impl::reference::dependencies after(const std::vector<impl::reference::task_reference>& deps) {
		return impl::reference::after(deps);
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

	// -- sequential --

	template<typename ... Args>
	auto sequential(impl::sequential::dependencies&& deps, impl::sequential::unreleased_treeture<Args>&& ... args) {
		return impl::sequential::sequential(std::move(deps),std::move(args)...);
	}

	template<typename F, typename ... R>
	auto sequential(impl::sequential::unreleased_treeture<F>&& f, impl::sequential::unreleased_treeture<R>&& ... rest) {
		return impl::sequential::sequential(std::move(f),std::move(rest)...);
	}

	template<typename ... Args, typename ... Impls>
	auto sequential(impl::sequential::dependencies&& deps, impl::sequential::lazy_unreleased_treeture<Args,Impls>&& ... args) {
		return impl::sequential::sequential(std::move(deps),std::move(args)...);
	}

	template<typename F, typename I, typename ... R, typename ... Impls>
	auto sequential(impl::sequential::lazy_unreleased_treeture<F,I>&& f, impl::sequential::lazy_unreleased_treeture<R,Impls>&& ... rest) {
		return impl::sequential::sequential(std::move(f),std::move(rest)...);
	}

	template<typename ... Args>
	auto sequential(impl::reference::dependencies&& deps, impl::reference::unreleased_treeture<Args>&& ... args) {
		return impl::reference::sequential(std::move(deps),std::move(args)...);
	}

	template<typename F, typename ... R>
	auto sequential(impl::reference::unreleased_treeture<F>&& f, impl::reference::unreleased_treeture<R>&& ... rest) {
		return impl::reference::sequential(std::move(f),std::move(rest)...);
	}


	// -- parallel --

	template<typename ... Args>
	auto parallel(impl::sequential::dependencies&& deps, impl::sequential::unreleased_treeture<Args>&& ... args) {
		return impl::sequential::parallel(std::move(deps),std::move(args)...);
	}

	template<typename F, typename ... R>
	auto parallel(impl::sequential::unreleased_treeture<F>&& f, impl::sequential::unreleased_treeture<R>&& ... rest) {
		return impl::sequential::parallel(std::move(f),std::move(rest)...);
	}

	template<typename ... Args, typename ... Impls>
	auto parallel(impl::sequential::dependencies&& deps, impl::sequential::lazy_unreleased_treeture<Args,Impls>&& ... args) {
		return impl::sequential::parallel(std::move(deps),std::move(args)...);
	}

	template<typename F, typename I, typename ... R, typename ... Impls>
	auto parallel(impl::sequential::lazy_unreleased_treeture<F,I>&& f, impl::sequential::lazy_unreleased_treeture<R,Impls>&& ... rest) {
		return impl::sequential::parallel(std::move(f),std::move(rest)...);
	}

	template<typename ... Args>
	auto parallel(impl::reference::dependencies&& deps, impl::reference::unreleased_treeture<Args>&& ... args) {
		return impl::reference::parallel(std::move(deps),std::move(args)...);
	}

	template<typename F, typename ... R>
	auto parallel(impl::reference::unreleased_treeture<F>&& f, impl::reference::unreleased_treeture<R>&& ... rest) {
		return impl::reference::parallel(std::move(f),std::move(rest)...);
	}




	// --- aggregation ---

	template<typename A, typename FA, typename B, typename FB, typename M>
	auto combine(impl::sequential::dependencies&& deps, impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b, M&& m, bool parallel = true) {
		return impl::sequential::combine(std::move(deps), std::move(a),std::move(b),std::move(m),parallel);
	}

	template<typename A, typename FA, typename B, typename FB, typename M>
	auto combine(impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b, M&& m, bool parallel = true) {
		return impl::sequential::combine(std::move(a),std::move(b),std::move(m),parallel);
	}

	template<typename A, typename B, typename M>
	auto combine(impl::reference::dependencies&& deps, impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b, M&& m, bool parallel = true) {
		return impl::reference::combine(std::move(deps), std::move(a),std::move(b),std::move(m),parallel);
	}

	template<typename A, typename B, typename M>
	auto combine(impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b, M&& m, bool parallel = true) {
		return impl::reference::combine(std::move(a),std::move(b),std::move(m),parallel);
	}

	template<typename A, typename B, typename M>
	auto combine(detail::completed_task<A>&& a, detail::completed_task<B>&& b, M&& m, bool = true) {
		return done(m(a.get(),b.get()));
	}

	template<typename A, typename B, typename FB, typename M>
	auto combine(detail::completed_task<A>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b, M&& m, bool parallel = true) {
		return core::combine(impl::sequential::done(a.get()), std::move(b), std::move(m), parallel);
	}

	template<typename A, typename FA, typename B, typename M>
	auto combine(impl::sequential::lazy_unreleased_treeture<A,FA>&& a, detail::completed_task<B>&& b, M&& m, bool parallel = true) {
		return core::combine(std::move(a), impl::sequential::done(b.get()), std::move(m), parallel);
	}

	template<typename A, typename B, typename M>
	auto combine(detail::completed_task<A>&& a, impl::reference::unreleased_treeture<B>&& b, M&& m, bool parallel = true) {
		return core::combine(impl::reference::done(a.get()), std::move(b), std::move(m), parallel);
	}

	template<typename A, typename B, typename M>
	auto combine(impl::reference::unreleased_treeture<A>&& a, detail::completed_task<B>&& b, M&& m, bool parallel = true) {
		return core::combine(std::move(a), impl::reference::done(b.get()), std::move(m), parallel);
	}

} // end namespace core
} // end namespace api
} // end namespace allscale

