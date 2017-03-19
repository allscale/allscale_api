#pragma once

namespace allscale {
namespace utils {

	// Replacement for std::declval due to a bug in Visual Studio (at least version 15 2017)
	#define DECL_VAL_REF(__TYPE__) reinterpret_cast<__TYPE__&>(*(__TYPE__*)(nullptr))

} // end namespace utils
} // end namespace allscale
