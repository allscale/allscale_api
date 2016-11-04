#pragma once

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/arrays.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace sequential {


	// --------------------------------------------------------------------------------------------
	//								   sequential treeture implementation
	// --------------------------------------------------------------------------------------------


	// ------------------------------------- Declarations -----------------------------------------

	/**
	 * The actual treeture, referencing the computation of a value.
	 */
	template<typename T>
	class treeture;

	/**
	 * A factory for producing a treeture. While the treeture factory exists, input dependencies
	 * may be added to the treeture to orchestrate it execution. Once the factory has produced
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
	class dependencies;


	// ------------------------------------- Definitions ------------------------------------------

	// -- treeture --

	template<>
	class treeture<void> {
	public:

		using value_type = void;

		treeture() {}

		template<typename Fun>
		explicit treeture(Fun&& fun) {
			fun();
		}

		void wait() {}

		void get() const {
			// nothing to do
		}

		treeture<void> getLeft() const {
			return {};
		}

		treeture<void> getRight() const {
			return {};
		}

	};

	template<typename T>
	class treeture {

		T value;

	public:

		using value_type = T;

		treeture(const T& value)
			: value(value) {}

		treeture(const T&& value)
			: value(std::move(value)) {}

		template<typename Fun>
		explicit treeture(Fun&& fun)
			: value(fun()) {}

		void wait() {}

		T get() const {
			return value;
		}

		treeture<void> getLeft() const {
			return {};
		}

		treeture<void> getRight() const {
			return {};
		}

	};


	template<typename Op, typename R = std::result_of_t<Op()>>
	treeture<R> make_treeture(Op&& op) {
		return treeture<R>(std::move(op));
	}

	// -- treeture_factory --

	template<typename T>
	class treeture_factory {

		treeture<T> res;

	public:

		using value_type = T;

		template<typename Fun>
		explicit treeture_factory(Fun&& fun)
			: res(fun()) {}

		treeture_factory(const treeture_factory&) =delete;
		treeture_factory(treeture_factory&&) =default;

		treeture_factory& operator=(const treeture_factory&) =delete;
		treeture_factory& operator=(treeture_factory&&) =default;

		template<typename V>
		treeture_factory&& after(const treeture<V>&) && {
			// for the sequential case, dependencies need not be recorded
			return std::move(*this);
		}

		template<typename V>
		treeture_factory&& after(treeture_factory<V>&& factory) && {
			return std::move(*this).after(factory.toTreeture());
		}

		treeture<T> toTreeture() const {
			return res;
		}

		T get() const {
			return toTreeture().get();
		}

		operator treeture<T>() const {
			return res;
		}

	};

	template<typename Gen, typename T = typename std::result_of_t<Gen()>::value_type>
	treeture_factory<T> make_treeture_factory(Gen&& gen) {
		return treeture_factory<T>(std::move(gen));
	}

	template<typename T,typename Gen>
	class lazy_treeture_factory_factory {

		mutable Gen gen;

	public:

		explicit lazy_treeture_factory_factory(Gen&& gen)
			: gen(std::move(gen)) {}

		template<typename V>
		lazy_treeture_factory_factory&& after(const treeture<V>&) && {
			// for the sequential case, dependencies need not be recorded
			return std::move(*this);
		}


		template<typename V>
		lazy_treeture_factory_factory&& after(treeture_factory<V>&& factory) && {
			return std::move(*this).after(factory.toTreeture());
		}

		template<typename V, typename A>
		lazy_treeture_factory_factory&& after(lazy_treeture_factory_factory<V,A>&& factory) && {
			return std::move(*this).after(factory.toTreeture());
		}

		treeture_factory<T> toFactory() const {
			return gen();
		}

		treeture<T> toTreeture() const {
			return toFactory();
		}

		T get() const {
			return toTreeture().get();
		}

		operator treeture_factory<T>() const {
			return toFactory();
		}

		operator treeture<T>() const {
			return toTreeture();
		}

	};

	template<typename Gen, typename T = typename std::result_of_t<Gen()>::value_type>
	lazy_treeture_factory_factory<T,Gen> make_lazy_treeture_factory_factory(Gen&& gen) {
		return lazy_treeture_factory_factory<T,Gen>(std::move(gen));
	}

	/**
	 * There are no dependencies to be recorded, so this object is an empty object.
	 */
	class dependencies {};


	// -------------------------------------- Operators -------------------------------------------


	inline dependencies after() {
		return {};
	}

	template<typename F, typename ... Rest>
	dependencies after(const treeture<F>&, Rest ... rest) {
		return after(std::move(rest)...);
	}

	template<typename F, typename ... Rest>
	dependencies after(treeture_factory<F>&& f, Rest ... rest) {
		f.get(); // wait for this one to be finished
		return after(std::move(rest)...); // wait for the rest
	}

	template<typename F, typename Gen, typename ... Rest>
	dependencies after(lazy_treeture_factory_factory<F,Gen>&& f, Rest ... rest) {
		f.get(); // wait for this one to be finished
		return after(std::move(rest)...); // wait for the rest
	}


	inline auto done() {
		return make_lazy_treeture_factory_factory([=](){
			return make_treeture_factory([=](){ return treeture<void>(); });
		});
	}

	template<typename T>
	auto done(const T& value) {
		return make_lazy_treeture_factory_factory([=](){
			return make_treeture_factory([=](){ return treeture<T>(value); });
		});
	}


	template<typename Op>
	auto spawn(dependencies&&, Op&& op) {
		return make_lazy_treeture_factory_factory([=](){
			return make_treeture_factory([=](){ return make_treeture(std::move(op)); });
		});
	}

	template<typename Op>
	auto spawn(Op&& op) {
		return spawn(after(),std::move(op));
	}


	auto seq() {
		return done();
	}

	template<typename F, typename FA, typename ... R, typename ... RA>
	auto seq(dependencies&&, lazy_treeture_factory_factory<F,FA>&& f, lazy_treeture_factory_factory<R,RA>&& ... rest) {
		return make_lazy_treeture_factory_factory([f,rest...]() mutable {
			return make_treeture_factory([f,rest...]() mutable {
				return make_treeture([f,rest...]() mutable {
					f.get();
					seq(std::move(rest)...).get();
				});
			});
		});
	}

	template<typename F, typename FA, typename ... R, typename ... RA>
	auto seq(lazy_treeture_factory_factory<F,FA>&& f, lazy_treeture_factory_factory<R,RA>&& ... rest) {
		return seq(after(), std::move(f),std::move(rest)...);
	}

	template<typename ... T, typename ... TA>
	auto par(dependencies&&, lazy_treeture_factory_factory<T,TA>&& ... tasks) {
		// for the sequential implementation, parallel is the same as sequential
		return seq(tasks...);
	}

	template<typename ... T, typename ... TA>
	auto par(lazy_treeture_factory_factory<T,TA>&& ... tasks) {
		return par(after(), tasks...);
	}


	template<typename A, typename AA, typename B, typename BA, typename M>
	auto combine(lazy_treeture_factory_factory<A,AA>&& a, lazy_treeture_factory_factory<B,BA>&& b, M&& m) {
		return make_lazy_treeture_factory_factory([=]() {
			return make_treeture_factory([=]() {
				return make_treeture([=]() {
					return m(a.get(),b.get());
				});
			});
		});
	}

	template<typename AA, typename BA>
	auto sum(lazy_treeture_factory_factory<int,AA>&& a, lazy_treeture_factory_factory<int,BA>&& b) {
		return combine(std::move(a),std::move(b),[](int a, int b) { return a+b; });
	}


} // end namespace sequential
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale

