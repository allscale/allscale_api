#pragma once

#ifndef ALLSCALE_WITH_HPX
	#include "allscale/utils/serializer.h"
#endif

#include <vector>

#include "allscale/utils/serializer/arrays.h"

namespace allscale {
namespace utils {

	/**
	 * Add support for serializing / de-serializing std::vectors.
	 */
	template<typename T, typename Allocator>
	struct serializer<std::vector<T,Allocator>,typename std::enable_if<is_trivially_serializable<T>::value,void>::type> {

		static std::vector<T,Allocator> load(ArchiveReader& reader) {

			// create the result
			std::vector<T,Allocator> res;

			// load the size
			auto size = reader.read<std::size_t>();

			// load all in one step
			res.resize(size);
			reader.read(&res[0],size);

			// done
			return res;
		}
		static void store(ArchiveWriter& writer, const std::vector<T,Allocator>& value) {

			// start with the size
			writer.write(value.size());

			// followed by all the elements
			writer.write(&value[0],value.size());
		}
	};


	/**
	 * Add support for serializing / de-serializing std::vectors.
	 */
	template<typename T, typename Allocator>
	struct serializer<std::vector<T,Allocator>,typename std::enable_if<!is_trivially_serializable<T>::value && is_serializable<T>::value,void>::type> {

		static std::vector<T,Allocator> load(ArchiveReader& reader) {

			// create the result
			std::vector<T,Allocator> res;

			// load the size
			auto size = reader.read<std::size_t>();

			// make some space
			res.reserve(size);

			// load the elements
			for(std::size_t i=0; i<size; i++) {
				res.emplace_back(reader.read<T>());
			}

			// done
			return res;
		}
		static void store(ArchiveWriter& writer, const std::vector<T,Allocator>& value) {

			// start with the size
			writer.write(value.size());

			// followed by all the elements
			writer.write(&value[0],value.size());
		}
	};

} // end namespace utils
} // end namespace allscale
