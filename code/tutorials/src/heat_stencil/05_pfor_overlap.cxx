#include <cstdlib>
#include <iostream>
#include <utility>

#include "allscale/api/user/data/static_grid.h"
#include "allscale/api/user/algorithm/pfor.h"

using namespace allscale::api::user;
using namespace allscale::api::user::algorithm;


int main() {

	const int N = 200;
	const int T = 100;

	const double k = 0.001;

	using Grid = data::StaticGrid<double,N,N>;
	using Point = Grid::coordinate_type;

	Grid bufferA;
	Grid bufferB;

	// initialize temperature
	Grid& temp = bufferA;
	auto ref = pfor(Point{0,0},Point{N,N},[&](const Point& p){
		temp[p] = 0;

		// one hot spot in the center
		if (p.x == N/2 && p.y == N/2) {
			temp[p] = 100;
		}
	});

	Grid* A = &bufferA;
	Grid* B = &bufferB;

	// compute simulation steps
	for(int t=0; t<T; t++) {

		ref = pfor(Point{1,1},Point{N-1,N-1},[A,B,k](const Point& p){
			auto i = p.x;
			auto j = p.y;
			(*B)[{i,j}] = (*A)[{i,j}] + k * (
					 (*A)[{i-1,j}] +
					 (*A)[{i+1,j}] +
					 (*A)[{i,j-1}] +
					 (*A)[{i,j+1}] +
					 (-4)*(*A)[{i,j}]
			);
		}, small_neighborhood_sync(ref));

		// output gradual reduction of central temperature
		if ((t % (T/10)) == 0) {
			ref = after(ref,Point{N/2,N/2},[B,N,t]{
				std::cout << "t=" << t << " - center: " << (*B)[{N/2,N/2}] << std::endl;
			});
		}

		// swap buffers
		std::swap(A,B);

	}

	// wait for completion
	ref.wait();

	// print end state
	std::cout << "t=" << T << " - center: " << temp[Point{N/2,N/2}] << std::endl;

	// check whether computation was successful
	return (temp[Point{N/2,N/2}] < 69) ? EXIT_SUCCESS : EXIT_FAILURE;
}
