#include <iostream>
#include <cstdlib>

#include "allscale/api/user/data/adaptive_grid.h"
#include "allscale/api/user/algorithm/pfor.h"


using namespace allscale::api::user;
using algorithm::pfor;

// adaptive grid layer definition
using TwoLayerCellConfig = data::CellConfig<
        2,                           // < we have a 2D adaptive grid
        data::layers<                // < with
            data::layer<2,2>         // < one 2x2 refinement layer
        >
    >;

using Grid = data::AdaptiveGrid<double,TwoLayerCellConfig>;
using Cell = typename Grid::element_type;
using Point = typename Grid::coordinate_type;

using Delta = allscale::utils::Vector<double, 2>;
using Sigma = allscale::utils::Vector<double, 2>;


/*
* Calculate the average of a cell
*/
double average(const Cell& cell) {
    int count = 0;
    double avg = 0;
    cell.forAllActiveNodes([&](const auto& element) {
        avg += element;
        count++;
    });
    return avg / (double)count;
}

/*
* Access a given index
*/
double access_index(const Grid& grid, int i, int j) {
    auto& cell = grid[{i / 2, j / 2}];
    if(cell.getActiveLayer() == 1) {
        return cell[{0, 0}];
    }
    return cell[{i % 2, j % 2}];
}

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
    //inner points
    pfor({0, 0}, up.size(), [&](const auto& pos) {
        up[pos].forAllActiveNodes([&](const allscale::utils::Vector<int64_t,2>& cell_pos, auto& element){
            double prev, prevm, lap;
            double ij, ip, im, jp, jm;

            int ip1, im1, jp1, jm1;

            int i = pos[0] * 2;
            int j = pos[1] * 2;

            if(up[pos].getActiveLayer() == 1 || u[pos].getActiveLayer() == 1) {
                ip1 = i + (pos[0] == 0 ? 2 : -1);
                im1 = i + (pos[0] == up.size()[0] - 1 ? -1 : 2);
                jp1 = j + (pos[1] == 0 ? 2 : -1);
                jm1 = j + (pos[1] == up.size()[1] - 1 ? -1 : 2);

                ip = average(u[{ip1 / 2, j / 2}]);
                im = average(u[{im1 / 2, j / 2}]);
                jp = average(u[{i / 2, jp1 / 2}]);
                jm = average(u[{i / 2, jm1 / 2}]);
                ij = average(u[{i / 2, j / 2}]);

                prev = average(u[pos]);
                prevm = average(um[pos]);
            }
            else {
                i += cell_pos[0];
                j += cell_pos[1];
                ip1 = i + (pos[0] == 0 ? 1 : -1);
                im1 = i + (pos[0] == up.size()[0] - 1 ? -1 : 1);
                jp1 = j + (pos[1] == 0 ? 1 : -1);
                jm1 = j + (pos[1] == up.size()[1] - 1 ? -1 : 1);

                ip = access_index(u, ip1, j);
                im = access_index(u, im1, j);
                jp = access_index(u, i, jp1);
                jm = access_index(u, i, jm1);
                ij = access_index(u, i, j);


                prev = access_index(u, i, j);
                prevm = access_index(um, i, j);
            }


            lap = (dt/delta.x) * (dt/delta.x) * ((ip - ij)
            - (ij - im))
            + (dt/delta.y) * (dt/delta.y) * ((jp - ij)
            - (ij - jm));

            element = config::a * 2 * prev - config::b * prevm + config::c * lap;
        });
    });
}


// -- init and update wrappers --

void initialize(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
	step<init_config>(up,u,um,dt,delta);
}

void update(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
	step<update_config>(up,u,um,dt,delta);
}


