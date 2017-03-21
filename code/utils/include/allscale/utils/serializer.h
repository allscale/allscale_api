#pragma once

#include <type_traits>

namespace allscale {
namespace utils {

	// ---------------------------------------------------------------------------------
	//									Declarations
	// ---------------------------------------------------------------------------------


	template<typename T, typename _ = void>
	struct serializer;

	template <typename T, typename _ = void>
	struct is_serializable;

	class Archive;

	template<typename T>
	typename std::enable_if<is_serializable<T>::value,void>::type
	serialize(Archive&, const T&);

	template<typename T>
	typename std::enable_if<is_serializable<T>::value,T>::type
	deserialize(Archive&);


	// ---------------------------------------------------------------------------------
	//									Definitions
	// ---------------------------------------------------------------------------------

	template<typename T>
	struct serializer<T, typename std::enable_if<
			// targeted types have to offer a static load member ...
			std::is_same<decltype(T::load(std::declval<Archive&>())),T>::value &&
			// ... and a store member function
			std::is_same<decltype(std::declval<const T&>().store(std::declval<Archive&>())),void>::value,
		void>::type> {

		static T load(Archive& a) {
			return T::load(a);
		}
		static void store(Archive& a, const T& value) {
			value.store(a);
		}
	};

	template<typename T>
	struct serializer<const T,typename std::enable_if<
			is_serializable<T>::value,
		void>::type> : public serializer<T> {};

	namespace detail {

		template<typename T>
		struct primitive_serializer {
			static T load(Archive&) {
				return T();		// TODO: actually load something
			}
			static void store(Archive&, int) {
				// TODO: actually store something
			}
		};

	} // end namespace detail

	template<> struct serializer<bool>          :  public detail::primitive_serializer<bool> {};

	template<> struct serializer<char>          :  public detail::primitive_serializer<char> {};
	template<> struct serializer<signed char>   :  public detail::primitive_serializer<signed char> {};
	template<> struct serializer<unsigned char> :  public detail::primitive_serializer<unsigned char> {};
	template<> struct serializer<char16_t>      :  public detail::primitive_serializer<char16_t> {};
	template<> struct serializer<char32_t>      :  public detail::primitive_serializer<char32_t> {};
	template<> struct serializer<wchar_t>       :  public detail::primitive_serializer<wchar_t> {};

	template<> struct serializer<short int>     :  public detail::primitive_serializer<short int> {};
	template<> struct serializer<int>           :  public detail::primitive_serializer<int> {};
	template<> struct serializer<long int>      :  public detail::primitive_serializer<long int> {};
	template<> struct serializer<long long int> :  public detail::primitive_serializer<long long int> {};

	template<> struct serializer<unsigned short int>     :  public detail::primitive_serializer<unsigned short int> {};
	template<> struct serializer<unsigned int>           :  public detail::primitive_serializer<unsigned int> {};
	template<> struct serializer<unsigned long int>      :  public detail::primitive_serializer<unsigned long int> {};
	template<> struct serializer<unsigned long long int> :  public detail::primitive_serializer<unsigned long long int> {};

	template<> struct serializer<float>       :  public detail::primitive_serializer<float> {};
	template<> struct serializer<double>      :  public detail::primitive_serializer<double> {};
	template<> struct serializer<long double> :  public detail::primitive_serializer<long double> {};


	template <typename T, typename _>
	struct is_serializable : public std::false_type {};

	template <typename T>
	struct is_serializable<T, typename std::enable_if<
			std::is_same<decltype(serializer<T>::load(std::declval<Archive&>())),T>::value &&
			std::is_same<decltype(serializer<T>::store(std::declval<Archive&>(), std::declval<const T&>())),void>::value,
		void>::type> : public std::true_type {};

	template <typename T>
	struct is_serializable<const T, typename std::enable_if<
			is_serializable<T>::value,
		void>::type> : public std::true_type {};


	template<typename T>
	typename std::enable_if<is_serializable<T>::value,void>::type
	serialize(Archive& a, const T& value) {
		serializer<T>::save(a,value);
	}

	template<typename T>
	typename std::enable_if<is_serializable<T>::value,T>::type
	deserialize(Archive& a) {
		return serializer<T>::load(a);
	}


} // end namespace utils
} // end namespace allscale
