#pragma once

#include <functional>
#include <vector>

#include "allscale/utils/functional_utils.h"
#include "allscale/utils/vector_utils.h"
#include "allscale/api/core/treeture.h"

namespace allscale {
namespace api {
namespace core {

	template<typename T> struct prec_fun;

	template<typename O, typename I>
	struct prec_fun<O(I)> {
		typedef std::function<treeture<O>(I)> type;
	};

	namespace detail {

		int rand(int x) {
			return (std::rand()/(float)RAND_MAX) * x;
		}

		template<typename T>
		T pickRandom(const T& t) {
			return t;
		}

		template<typename T, typename ... Others>
		T pickRandom(const T& first, const Others& ... others) {
			if (rand(sizeof...(Others)+1) == 0) return first;
			return pickRandom(others...);
		}

		template<typename E>
		E pickRandom(const std::vector<E>& list) {
			return list[rand(list.size())];
		}

	} // end namespace detail

	// ----- function handling ----------

	namespace detail {

		namespace detail {

			template<typename Function> struct fun_type_of_helper { };

			template<typename R, typename ... A>
			struct fun_type_of_helper<R(A...)> {
			  typedef R type(A...);
			};

			// get rid of const modifier
			template<typename T>
			struct fun_type_of_helper<const T> : public fun_type_of_helper<T> {};

			// get rid of pointers
			template<typename T>
			struct fun_type_of_helper<T*> : public fun_type_of_helper<T> {};

			// handle class of member function pointers
			template<typename R, typename C, typename ... A>
			struct fun_type_of_helper<R(C::*)(A...)> : public fun_type_of_helper<R(A...)> {};

			// get rid of const modifier in member function pointer
			template<typename R, typename C, typename ... A>
			struct fun_type_of_helper<R(C::*)(A...) const> : public fun_type_of_helper<R(A...)> {};

		} // end namespace detail


		template <typename Lambda>
		struct fun_type_of : public detail::fun_type_of_helper<decltype(&Lambda::operator())> { };

		template<typename R, typename ... P>
		struct fun_type_of<R(P...)> : public detail::fun_type_of_helper<R(P...)> { };

		template<typename R, typename ... P>
		struct fun_type_of<R(*)(P...)> : public fun_type_of<R(P...)> { };

		template<typename R, typename ... P>
		struct fun_type_of<R(* const)(P...)> : public fun_type_of<R(P...)> { };

		template<typename R, typename C, typename ... P>
		struct fun_type_of<R(C::*)(P...)> : public detail::fun_type_of_helper<R(C::*)(P...)> { };

		template<typename R, typename C, typename ... P>
		struct fun_type_of<R(C::* const)(P...)> : public fun_type_of<R(C::*)(P...)> { };


	} // end namespace detail

	// --- function definitions ---

	template<
		typename FunctorType,
		typename FunctionType = typename detail::fun_type_of<FunctorType>::type
	>
	std::function<FunctionType> toFunction(const FunctorType& f) {
		return std::function<FunctionType>(f);
	}

	template<typename OB, typename OS, typename I, typename ... Funs>
	struct fun_def {
		using in_type = I;
		using out_type = typename to_treeture<OB>::type;

		std::function<bool(I)> bc_test;
		std::vector<std::function<OB(I)>> base;
		std::vector<std::function<OS(I,Funs...)>> step;

		fun_def(
			const std::function<bool(I)>& test,
			const std::vector<std::function<OB(I)>>& base,
			const std::vector<std::function<OS(I,Funs...)>>& step
		) : bc_test(test), base(base), step(step) {}

		out_type operator()(const I& in, const Funs& ... funs) const {

			// to enable member-variable capturing, it seams like those have to be addressed by a local reference first
			auto& base_cpy = base;
			auto& step_cpy = step;

			return (bc_test(in))
				? out_type::spawn(
						[=]()->OB { return detail::pickRandom(base_cpy)(in); }
					)
				: out_type::spawn(
						[=]()->OB { return detail::pickRandom(base_cpy)(in); },
						[=]()->out_type { return detail::pickRandom(step_cpy)(in, funs...); }
				  	)
				;
		}
	};

	namespace detail {

		template<typename T>
		struct is_fun_def : public std::false_type {};

		template<typename OB, typename OS, typename I, typename ... T>
		struct is_fun_def<fun_def<OB,OS,I,T...>> : public std::true_type {};

		template<typename T>
		struct is_fun_def<const T> : public is_fun_def<T> {};

		template<typename T>
		struct is_fun_def<T&> : public is_fun_def<T> {};

		template<typename T>
		struct is_fun_def<T&&> : public is_fun_def<T> {};

		template<
			typename OB,
			typename OS,
			typename I,
			typename ... Funs
		>
		fun_def<OB,OS,I,Funs...> fun_intern(
				const std::function<bool(I)>& test,
				const std::vector<std::function<OB(I)>>& base,
				const std::vector<std::function<OS(I,Funs...)>>& step
		) {
			return fun_def<OB,OS,I,Funs...>(test, base, step);
		}

		template<
			typename OB,
			typename OS,
			typename I,
			typename ... Funs
		>
		fun_def<OB,OS,I,Funs...> fun_intern(
				const std::function<bool(I)>& test,
				const std::function<OB(I)>& base,
				const std::vector<std::function<OS(I,Funs...)>>& step
		) {
			return fun_intern<OB,OS,I,Funs...>(test, utils::toVector(base), step);
		}

