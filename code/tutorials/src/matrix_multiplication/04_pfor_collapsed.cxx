#include <cstdlib>
#include <iostream>

#include "allscale/api/user/algorithm/pfor.h"
#include "allscale/api/user/data/static_grid.h"

using namespace allscale::api::user;
using namespace allscale::api::user::algorithm;

const int N = 100;

using Matrix = data::StaticGrid<double,N,N>;
using Point = allscale::utils::Vector<int,2>;

Matrix id() {
	Matrix res;
	for(int i=0; i<N; i++) {
		for(int j=0; j<N; j++) {
			res[{i,j}] = (i == j) ? 1 : 0;
		}
	}
	return res;
}

// computes the product of two matrices
Matrix operator*(const Matrix& a, const Matrix& b) {
	Matrix c;
	// in parallel, for each resulting element ...
	pfor(Point{N,N},[&](const Point& p) {
		c[p] = 0;
		for(int k=0; k<N; ++k) {
			c[p] += a[{p.x,k}] * b[{k,p.y}];
		}
	});
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
