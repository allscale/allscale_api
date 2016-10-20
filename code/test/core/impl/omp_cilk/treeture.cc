#define OMP_CILK_IMPL
#define BENCH_VALUE 10
#include "../../treeture.cc"

TEST(Basic,ImplCheck) {
	EXPECT_EQ("OpenMP/Cilk", allscale::api::core::impl::getImplementationName());
}
