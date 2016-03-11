#include <gtest/gtest.h>

#include "allscale/utils/large_array.h"
#include "allscale/utils/string_utils.h"

namespace allscale {
namespace utils {

	using namespace detail;

	TEST(Intervals,Covered) {

		Intervals r;
		r.add(40,50);
		r.add(60,75);

		EXPECT_EQ("{[40-50],[60-75]}",toString(r));

		EXPECT_FALSE(r.covers(39));
		EXPECT_TRUE(r.covers(40));
		EXPECT_TRUE(r.covers(49));
		EXPECT_FALSE(r.covers(50));

		for(int i=0; i<100; i++) {
			EXPECT_EQ((40<=i && i<50) || (60<=i && i<75),r.covers(i)) << "Error with i=" << i;
		}
	}

	TEST(Intervals,CoveresAll) {

		Intervals r;
		r.add(40,50);
		r.add(60,75);

		EXPECT_TRUE(r.coversAll(44,46));
		EXPECT_TRUE(r.coversAll(60,75));
		EXPECT_TRUE(r.coversAll(60,70));
		EXPECT_TRUE(r.coversAll(65,75));

		EXPECT_FALSE(r.coversAll(75,76));
		EXPECT_FALSE(r.coversAll(59,60));
		EXPECT_FALSE(r.coversAll(59,75));
		EXPECT_FALSE(r.coversAll(59,76));
		EXPECT_FALSE(r.coversAll(60,76));

		EXPECT_FALSE(r.coversAll(45,65));

		for(int i=0; i<100; i++) {
			for(int j=i+1; j<100; j++) {
				EXPECT_EQ((40<=i && i<50 && j<=50) || (60<=i && i<75 && j<=75),r.coversAll(i,j)) << "Error with i=" << i << " j=" << j;
			}
		}

	}

	TEST(Intervals,CoveresAny) {

		Intervals r;
		r.add(40,50);
		r.add(60,75);

		EXPECT_TRUE(r.coversAny(44,46));
		EXPECT_TRUE(r.coversAny(60,75));
		EXPECT_TRUE(r.coversAny(60,70));
		EXPECT_TRUE(r.coversAny(65,75));

		EXPECT_FALSE(r.coversAny(75,76));
		EXPECT_FALSE(r.coversAny(59,60));
		EXPECT_FALSE(r.coversAny(50,60));
		EXPECT_FALSE(r.coversAny(10,20));
		EXPECT_FALSE(r.coversAny(80,90));

		EXPECT_TRUE(r.coversAny(59,75));
		EXPECT_TRUE(r.coversAny(5,95));
		EXPECT_TRUE(r.coversAny(45,55));
		EXPECT_TRUE(r.coversAny(55,65));

		for(int i=0; i<100; i++) {
			for(int j=i+1; j<100; j++) {
				bool covered = false;
				for(int k=i; k<j && !covered; k++) {
					if (r.covers(k)) covered = true;
				}
				EXPECT_EQ(covered,r.coversAny(i,j)) << "Error with i=" << i << " j=" << j;
			}
		}

	}


