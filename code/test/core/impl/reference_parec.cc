#define REFERENCE_IMPL
#include "../parec.cc"

TEST(Parec,ImplCheck) {
	EXPECT_EQ("Reference SharedMemory", PAREC_IMPL);
}
