#include <cstdlib>
#include <iostream>
#include <utility>

double** allocateMemory(const int N) {
	double** res = new double*[N];
	double* buffer = new double[N*N];
	for(int i = 0; i<N; i++) {
		res[i] = &(buffer[i*N]);
	}
	return res;
}

void freeMemory(double** memory) {
	delete[] memory[0];
	delete[] memory;
}

int main() {

	const int N = 200;
	const int T = 100;

	const double k = 0.001;

	auto A = allocateMemory(N);
	auto B = allocateMemory(N);

	// initialize temperature
	for(int i=0; i<N; i++) {
		for(int j=0; j<N; j++) {
			A[i][j] = 0;
			B[i][j] = 0;

			// one hot spot in the center
			if (i == N/2 && j == N/2) {
				A[i][j] = 100;
			}
		}
	}

	// compute simulation steps
	for(int t=0; t<T; t++) {

		// update step
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
		std::swap(A,B);

	}

	// print end state
	std::cout << "t=" << T << " - center: " << A[N/2][N/2] << std::endl;

	// check whether computation was successful
	bool res = (A[N / 2][N / 2] < 69) ? EXIT_SUCCESS : EXIT_FAILURE;

	freeMemory(A);
	freeMemory(B);

	return res;
}
