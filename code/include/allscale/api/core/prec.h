#pragma once

#include <functional>
#include <vector>

#include "allscale/utils/functional_utils.h"
#include "allscale/utils/tuple_utils.h"
#include "allscale/utils/vector_utils.h"

#include "allscale/api/core/treeture.h"

namespace allscale {
namespace api {
namespace core {

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

		template<unsigned L>
		struct random_caller {

			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(unsigned pos, const std::tuple<Versions...>& version, const Args& ... args) {
				if (pos == L) return std::get<L>(version)(args...);
				return random_caller<L-1>().template callRandom<Res>(pos,version,args...);
			}

			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(const std::tuple<Versions...>& version, const Args& ... args) {
				int index = rand(sizeof...(Versions));
				return callRandom<Res>(index, version, args...);
			}

		};


		template<typename Out, typename In>
		struct result_wrapper {

			template<typename Fun>
			Out operator()(Fun&& fun) {
				return fun();
			}

		};

		template<typename T>
		struct result_wrapper<detail::completed_task<T>,T> {

			template<typename Fun>
			completed_task<T> operator()(Fun&& fun) {
				return done(fun());
			}

		};

		template<>
		struct result_wrapper<detail::completed_task<void>,void> {

			template<typename Fun>
			completed_task<void> operator()(Fun&& fun) {
				fun();
				return done();
			}

		};

		template<typename T>
		struct result_wrapper<impl::sequential::unreleased_treeture<T>,T> : public result_wrapper<detail::completed_task<T>,T> {};

		template<typename T,typename Gen>
		struct result_wrapper<impl::sequential::lazy_unreleased_treeture<T,Gen>,T> : public result_wrapper<detail::completed_task<T>,T> {};

		template<typename T>
		struct result_wrapper<impl::reference::unreleased_treeture<T>,T> : public result_wrapper<detail::completed_task<T>,T> {};

//		template<>
//		struct result_wrapper<void> {
//
//			template<typename Res, typename Fun>
//			Res wrap(Fun&& fun) {
//				fun();
//				return done();
//			}
//
//		};
//
//		template<typename O>
//		struct result_wrapper<detail::completed_task<O>> {
//
//			template<typename Res, typename Fun>
//			Res wrap(Fun&& fun) {
//				return fun();
//			}
//
//		};
//
//		template<typename O>
//		struct result_wrapper<impl::sequential::unreleased_treeture<O>> {
//
//			template<typename Res, typename Fun>
//			Res wrap(Fun&& fun) {
//				return fun();
//			}
//
//		};
//
//		template<typename O, typename F>
//		struct result_wrapper<impl::sequential::lazy_unreleased_treeture<O,F>> {
//
//			template<typename Res, typename Fun>
//			Res wrap(Fun&& fun) {
//				return fun();
//			}
//
//		};
//
//		template<typename O>
//		struct result_wrapper<impl::reference::unreleased_treeture<O>> {
//
//			template<typename Res, typename Fun>
//			Res wrap(Fun&& fun) {
//				return fun();
//			}
//
//		};


		template<>
		struct random_caller<0> {
			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(unsigned, const std::tuple<Versions...>& versions, const Args& ... args) {
				using res_type = decltype(std::get<0>(versions)(args...));
				result_wrapper<Res,res_type> wrap;
				return wrap([&](){ return std::get<0>(versions)(args...); });
			}

			template<
				typename Res,
				typename ... Versions,
				typename ... Args
			>
			Res callRandom(const std::tuple<Versions...>& versions, const Args& ... args) {
				using res_type = decltype(std::get<0>(versions)(args...));
				result_wrapper<Res,res_type> wrap;
				return wrap([&](){ return std::get<0>(versions)(args...); });
			}

		};

	} // end namespace detail

	// ----- function handling ----------

	template<
		typename O,
		typename I,
		typename BaseCaseTest,
		typename BaseCases,
		typename StepCases
	>
	struct fun_def;

	template<
		typename O,
		typename I,
		typename BaseCaseTest,
		typename ... BaseCases,
		typename ... StepCases
	>
	struct fun_def<O,I,BaseCaseTest,std::tuple<BaseCases...>,std::tuple<StepCases...>> {
		typedef I in_type;
		typedef O out_type;

		BaseCaseTest bc_test;
		std::tuple<BaseCases...> base;
		std::tuple<StepCases...> step;

		fun_def(
			const BaseCaseTest& test,
			const std::tuple<BaseCases...>& base,
			const std::tuple<StepCases...>& step
		) : bc_test(test), base(base), step(step) {}

