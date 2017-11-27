#include <iostream>
#include <cstdlib>

#include "allscale/api/user/data/adaptive_grid.h"
#include "allscale/api/user/algorithm/pfor.h"

#include "wave_log.h"

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

// TODO: improve this by re-introducing the hierarchical cell address

double getValue(const Cell& cell) {
    if (cell.getActiveLayer() == 1) {
        return cell[{0,0}];
    }
    return average(cell);
}

double getValue(const Cell& cell, const Point& pos) {
    if (cell.getActiveLayer() == 1) {
        return getValue(cell);
    }
    return cell[pos];
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
	auto zero = Grid::coordinate_type(0);
    pfor(zero, up.size(), [&](const auto& pos) {

        // TODO: this is ugly and needs improvement

        // check that all cells are on the same resolution
        assert_eq(up[pos].getActiveLayer(),u[pos].getActiveLayer());
        assert_eq(u[pos].getActiveLayer(),um[pos].getActiveLayer());

        if (um[pos].getActiveLayer() == 1) {

            auto i = pos[0];
			auto j = pos[1];
            //set variables to handle border values
			auto im1 = i + (i == 0 ? 0 : -1);
			auto ip1 = i + (i == up.size()[0] - 1 ? 0 : 1);
			auto jm1 = j + (j == 0 ? 0 : -1);
			auto jp1 = j + (j == up.size()[1] - 1 ? 0 : 1);

            // extract neighbor values
            double nu = getValue(u[{i,jm1}]);
            double nd = getValue(u[{i,jp1}]);
            double nl = getValue(u[{im1,j}]);
            double nr = getValue(u[{ip1,j}]);
            double nc = getValue(u[{i,j}]);

            double lap = (dt/delta.x) * (dt/delta.x) * ((nr - nc) - (nc - nl))
                       + (dt/delta.y) * (dt/delta.y) * ((nd - nc) - (nc - nu));

            // flux correction for fine-grained neighbors
            // see: https://authors.library.caltech.edu/28207/1/cit-asci-tr282.pdf
            double dF = 0;

            if (u[{i,jm1}].getActiveLayer() == 0) {
                auto cur = - ((dt/delta.y) * (dt/delta.y) * (nc - nu));
                cur += .25 * (dt/(delta.y/2)) * (dt/(delta.y/2)) * (nc - getValue(u[{i,jm1}],{0,1}));
                cur += .25 * (dt/(delta.y/2)) * (dt/(delta.y/2)) * (nc - getValue(u[{i,jm1}],{1,1}));
                dF -= cur;
            }

            if (u[{i,jp1}].getActiveLayer() == 0) {
                auto cur = - ((dt/delta.y) * (dt/delta.y) * (nd - nc));
                cur += .25 * (dt/(delta.y/2)) * (dt/(delta.y/2)) * (getValue(u[{i,jp1}],{0,0}) - nc);
                cur += .25 * (dt/(delta.y/2)) * (dt/(delta.y/2)) * (getValue(u[{i,jp1}],{1,0}) - nc);
                dF += cur;
            }

            if (u[{im1,j}].getActiveLayer() == 0) {
                auto cur = - ((dt/delta.x) * (dt/delta.x) * (nc - nl));
                cur += .25 * (dt/(delta.x/2)) * (dt/(delta.x/2)) * (nc - getValue(u[{im1,j}],{1,0}));
                cur += .25 * (dt/(delta.x/2)) * (dt/(delta.x/2)) * (nc - getValue(u[{im1,j}],{1,1}));
                dF -= cur;
            }

            if (u[{ip1,j}].getActiveLayer() == 0) {
                auto cur = - ((dt/delta.x) * (dt/delta.x) * (nr - nc));
                cur += .25 * (dt/(delta.x/2)) * (dt/(delta.x/2)) * (getValue(u[{ip1,j}],{0,0}) - nc);
                cur += .25 * (dt/(delta.x/2)) * (dt/(delta.x/2)) * (getValue(u[{ip1,j}],{0,1}) - nc);
                dF += cur;
            }

            up[{i, j}] = config::a * 2 * getValue(u[{i, j}]) - config::b * getValue(um[{i, j}]) + config::c * lap + dF;


        } else {

            // update each cell on the lower resolution independently
            up[pos].forAllActiveNodes([&](const Point& cell_pos, double& element) {

                // TODO: this is really really ugly => add hierarchical addresses

                // the global position
				auto i = pos[0];
				auto j = pos[1];

                // the sub-cell position
				auto si = cell_pos[0];
				auto sj = cell_pos[1];

                //set variables to handle border values
                auto sim1 = (si == 0) ? 1 : 0;        // < implicitly assuming a 2x2 resolution, bad!
                auto sip1 = (si == 0) ? 1 : 0;        // < implicitly assuming a 2x2 resolution, bad!
                auto sjm1 = (sj == 0) ? 1 : 0;        // < implicitly assuming a 2x2 resolution, bad!
                auto sjp1 = (sj == 0) ? 1 : 0;        // < implicitly assuming a 2x2 resolution, bad!

                // compute neighbor top-level coordinates
                auto im1 = (si == 0) ? i - 1 : i;
                auto ip1 = (si == 1) ? i + 1 : i;
                auto jm1 = (sj == 0) ? j - 1 : j;
                auto jp1 = (sj == 1) ? j + 1 : j;

                // avoid over-shooting boundaries
                if (im1 < 0)             { im1 = 0;              sim1 = 0; }
                if (ip1 >= up.size()[0]) { ip1 = up.size()[0]-1; sip1 = 1; }
                if (jm1 < 0)             { jm1 = 0;              sjm1 = 0; }
                if (jp1 >= up.size()[1]) { jp1 = up.size()[1]-1; sjp1 = 1; }

                // extract neighbor values
                double nu = getValue(u[{i,jm1}],{si,sjm1});
                double nd = getValue(u[{i,jp1}],{si,sjp1});
                double nl = getValue(u[{im1,j}],{sim1,sj});
                double nr = getValue(u[{ip1,j}],{sip1,sj});
                double nc = getValue(u[{i,j}],{si,sj});

                double lap = (dt/(delta.x/2)) * (dt/(delta.x/2)) * ((nr - nc) - (nc - nl))
                           + (dt/(delta.y/2)) * (dt/(delta.y/2)) * ((nd - nc) - (nc - nu));

                element = config::a * 2.0 * getValue(u[{i, j}],{si,sj}) - config::b * getValue(um[{i, j}],{si,sj}) + config::c * lap;

            });

        }

    });
}


