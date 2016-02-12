#pragma once

#include <functional>
#include <type_traits>
#include <vector>

#include "allscale/api/core/impl/simple/runtime.h"

namespace allscale {
namespace api {
namespace core {
inline namespace simple {

	// ---------------------------------------------------------------------------------------------
	//											Factories
	// ---------------------------------------------------------------------------------------------

	// -- completed futures --

	template<typename T>
	Future<T> done(const T& value) {
		return Future<T>(value);
	}

	inline Future<void> done() {
		return Future<void>();
	}


	// -- atomic futures --

	template<
		typename T,
		typename R = typename std::result_of<T()>::type
	>
	Future<R> atom(const T& task) {
		return spawn(task);
	}


	// -- composed futures --

	template<typename V, typename ... Subs>
	Future<V> aggregate(V(* aggregator)(const std::vector<Future<V>>&), Subs&& ... subs) {
		return Future<V>(internal::Kind::Parallel, aggregator, subs...);
	}

	template<typename ... Subs>
	Future<void> par(Subs&& ... subs) {
		return Future<void>(internal::Kind::Parallel, subs...);
	}

	template<typename ... Subs>
	Future<void> seq(Subs&& ... subs) {
		return Future<void>(internal::Kind::Sequential, subs...);
	}


} // end namespace simple
} // end namespace core
} // end namespace api
} // end namespace allscale


