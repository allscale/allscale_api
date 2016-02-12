#pragma once

#include <vector>
#include <iostream>

#include "allscale/utils/printer/join.h"

namespace std {

	template<typename E>
	ostream& operator<<(ostream& out, const vector<E>& data) {
		return out << "[" << allscale::utils::join(",", data) << "]";
	}

}
