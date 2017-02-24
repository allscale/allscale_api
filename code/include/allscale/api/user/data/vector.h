#pragma once

#include <array>
#include <algorithm>

#include "allscale/utils/printer/arrays.h"
#include "allscale/utils/assert.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	// generic vector implementation
	template<typename T, std::size_t Dims>
	class Vector {

		std::array<T, Dims> data;

	public:

		Vector() {};

		Vector(const T& e) {
			for(size_t i = 0; i < Dims; i++) { data[i] = e; }
		}

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		Vector(const std::array<T,Dims>& other)
			: data(other) {}

		Vector(const std::initializer_list<T>& values) {
			assert_le(Dims, values.size()) << "Expected initializer list of size less or equal " << Dims << " but got " << values.size();
			size_t pos = 0;
			for(const auto& cur : values) { data[pos++] = cur; }
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

		bool operator>(const Vector& other) const {
			return data > other.data;
		}

		// allow implicit casts to std::array
		operator const std::array<T, Dims>&() const { return data; }

		bool dominatedBy(const Vector<T,Dims>& other) const {
			for(size_t i=0; i<Dims; i++) {
				if (other[i] < data[i]) return false;
			}
			return true;
		}

		bool strictlyDominatedBy(const Vector<T,Dims>& other) const {
			for(size_t i=0; i<Dims; i++) {
				if (other[i] <= data[i]) return false;
			}
			return true;
		}

		// Adds printer support to this vector.
		friend std::ostream& operator<<(std::ostream& out, const Vector& vec) {
			return out << vec.data;
		}
	};

	template<typename T, std::size_t Dims, typename S>
	Vector<T,Dims>& operator+=(Vector<T,Dims>& a, const Vector<S,Dims>& b) {
		for(size_t i = 0; i<Dims; i++) {
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
	Vector<T,Dims> operator/(const Vector<T, Dims>& vec, const S& fac) {
		Vector<T,Dims> res(vec);
		return res /= fac;
	}

	template<typename T, std::size_t Dims, typename Lambda>
	Vector<T,Dims> elementwise(const Vector<T,Dims>& a, const Vector<T,Dims>& b, const Lambda& op) {
		Vector<T,Dims> res;
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

	template<typename T, std::size_t Dims>
	Vector<T,Dims> elementwiseProduct(const Vector<T,Dims>& a, const Vector<T,Dims>& b) {
		return elementwise(a,b,[](const T& a, const T& b) { return a*b; });
	}

	template<typename T, std::size_t Dims>
	Vector<T,Dims> elementwiseDivision(const Vector<T,Dims>& a, const Vector<T,Dims>& b) {
		return elementwise(a,b,[](const T& a, const T& b) { return a/b; });
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
	private:
		struct IndexAccessHelper {
			std::array<T,3> data;
		};

	public:
		T x, y, z;

		Vector() {};

		Vector(const T& e) : x(e), y(e), z(e) { }

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		Vector(const std::array<T,3>& other) : x(other[0]), y(other[1]), z(other[2]) {}

		Vector(const std::initializer_list<T>& values) {
			assert_le(3, values.size()) << "Expected initializer list of size less or equal 3 but got " << values.size();
			size_t pos = 0;
			for(const auto& cur : values) { (*this)[pos++] = cur; }
		}

		T& operator[](int i) {
			return reinterpret_cast<IndexAccessHelper*>(this)->data[i];
		}

		const T& operator[](int i) const {
			return reinterpret_cast<const IndexAccessHelper*>(this)->data[i];
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
			return std::tie(x,y,z) < std::tie(other.x,other.y,other.z);
		}

		bool operator>(const Vector& other) const {
			return std::tie(x,y,z) > std::tie(other.x,other.y,other.z);
		}

		operator const std::array<T, 3>&() const { return reinterpret_cast<const IndexAccessHelper*>(this)->data; }

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
	private:
		struct IndexAccessHelper {
			std::array<T,2> data;
		};

	public:
		T x, y;

		Vector() {};

		Vector(const T& e) : x(e), y(e) { }

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		Vector(const std::array<T,2>& other) : x(other[0]), y(other[1]) {}

		Vector(const std::initializer_list<T>& values) {
			assert_le(2, values.size()) << "Expected initializer list of size less or equal 2 but got " << values.size();
			size_t pos = 0;
			for(const auto& cur : values) { (*this)[pos++] = cur; }
		}

		T& operator[](int i) {
			return reinterpret_cast<IndexAccessHelper*>(this)->data[i];
		}

		const T& operator[](int i) const {
			return reinterpret_cast<const IndexAccessHelper*>(this)->data[i];
		}

		Vector& operator=(const Vector& other) = default;
		Vector& operator=(Vector&& other) = default;

		bool operator==(const Vector& other) const {
			return std::tie(x,y) == std::tie(other.x,other.y);
		}

		bool operator!=(const Vector& other) const {
			return !(*this == other);
		}

		bool operator<(const Vector& other) const {
			return std::tie(x,y) < std::tie(other.x,other.y);
		}

		bool operator>(const Vector& other) const {
			return std::tie(x,y) > std::tie(other.x,other.y);
		}

		operator const std::array<T, 2>&() const { return reinterpret_cast<const IndexAccessHelper*>(this)->data; }

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

	};

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
