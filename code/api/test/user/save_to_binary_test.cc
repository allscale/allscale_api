#include <gtest/gtest.h>

#include "allscale/api/user/save_to_binary.h"

namespace allscale {
namespace api {
namespace user {

#define outerSize 5ul
#define innerSize 8ul

void check(std::vector<std::vector<double>> original, std::vector<std::vector<double>> loaded) {
	EXPECT_EQ(outerSize, loaded.size());

	for(size_t i = 0; i < outerSize; ++i) {
		EXPECT_EQ(innerSize, loaded[i].size());

		for(size_t j = 0; j < innerSize; ++j)
			EXPECT_EQ(original[i][j], loaded[i][j]);
	}
}

TEST(SaveToBinary, ArtificialVectorOfVectors) {

	std::vector<std::vector<double>> vecVec;
	std::string filename("testfile.dat");

	// generate arbitrary data
	for(size_t i = 0; i < outerSize; ++i) {
		vecVec.push_back(std::vector<double>());

		for(size_t j = 0; j < innerSize; ++j)
			vecVec[i].push_back((0.3+i)*j);
	}

	// stream
	saveVecVecToFile(vecVec, filename, innerSize);

	std::vector<std::vector<double>> loaded = readVecVecFromFile<double>(filename, outerSize, innerSize);

	check(vecVec, loaded);
	EXPECT_EQ(0, std::remove(filename.c_str()));

#ifndef _MSC_VER

	// memory mapped io
	saveVecVecToFileMM<double>(vecVec, filename, outerSize, innerSize);
	loaded = readVecVecFromFileMM<double>(filename, outerSize, innerSize);

	check(vecVec, loaded);
	EXPECT_EQ(0, std::remove(filename.c_str()));

#endif

}


} // end namespace user
} // end namespace api
} // end namespace allscale