// -- init and update wrappers --

void initialize(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
    step<init_config>(up,u,um,dt,delta);
}

void update(Grid& up, const Grid& u, const Grid& um, double dt, const Delta delta) {
    step<update_config>(up,u,um,dt,delta);
}

double id(double x) {
    return x;
}

void refine(Cell& cell) {
    // see whether there is something to do
    if (cell.getActiveLayer() == 0) return;
    // refine this cell
    cell.refine(&id);
}

void coarsen(Cell& cell) {
    // see whether there is something to do
    if (cell.getActiveLayer() == 1) return;
    // refine this cell
    cell.coarsen(&id);
}


void adapt(Grid& up, Grid& u, Grid& um) {
    const double threshold_refine  = 0.002;
    const double threshold_coarsen = 0.001;

    // process each element independently
    pfor({0, 0}, u.size(), [&](const auto& pos) {

        // compute the speed of change (~first derivation)
        double change = fabs(getValue(u[pos]) - getValue(um[pos]));

        // alter refinement level if necessary
        if (change > threshold_refine) {
            // refine buffers
            up[pos].setActiveLayer(0);
            refine(u[pos]);
            refine(um[pos]);
        } else if (change < threshold_coarsen) {
            // coarsen buffers
            up[pos].setActiveLayer(1);
            coarsen(u[pos]);
            coarsen(um[pos]);
        } else {
            // stay as it is
        }

    });
}


// -- a function to create an initial wave --

