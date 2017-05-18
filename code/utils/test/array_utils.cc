#include <gtest/gtest.h>

#include "allscale/utils/array_utils.h"

namespace allscale {
namespace utils {


struct DontConstructMe {
	DontConstructMe() = delete;
	int x = 0;
	DontConstructMe(int x) : x(x) {}
};

TEST(VectorUtils, Constant) {

    auto arr = build_array<5>([] { return DontConstructMe(5); });

    for(DontConstructMe val : arr) {
        EXPECT_EQ(val.x, 5);
    }
}


TEST(VectorUtils, List) {
	int vals[7] = {0,1,2,3,4,5,6};
	int* valsPtr = vals;

    auto arr = build_array<7>([&valsPtr] { return DontConstructMe(*(valsPtr++)); });

    for(int i = 0; i < 5; ++i) {
        EXPECT_EQ(arr[i].x, vals[i]);
    }
}

} // end namespace utils
} // end namespace allscale
