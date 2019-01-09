#pragma once

#include <array>
#include <algorithm>
#include <tuple>

#include "allscale/utils/printer/arrays.h"
#include "allscale/utils/assert.h"
#include "allscale/utils/unused.h"
#include "allscale/utils/serializer/arrays.h"

namespace allscale {
namespace utils {

	namespace detail {

		template<typename T, std::size_t Dims>
		std::array<T,Dims> fill(const T&);

	}

	// generic vector implementation
	template<typename T, std::size_t Dims>
	class Vector {

		std::array<T, Dims> data;

	public:

		using element_type = T;

		Vector() = default;

		Vector(const T& e) : data(detail::fill<T,Dims>(e)) {}

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		template<typename R>
		Vector(const Vector<R,Dims>& other)
			: data(other.data) {}

		template<typename R>
		Vector(const std::array<R,Dims>& other)
			: data(other) {}

		template<typename ... Rest>
		Vector(T a, Rest ... rest) : data{ {a,T(rest)...} } {
			static_assert(Dims == sizeof...(rest)+1, "Invalid number of components!");
		}

		Vector& operator=(const Vector& other) = default;
		Vector& operator=(Vector&& other) = default;

		T& operator[](const std::size_t index) {
			return data[index];
		}

		const T& operator[](const std::size_t index) const {
			return data[index];
		}

		// relational operators
		// defined in-class, since the private std::array data member has matching operators to forward to

		bool operator==(const Vector& other) const {
			return data == other.data;
		}

		bool operator!=(const Vector& other) const {
			return !(data == other.data);
		}

		bool operator<(const Vector& other) const {
			return data < other.data;
		}

		bool operator<=(const Vector& other) const {
			return data <= other.data;
		}

		bool operator>=(const Vector& other) const {
			return data >= other.data;
		}

		bool operator>(const Vector& other) const {
			return data > other.data;
		}

		// allow implicit casts to std::array
		operator const std::array<T, Dims>&() const { return data; }

		bool dominatedBy(const Vector<T,Dims>& other) const {
			for(std::size_t i=0; i<Dims; i++) {
				if (other[i] < data[i]) return false;
			}
			return true;
		}

		bool strictlyDominatedBy(const Vector<T,Dims>& other) const {
			for(std::size_t i=0; i<Dims; i++) {
				if (other[i] <= data[i]) return false;
			}
			return true;
		}

		// Adds printer support to this vector.
		friend std::ostream& operator<<(std::ostream& out, const Vector& vec) {
			return out << vec.data;
		}

		template<typename X = T>
		std::enable_if_t<is_serializable<X>::value, void>
		store(utils::ArchiveWriter& writer) const {
			writer.write(data);
		}

		template<typename X = T>
		static std::enable_if_t<is_serializable<X>::value, Vector>
		load(utils::ArchiveReader& reader) {
			return { reader.read<std::array<X, Dims>>()};
		}

	};

	namespace detail {

		template<typename T, std::size_t Dims, std::size_t Index>
		struct filler {
			template<typename ... Args>
			static std::array<T,Dims> fill(const T& e, const Args& ... a) {
				return filler<T,Dims,Index-1>::fill(e,e,a...);
			}
		};

		template<typename T, std::size_t Dims>
		struct filler<T,Dims,0> {
			template<typename ... Args>
			static std::array<T,Dims> fill(const T&, const Args& ... a) {
				return {{a...}};
			}
		};