void setupWave(Grid& u, const Point& center, double amp, const Sigma s) {
    // update u to model a bell-shaped surface wave
    pfor({0, 0}, u.size(), [&](const auto& pos) {
        // move to coarsest layer
        u[pos].setActiveLayer(1);

        // set value on this layer
        double diffx = (double)(pos[0] - center.x);
        double diffy = (double)(pos[1] - center.y);
        u[pos] = amp * exp(- (diffx * diffx / (2 * s.x * s.x) + diffy * diffy / (2 * s.y * s.y)));
    });
}

// a utility to compute the volume, which must be constant over the simulation
double volume(const Grid& u) {
    auto size = u.size();
    double sum = 0;
    for(int i=0; i<size.x;i++) {
        for(int j=0; j<size.y; j++) {
            sum += getValue(u[{i,j}]);
        }
    }
    return sum;
}

// a utility to plot the current state of the adaptive mesh
void plot(const Grid& u) {

    // scale the plot down to may 51 x 51
    auto size = u.size();
	int scale = std::max({(int)(size.x / 51), (int)(size.y / 51), 1 });

    double sum = 0;
    for(int i=0; i<size.x;i+=scale) {

        // plot value of the surface
        for(int j=0; j<size.y; j+=scale) {
            auto v = getValue(u[{i,j}]);
            sum += v;
            std::cout << (
                    (v >  0.3) ? 'X' :
                    (v >  0.1) ? '+' :
                    (v > -0.1) ? '-' :
                    (v > -0.3) ? '.' :
                                 ' '
            );
        }

        // also plot refinement level
        std::cout << "     ";
        for(int j=0; j<size.y; j+=scale) {
            auto layer = u[{i,j}].getActiveLayer();
            std::cout << ((layer == 1) ? '-' : '+');
        }

        std::cout << "\n";
    }
    std::cout << "Volume: " << sum << "\n";
    std::cout << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}


int main() {

    // -- simulation parameters --

    const int N = 100;
    const double T = 200;

    const double dt = 0.25;
    const double dx = 4;
    const double dy = 4;

    const int rows = N;
    const int columns = N;

    // -- output csv for plot.rb script to draw gnuplot or output ascii plot --
    const bool gnuplot = std::getenv("WAVE_GNUPLOT") != nullptr;
    const bool asciiplot = std::getenv("WAVE_ASCIIPLOT") != nullptr;

    // -- initialization --

    // initialize buffers for future, present, and past
    Grid up({rows, columns});
    Grid u({rows, columns});
    Grid um({rows, columns});

    // set up the initial surface disturbance (in the form of a wave)
    setupWave(u,{N/4,N/4},1,{N/8,N/8});

    up.forEach([](Cell& cell) {
        cell.setActiveLayer(1);
        cell = 0.;
    });

    um.forEach([](Cell& cell) {
        cell.setActiveLayer(1);
    });

    // initialize simulation (setting up the um state)
    initialize(um,u,up,dt,{dx,dy});

    // enable printing for gnuplot
    std::unique_ptr<WaveLog> log = nullptr;
    if(gnuplot)
        log = std::make_unique<WaveLog>(std::cout, 2 * rows, 2 * columns);

    // -- simulation --

    //simulate time steps
    double vol_0,vol_1;
    for(double t=0; t<=T; t+=dt) {

        // get the volume before
        vol_0 = volume(u);

        // adapt cell refinement to current simulation values
        adapt(up, u, um);

        // get the volume after adapting -- must not have changed
        vol_1 = volume(u);

        // adapting must not change the volume
        assert_lt(fabs(vol_0-vol_1),0.01)
            << "Before: " << vol_0 << "\n"
            << "After:  " << vol_1;

        // simulate
        update(up, u, um, dt, {dx,dy});

        // compute new volume
        vol_1 = volume(up);

        // print out current state
        if(gnuplot)
            log->print(std::cout, t, up);
        else if(asciiplot)
            plot(up);

        // update must not have altered the volume either
        assert_lt(fabs(vol_0-vol_1),0.01)
            << "Before: " << vol_0 << "\n"
            << "After:  " << vol_1;

        // rotate buffers
        std::swap(um, u);
        std::swap(u, up);
    }

    return EXIT_SUCCESS;
}
