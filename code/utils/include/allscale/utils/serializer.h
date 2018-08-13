#pragma once

#include <cstring>
#include <sstream>
#include <type_traits>
#include <vector>

#include "allscale/utils/assert.h"

#if defined(ALLSCALE_WITH_HPX)
#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>
#include <hpx/runtime/serialization/array.hpp>
#endif

namespace allscale {
namespace utils {

	// ---------------------------------------------------------------------------------
	//									Declarations
	// ---------------------------------------------------------------------------------

	/**
	 * An archive contains the serialized version of some data structure (fragment).
	 * It enables the exchange of data between e.g. address spaces.
	 */
	class Archive;

	/**
	 * An archive writer is a builder for archives. It is utilized for serializing objects.
	 */
	class ArchiveWriter;

	/**
	 * An archive reader is a utility to reconstruct data structures from archives.
	 */
	class ArchiveReader;

	/**
	 * A serializer describes the way types are converted to and restored from archives.
	 */
	template<typename T, typename _ = void>
	struct serializer;

	/**
	 * This type trait can be utilized to test whether a given type is serializable,
	 * thus packable into an archive, or not.
	 */
	template <typename T, typename _ = void>
	struct is_serializable;

	/**
	 * This type trait can be utilized to test whether a given type is trivially serializable,
	 * thus can be archived and restored through a simple memcpy.
	 */
	template<typename T, typename _ = void>
	struct is_trivially_serializable;

	/**
	 * A marker class for trivially serializable types.
	 */
	struct trivially_serializable {};

	/**
	 * A facade function for packing an object into an archive.
	 */
	template<typename T>
	typename std::enable_if<is_serializable<T>::value,Archive>::type
	serialize(const T&);

	/**
	 * A facade function for unpacking an object from an archive.
	 */
	template<typename T>
	typename std::enable_if<is_serializable<T>::value,T>::type
	deserialize(Archive&);


	// ---------------------------------------------------------------------------------
	//									Definitions
	// ---------------------------------------------------------------------------------


	namespace detail {

		/**
		 * A simple, initial, functionally complete implementation of a data buffer
		 * for storing data within an archive.
		 */
		class DataBuffer {

			// check some underlying assumption
			static_assert(sizeof(char)==1, "If a char is more than a byte, this implementation needs to be checked.");

			// the actual data store (std::vector handles the dynamic growing for us)
			std::vector<char> data;

		public:

			DataBuffer() {}

			DataBuffer(const DataBuffer&) = default;
			DataBuffer(DataBuffer&&) = default;

			DataBuffer(const std::vector<char>& data) : data(data) {}
			DataBuffer(std::vector<char>&& data) : data(std::move(data)) {}

			DataBuffer& operator=(const DataBuffer&) = default;
			DataBuffer& operator=(DataBuffer&&) = default;

			/**
			 * The main function for appending data to this buffer.
			 */
			void append(const char* start, std::size_t count) {
				// create space
				auto pos = data.size();
				data.resize(pos + count / sizeof(char));

				// append at end
				std::memcpy(&data[pos],start,count);

			}

			/**
			 * Obtains the number of bytes this buffer is occupying.
			 */
			std::size_t size() const {
				return data.size() * sizeof(char);
			}

			/**
			 * Obtains a pointer to the begin of the internally maintained buffer (inclusive).
			 */
			const char* begin() const {
				return &data.front();
			}

			/**
			 * Obtains a pointer to the end of the internally maintained buffer (exclusive).
			 */
			const char* end() const {
				return &data.back() + 1;
			}

			/**
			 * Support implicit conversion of this buffer to a vector of characters.
			 */
			operator const std::vector<char>&() const {
				return data;
			}

			/**
			 * Also enable the implicit hand-off of the ownership of the underlying char store.
			 */
			operator std::vector<char>() && {
				return std::move(data);
			}


		};

	} // end namespace detail


	class Archive {

		friend class ArchiveWriter;
		friend class ArchiveReader;

		// the data represented by this archive
		detail::DataBuffer data;

		Archive(detail::DataBuffer&& data)
			: data(std::move(data)) {}

	public:

		Archive(const Archive&) = default;
		Archive(Archive&&) = default;

		Archive(const std::vector<char>& buffer) : data(buffer) {}
		Archive(std::vector<char>&& buffer) : data(std::move(buffer)) {}

		Archive& operator=(const Archive&) = default;
		Archive& operator=(Archive&&) = default;

		/**
		 * Support implicit conversion of this archive to a vector of characters.
		 */
		operator const std::vector<char>&() const {
			return data;
		}

		/**
		 * Also enable the implicit hand-off of the ownership of the underlying buffer.
		 */
		operator std::vector<char>() && {
			return std::move(data);
		}

		/**
		 * Provide explicit access to the underlying char buffer.
		 */
		const std::vector<char>& getBuffer() const {
			return data;
		}

		// --- serializer support ---

		void store(ArchiveWriter& out) const;

		static Archive load(ArchiveReader& in);

	};

#if !defined(ALLSCALE_WITH_HPX)
	class ArchiveWriter {

		// the buffer targeted by this archive writer
		detail::DataBuffer data;

