#include <iostream>
#include <cstdlib>

#include "allscale/api/user/data/grid.h"
#include "allscale/utils/vector.h"


using namespace allscale::api::user::data;

void update(Grid<double, 2>& up, const Grid<double, 2>& u, const Grid<double, 2>& um, const allscale::utils::Vector<double, 3> influence, double dt, const allscale::utils::Vector<double, 2> delta) {
    allscale::api::user::algorithm::pfor({0, 0}, up.size(), [&](const auto& pos) {
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

        up[{i, j}] = influence[0] * 2 * u[{i, j}] - influence[1] * um[{i, j}] + influence[2] * lap;
    });
}

int main() {
    double t = 0;
    const double tstop = 60;
    const double dt = 0.25;
    const allscale::utils::Vector<double, 2> delta(2, 2);
    allscale::utils::Vector<double, 3> influence(0.5, 0., 0.5);

    const double amp = 1;
    const double x0 = 100;
    const double y0 = 100;
    const double sx = 20;
    const double sy = 20;


    const int rows = 200;
    const int columns = 200;

    
    Grid<double, 2> up({rows, columns});
    Grid<double, 2> u({rows, columns});
    Grid<double, 2> um({rows, columns});

    Grid<double, 2> init({rows, columns});

    init.forEach([](double& element) {
        element = 0.;
    });

    //init u
    allscale::api::user::algorithm::pfor({0, 0}, u.size(), [&](const auto& pos) {
        int i = pos[0];
        int j = pos[1];
        double diffx = j - x0;
        double diffy = i - y0;
        u[pos] = amp * exp(- (diffx * diffx / (2 * sx * sx) + diffy * diffy / (2 * sy * sy)));
    });

    //init um
    update(um, u, init, influence, dt, delta);

    influence[0] = 1;
    influence[1] = 1;
    influence[2] = 1;

    //simulate until t > tstop
    while(t <= tstop) {
        t += dt;
        //simulate one simulation step
        update(up, u, um, influence, dt, delta);
        std::swap(um, u);
        std::swap(u, up);
    }

    return EXIT_SUCCESS;
}