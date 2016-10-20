#pragma once

/**
 * This header file formalizes the general, public interface of treetures, independent
 * of any actual implementation.
 *
 * TODO: extend on this here ...
 */

// This header starts by importing the actual future implementation. The
// implementation includes:
//		- the definintion of the future
//		- a minimal set of basic factory functions
//
#ifdef OMP_CILK_IMPL
	// a OpenMP / Cilk based implementation
	#include "allscale/api/core/impl/omp_cilk/treeture.h"
#else
	#ifndef REFERENCE_IMPL
		#define REFERENCE_IMPL 1
	#endif
	// a reference-runtime based implementation
	#include "allscale/api/core/impl/reference/treeture.h"
#endif

namespace allscale {
namespace api {
namespace core {


	// ---------------------------------------------------------------------------------------------
	//											Treetures
	// ---------------------------------------------------------------------------------------------

	namespace detail {
		template<typename T>
		using treeture_impl = impl::treeture<T>;
	}


	template<typename T>
	class treeture;


	template<>
	class treeture<void> : public detail::treeture_impl<void> {

		using impl = detail::treeture_impl<void>;
		
		treeture() {}

		template<typename ... Args>
		treeture(Args&& ... args) : impl(std::move(args)...) {}

	public:

		using value_type = void;

		treeture(const treeture&) = default;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = default;
		treeture& operator=(treeture&& other) = default;

		// support implicit cast to void treeture
		template<typename T>
		treeture(const treeture<T>& t) : impl(t) {}

		// support implicit cast to void treeture
		template<typename T>
		treeture(treeture<T>&& t) : impl(std::move(t)) {}

		void wait() { impl::wait(); }

		void get() { impl::get(); }

		treeture& descentLeft() {
			impl::descentLeft();
			return *this;
		}

		treeture& descentRight() {
			impl::descentRight();
			return *this;
		}

		treeture getLeft() const {
			return impl::getLeft();
		}

		treeture getRight() const {
			return impl::getRight();
		}


		// -- factories --

		static treeture done() {
			return impl::done();
		}

		template<typename Action>
		static treeture spawn(const Action& a) {
			return impl::spawn(a);
		}

		template<typename Process, typename Split>
		static treeture spawn(const Process& p, const Split& s) {
			return impl::spawn(p,s);
		}

		template<typename A, typename B>
		static treeture combine(treeture<A>&& a, treeture<B>&& b, bool parallel = true) {
			return impl::combine(std::move(a), std::move(b), parallel);
		}

	};

	template<typename T>
	class treeture : public detail::treeture_impl<T> {

		using impl = detail::treeture_impl<T>;

		template<typename ... Args>
		treeture(Args&& ... args) : impl(std::move(args)...) {}

	public:

		using value_type = T;

		treeture(const treeture&) = delete;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = delete;
		treeture& operator=(treeture&& other) = default;

		void wait() { impl::wait(); }

		const T& get() { return impl::get(); }

		treeture<void> getLeft() const {
			return impl::getLeft();
		}

		treeture<void> getRight() const {
			return impl::getRight();
		}

		// -- factories --

		static treeture done(const T& value) {
			return impl::done(value);
		}

		template<typename Action>
		static treeture spawn(const Action& a) {
			return impl::spawn(a);
		}

		template<typename Process, typename Split>
		static treeture spawn(const Process& p, const Split& s) {
			return impl::spawn(p,s);
		}

		template<typename A, typename B, typename C>
		static treeture combine(treeture<A>&& a, treeture<B>&& b, C&& merge, bool parallel = true) {
			return impl::combine(std::move(a),std::move(b), std::move(merge),parallel);
		}

	};



	// ---------------------------------------------------------------------------------------------
	//											Operators
	// ---------------------------------------------------------------------------------------------

	// --- basic ---

	inline treeture<void> done() {
		return treeture<void>::done();
	}

	template<typename T>
	treeture<T> done(const T& value) {
		return treeture<T>::done(value);
	}


	template<
		typename Action,
		typename R = std::result_of_t<Action()>
	>
	treeture<R> spawn(const Action& a) {
		return treeture<R>::spawn(a);
	}

	template<
		typename Process,
		typename Split,
		typename R = std::result_of_t<Process()>
	>
	treeture<R> spawn(const Process& p, const Split& s) {
		return treeture<R>::spawn(p,s);
	}


	// --- control flow ---

	namespace detail {

		template<typename R>
		treeture<R> to_treeture(treeture<R>&& a) {
			return std::move(a);
		}

		template<typename A, typename R = std::result_of_t<A()>>
		treeture<R> to_treeture(A&& a) {
			return spawn(a);
		}

		template<typename T>
		struct value_type {
			using type = typename std::result_of_t<T()>;
		};

		template<typename T>
		struct value_type<treeture<T>> {
			using type = T;
		};

		template<typename T>
		using value_type_t = typename value_type<T>::type;

	}


	inline treeture<void> parallel() {
		return done();
	}

	template<typename A>
	treeture<void> parallel(A&& a) {
		return detail::to_treeture(std::move(a));
	}

	template<typename A, typename B>
	treeture<void> parallel(A&& a, B&& b) {
		return treeture<void>::combine(detail::to_treeture(std::move(a)),detail::to_treeture(std::move(b)),true);
	}

	template<typename F, typename ... Rest>
	treeture<void> parallel(F&& first, Rest&& ... rest) {
		// TODO: balance this tree
		return parallel(first, parallel(rest...));
	}


	inline treeture<void> sequence() {
		return done();
	}

	template<typename A>
	treeture<void> sequence(A&& a) {
		return detail::to_treeture(std::move(a));
	}

	template<typename A, typename B>
	treeture<void> sequence(A&& a, B&& b) {
		return treeture<void>::combine(detail::to_treeture(std::move(a)),detail::to_treeture(std::move(b)),false);
	}

	template<typename F, typename ... Rest>
	treeture<void> sequence(F&& first, Rest&& ... rest) {
		// TODO: balance this tree
		return sequence(std::move(first), sequence(rest...));
	}

	// --- aggregation ---

	template<typename A, typename B, typename R = detail::value_type_t<A>>
	treeture<R> add(A&& a, B&& b) {
		return treeture<R>::combine(detail::to_treeture(std::move(a)),detail::to_treeture(std::move(b)),[](R a, R b) { return a + b; });
	}


} // end namespace core
} // end namespace api
} // end namespace allscale

