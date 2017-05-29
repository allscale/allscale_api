#include <cstdlib>

const int N = 100;

double** createMatrix() {
	double** res = new double*[N];
	double* buffer = new double[N*N];
	for(int i=0; i<N; i++) {
		res[i] = &(buffer[i*N]);
	}
	return res;
}

void freeMatrix(double** matrix) {
	delete [] matrix[0];
	delete [] matrix;
}

double** id() {
	double** res = createMatrix();
	for(int i=0; i<N; i++) {
		for(int j=0; j<N; j++) {
			res[i][j] = (i == j) ? 1 : 0;
		}
	}
	return res;
}

bool equal(double** a, double** b) {
	for(int i=0; i<N; i++) {
		for(int j=0; j<N; j++) {
			if (a[i][j] != b[i][j]) return false;
		}
	}
	return true;
}

// computes the product of two matrices
double** mm(double** a, double** b) {
	double** c = createMatrix();
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
	auto c = mm(a,b);

	auto res = equal(a,c);

	// free matrices
	freeMatrix(a);
	freeMatrix(b);
	freeMatrix(c);

	// check that the result is correct
	return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
