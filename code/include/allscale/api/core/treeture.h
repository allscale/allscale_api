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


	// ------------------------------------- Declarations -----------------------------------------

	/**
	 * The actual treeture, referencing the computation of a value.
	 */
	template<typename T>
	class treeture;

	/**
	 * A factory for producing a treeture. While the treeture factory exists, input dependencies
	 * may be added to the treeture to orchestrate its execution. Once the factory has produced
	 * the treeture, the dependencies get frozen and the associated task is scheduled for execution.
	 */
	template<typename T>
	class treeture_factory;

	/**
	 * A treeture factory factory that is delaying the creation of the factory until
	 * the actual evaluation. This is to prevent the need to materialize the entire computation
	 * tree before being able to start the computation.
	 */
	template<typename T, typename Gen>
	class lazy_treeture_factory_factory;

	/**
	 * A class to model task dependencies
	 */
	template<typename ... Vs>
	class dependencies;


	// ------------------------------------- Definitions ------------------------------------------


	template<>
	class treeture<void> : public detail::treeture_impl<void> {

		using impl = detail::treeture_impl<void>;
		
		treeture(detail::treeture_impl<void>&& treeture) : impl(std::move(treeture)) {}

	public:

		using value_type = void;

		treeture() {}

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

		void start() { impl::start(); }

		void wait() const { impl::wait(); }

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

		treeture& after(const treeture& dep) {
			return after({dep});
		}

		treeture& after(treeture_dependencies&& deps) {
			impl::after(std::move(deps));
			return *this;
		}

		// -- factories --

		static treeture done() {
			return impl::done();
		}

		template<typename Action>
		static treeture spawn(treeture_dependencies&& deps, const Action& a) {
			return impl::spawn(std::move(deps),a);
		}

		template<typename Process, typename Split>
		static treeture spawn(treeture_dependencies&& deps, const Process& p, const Split& s) {
			return impl::spawn(std::move(deps),p,s);
		}

		template<typename A, typename B>
		static treeture combine(treeture_dependencies&& deps, treeture<A>&& a, treeture<B>&& b, bool parallel = true) {
			return impl::combine(std::move(deps),std::move(a), std::move(b), parallel);
		}

	};

	template<typename T>
	class treeture : public detail::treeture_impl<T> {

		using impl = detail::treeture_impl<T>;

		template<typename ... Args>
		treeture(Args&& ... args) : impl(std::move(args)...) {}

	public:

		using value_type = T;

		treeture(const T& value) : impl(value) {}
		treeture(T&& value) : impl(std::move(value)) {}

		treeture(const treeture&) = delete;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = delete;
		treeture& operator=(treeture&& other) = default;

		void start() { impl::start(); }

		void wait() const { impl::wait(); }

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
		static treeture spawn(treeture_dependencies&& deps, const Action& a) {
			return impl::spawn(std::move(deps),a);
		}

		template<typename Process, typename Split>
		static treeture spawn(treeture_dependencies&& deps, const Process& p, const Split& s) {
			return impl::spawn(std::move(deps),p,s);
		}

		template<typename A, typename B, typename C>
		static treeture combine(treeture_dependencies&& deps, treeture<A>&& a, treeture<B>&& b, C&& merge, bool parallel = true) {
			return impl::combine(std::move(deps),std::move(a),std::move(b), std::move(merge),parallel);
		}

	};



	// ---------------------------------------------------------------------------------------------
	//											Type Traits
	// ---------------------------------------------------------------------------------------------

	template<typename T>
	struct to_treeture {
		using type = treeture<T>;
	};

	template<typename T>
	struct to_treeture<treeture<T>> {
		using type = treeture<T>;
	};

	template<typename T>
	struct is_treeture : public std::false_type {};

	template<typename T>
	struct is_treeture<treeture<T>> : public std::true_type {};




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
		return treeture<R>::spawn(treeture_dependencies(),a);
	}

	template<
		typename Process,
		typename Split,
		typename R = std::result_of_t<Process()>
	>
	treeture<R> spawn(const Process& p, const Split& s) {
		return treeture<R>::spawn(treeture_dependencies(),p,s);
	}


	// --- control flow ---


	template<typename A>
	treeture<void> parallel(treeture<A>&& a) {
		return std::move(a);
	}

	template<typename A, typename B>
	treeture<void> parallel(treeture<A>&& a, treeture<B>&& b) {
		return treeture<void>::combine(treeture_dependencies(),std::move(a),std::move(b),true);
	}

	template<typename F, typename ... Rest>
	treeture<void> parallel(treeture<F>&& first, treeture<Rest>&& ... rest) {
		// TODO: balance this tree
		return parallel(std::move(first), parallel(std::move(rest)...));
	}


	template<typename A>
	treeture<void> sequence(treeture<A>&& a) {
		return std::move(a);
	}

	template<typename A, typename B>
	treeture<void> sequence(treeture<A>&& a, treeture<B>&& b) {
		return treeture<void>::combine(treeture_dependencies(),std::move(a),std::move(b),false);
	}

	template<typename F, typename ... Rest>
	treeture<void> sequence(treeture<F>&& first, treeture<Rest>&& ... rest) {
		// TODO: balance this tree
		return sequence(std::move(first), sequence(std::move(rest)...));
	}

	// --- aggregation ---

	template<typename A, typename B, typename R = decltype(std::declval<A>() + std::declval<B>())>
	treeture<R> add(treeture<A>&& a, treeture<B>&& b) {
		return treeture<R>::combine(treeture_dependencies(),std::move(a),std::move(b),[](R a, R b) { return a + b; });
	}


} // end namespace core
} // end namespace api
} // end namespace allscale

