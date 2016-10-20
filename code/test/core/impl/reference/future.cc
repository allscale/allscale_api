#define REFERENCE_IMPL
#include "../../future.cc"

TEST(Parec,ImplCheck) {
	EXPECT_EQ("Reference SharedMemory", PAREC_IMPL);
}