		fun_def(const fun_def& other) = default;
		fun_def(fun_def&& other) = default;

		fun_def& operator=(const fun_def&) = delete;
		fun_def& operator=(fun_def&&) = delete;

		template<typename ... Funs>
		impl::sequential::unreleased_treeture<O> sequentialCall(const I& in, const Funs& ... funs) const {
			// check for the base case, producing a value to be wrapped
			if (bc_test(in)) {
				return impl::sequential::spawn([&]{ return detail::random_caller<sizeof...(BaseCases)-1>().template callRandom<O>(base, in); });
			}

			// run sequential step case producing an immediate value
			return detail::random_caller<sizeof...(StepCases)-1>().template callRandom<impl::sequential::unreleased_treeture<O>>(step, in, funs.sequential_call()...);
		}


		template<typename ... Funs>
		impl::reference::unreleased_treeture<O> parallelCall(const I& in, const Funs& ... funs) const {
			// check for the base case
			const auto& base = this->base;
			if (bc_test(in)) return impl::reference::spawn([=] {
				return detail::random_caller<sizeof...(BaseCases)-1>().template callRandom<O>(base, in);
			});

			// run step case
			const auto& step = this->step;
			return impl::reference::spawn(
					// the process version (sequential):
					[=] { return detail::random_caller<sizeof...(StepCases)-1>().template callRandom<impl::sequential::unreleased_treeture<O>>(step, in, funs.sequential_call()...).get(); },
					// the split version (parallel):
					[=] { return detail::random_caller<sizeof...(StepCases)-1>().template callRandom<impl::reference::unreleased_treeture<O>>(step, in, funs.parallel_call()...); }
			);
		}

	};



	namespace detail {

		template<typename T>
		struct is_fun_def : public std::false_type {};

		template<typename O, typename I, typename ... T>
		struct is_fun_def<fun_def<O,I,T...>> : public std::true_type {};

		template<typename T>
		struct is_fun_def<const T> : public is_fun_def<T> {};

		template<typename T>
		struct is_fun_def<T&> : public is_fun_def<T> {};

		template<typename T>
		struct is_fun_def<T&&> : public is_fun_def<T> {};

	}

	template<
		typename BT, typename First_BC, typename ... BC, typename ... SC,
		typename O = typename utils::lambda_traits<First_BC>::result_type,
		typename I = typename utils::lambda_traits<First_BC>::arg1_type
	>
	fun_def<O,I,BT,std::tuple<First_BC,BC...>,std::tuple<SC...>>
	fun(const BT& a, const std::tuple<First_BC,BC...>& b, const std::tuple<SC...>& c) {
		return fun_def<O,I,BT,std::tuple<First_BC,BC...>,std::tuple<SC...>>(a,b,c);
	}

	template<
		typename BT, typename BC, typename SC,
		typename filter = typename std::enable_if<!utils::is_tuple<BC>::value && !utils::is_tuple<SC>::value,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c) -> decltype(fun(a,std::make_tuple(b),std::make_tuple(c))) {
		return fun(a,std::make_tuple(b),std::make_tuple(c));
	}

