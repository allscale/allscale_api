#pragma once

#include "allscale/utils/serializer.h"

#include "allscale/utils/optional.h"

namespace allscale {
namespace utils {

	/**
	 * Add support for serializing / de-serializing optional values.
	 */
	template<typename T>
	struct serializer<allscale::utils::optional<T>,typename std::enable_if<is_serializable<T>::value,void>::type> {

		static allscale::utils::optional<T> load(ArchiveReader& reader) {
			bool present = reader.read<bool>();
			if (!present) return {};
			return reader.read<T>();
		}

		static void store(ArchiveWriter& writer, const allscale::utils::optional<T>& value) {
			writer.write<bool>(bool(value));
			if (bool(value)) writer.write<T>(*value);
		}
	};


} // end namespace utils
} // end namespace allscale
