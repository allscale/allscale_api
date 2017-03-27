#pragma once

#include <type_traits>

#include "allscale/utils/type_list.h"

namespace allscale {
namespace utils {

	// --------------------------------------------------------------------
	//								Declarations
	// --------------------------------------------------------------------


	/**
	 * A static map mapping a given value to each of a given list of types.
	 */
	template<typename Keys, typename Value>
	class StaticMap;

	/**
	 * An auxiliary type for forming lists of keys.
	 */
	template<typename ... Keys>
	struct keys {};


	// --------------------------------------------------------------------
	//								Definitions
	// --------------------------------------------------------------------

	namespace key_utils {

		template<typename T>
		struct is_keys : public std::false_type {};

		template<typename ... Keys>
		struct is_keys<keys<Keys...>> : public std::true_type {};

		template<typename T>
		struct invalid_key : public std::false_type {};
	}

	template<typename Keys, typename Value>
	class StaticMap {

		static_assert(key_utils::is_keys<Keys>::value, "First template parameters must be of form keys<...>");

	};


	template<typename ... Keys, typename Value>
	class StaticMap<keys<Keys...>,Value> {

		using key_list = type_list<Keys...>;

		std::array<Value,key_list::length> values;

	public:

		template<typename Key>
		Value& get() {
			return values[type_index<Key,key_list>::value];
		}

		template<typename Key>
		const Value& get() const {
			return values[type_index<Key,key_list>::value];
		}

		template<typename Body>
		void forEach(const Body& body) {
			for(auto& cur : values) {
				body(cur);
			}
		}

		template<typename Body>
		void forEach(const Body& body) const {
			for(const auto& cur : values) {
				body(cur);
			}
		}

	};

} // end namespace utils
} // end namespace allscale