	template<
		typename BT, typename BC, typename SC,
		typename filter = typename std::enable_if<!utils::is_tuple<BC>::value && utils::is_tuple<SC>::value,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c) -> decltype(fun(a,std::make_tuple(b),c)) {
		return fun(a,std::make_tuple(b),c);
	}

	template<
		typename BT, typename BC, typename SC,
		typename filter = typename std::enable_if<utils::is_tuple<BC>::value && !utils::is_tuple<SC>::value,int>::type
	>
	auto fun(const BT& a, const BC& b, const SC& c) -> decltype(fun(a,b,std::make_tuple(c))) {
		return fun(a,b,std::make_tuple(c));
	}


	// --- add pick wrapper support ---

	template<typename F, typename ... Fs>
	std::tuple<F,Fs...> pick(const F& f, const Fs& ... fs) {
		return std::make_tuple(f,fs...);
	}


	// --- recursive definitions ---

	template<typename ... Defs> struct rec_defs;


	namespace detail {


		template<
			unsigned i,
			typename ... Defs
		>
		struct callable {

			using I = typename utils::type_at<i,utils::type_list<Defs...>>::type::in_type;
			using O = typename utils::type_at<i,utils::type_list<Defs...>>::type::out_type;

			rec_defs<Defs...> defs;

			callable(const rec_defs<Defs...>& defs) : defs(defs) {};

			auto sequential_call() const {
				return [=](const I& in) {
					return impl::sequential::make_lazy_unreleased_treeture([=](){
						return defs.template sequentialCall<i,O,I>(in);
					});
				};
			}

			auto parallel_call() const {
				return [=](const I& in)->impl::reference::unreleased_treeture<O> {
					return defs.template parallelCall<i,O,I>(in);
				};
			}
		};

		template<
			unsigned i,
			typename ... Defs
		>
		callable<i,Defs...> createCallable(const rec_defs<Defs...>& defs) {
			return callable<i,Defs...>(defs);
		}

		template<unsigned n>
		struct caller {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			impl::sequential::unreleased_treeture<O> sequentialCall(const F& f, const I& i, const D& d, const Args& ... args) const {
				return caller<n-1>().template sequentialCall<O>(f,i,d,createCallable<n>(d),args...);
			}
			template<typename O, typename F, typename I, typename D, typename ... Args>
			impl::reference::unreleased_treeture<O> parallelCall(const F& f, const I& i, const D& d, const Args& ... args) const {
				return caller<n-1>().template parallelCall<O>(f,i,d,createCallable<n>(d),args...);
			}
		};

		template<>
		struct caller<0> {
			template<typename O, typename F, typename I, typename D, typename ... Args>
			auto sequentialCall(const F& f, const I& i, const D& d, const Args& ... args) const {
				return f.sequentialCall(i,createCallable<0>(d),args...);
			}
			template<typename O, typename F, typename I, typename D, typename ... Args>
			impl::reference::unreleased_treeture<O> parallelCall(const F& f, const I& i, const D& d, const Args& ... args) const {
				return f.parallelCall(i,createCallable<0>(d),args...);
			}
		};


		template<typename T>
		struct is_rec_def : public std::false_type {};

		template<typename ... Defs>
		struct is_rec_def<rec_defs<Defs...>> : public std::true_type {};

		template<typename T>
		struct is_rec_def<const T> : public is_rec_def<T> {};

		template<typename T>
		struct is_rec_def<T&> : public is_rec_def<T> {};

		template<typename T>
		struct is_rec_def<T&&> : public is_rec_def<T> {};

	}


	template<typename ... Defs>
	struct rec_defs : public std::tuple<Defs...> {

		template<typename ... Args>
		rec_defs(const Args& ... args) : std::tuple<Defs...>(args...) {}

		rec_defs(const rec_defs&) = default;
		rec_defs(rec_defs&&) = default;

		rec_defs& operator=(const rec_defs&) = delete;
		rec_defs& operator=(rec_defs&&) = delete;

		template<
			unsigned i,
			typename O,
			typename I
		>
		impl::sequential::unreleased_treeture<O> sequentialCall(const I& in) const {
			// call target function with a spawn
			return detail::caller<sizeof...(Defs)-1>().template sequentialCall<O>(std::get<i>(*this),in,*this);
		}

		template<
			unsigned i,
			typename O,
			typename I
		>
		impl::reference::unreleased_treeture<O> parallelCall(const I& in) const {
			// call target function with a spawn
			return detail::caller<sizeof...(Defs)-1>().template parallelCall<O>(std::get<i>(*this),in,*this);
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
		unsigned i = 0,
		typename ... Defs,
		typename I = typename utils::type_at<i,utils::type_list<Defs...>>::type::in_type,
		typename O = typename utils::type_at<i,utils::type_list<Defs...>>::type::out_type
	>
	auto prec(const rec_defs<Defs...>& defs) {
		return [=](const I& in)->treeture<O> {
			return defs.template parallelCall<i,O,I>(in);
		};
	}


	template<
		unsigned i = 0,
		typename First,
		typename ... Rest,
		typename dummy = typename std::enable_if<detail::is_fun_def<First>::value,int>::type
	>
	auto prec(const First& f, const Rest& ... r) {
		#ifdef __clang__
			// works e.g. for clang
			return prec<i>(group(f,r...));
		#else
			// required to circumvent GCC bug
			using I = typename utils::type_at<i,utils::type_list<First,Rest...>>::type::in_type;
			using O = typename utils::type_at<i,utils::type_list<First,Rest...>>::type::out_type;

			auto defs = group(f,r...);
			return [=](const I& in)->treeture<O> {
				return defs.template parallelCall<i,O,I>(in);
			};
		#endif
	}

	template<
		typename BT, typename BC, typename SC,
		typename dummy = typename std::enable_if<!detail::is_fun_def<BT>::value,int>::type
	>
	auto prec(const BT& t, const BC& b, const SC& s) {
		return prec<0>(fun(t,b,s));
	}

} // end namespace core
} // end namespace api
} // end namespace allscale
