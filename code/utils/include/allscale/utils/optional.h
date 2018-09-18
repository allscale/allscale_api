#pragma once

#include "allscale/utils/assert.h"

#include <ostream>

namespace allscale {
namespace utils {

	template<typename T>
	class optional {

		// true, if the object is valid, false otherwise
		bool valid;

		// the stored object, if present
		char obj[sizeof(T)];

	public:

		optional() : valid(false) {};

		optional(const optional& other) : valid(other.valid) {
			if (valid) new (obj) T(other.asRef());
		}

		optional(optional&& other) : valid(other.valid) {
			if (valid) new (obj) T(std::move(other.asRef()));
			other.discard();
		}

		optional(const T& val) : valid(true) {
			new (obj) T(val);
		}

		optional(T&& val) : valid(true) {
			new (obj) T(std::move(val));
		};

		~optional() {
			if (valid) asPtr()->~T();
		}

		// Operator:

		explicit operator bool() const {
			return valid;
		}

		T& operator*() {
			return asRef();
		}

		const T& operator*() const {
			return asRef();
		}

		optional& operator=(const optional& other) {
			if (this == &other) return *this;

			// discard old state
			discard();

			// if the other is representing none => done
			if (!other.valid) return *this;

			// copy other state
			if (!valid) {
				valid = true;
				new (obj) T(other.asRef());
			} else {
				asRef() = other.asRef();
			}

			// done
			return *this;
		}

		optional& operator=(optional&& other) {
			if (this == &other) return *this;

			// discard old state
			discard();

			// if the other is representing none => done
			if (!other.valid) return *this;

			// copy other state
			if (!valid) {
				valid = true;
				new (obj) T(std::move(other.asRef()));
			} else {
				asRef() = std::move(other.asRef());
			}

			// invalidate other
			other.discard();

			// done
			return *this;
		}

		bool operator==(const optional& other) const {
			return (!valid && !other.valid) || (valid && other.valid && asRef() == other.asRef());
		}

		bool operator!=(const optional& other) const {
			return !(*this == other);
		}

		bool operator<(const optional& other) const {
			if (!valid) return other.valid;
			if (!other.valid) return false;
			return asRef() < other.asRef();
		}

		friend std::ostream& operator<<(std::ostream& out, const optional& opt) {
			if (!bool(opt)) return out << "Nothing";
			return out << "Just(" << *opt << ")";
		}

	private:

		T* asPtr() {
			return reinterpret_cast<T*>((void*)&obj[0]);
		}

		const T* asPtr() const {
			return reinterpret_cast<const T*>((void*)&obj[0]);
		}

		T& asRef() {
			assert_true(valid);
			return *asPtr();
		}

		const T& asRef() const {
			assert_true(valid);
			return *asPtr();
		}

		void discard() {
			if (!valid) return;
			asPtr()->~T();
			valid = false;
		}

	};

	template<typename T>
	optional<T> make_optional(const T& val) {
		return { val };
	}

	template<typename T>
	optional<T> make_optional(T&& val) {
		return { std::move(val) };
	}

} // end namespace utils
} // end namespace allscale
