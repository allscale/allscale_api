#pragma once

#ifndef ALLSCALE_WITH_HPX
	#include "allscale/utils/serializer.h"
#endif

#include <map>

namespace allscale {
namespace utils {

	/**
	 * Add support for serializing / de-serializing std::maps.
	 */
	template<typename K, typename V, typename Comparator, typename Allocator>
	struct serializer<std::map<K,V,Comparator,Allocator>,typename std::enable_if<is_serializable<K>::value && is_serializable<V>::value,void>::type> {

		static std::map<K,V,Comparator,Allocator> load(ArchiveReader& reader) {

			// create the result
			std::map<K,V,Comparator,Allocator> res;

			// get the number of key/value pairs
			std::size_t numElements = reader.read<std::size_t>();

			// load all the pairs
			for(std::size_t i = 0; i < numElements; ++i) {
				const auto& key = reader.read<K>();
				const auto& value = reader.read<V>();
				res.emplace(std::move(key), std::move(value));
			}

			// done
			return res;
		}
		static void store(ArchiveWriter& writer, const std::map<K,V,Comparator,Allocator>& value) {

			// start with the size
			writer.write<std::size_t>(value.size());

			// followed by all pairs
			for(const auto& element : value) {
				writer.write(element.first);
				writer.write(element.second);
			}
		}
	};

} // end namespace utils
} // end namespace allscale