	TEST(Intervals,Add) {

		Intervals i;

		EXPECT_EQ("{}",toString(i));

		// add a simple interval
		i.add(40,50);
		EXPECT_EQ("{[40-50]}",toString(i));

		// add very same interval
		i.add(40,50);
		EXPECT_EQ("{[40-50]}",toString(i));

		// add an interval at the last position
		i.add(140,150);
		EXPECT_EQ("{[40-50],[140-150]}",toString(i));

		// add an interval at the first position
		i.add(10,20);
		EXPECT_EQ("{[10-20],[40-50],[140-150]}",toString(i));

		// ad one between two intervals
		i.add(28,32);
		EXPECT_EQ("{[10-20],[28-32],[40-50],[140-150]}",toString(i));


		// add a subset interval
		i = Intervals();
		i.add(40,50);
		EXPECT_EQ("{[40-50]}",toString(i));
		i.add(45,48);
		EXPECT_EQ("{[40-50]}",toString(i));

		// add a super-set interval
		i = Intervals();
		i.add(40,50);
		EXPECT_EQ("{[40-50]}",toString(i));
		i.add(30,60);
		EXPECT_EQ("{[30-60]}",toString(i));


		// add an interval at the beginning
		i = Intervals();
		i.add(40,50);
		EXPECT_EQ("{[40-50]}",toString(i));
		i.add(30,45);
		EXPECT_EQ("{[30-50]}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-50]}",toString(i));

		// add an interval at the end
		i = Intervals();
		i.add(40,50);
		EXPECT_EQ("{[40-50]}",toString(i));
		i.add(45,55);
		EXPECT_EQ("{[40-55]}",toString(i));
		i.add(55,60);
		EXPECT_EQ("{[40-60]}",toString(i));


		// gap-filler
		i = Intervals();
		i.add(40,50); i.add(60,70);
		EXPECT_EQ("{[40-50],[60-70]}",toString(i));
		i.add(50,60);
		EXPECT_EQ("{[40-70]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(51,70);
		EXPECT_EQ("{[40-50],[51-70]}",toString(i));
		i.add(50,51);
		EXPECT_EQ("{[40-70]}",toString(i));


		// long range coverage
		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(5,95);
		EXPECT_EQ("{[5-95]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(25,85);
		EXPECT_EQ("{[20-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(25,65);
		EXPECT_EQ("{[20-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(35,65);
		EXPECT_EQ("{[20-30],[35-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(25,55);
		EXPECT_EQ("{[20-55],[60-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(35,65);
		EXPECT_EQ("{[20-30],[35-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.add(35,55);
		EXPECT_EQ("{[20-30],[35-55],[60-70],[80-90]}",toString(i));

	}

	TEST(Intervals,Remove) {

		Intervals i;

		// -- small ranges --

		i = Intervals();
		EXPECT_EQ("{}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-30]}",toString(i));
		i.remove(20,30);
		EXPECT_EQ("{}",toString(i));


		i = Intervals();
		EXPECT_EQ("{}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-30]}",toString(i));
		i.remove(23,28);
		EXPECT_EQ("{[20-23],[28-30]}",toString(i));

		i = Intervals();
		EXPECT_EQ("{}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-30]}",toString(i));
		i.remove(15,25);
		EXPECT_EQ("{[25-30]}",toString(i));

		i = Intervals();
		EXPECT_EQ("{}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-30]}",toString(i));
		i.remove(25,35);
		EXPECT_EQ("{[20-25]}",toString(i));


		i = Intervals();
		EXPECT_EQ("{}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-30]}",toString(i));
		i.remove(20,25);
		EXPECT_EQ("{[25-30]}",toString(i));

		i = Intervals();
		EXPECT_EQ("{}",toString(i));
		i.add(20,30);
		EXPECT_EQ("{[20-30]}",toString(i));
		i.remove(25,30);
		EXPECT_EQ("{[20-25]}",toString(i));


		// -- long range coverage --
		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(5,95);
		EXPECT_EQ("{}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(25,85);
		EXPECT_EQ("{[20-25],[85-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(25,65);
		EXPECT_EQ("{[20-25],[65-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(35,65);
		EXPECT_EQ("{[20-30],[65-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(25,55);
		EXPECT_EQ("{[20-25],[60-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(35,65);
		EXPECT_EQ("{[20-30],[65-70],[80-90]}",toString(i));

		i = Intervals();
		i.add(40,50); i.add(60,70); i.add(20,30); i.add(80,90);
		EXPECT_EQ("{[20-30],[40-50],[60-70],[80-90]}",toString(i));
		i.remove(35,55);
		EXPECT_EQ("{[20-30],[60-70],[80-90]}",toString(i));


	}

	TEST(LargeArray, Basic) {

		// create a large array
		LargeArray<int> a(1000);

		// allocate a range
		a.allocate(40,100);
		a.allocate(80,200);

		// access range
		for(int i=40; i<200; ++i) {
			a[i] = 12;
		}

		a.free(60,140);

		// access range
		for(int i=40; i<200; ++i) {
			if (i<60 || i>=140) a[i] = 12;
		}

	}

	TEST(LargeArray, Huge) {

		// allocate 1 GiB
		int N = (1024 * 1024 * 1024) / sizeof(int);
		int A = N/2 + 10233;
		int B = A + N/4;

		// create a large array
		LargeArray<int> a(N);

		// allocate a range
		a.allocate(0,N);

		// initialize range
		for(int i=0; i<N; i++) {
			a[i] = i;
		}

		// free a section in the middle
		a.free(A,B);

		for(int i=0; i<N; i++) {
			if (i < A || i >= B) {
				EXPECT_EQ(a[i],i) << "Error at index " << i;
			}
		}

	}

	TEST(LargeArray, MemoryManagement) {

		using value_t = uint64_t;

		// lets check 100 GB
		long N = (100ll * 1024 * 1024 * 1024) / sizeof(value_t);

		// create a large array
		LargeArray<value_t> a(N);

		long step = (1000*1000) / sizeof(value_t);					// we walk in blocks of 1 MB (not MiB to not be a multiple of page sizes)
		long stride = sysconf(_SC_PAGESIZE) / sizeof(value_t) / 2;	// one element every page size

		ASSERT_NE(0, step % sysconf(_SC_PAGESIZE));
		for(long i = 0; i<N; i+= step) {

			value_t low = i;
			value_t hig = std::min(i+step,N);

			// allocate next section
			a.allocate(low,hig);

			// fill fragment
			for(value_t j=low; j<hig; j+=stride) {
				a[j] = j;
			}

			// check previous fragment
			if (low == 0) continue;

			low -= step;
			hig -= step;

			for(value_t j=low; j<hig; j+=stride) {
				if (a[j] != j) EXPECT_TRUE(false) << "Error for j=" << j << "\n";
			}

			// release old section
			a.free(low,hig);

		}

		// run in reverse direction
		for(long i = N; i>=0; i-= step) {

			int64_t low = std::max<long>(0,i-step);
			int64_t hig = i;

			// allocate next section
			a.allocate(low,hig);

			// fill fragment
			for(int64_t j=hig; j>low; j-=stride) {
				a[j-1] = j-1;
			}

			// check previous fragment
			if ((long)(hig) == N) continue;

			low += step;
			hig += step;

			for(int64_t j=hig; j>low; j-=stride) {
				if (a[j-1] != (value_t)(j-1)) EXPECT_TRUE(false) << "Error for j=" << j << "\n";
			}

			// release old section
			a.free(low,hig);
		}

	}

} // end namespace utils
} // end namespace allscale
