#pragma once

#include <sys/mman.h>
#include <unistd.h>
#include <cstddef>

#include <algorithm>
#include <type_traits>

#include "allscale/utils/assert.h"

#include "allscale/utils/printer/vectors.h"

namespace allscale {
namespace utils {


	class Intervals {

		std::vector<std::size_t> data;

	public:

		void add(std::size_t from, std::size_t to) {

			// skip empty ranges
			if (from >= to) return;

			// insert first element
			if (data.empty()) {
				data.push_back(from);
				data.push_back(to);
			}

			// find positions for from and to
			auto it_begin = data.begin();
			auto it_end = data.end();

			auto it_from = std::upper_bound(it_begin, it_end, from);
			auto it_to = std::upper_bound(it_begin, it_end, to-1);

			std::size_t idx_from = std::distance(it_begin,it_from);
			std::size_t idx_to = std::distance(it_begin,it_to);

			// whether insertion is at a common place
			if (it_from == it_to) {

				// if it is between ranges ...
				if (idx_to % 2 == 0) {

					// check whether it is a gap closing a range
					if (idx_to > 1 && idx_to < data.size() && data[idx_to-1] == from && data[idx_to] == to) {
						data.erase(it_from-1,it_to+1);
						return;
					}

					// check whether it is connecting to the one on the left
					if (idx_to > 1 && data[idx_to-1] == from) {
						data[idx_to-1] = to;
						return;
					}

					// check whether it is connecting to the one on the right
					if (idx_to < data.size() && data[idx_to] == to) {
						data[idx_to] = from;
						return;
					}
				}

				// check whether it is the end
				if (it_from == it_end) {
					data.push_back(from);
					data.push_back(to);
					return;
				}

				// check whether it is within an interval
				if ((idx_from % 2) == 1) {
					return;		// nothing to add
				}

				// insert new pair at insertion position
				data.insert(it_from,2,from);
				data[idx_from+1] = to;

				return;
			}

			// if from references an existing start value => correct it
			if (idx_from % 2 == 0) {
				data[idx_from] = from;
				++it_from;
			} else {
				// all fine
			}

			// correct end of last closed interval
			if (idx_to % 2 == 0) {
				data[idx_to-1] = to;
				it_to -= 1;
			} else {
				// nothing to do here
			}

			if (it_from < it_to) data.erase(it_from,it_to);

		}

		void remove(std::size_t from, std::size_t to) {

			// quick exits
			if (from >= to) return;
			if (data.empty()) return;

			// find positions for from and to
			auto it_begin = data.begin();
			auto it_end = data.end();

			auto it_from = std::upper_bound(it_begin, it_end, from);
			auto it_to = std::upper_bound(it_begin, it_end, to-1);

			std::size_t idx_from = std::distance(it_begin,it_from);
			std::size_t idx_to = std::distance(it_begin,it_to);

			// in case they are both at the same spot
			if (idx_from == idx_to) {

				// if it is between two intervals ..
				if (idx_from % 2 == 0) return;		// .. there is nothing to delete

				// it is within a single interval
				assert_eq(1, idx_from % 2);

				// check whether full interval is covered
				if (data[idx_from-1] == from && data[idx_to] == to) {
					data.erase(it_from-1,it_to+1);
					return;
				}

				// check if lower boundary matches
				if (data[idx_from-1] == from) {
					data[idx_from-1] = to;
					return;
				}

				// check if lower boundary matches
				if (data[idx_to] == to) {
					data[idx_to] = from;
					return;
				}

				data.insert(it_from,2,from);
				data[idx_from+1] = to;
				return;

			}

			if (idx_from % 2 == 1) {
				data[idx_from] = from;
				it_from++;
			}

			if (idx_to % 2 == 1) {
				data[idx_to-1] = to;
				it_to--;
			}

			// delete nodes in-between
			data.erase(it_from,it_to);
			return;

		}

		bool covers(std::size_t idx) const {
			auto begin = data.begin();
			auto end = data.end();
			auto pos = std::upper_bound(begin, end, idx);
			return pos != end && ((std::distance(begin,pos) % 2) == 1);
		}

		bool coversAll(std::size_t from, std::size_t to) {
			if (from >= to) return true;
			auto begin = data.begin();
			auto end = data.end();
			auto a = std::upper_bound(begin, end, from);
			auto b = std::upper_bound(begin, end, to-1);
			return a == b && a != end && ((std::distance(begin,a) % 2) == 1);
		}

		bool coversAny(std::size_t from, std::size_t to) {
			if (from >= to) return false;
			auto begin = data.begin();
			auto end = data.end();
			auto a = std::upper_bound(begin, end, from);
			auto b = std::upper_bound(begin, end, to-1);
			return a < b || (a == b && a != end && ((std::distance(begin,a) % 2) == 1));
		}

