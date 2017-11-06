#include <iostream>
#include <cstdlib>

#include "allscale/api/user/data/adaptive_grid.h"
#include "allscale/api/user/algorithm/pfor.h"

using namespace allscale::api::user::data;

using TwoLayerCellConfig = CellConfig<2, layers<layer<2, 2>>>;

/*
* Calculate the average of a cell
*/
template<typename T, std::size_t Dims, typename Layers>
T average(AdaptiveGridCell<T, CellConfig<Dims, Layers>>& cell) {
    int count = 0;
    T avg{};
    cell.forAllActiveNodes([&](const auto& element) {
        avg += element;
        count++;
    });

    return avg / (double)count;
}

/*
* Access a given index
*/
template<typename Grid>
double access_index(Grid& grid, int i, int j) {
    auto& cell = grid[{i / 2, j / 2}];
    if(cell.getActiveLayer() == 1) {
        return cell[{0, 0}];
    }
    return cell[{i % 2, j % 2}];
}

void update(AdaptiveGrid<double, TwoLayerCellConfig>& up, AdaptiveGrid<double, TwoLayerCellConfig>& u, AdaptiveGrid<double, TwoLayerCellConfig>& um, const allscale::utils::Vector<double, 3> influence, double dt, const allscale::utils::Vector<double, 2> delta) {
    //inner points
    allscale::api::user::algorithm::pfor({0, 0}, up.size(), [&](const auto& pos) {
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

            element = influence[0] * 2 * prev - influence[1] * prevm + influence[2] * lap;
        });
    });
}

void adapt(AdaptiveGrid<double, TwoLayerCellConfig>& up, AdaptiveGrid<double, TwoLayerCellConfig>& u, AdaptiveGrid<double, TwoLayerCellConfig>& um) {
    const double threshold_refine = 0.05;
    const double threshold_coarsen = threshold_refine / 100;

    allscale::api::user::algorithm::pfor({0, 0}, u.size(), [&](const auto& pos) {
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

int main() {
    double t = 0;
    const double tstop = 60;
    const double dt = 0.25;
    int step_no = 0;
    const double amp = 1;
    const double x0 = 100;
    const double y0 = 100;
    const double sx = 20;
    const double sy = 20;


    const int rows = 100;
    const int columns = 100;

    const allscale::utils::Vector<double, 2> delta(2, 2);
    allscale::utils::Vector<double, 3> influence(0.5, 0., 0.5);

    AdaptiveGrid<double, TwoLayerCellConfig> up({ rows, columns });
    AdaptiveGrid<double, TwoLayerCellConfig> u({ rows, columns });
    AdaptiveGrid<double, TwoLayerCellConfig> um({ rows, columns });
    AdaptiveGrid<double, TwoLayerCellConfig> init({ rows, columns });
    um.forEach([](auto& cell) {
        cell.forAllActiveNodes([](auto& element) {
            element = 0.;
        });
    });

    init.forEach([](auto& cell) {
        cell.forAllActiveNodes([](auto& element) {
            element = 0.;
        });
    });
    
    allscale::api::user::algorithm::pfor({0, 0}, u.size(), [&](const auto& pos) {
        u[pos].forAllActiveNodes([&](const allscale::utils::Vector<int64_t,2>& cell_pos, auto& element) {
            int i = pos[0] * 2 + cell_pos[0];
            int j = pos[1] * 2 + cell_pos[1];

            double diffx = j - x0;
            double diffy = i - y0;
            element = amp * exp(- (diffx * diffx / (2 * sx * sx) + diffy * diffy / (2 * sy * sy)));
        });
    });

    //init um
    update(um, u, init, influence, dt, delta);

    influence[0] = 1;
    influence[1] = 1;
    influence[2] = 1;

    while(t <= tstop) {
        t += dt;
        step_no++;

        /*
        * adapt cell refinement to current simulation values
        */
        adapt(up, u, um);
       

        /*
        * simulate
        */
        update(up, u, um, influence, dt, delta);
        std::swap(um, u);
        std::swap(u, up);
    }

    return EXIT_SUCCESS;
}