		template<typename T, std::size_t Dims>
		std::array<T,Dims> fill(const T& e) {
			return filler<T,Dims,Dims>::fill(e);
		}

	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims>& operator+=(Vector<T,Dims>& a, const Vector<S,Dims>& b) {
		for(std::size_t i = 0; i<Dims; i++) {
			a[i] += b[i];
		}
		return a;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims>& operator-=(Vector<T,Dims>& a, const Vector<S,Dims>& b) {
		for(size_t i = 0; i<Dims; i++) {
			a[i] -= b[i];
		}
		return a;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims>& operator*=(Vector<T,Dims>& a, const S& fac) {
		for(size_t i =0; i<Dims; i++) {
			a[i] *= fac;
		}
		return a;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims>& operator/=(Vector<T,Dims>& a, const S& fac) {
		for(size_t i =0; i<Dims; i++) {
			a[i] /= fac;
		}
		return a;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims>& operator%=(Vector<T,Dims>& a, const S& fac) {
		for(size_t i =0; i<Dims; i++) {
			a[i] %= fac;
		}
		return a;
	}

	template<typename T, std::size_t Dims>
	Vector<T,Dims>& operator%=(Vector<T,Dims>& a, const Vector<T,Dims>& b) {
		for(size_t i =0; i<Dims; i++) {
			a[i] %= b[i];
		}
		return a;
	}


	template<typename T, std::size_t Dims>
	Vector<T,Dims> operator+(const Vector<T,Dims>& a, const Vector<T,Dims>& b) {
		Vector<T,Dims> res(a);
		return res += b;
	}

	template<typename T, std::size_t Dims>
	Vector<T,Dims> operator-(const Vector<T, Dims>& a, const Vector<T, Dims>& b) {
		Vector<T,Dims> res(a);
		return res -= b;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims> operator*(const Vector<T, Dims>& vec, const S& fac) {
		Vector<T,Dims> res(vec);
		return res *= fac;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T, Dims> operator*(const S& fac, const Vector<T, Dims>& vec) {
		return vec * fac;
	}

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims> operator/(const Vector<T, Dims>& vec, const S& fac) {
		Vector<T,Dims> res(vec);
		return res /= fac;
	}

	template<typename T, std::size_t Dims>
	Vector<T,Dims> operator%(const Vector<T, Dims>& a, const Vector<T,Dims>& b) {
		Vector<T,Dims> res(a);
		return res %= b;
	}

	template<std::size_t Dims, typename Lambda, typename A, typename B, typename R = decltype(std::declval<Lambda>()(std::declval<A>(), std::declval<B>()))>
	Vector<R,Dims> elementwise(const Vector<A,Dims>& a, const Vector<B,Dims>& b, const Lambda& op) {
		Vector<R,Dims> res;
		for(unsigned i=0; i<Dims; i++) {
			res[i] = op(a[i],b[i]);
		}
		return res;
	}

	template<typename T, std::size_t Dims>
	Vector<T,Dims> elementwiseMin(const Vector<T,Dims>& a, const Vector<T,Dims>& b) {
		return elementwise(a,b,[](const T& a, const T& b) { return std::min<T>(a,b); });
	}

	template<typename T, std::size_t Dims>
	Vector<T,Dims> elementwiseMax(const Vector<T,Dims>& a, const Vector<T,Dims>& b) {
		return elementwise(a,b,[](const T& a, const T& b) { return std::max<T>(a,b); });
	}

	template<std::size_t Dims, typename A, typename B, typename R = decltype(std::declval<A>() * std::declval<B>())>
	Vector<R,Dims> elementwiseProduct(const Vector<A,Dims>& a, const Vector<B,Dims>& b) {
		return elementwise(a,b,[](const A& a, const B& b) { return a*b; });
	}

	template<std::size_t Dims, typename A, typename B, typename R = decltype(std::declval<A>() / std::declval<B>())>
	Vector<R,Dims> elementwiseDivision(const Vector<A,Dims>& a, const Vector<B,Dims>& b) {
		return elementwise(a,b,[](const A& a, const B& b) { return a/b; });
	}

	template<std::size_t Dims, typename A, typename B, typename R = decltype(std::declval<A>() % std::declval<B>())>
	Vector<R,Dims> elementwiseRemainder(const Vector<A,Dims>& a, const Vector<B,Dims>& b) {
		return elementwise(a,b,[](const A& a, const B& b) { return a % b; });
	}

	template<std::size_t Dims, typename A, typename B, typename R = decltype(std::declval<A>() % std::declval<B>())>
	Vector<R,Dims> elementwiseModulo(const Vector<A,Dims>& a, const Vector<B,Dims>& b) {
		return elementwiseRemainder(a,b);
	}


	template<typename T, std::size_t Dims>
	T sumOfSquares(const Vector<T,Dims>& vec) {
		T sum = T();
		for(unsigned i = 0; i < Dims; i++) {
			sum += vec[i] * vec[i];
		}
		return sum;
	}

	// specialization for 3-dimensional vectors, providing access to named data members x, y, z
	template <typename T>
	class Vector<T, 3> {
	public:

		using element_type = T;

		T x, y, z;

		Vector() = default;

		Vector(const T& e) : x(e), y(e), z(e) { }

		Vector(T x, T y, T z) : x(x), y(y), z(z) { }

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		template<typename R>
		Vector(const Vector<R,3>& other) : x(other.x), y(other.y), z(other.z) {}

		template<typename R>
		Vector(const std::array<R,3>& other) : x(other[0]), y(other[1]), z(other[2]) {}

		T& operator[](std::size_t i) {
			return (i==0) ? x : (i==1) ? y : z;
		}

		const T& operator[](std::size_t i) const {
			return (i==0) ? x : (i==1) ? y : z;
		}

		Vector& operator=(const Vector& other) = default;
		Vector& operator=(Vector&& other) = default;

		bool operator==(const Vector& other) const {
			return std::tie(x,y,z) == std::tie(other.x,other.y,other.z);
		}

		bool operator!=(const Vector& other) const {
			return !(*this == other);
		}

		bool operator<(const Vector& other) const {
			return asArray() < other.asArray();
		}

		bool operator<=(const Vector& other) const {
			return asArray() <= other.asArray();
		}

		bool operator>=(const Vector& other) const {
			return asArray() >= other.asArray();
		}

		bool operator>(const Vector& other) const {
			return asArray() > other.asArray();
		}

		operator const std::array<T, 3>&() const { return asArray(); }

		const std::array<T,3>& asArray() const {
			return reinterpret_cast<const std::array<T,3>&>(*this);
		}

		bool dominatedBy(const Vector& other) const {
			return other.x >= x && other.y >= y && other.z >= z;
		}

		bool strictlyDominatedBy(const Vector& other) const {
			return other.x > x && other.y > y && other.z > z;
		}

		// Adds printer support to this vector.
		friend std::ostream& operator<<(std::ostream& out, const Vector& vec) {
			return out << "[" << vec.x << "," << vec.y << "," << vec.z << "]";
		}

		template<typename X = T>
		std::enable_if_t<is_serializable<X>::value, void>
		store(utils::ArchiveWriter& writer) const {
			writer.write(x);
			writer.write(y);
			writer.write(z);
		}

		template<typename X = T>
		static std::enable_if_t<is_serializable<X>::value, Vector>
		load(utils::ArchiveReader& reader) {
			return { reader.read<X>(), reader.read<X>(), reader.read<X>() };
		}

	};

	template<typename T>
	Vector<T, 3> crossProduct(const Vector<T, 3>& a, const Vector<T, 3>& b) {
		return Vector<T, 3> {
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0]
		};
	}

	// specialization for 2-dimensional vectors, providing access to named data members x, y
	template <typename T>
	class Vector<T, 2> {
	public:

		using element_type = T;

		T x, y;

		Vector() = default;

		Vector(const T& e) : x(e), y(e) { }

		Vector(T x, T y) : x(x), y(y) { }

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		template<typename R>
		Vector(const Vector<R,2>& other) : x(other.x), y(other.y) {}

		template<typename R>
		Vector(const std::array<R,2>& other) : x(other[0]), y(other[1]) {}

		T& operator[](std::size_t i) {
			return (i == 0) ? x : y;
		}

		const T& operator[](std::size_t i) const {
			return (i == 0) ? x : y;
		}

		Vector& operator=(const Vector& other) = default;
		Vector& operator=(Vector&& other) = default;

		bool operator==(const Vector& other) const {
			return asArray() == other.asArray();
		}

		bool operator!=(const Vector& other) const {
			return !(*this == other);
		}

		bool operator<(const Vector& other) const {
			return asArray() < other.asArray();
		}

		bool operator<=(const Vector& other) const {
			return asArray() <= other.asArray();
		}

		bool operator>=(const Vector& other) const {
			return asArray() >= other.asArray();
		}

		bool operator>(const Vector& other) const {
			return asArray() > other.asArray();
		}

		operator const std::array<T, 2>&() const { return asArray(); }

		const std::array<T,2>& asArray() const {
			return reinterpret_cast<const std::array<T,2>&>(*this);
		}

		bool dominatedBy(const Vector& other) const {
			return other.x >= x && other.y >= y;
		}

		bool strictlyDominatedBy(const Vector& other) const {
			return other.x > x && other.y > y;
		}

		// Adds printer support to this vector.
		friend std::ostream& operator<<(std::ostream& out, const Vector& vec) {
			return out << "[" << vec.x << "," << vec.y << "]";
		}

		template<typename X = T>
		std::enable_if_t<is_serializable<X>::value, void>
		store(utils::ArchiveWriter& writer) const {
			writer.write(x);
			writer.write(y);
		}
		
		template<typename X = T>
		static std::enable_if_t<is_serializable<X>::value, Vector>
		load(utils::ArchiveReader& reader) {
			return { reader.read<X>(), reader.read<X>() };
		}

	};

	/**
	 * Add support for trivially serializing / de-serializing Vector instances.
	 */
	template<typename T, std::size_t Dims>
	struct is_trivially_serializable<Vector<T,Dims>, typename std::enable_if<is_trivially_serializable<T>::value>::type> : public std::true_type {};

} // end namespace utils
} // end namespace allscale

#if defined(ALLSCALE_WITH_HPX)
#include <hpx/traits/is_bitwise_serializable.hpp>

namespace hpx { namespace traits {
    template <typename T, std::size_t Dims>
    struct is_bitwise_serializable<allscale::utils::Vector<T, Dims>>
      : is_bitwise_serializable<T>
    {};
}}

#endif
