
#define REFERENCE_IMPL
#include "../prec.cc"

TEST(Parec,ImplCheck) {
	EXPECT_EQ("Reference SharedMemory", PAREC_IMPL);
}
