#pragma once

#include "allscale/utils/serializer.h"

#include <utility>

namespace allscale {
namespace utils {

	/**
	 * Add support for serializing / de-serializing pairs of trivial element types.
	 */
	template<typename A, typename B>
	struct is_trivially_serializable<std::pair<A,B>, typename std::enable_if<is_trivially_serializable<A>::value && is_trivially_serializable<B>::value>::type> : public std::true_type {};

	/**
	 * Add support for serializing / de-serializing pairs of non-trivial element types.
	 */
	template<typename A, typename B>
	struct serializer<std::pair<A,B>,typename std::enable_if<
				is_serializable<A>::value && is_serializable<B>::value && (!is_trivially_serializable<A>::value || !is_trivially_serializable<B>::value),
			void>::type> {

		static std::pair<A,B> load(ArchiveReader& reader) {
			A a = reader.read<A>();
			B b = reader.read<B>();
			return std::make_pair(std::move(a),std::move(b));
		}
		static void store(ArchiveWriter& writer, const std::pair<A,B>& value) {
			writer.write<A>(value.first);
			writer.write<B>(value.second);
		}
	};


} // end namespace utils
} // end namespace allscale
