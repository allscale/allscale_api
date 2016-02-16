#pragma once

#include <array>

#include "allscale/utils/printer/arrays.h"

namespace allscale {
namespace api {
namespace user {
namespace data {


	template<typename T, std::size_t Dims>
	class Vector : public std::array<T,Dims> {

		using super = std::array<T,Dims>;

	public:

		Vector() {};

		Vector(const T& e) {
			for(size_t i = 0; i < Dims; i++) (*this)[i] = e;
		}

		Vector(const Vector&) = default;
		Vector(Vector&&) = default;

		Vector(const std::array<T,Dims>& other)
			: super(other) {}

		Vector(const std::initializer_list<T>& values) {
			size_t pos = 0;
			for(const auto& cur : values) {
				(*this)[pos++] = cur;
				if (pos > Dims) return;
			}
		}

		Vector& operator=(const Vector& other) = default;
		Vector& operator=(Vector&& other) = default;

		Vector& operator+=(const Vector& other) {
			for(size_t i = 0; i<Dims; i++) {
				(*this)[i] += other[i];
			}
			return *this;
		}

		Vector& operator-=(const Vector& other) {
			for(size_t i = 0; i<Dims; i++) {
				(*this)[i] -= other[i];
			}
			return *this;
		}

		template<typename S>
		Vector& operator*=(const S& s) {
			for(size_t i =0; i<Dims; i++) {
				(*this)[i] *= s;
			}
			return *this;
		}

		template<typename S>
		Vector& operator/=(const S& s) {
			for(size_t i =0; i<Dims; i++) {
				(*this)[i] /= s;
			}
			return *this;
		}

		Vector operator+(const Vector& other) const {
			return Vector(*this) += other;
		}

		Vector operator-(const Vector& other) const {
			return Vector(*this) -= other;
		}

		template<typename S>
		Vector operator*(const S& factor) const {
			return Vector(*this) *= factor;
		}

		template<typename S>
		Vector operator/(const S& divisor) const {
			return Vector(*this) /= divisor;
		}

	};


} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
