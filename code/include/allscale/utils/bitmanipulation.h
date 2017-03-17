#pragma once

#ifdef _MSC_VER
	#include <intrin.h>
#endif

namespace allscale {
namespace utils {

	/**
	 * A wrapper function for counting leading zeros
	 */
	inline int countLeadingZeros(int value) {
		#ifdef _MSC_VER
			unsigned long retVal = 0;
			if(_BitScanReverse(&retVal, value))
				return 31-retVal;
		#else
			return __builtin_clz(value);
		#endif
	}

	/**
	* A wrapper function for counting trailing zeros
	*/
	inline int countTrailingZeros(int value) {
		#ifdef _MSC_VER
			unsigned long retVal = 0;
			if(_BitScanForward(&retVal, value))
				return 31 - retVal;
		#else
			return __builtin_ctz(value);
		#endif
	}

} // end namespace utils
} // end namespace allscale
