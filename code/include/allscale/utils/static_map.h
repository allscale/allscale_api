#pragma once

#include <type_traits>

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


	template<typename First, typename ... Rest, typename Value>
	class StaticMap<keys<First,Rest...>,Value> {

		using nested_type = StaticMap<keys<Rest...>,Value>;

		Value value;

		nested_type nested;

	public:

		template<typename Key>
		typename std::enable_if<std::is_same<First,Key>::value, Value>::type& get() {
			return value;
		}

		template<typename Key>
		const typename std::enable_if<std::is_same<First,Key>::value, Value>::type& get() const {
			return value;
		}

		template<typename Key>
		typename std::enable_if<!(std::is_same<First,Key>::value), Value>::type& get() {
			return nested.template get<Key>();
		}

		template<typename Key>
		const typename std::enable_if<!(std::is_same<First,Key>::value), Value>::type& get() const {
			return nested.template get<Key>();
		}

	};

	template<typename Key, typename Value>
	class StaticMap<keys<Key>,Value> {

		Value value;

	public:

		template<typename CurKey>
		typename std::enable_if<std::is_same<CurKey,Key>::value, Value>::type& get() {
			return value;
		}

		template<typename CurKey>
		const typename std::enable_if<std::is_same<CurKey,Key>::value, Value>::type& get() const {
			return value;
		}

		template<typename CurKey>
		typename std::enable_if<!(std::is_same<CurKey,Key>::value), Value>::type& get() {
			static_assert(key_utils::invalid_key<CurKey>::value,"Invalid key!");
		}

		template<typename CurKey>
		const typename std::enable_if<!(std::is_same<CurKey,Key>::value), Value>::type& get() const {
			static_assert(key_utils::invalid_key<CurKey>::value,"Invalid key!");
		}

	};

	template<typename Value>
	class StaticMap<keys<>,Value> {


	public:

		// contains nothing!

		template<typename Key>
		Value& get() {
			static_assert(key_utils::invalid_key<Key>::value,"Invalid key!");
		}

		template<typename Key>
		const Value& get() const {
			static_assert(key_utils::invalid_key<Key>::value,"Invalid key!");
		}


	};

} // end namespace utils
} // end namespace allscale
