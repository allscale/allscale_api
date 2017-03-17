#pragma once

#include <tuple>

namespace allscale {
namespace utils {


	template<typename T>
	struct is_tuple : public std::false_type {};

	template<typename ... E>
	struct is_tuple<std::tuple<E...>> : public std::true_type {};

	template<typename T>
	struct is_tuple<const T> : public is_tuple<T> {};

	template<typename T>
	struct is_tuple<T&> : public is_tuple<T> {};


} // end namespace utils
} // end namespace allscale
