#define OMP_CILK_IMPL
#include "../future.cc"

TEST(Parec,ImplCheck) {
	EXPECT_EQ("OpenMP/Cilk", PAREC_IMPL);
}
