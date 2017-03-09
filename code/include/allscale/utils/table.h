#pragma once

#include <cstring>

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/join.h"

namespace allscale {
namespace utils {

	/**
	 * A container for a const-sized array of elements, which may or may
	 * not be owned by instances of this type.
	 */
	template<typename T>
	class Table {

		std::size_t length;

		T* data;

		bool owned;

	public:

		using const_iterator = const T*;
		using iterator = T*;

		Table()
			: length(0), data(nullptr), owned(false) {}

		Table(std::size_t size)
			: length(size), data(allocate(length)), owned(true) {

			// see whether there is something to do
			if (std::is_trivially_default_constructible<T>::value) return;

			// use in-place default constructor
			for(auto& cur : *this) {
				new (&cur) T();
			}
		}

		Table(T* data, std::size_t size)
			: length(size), data(data), owned(false) {}

		Table(T* begin, T* end)
			: Table(begin,std::distance(begin,end)) {}


		Table(const Table& other)
			: length(other.length),
			  data(allocate(length)),
			  owned(true) {

			// see whether there is something to do
			if (std::is_trivially_copy_constructible<T>::value) {
				std::memcpy(data,other.data,sizeof(T)*length);
				return;
			}

			// use in-place constructor to copy data
			for(std::size_t i=0; i<length; i++) {
				new (&data[i]) T(other.data[i]);
			}
		}

		Table(Table&& other)
			: length(other.length),
			  data(other.data),
			  owned(other.owned) {
			other.owned = false;
		}

		~Table() {
			// if nothing is owned, nothing is done
			if (!owned) return;

			// call destructor for owned elements
			if (!std::is_trivially_destructible<T>::value) {
				for(auto& cur : *this) {
					cur.~T();
				}
			}

			// free the owned memory
			free(data);
		}


		Table& operator=(const Table& other) {

			// shortcut for stupid stuff
			if (this == &other) return *this;

			// free old state
			this->~Table();

			// create a copy of the new state
			new (this) T(other);

			// done
			return *this;
		}

		Table& operator=(Table&& other) {

			// shortcut for stupid stuff
			assert_ne(this,&other) << "Should not be possible!";

			// free old state
			this->~Table();

			// create a copy of the new state
			new (this) T(std::move(other));

			// done
			return *this;
		}

		bool empty() const {
			return length == 0;
		}

		std::size_t size() const {
			return length;
		}

		T& operator[](std::size_t i) {
			return data[i];
		}

		const T& operator[](std::size_t i) const {
			return data[i];
		}

		const_iterator begin() const {
			return data;
		}

		const_iterator cbegin() const {
			return data;
		}

		iterator begin() {
			return data;
		}

		const_iterator end() const {
			return data + length;
		}

		const_iterator cend() const {
			return data + length;
		}

		iterator end() {
			return data + length;
		}

		bool isOwner() const {
			return owned;
		}

		friend std::ostream& operator<<(std::ostream& out, const Table& table) {
			return out << "[" << join(",",table) << "]";
		}

	private:

		static T* allocate(std::size_t size) {
			return reinterpret_cast<T*>(malloc(sizeof(T)*size));
		}

	};

} // end namespace utils
} // end namespace allscale