void adapt(Grid& up, Grid& u, Grid& um) {
    const double threshold_refine = 0.05;
    const double threshold_coarsen = threshold_refine / 100;

    pfor({0, 0}, u.size(), [&](const auto& pos) {
        bool refine = false;
        bool coarsen = false;

        if((pos[0] > 0 && std::abs(average(u[pos]) - average(u[{pos[0] - 1, pos[1]}])) > threshold_refine)
        || (pos[0] < u.size()[0] - 1 && std::abs(average(u[pos]) - average(u[{pos[0] + 1, pos[1]}])) > threshold_refine)
        || (pos[1] > 0 && std::abs(average(u[pos]) - average(u[{pos[0], pos[1] - 1}])) > threshold_refine)
        || (pos[1] < u.size()[1] - 1 && std::abs(average(u[pos]) - average(u[{pos[0], pos[1] + 1}])) > threshold_refine)) {
            refine = true;
        }

        if(!(pos[0] > 0 && std::abs(average(u[pos]) - average(u[{pos[0] - 1, pos[1]}])) > threshold_coarsen)
        && !(pos[0] < u.size()[0] - 1 && std::abs(average(u[pos]) - average(u[{pos[0] + 1, pos[1]}])) > threshold_coarsen)
        && !(pos[1] > 0 && std::abs(average(u[pos]) - average(u[{pos[0], pos[1] - 1}])) > threshold_coarsen)
        && !(pos[1] < u.size()[1] - 1 && std::abs(average(u[pos]) - average(u[{pos[0], pos[1] + 1}])) > threshold_coarsen)) {
            coarsen = true;
        }

        assert_false(refine && coarsen);

        if(refine && up[pos].getActiveLayer() != 0) {
            up[pos].setActiveLayer(0);
            u[pos].refineGrid([](const double& element) { 			
                allscale::utils::StaticGrid<double, 2, 2> newGrid;
                newGrid.forEach([element](auto& e) { e = element; });
                return newGrid;
            });
            um[pos].refineGrid([](const double& element) { 			
                allscale::utils::StaticGrid<double, 2, 2> newGrid;
                newGrid.forEach([element](auto& e) { e = element; });
                return newGrid;
            });
        }
        if(coarsen && up[pos].getActiveLayer() != 1) {
            up[pos].setActiveLayer(1);
            u[pos].coarsenGrid([&](const auto& grid) { 			
                double count = 0;
                grid.forEach([&](const double& element) { count += element; });
                return count / (grid.size()[0] * grid.size()[1]);
            });
            um[pos].coarsenGrid([&](const auto& grid) { 			
                double count = 0;
                grid.forEach([&](const double& element) { count += element; });
                return count / (grid.size()[0] * grid.size()[1]);
            });
        }
    });
}


// -- a function to create an initial wave --

void setupWave(Grid& u, const Point& center, double amp, const Sigma s) {
    // update u to model a bell-shaped surface wave
    algorithm::pfor({0, 0}, u.size(), [&](const auto& pos) {
        // move to coarsest layer
        u[pos].setActiveLayer(0);

        // set value on this layer
        int i = pos[0];
        int j = pos[1];
        double diffx = j - center.x;
        double diffy = i - center.y;
        u[pos] = amp * exp(- (diffx * diffx / (2 * s.x * s.x) + diffy * diffy / (2 * s.y * s.y)));
    });
}


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

    up.forEach([](Cell& cell) {
        cell = 0.;
    });

    // initialize simulation (setting up the um state)
    initialize(um,u,up,dt,{dx,dy});

    // -- simulation --

    //simulate time steps
    for(double t=0; t<=T; t+=dt) {
        std::cout << "t=" << t << "\n";

        // adapt cell refinement to current simulation values
//        adapt(up, u, um);

        // simulate
        update(up, u, um, dt, {dx,dy});

        // print out current state
        {
            double sum = 0;
            for(int i=0; i<rows;i++) {
                for(int j=0; j<columns; j++) {
                    auto v = average(u[{i,j}]);
                    sum += v;
                    std::cout << (
                            (v >  0.3) ? 'X' :
                            (v >  0.1) ? '+' :
                            (v > -0.1) ? '-' :
                            (v > -0.3) ? '.' :
                                         ' '
                    );
                }
                std::cout << "     ";
                for(int j=0; j<columns; j++) {
                    auto layer = u[{i,j}].getActiveLayer();
                    std::cout << ((layer == 0) ? '-' : '+');
                }

                std::cout << "\n";
            }
            std::cout << "Volume: " << sum << "\n";
            std::cout << "\n";

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // rotate buffers
        std::swap(um, u);
        std::swap(u, up);
    }

    return EXIT_SUCCESS;
}