	public:

		ArchiveWriter() {}

		ArchiveWriter(const ArchiveWriter&) = delete;
		ArchiveWriter(ArchiveWriter&&) = default;

		ArchiveWriter& operator=(const ArchiveWriter&) = delete;
		ArchiveWriter& operator=(ArchiveWriter&&) = default;

		/**
		 * Appends a given number of bytes to the end of the underlying data buffer.
		 */
		void write(const char* src, std::size_t count) {
			data.append(src,count);
		}

		/**
		 * Appends an array of elements to the end of the underlying data buffer.
		 */
		template <typename T>
		void write(const T* value, std::size_t count) {
			if (is_trivially_serializable<T>::value) {
				write(reinterpret_cast<const char*>(value),sizeof(T)*count);
			} else {
				for(std::size_t i=0; i<count; i++) {
					write(value[i]);
				}
			}
		}

		/**
		 * A utility function wrapping the invocation of the serialization mechanism.
		 */
		template<typename T>
		std::enable_if_t<is_serializable<T>::value,void>
		write(const T& value) {
			// use serializer to store object of this type
			serializer<T>::store(*this,value);
		}

		/**
		 * Obtains the archive produces by this writer. After the call,
		 * this writer must not be used any more.
		 */
		Archive toArchive() && {
			return std::move(data);
		}

	};
#else
	class ArchiveWriter {
		hpx::serialization::output_archive &ar_;

	public:
		ArchiveWriter(hpx::serialization::output_archive &ar) : ar_(ar) {}

		/**
		 * Appends a given number of bytes to the end of the underlying data buffer.
		 */
		void write(const char* src, std::size_t count) {
			ar_ & hpx::serialization::make_array(src, count);
		}

		template <typename T>
		void write(const T* value, std::size_t count)
		{
			ar_ & hpx::serialization::make_array(value, count);
		}

		/**
		 * A utility function wrapping the invocation of the serialization mechanism.
		 */
		template<typename T>
		std::enable_if_t<is_serializable<T>::value,void>
		write(const T& value) {
// 			// use serializer to store object of this type
			serializer<T>::store(*this,value);
		}

		template<typename T>
		std::enable_if_t<!is_serializable<T>::value,void>
		write(const T& value) {
			ar_ & value;
		}
	};
#endif

#if !defined(ALLSCALE_WITH_HPX)
	class ArchiveReader {

		// the current point of the reader
		const char* cur;

		// the end of the reader (only checked for debugging)
		const char* end;

	public:

		/**
		 * A archive reader can only be obtained from an existing archive.
		 */
		ArchiveReader(const Archive& archive)
			: cur(archive.data.begin()), end(archive.data.end()) {}

		ArchiveReader(const ArchiveReader&) = delete;
		ArchiveReader(ArchiveReader&&) = default;

		ArchiveReader& operator=(const ArchiveReader&) = delete;
		ArchiveReader& operator=(ArchiveReader&&) = default;

		/**
		 * Reads a number of bytes from the underlying buffer.
		 */
		void read(char* dst, std::size_t count) {
			// copy the data
			std::memcpy(dst,cur,count);
			// move pointer forward
			cur += count;

			// make sure that we did not cross the end of the buffer
			assert_le(cur,end);
		}

		/**
		 * Reads an array of elements from the underlying buffer.
		 */
		template <typename T>
		void read(T* dst, std::size_t count) {
			if (is_trivially_serializable<T>::value) {
				read(reinterpret_cast<char*>(dst),sizeof(T)*count);
			} else {
				for(std::size_t i = 0; i<count; i++) {
					dst[i] = read<T>();
				}
			}
		}

		/**
		 * A utility function wrapping up the de-serialization of an object
		 * of type T from the underlying buffer.
		 */
		template<typename T>
		std::enable_if_t<is_serializable<T>::value,T>
		read() {
			// use serializer to restore object of this type
			return serializer<T>::load(*this);
		}

	};
#else
	class ArchiveReader {
		hpx::serialization::input_archive &ar_;

	public:
		ArchiveReader(hpx::serialization::input_archive &ar) : ar_(ar) {}

		/**
		 * Reads a number of bytes from the underlying buffer.
		 */
		void read(char* dst, std::size_t count) {
			ar_ & hpx::serialization::make_array(dst, count);
		}

		template <typename T>
		void read(T* dst, std::size_t count) {
			hpx::serialization::array<T> arr(dst, count);
			ar_ & arr;
		}

		/**
		 * A utility function wrapping up the de-serialization of an object
		 * of type T from the underlying buffer.
		 */
		template<typename T>
		std::enable_if_t<is_serializable<T>::value,T>
		read() {
			// use serializer to restore object of this type
			return serializer<T>::load(*this);
		}

		template<typename T>
		std::enable_if_t<!is_serializable<T>::value,T>
		read() {
			// use serializer to restore object of this type
			T t;
			ar_ & t;
			return t;
		}
	};
#endif

	namespace detail {

		struct not_trivially_serializable {};

	}

	template<typename ... Ts>
	struct all_serializable;

	template<>
	struct all_serializable<> : public std::true_type {};

