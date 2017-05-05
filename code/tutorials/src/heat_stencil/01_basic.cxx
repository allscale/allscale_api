#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>


int main() {

	const int N = 200;
	const int T = 100;

	const double k = 0.001;

	using Grid = std::array<std::array<double,N>,N>;

	auto bufferA = std::make_unique<Grid>();
	auto bufferB = std::make_unique<Grid>();

	// initialize temperature
	Grid& temp = *bufferA;
	for(int i=0; i<N; i++) {
		for(int j=0; j<N; j++) {
			temp[i][j] = 0;

			// one hot spot in the center
			if (i == N/2 && j == N/2) {
				temp[i][j] = 100;
			}
		}
	}

	// compute simulation steps
	for(int t=0; t<T; t++) {

		// update step
		Grid& A = *bufferA;
		Grid& B = *bufferB;

		for(int i=1; i<N-1; i++) {
			for(int j=1; j<N-1; j++) {
				B[i][j] = A[i][j] + k * (
						 A[i-1][j] +
						 A[i+1][j] +
						 A[i][j-1] +
						 A[i][j+1] +
						 (-4)*A[i][j]
				);
			}
		}

		// output gradual reduction of central temperature
		if ((t % (T/10)) == 0) {
			std::cout << "t=" << t << " - center: " << B[N/2][N/2] << std::endl;
		}

		// swap buffers
		std::swap(bufferA,bufferB);

	}

	// print end state
	std::cout << "t=" << T << " - center: " << (*bufferA)[N/2][N/2] << std::endl;

	// check whether computation was successful
	return ((*bufferA)[N/2][N/2] < 69) ? EXIT_SUCCESS : EXIT_FAILURE;
}
