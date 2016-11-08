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



	// ---------------------------------------------------------------------------------------------
	//						               Auxiliary Construct
	// ---------------------------------------------------------------------------------------------


	namespace detail {

		template<typename T>
		struct completed_task {

			T value;

			operator impl::sequential::unreleased_treeture<T>() {
				return impl::sequential::done(value);
			}

			operator impl::reference::unreleased_treeture<T>() {
				return impl::reference::done(value);
			}

			T get() {
				return value;
			}

		};

		template<>
		struct completed_task<void> {

			operator impl::sequential::treeture<void>() {
				return impl::sequential::done();
			}

			operator impl::reference::treeture<void>() {
				return impl::reference::done();
			}

		};

	}


	// ---------------------------------------------------------------------------------------------
	//											Operators
	// ---------------------------------------------------------------------------------------------

	// --- dependencies ---

	// -- sequential --

	template<typename F, typename ... Rest>
	impl::sequential::dependencies after(const impl::sequential::treeture<F>& f, Rest ... rest) {
		return impl::sequential::after(std::move(f),std::move(rest)...);
	}

	template<typename F, typename ... Rest>
	impl::sequential::dependencies after(impl::sequential::unreleased_treeture<F>&& f, Rest ... rest) {
		return impl::sequential::after(std::move(f),std::move(rest)...);
	}

	template<typename F, typename Gen, typename ... Rest>
	impl::sequential::dependencies after(impl::sequential::lazy_unreleased_treeture<F,Gen>&& f, Rest ... rest) {
		return impl::sequential::after(std::move(f),std::move(rest)...);
	}

	// -- reference --

	template<typename F, typename ... Rest>
	impl::reference::dependencies after(const impl::reference::treeture<F>& f, Rest&& ... rest) {
		return impl::reference::after(std::move(f),std::move(rest)...);
	}

	template<typename F, typename ... Rest>
	impl::reference::dependencies after(const impl::reference::unreleased_treeture<F>& f, Rest&& ... rest) {
		return impl::reference::after(std::move(f),std::move(rest)...);
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


	// --- specific aggregators ---

	template<typename A, typename FA, typename B, typename FB, typename R = decltype(std::declval<A>() + std::declval<B>())>
	auto add(impl::sequential::lazy_unreleased_treeture<A,FA>&& a, impl::sequential::lazy_unreleased_treeture<B,FB>&& b) {
		return core::combine(std::move(a),std::move(b),[](const R& a, const R& b) { return a + b; });
	}

	template<typename A, typename B, typename R = decltype(std::declval<A>() + std::declval<B>())>
	auto add(impl::reference::unreleased_treeture<A>&& a, impl::reference::unreleased_treeture<B>&& b) {
		return core::combine(std::move(a),std::move(b),[](const R& a, const R& b) { return a + b; });
	}

} // end namespace core
} // end namespace api
} // end namespace allscale