	template<typename T, typename ... Rest>
	struct all_serializable<T,Rest...> : public std::conditional<is_serializable<T>::value, all_serializable<Rest...>,std::false_type>::type {};



	template<typename ... Ts>
	struct all_trivially_serializable;

	template<>
	struct all_trivially_serializable<> : public std::true_type {};

	template<typename T, typename ... Rest>
	struct all_trivially_serializable<T,Rest...> : public std::conditional<is_trivially_serializable<T>::value, all_trivially_serializable<Rest...>,std::false_type>::type {};


	/**
	 * A utility to mark trivially serializable dependent types.
	 */
	template<typename ... Ts>
	using trivially_serializable_if_t = typename std::conditional<
			all_trivially_serializable<Ts...>::value,
			trivially_serializable,detail::not_trivially_serializable
		>::type;



	template<typename T, typename _>
	struct is_trivially_serializable : public std::false_type {};

	template<typename T>
	struct is_trivially_serializable<T, typename std::enable_if<std::is_base_of<trivially_serializable,T>::value && std::is_convertible<T*,trivially_serializable*>::value,void>::type> : public std::true_type {};

	template <typename T, typename _>
	struct is_serializable : public std::false_type {};

	template <typename T>
	struct is_serializable<T, typename std::enable_if<
			// everything that has a proper serializer instance is serializable
			std::is_same<decltype(&serializer<T>::load), T(*)(ArchiveReader&)>::value &&
			std::is_same<decltype(&serializer<T>::store), void(*)(ArchiveWriter&, const T&)>::value,
		void>::type> : public std::true_type {};



	/**
	 * Adds support for the serialization to every trivially serializable type.
	 */
	template<typename T>
	struct serializer<T, typename std::enable_if<is_trivially_serializable<T>::value,void>::type> {
		static T load(ArchiveReader& a) {
			T res;
			a.read(reinterpret_cast<char*>(&res),sizeof(T));
			return res;
		}
		static void store(ArchiveWriter& a, const T& value) {
			a.write(reinterpret_cast<const char*>(&value),sizeof(T));
		}
	};

	/**
	 * Adds support for the serialization to every type T supporting
	 *
	 * 	- a static member function  T load(ArchiveReader&)
	 * 	- a member function		 void store(ArchiveWriter&)
	 *
	 * Thus, serialization / deserialization can be integrated through member functions.
	 */
	template<typename T>
	struct serializer<T, typename std::enable_if<
			// it is not a trivially serializable type
			!is_trivially_serializable<T>::value &&
			// targeted types have to offer a static load member ...
			std::is_same<decltype(T::load(std::declval<ArchiveReader&>())),T>::value &&
			// ... and a store member function
			std::is_same<decltype(std::declval<const T&>().store(std::declval<ArchiveWriter&>())),void>::value,
		void>::type> {

		static T load(ArchiveReader& a) {
			return T::load(a);
		}
		static void store(ArchiveWriter& a, const T& value) {
			value.store(a);
		}
	};


	// -- primitive type serialization --

	template<typename T>
	struct is_trivially_serializable<T,std::enable_if_t<std::is_arithmetic<T>::value,void>> : public std::true_type {};


	// -- enumeration type serialization --

	template<typename T>
	struct is_trivially_serializable<T,std::enable_if_t<std::is_enum<T>::value,void>> : public std::true_type {};


	// -- facade functions --
#if !defined(ALLSCALE_WITH_HPX)
	template<typename T>
	typename std::enable_if<is_serializable<T>::value,Archive>::type
	serialize(const T& value) {
		ArchiveWriter writer;
		writer.write(value);
		return std::move(writer).toArchive();
	}

	template<typename T>
	typename std::enable_if<is_serializable<T>::value,T>::type
	deserialize(Archive& a) {
		return ArchiveReader(a).read<T>();
	}
#endif

	inline void Archive::store(ArchiveWriter& out) const {
		out.write(data.size());
		out.write(data.begin(),data.size());
	}

	inline Archive Archive::load(ArchiveReader& in) {
		auto size = in.read<std::size_t>();
		std::vector<char> data(size);
		in.read(&data[0],size);
		return std::move(data);
	}

} // end namespace utils
} // end namespace allscale

#if defined(ALLSCALE_WITH_HPX)
namespace hpx {
namespace serialization {
	template <typename T>
	typename std::enable_if<
		::allscale::utils::is_serializable<T>::value &&
		!(std::is_integral<T>::value || std::is_floating_point<T>::value),
		output_archive&
	>::type
	serialize(output_archive & ar, T const & t, int) {
		allscale::utils::ArchiveWriter writer(ar);
		writer.write(t);
		return ar;
	}

	template <typename T>
	typename std::enable_if<
		::allscale::utils::is_serializable<T>::value &&
		!(std::is_integral<T>::value || std::is_floating_point<T>::value),
		input_archive&
	>::type
	serialize(input_archive & ar, T & t, int) {
		allscale::utils::ArchiveReader reader(ar);
		t = reader.read<T>();
		return ar;
	}
} // end namespace serialization
} // end namespace hpx
#endif