		template<
			typename OB,
			typename OS,
			typename I,
			typename ... Funs
		>
		fun_def<OB,OS,I,Funs...> fun_intern(
				const std::function<bool(I)>& test,
				const std::function<OB(I)>& base,
				const std::function<OS(I,Funs...)>& step
		) {
			return fun_intern<OB,OS,I,Funs...>(test, base, utils::toVector(step));
		}

	}


	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<
				!utils::is_vector<BT>::value && !utils::is_vector<BC>::value && !utils::is_vector<SC>::value &&
				!utils::is_std_function<BT>::value && !utils::is_std_function<BC>::value && !utils::is_std_function<SC>::value
			,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c)->decltype(detail::fun_intern(toFunction(a), toFunction(b), toFunction(c))) {
		return detail::fun_intern(toFunction(a), toFunction(b), toFunction(c));
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<
				!utils::is_vector<BT>::value && !utils::is_vector<BC>::value &&
				!utils::is_std_function<BT>::value && !utils::is_std_function<BC>::value
			,int>::type
	>
	auto fun(const BT& a, const BC& b, const std::vector<SC>& c)->decltype(detail::fun_intern(toFunction(a), toFunction(b),c)) {
		return detail::fun_intern(toFunction(a), toFunction(b),c);
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<
				!utils::is_vector<BT>::value && !utils::is_std_function<BT>::value
			,int>::type
	>
	auto fun(const BT& a, const std::vector<BC>& b, const std::vector<SC>& c)->decltype(detail::fun_intern(toFunction(a), b,c)) {
		return detail::fun_intern(toFunction(a), b,c);
	}


	// --- add pick wrapper support ---

	template<typename F, typename ... Fs>
	std::vector<std::function<typename detail::fun_type_of<F>::type>> pick(const F& f, const Fs& ... fs) {
		typedef typename std::function<typename detail::fun_type_of<F>::type> fun_type;
		return std::vector<fun_type>({f,fs...});
	}


	// --- recursive definitions ---

	template<typename ... Defs> struct rec_defs;

	template<
		unsigned i = 0,
		typename ... Defs,
		typename I = typename utils::type_at<i,utils::type_list<Defs...>>::type::in_type,
		typename O = typename utils::type_at<i,utils::type_list<Defs...>>::type::out_type
	>
	std::function<O(I)> prec(const rec_defs<Defs...>& );


	namespace detail {

		template<unsigned n>
		struct caller {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			O call(const F& f, const I& i, const D& d, const Args& ... args) const {
				return caller<n-1>().template call<O>(f,i,d,prec<n>(d),args...);
			}
		};

		template<>
		struct caller<0> {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			typename std::enable_if<is_treeture<O>::value,O>::type
			call(const F& f, const I& i, const D& d, const Args& ... args) const {
				return f(i,prec<0>(d),args...);
			}
			template<typename O, typename F, typename I, typename D, typename ... Args>
			typename std::enable_if<!is_treeture<O>::value,O>::type
			call(const F& f, const I& i, const D& d, const Args& ... args) const {
				return treeture<O>::spawn([=]()->O { return f(i,prec<0>(d),args...); });
			}
		};


		template<typename T>
		struct is_rec_def : public std::false_type {};

		template<typename ... Defs>
		struct is_rec_def<rec_defs<Defs...>> : public std::true_type {};

		template<typename T>
		struct is_rec_def<T&> : public is_rec_def<T> {};

		template<typename T>
		struct is_rec_def<const T> : public is_rec_def<T> {};

	}


	template<typename ... Defs>
	struct rec_defs : public std::tuple<Defs...> {
		template<typename ... Args>
		rec_defs(const Args& ... args) : std::tuple<Defs...>(args...) {}


		template<
			unsigned i,
			typename O,
			typename I
		>
		O call(const I& in) const {
			// get targeted function
			auto& x = std::get<i>(*this);

			// call target function with an async
			return detail::caller<sizeof...(Defs)-1>().template call<O>(x,in,*this);
		}

	};


	template<
		typename ... Defs
	>
	rec_defs<Defs...> group(const Defs& ... defs) {
		return rec_defs<Defs...>(defs...);
	}

	// --- prec operator ---


	template<
		unsigned i,
		typename ... Defs,
		typename I,
		typename O
	>
	std::function<O(I)> prec(const rec_defs<Defs...>& defs) {
		return [=](const I& in)->O {
			return defs.template call<i,O,I>(in);
		};
	}


	template<
		unsigned i = 0,
		typename First,
		typename ... Rest,
		typename dummy = typename std::enable_if<detail::is_fun_def<First>::value,int>::type
	>
	auto prec(const First& f, const Rest& ... r)->decltype(prec<i>(group(f,r...))) {

		// This fix is required to circumvent a internal compiler error in GCC getting troubles with
		// the template instantiation. Thus, it has to be done manually (also inlining the call)
		#ifndef __GNUC__
			// works e.g. for clang
			return prec<i>(group(f,r...));
		#else
			// required to circumvent GCC bug
			using I = typename utils::type_at<i,utils::type_list<First,Rest...>>::type::in_type;
			using O = typename utils::type_at<i,utils::type_list<First,Rest...>>::type::out_type;

			auto defs = group(f,r...);
			return [=](const I& in)->O {
				return defs.template call<i,O,I>(in);
			};
		#endif
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<!detail::is_fun_def<BT>::value,int>::type
	>
	auto prec(const BT& t, const BC& b, const SC& s)->decltype(prec<0>(fun(t,b,s))) {
		return prec<0>(fun(t,b,s));
	}




} // end namespace core
} // end namespace api
} // end namespace allscale
