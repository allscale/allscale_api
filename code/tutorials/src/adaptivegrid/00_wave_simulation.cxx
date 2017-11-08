#include <iostream>
#include <cstdlib>

#include "allscale/api/user/data/grid.h"
#include "allscale/utils/vector.h"

using namespace allscale::api::user;
using algorithm::pfor;

using Grid = data::Grid<double,2>;
using Point = typename Grid::coordinate_type;

using Delta = allscale::utils::Vector<double, 2>;
using Sigma = allscale::utils::Vector<double, 2>;

// -- a generic update function for the init and update step --

struct init_config {
	static constexpr double a = .5;
	static constexpr double b =  0;
	static constexpr double c = .5;
};

struct update_config {
	static constexpr double a = 1;
	static constexpr double b = 1;
	static constexpr double c = 1;
};

template<typename config>
void step(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
    pfor({0, 0}, up.size(), [&](const auto& pos) {
        int i = pos[0];
        int j = pos[1];
        //set variables to handle border values
        int im1 = i + (i == 0 ? 1 : -1);
        int ip1 = i + (i == up.size()[0] - 1 ? -1 : 1);
        int jm1 = j + (j == 0 ? 1 : -1);
        int jp1 = j + (j == up.size()[1] - 1 ? -1 : 1);

        double lap = (dt/delta.x) * (dt/delta.x) * ((u[{ip1, j}] - u[{i, j}])
            - (u[{i, j}] - u[{im1, j}]))
            + (dt/delta.y) * (dt/delta.y) * ((u[{i, jp1}] - u[{i, j}])
            - (u[{i, j}] - u[{i, jm1}]));

        up[{i, j}] = config::a * 2 * u[{i, j}] - config::b * um[{i, j}] + config::c * lap;
    });
}

// -- init and update wrappers --

void initialize(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
	step<init_config>(up,u,um,dt,delta);
}

void update(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
	step<update_config>(up,u,um,dt,delta);
}


// -- a function to create an initial wave --

void setupWave(Grid& u, const Point& center, double amp, const Sigma s) {
    // update u to model a bell-shaped surface wave
    algorithm::pfor({0, 0}, u.size(), [&](const auto& pos) {
        int i = pos[0];
        int j = pos[1];
        double diffx = j - center.x;
        double diffy = i - center.y;
        u[pos] = amp * exp(- (diffx * diffx / (2 * s.x * s.x) + diffy * diffy / (2 * s.y * s.y)));
    });
}


// -- the actual simulation --

int main() {

    // -- simulation parameters --

    const int N = 51;
    const double T = 600;

    const double dt = 0.25;
    const double dx = 2;
    const double dy = 2;

    const int rows = N;
    const int columns = N;

    // -- initialization --

    // initialize buffers for future, present, and past
    Grid up({rows, columns});
    Grid u({rows, columns});
    Grid um({rows, columns});

    // set up the initial surface disturbance (in the form of a wave)
    setupWave(u,{N/4,N/4},1,{N/8,N/8});

    up.forEach([](double& element) {
        element = 0.;
    });

    // initialize simulation (setting up the um state)
    initialize(um,u,up,dt,{dx,dy});


    // -- simulation --

    //simulate time steps
    for(double t =0; t<=T; t+=dt) {
        std::cout << "t=" << t << "\n";

        //simulate one simulation step
        update(up, u, um, dt, {dx,dy});

        // print out current state
        {
            double sum = 0;
            for(int i=0; i<rows;i++) {
                for(int j=0; j<columns; j++) {
                    auto v = u[{i,j}];
                    sum += v;
                    std::cout << (
                            (v >  0.3) ? 'X' :
                            (v >  0.1) ? '+' :
                            (v > -0.1) ? '-' :
                            (v > -0.3) ? '.' :
                                         ' '
                    );
                }
                std::cout << "\n";
            }
            std::cout << "Volume: " << sum << "\n";
            std::cout << "\n";

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // rotate grids
        std::swap(um, u);
        std::swap(u, up);
    }

    return EXIT_SUCCESS;
}
