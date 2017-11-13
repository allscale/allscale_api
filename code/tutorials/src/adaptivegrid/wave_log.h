#include <array>
#include <vector>
#include <ostream>

#include "allscale/api/user/data/adaptive_grid.h"
#include "allscale/api/user/data/grid.h"

using namespace allscale::api::user;

using TwoLayerCellConfig = data::CellConfig<2, data::layers<data::layer<2,2>>>;
using AGrid = data::AdaptiveGrid<double,TwoLayerCellConfig>;
using Cell = typename AGrid::element_type;

class WaveLog {
public:
    WaveLog(std::ostream& out, int rows, int columns) {
        out << "t";

        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < columns; j++) {
                out << "," << i << ":" << j << "";
            }
        }
        out << std::endl;
    };

    void print(std::ostream& out, double t, const AGrid& grid) {
        out << t;
        for(int i = 0; i < grid.size()[0] * 2; i++) {
            for(int j = 0; j < grid.size()[1] * 2; j++) {
                const Cell& cell = grid[{i / 2, j / 2}];
                if(cell.getActiveLayer() == 0) {
                    out << "," << cell[{i % 2, j % 2}];
                }
                else {
                    out << "," << cell[{0, 0}];
                }
            }
        }
        out << std::endl;
    };

    void print(std::ostream& out, double t, const data::Grid<double, 2>& grid) {
        out << t;
        for(int i = 0; i < grid.size()[0]; i++) {
            for(int j = 0; j < grid.size()[1]; j++) {
                out << "," << grid[{i, j}];
            }
        }
        out << std::endl;
    };

};