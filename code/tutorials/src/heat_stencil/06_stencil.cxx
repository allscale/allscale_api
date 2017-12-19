#include <cstdlib>
#include <iostream>
#include <utility>

#include "allscale/api/user/data/static_grid.h"
#include "allscale/api/user/algorithm/stencil.h"

using namespace allscale::api::user;
using namespace allscale::api::user::algorithm;


int main() {

	const int N = 200;
	const int T = 100;

	const double k = 0.001;

	using Grid = data::StaticGrid<double,N,N>;
	using Point = Grid::coordinate_type;

	Grid temp;

	// initialize temperature
	pfor(Point{N,N},[&](const Point& p){
		temp[p] = 0;

		// one hot spot in the center
		if (p.x == N/2 && p.y == N/2) {
			temp[p] = 100;
		}
	});

	// compute simulation steps
	stencil(temp,T,
		// inner elements
		[k,T,N](time_t, const Point& p, const Grid& temp)->double {
			return temp[p] + k * (
					 temp[p+Point{-1,0}] +
					 temp[p+Point{+1,0}] +
					 temp[p+Point{0,-1}] +
					 temp[p+Point{0,+1}] +
					 (-4)*temp[p]
			);
		},
		// boundaries
		[k,T,N](time_t, const Point&, const Grid&)->double {
			// boundaries are constants
			return 0;
		},
		// an observer
		observer(
			[T](time_t t){ return (t % (T/10)) == 0; },
			[N](const Point& p){ return p.x == N/2 && p.y == N/2; },
			[](time_t t, const Point&, double value){
				std::cout << "t=" << t << " - center: " << value << std::endl;
			}
		)
	);

	// print end state
	std::cout << "t=" << T << " - center: " << temp[Point{N/2,N/2}] << std::endl;

	// check whether computation was successful
	return (temp[Point{N/2,N/2}] < 69) ? EXIT_SUCCESS : EXIT_FAILURE;
}
