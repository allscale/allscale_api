#include <array>
#include <cstdlib>

const int N = 100;

using Matrix = std::array<std::array<double,N>,N>;

Matrix id() {
	Matrix res;
	for(int i=0; i<N; i++) {
		for(int j=0; j<N; j++) {
			res[i][j] = (i == j) ? 1 : 0;
		}
	}
	return res;
}

// computes the product of two matrices
Matrix operator*(const Matrix& a, const Matrix& b) {
	Matrix c;
	for(int i=0; i<N; ++i) {
		for(int j=0; j<N; ++j) {
			c[i][j] = 0;
			for(int k=0; k<N; ++k) {
				c[i][j] += a[i][k] * b[k][j];
			}
		}
	}
	return c;
}


int main() {

	// create two matrices
	auto a = id();
	auto b = id();

	// compute the product
	auto c = a * b;

	// check that the result is correct
	return (c == a) ? EXIT_SUCCESS : EXIT_FAILURE;
}
