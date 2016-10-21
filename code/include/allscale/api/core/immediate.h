#pragma once

#include <type_traits>
#include <utility>

namespace allscale {
namespace api {
namespace core {


	// -----------------------------------------------------------------
	//                             Immediate
	// -----------------------------------------------------------------

	template<typename T>
	class immediate {

		T value;

	public:

		using value_type = T;

		immediate() {}

		immediate(const T& value) : value(value) {}
		immediate(T&& value) : value(std::move(value)) {}

		immediate(const immediate&) = delete;
		immediate(immediate&& other) = default;

		immediate& operator=(const immediate&) = delete;
		immediate& operator=(immediate&& other) = default;

		const T& get() const {
			return value;
		}

	};

	template<>
	class immediate<void> {
	public:

		using value_type = void;

		immediate() {}

		immediate(const immediate&) = delete;
		immediate(immediate&& other) = default;

		immediate& operator=(const immediate&) = delete;
		immediate& operator=(immediate&& other) = default;

		void get() const {
			// nothing
		}

	};


	namespace detail {

		template<
			typename Lambda,
			typename Result
		>
		struct evaluator {
			static immediate<Result> process(const Lambda& l) {
				return immediate<Result>(l());
			}
		};

		template<
			typename Lambda
		>
		struct evaluator<Lambda,void> {
			static immediate<void> process(const Lambda& l) {
				l();
				return immediate<void>();
			}
		};
	}


	template<
		typename Lambda,
		typename O = typename std::result_of_t<Lambda()>
	>
	immediate<O> evaluate(const Lambda& lambda) {
		return detail::evaluator<Lambda,O>::process(lambda);
	}


	// ---------------------------------------------------------------------------------------------
	//											Operators
	// ---------------------------------------------------------------------------------------------



	// --- control flow ---

	template<typename A, typename ... Rest>
	immediate<void> parallel(immediate<A>&&, Rest&& ...) {
		// nothing to do here -- the value has already been computed
		return immediate<void>();
	}

//	template<typename A, typename ... Rest>
//	immediate<void> sequence(immediate<A>&& a, Rest&& ... rest) {
//		// TODO: the order has to be enforced!!
//		// nothing to do here -- the value has already been computed
//		return immediate<void>();
//	}


	// --- aggregation ---

	template<typename A, typename B, typename R = decltype(std::declval<A>() + std::declval<B>())>
	immediate<R> add(immediate<A>&& a, immediate<B>&& b) {
		return immediate<R>(a.get() + b.get());
	}


} // end namespace core
} // end namespace api
} // end namespace allscale