		void swap(Intervals& other) {
			data.swap(other.data);
		}

		friend std::ostream& operator<<(std::ostream& out, const Intervals& cur) {
			out << "{";
			for(unsigned i=0; i<cur.data.size(); i+=2) {
				if (i != 0) out << ",";
				out << "[" << cur.data[i] << "-" << cur.data[i+1] << "]";
			}
			return out << "}";
		}

	};



	template<typename T>
	class LargeArray {

		// check that the element type is a trivial type
		static_assert(std::is_trivial<T>::value, "Current implementation of LargeArray only supports trivial element types.");

		/**
		 * A pointer to the first element of the array.
		 */
		T* data;

		/**
		 * The size of this large array.
		 */
		std::size_t size;

		/**
		 * The list of active ranges in this large array (for which the memory is kept alive).
		 */
		Intervals active_ranges;

	public:

		/**
		 * Creates a new large array of the given size.
		 */
		LargeArray(std::size_t size) : size(size) {
			// allocate the address space
			data = (T*)mmap(nullptr,sizeof(T)*size,
					PROT_READ | PROT_WRITE,
					MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE,
					-1,0
				);
			assert_ne((void*)-1,(void*)data);
		}

		/**
		 * Explicitly deleted copy constructor.
		 */
		LargeArray(const LargeArray&) = delete;

		/**
		 * A move constructor for large arrays.
		 */
		LargeArray(LargeArray&& other)
			: data(other.data), size(other.size), active_ranges(std::move(other.active_ranges)) {
			other.data = nullptr;
		}

		/**
		 * Destroys this array.
		 */
		~LargeArray() {
			// free the data
			if (data != nullptr) munmap(data,sizeof(T)*size);
		}

		/**
		 * Explicitly deleted copy-assignment operator.
		 */
		LargeArray& operator=(const LargeArray&) = delete;

		/**
		 * Implementation of move assignment operator.
		 */
		LargeArray& operator=(LargeArray&& other) {
			assert_ne(data,other.data);
			if (data) munmap(data, sizeof(T)*size);
			data = other.data;
			size = other.size;
			active_ranges.swap(other.active_ranges);
			other.data = nullptr;
			return *this;
		}

		/**
		 * Allocates the given range within this large array.
		 * After this call, the corresponding sub-range can be accessed.
		 */
		void allocate(std::size_t start, std::size_t end) {
			// check for emptiness
			if (start >= end) return;
			assert_le(end, size) << "Invalid range " << start << " - " << end << " for array of size " << size;
			active_ranges.add(start,end);
			// TODO: record allocated range
		}

		/**
		 * Frees the given range, thereby deleting the content and freeing the
		 * associated memory pages.
		 */
		void free(std::size_t start, std::size_t end) {

			// check for emptiness
			if (start >= end) return;
			assert_le(end, size) << "Invalid range " << start << " - " << end << " for array of size " << size;

			// remove range from active ranges
			active_ranges.remove(start,end);

			// get address of lower boundary
			uintptr_t ptr_start = (uintptr_t)(data + start);
			uintptr_t ptr_end = (uintptr_t)(data + end);

			auto page_size = getPageSize();
			uintptr_t pg_start = ptr_start - (ptr_start % page_size);
			uintptr_t pg_end = ptr_end - (ptr_end % page_size) + page_size;

			std::size_t idx_start = (pg_start - (uintptr_t)(data)) / sizeof(T);
			std::size_t idx_end   = (pg_end - (uintptr_t)(data)) / sizeof(T);

			assert_le(idx_start,start);
			assert_le(end,idx_end);

			if (active_ranges.coversAny(idx_start,start)) pg_start += page_size;
			if (active_ranges.coversAny(end,idx_end))     pg_end -= page_size;
			pg_end = std::min(pg_end,ptr_end);

			if (pg_start >= pg_end) return;

			void* section_start = (void*)pg_start;
			std::size_t length = pg_end - pg_start;

			munmap(section_start, length);
			auto res = mmap(section_start, length,
					PROT_READ | PROT_WRITE,
					MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE | MAP_FIXED,
					-1,0
				);
			if ((void*)-1 == (void*)res) {
				assert_ne((void*)-1,(void*)res);
			}
		}

		T& operator[](std::size_t pos) {
			return data[pos];
		}

		const T& operator[](std::size_t pos) const {
			return data[pos];
		}

	private:

		static long getPageSize() {
			static const long PAGE_SIZE = sysconf(_SC_PAGESIZE);
			return PAGE_SIZE;
		}

		bool isActiveIndex(std::size_t idx) const {
			return active_ranges.covers(idx);
		}

	};




} // end namespace utils
} // end namespace allscale
