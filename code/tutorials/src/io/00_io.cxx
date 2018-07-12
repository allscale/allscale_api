#include <cstdlib>

#include "allscale/api/user/data/static_grid.h"
#include "allscale/api/core/io.h"

using namespace allscale::api::user;
using namespace allscale::api::core;

// -- size of the grid --
const int size = 10;

// -- files that store the data --
const std::string binary_filename = "binary";
const std::string text_filename = "text";

// -- grid --
using Grid = data::StaticGrid<int, size, size>;

// -- store --
void store() {
    Grid grid;

    // -- initialize the grid --
    algorithm::pfor(Grid::coordinate_type{}, grid.size(), [&](const auto& p){
        grid[p] = (int)(p[0] * size + p[1]);
	});

    // -- store the grid --
    FileIOManager& manager = FileIOManager::getInstance();

    Entry binary = manager.createEntry(binary_filename, Mode::Binary);

    auto output_stream = manager.openOutputStream(binary);

    // write data to file in parallel
    algorithm::pfor(Grid::coordinate_type{}, grid.size(), [&](const auto& p){
        output_stream.atomic([&](auto& out) {
            out.write(p);
            out.write(grid[p]);
        });
	});

    manager.close(output_stream);
}

// -- load --
void load() {
    Grid grid;

    FileIOManager& manager = FileIOManager::getInstance();

    Entry binary = manager.createEntry(binary_filename, Mode::Binary);

    // -- read data --
    auto in = manager.openInputStream(binary);
    for(int i = 0; i < size * size; i++) {
        auto coord = in.read<Grid::coordinate_type>();
        auto count = in.read<int>();
        grid[coord] = count;
    }

    // -- write data ordered --
    Entry text = manager.createEntry(text_filename, Mode::Text);

    auto output_stream_text = manager.openOutputStream(text);

    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            output_stream_text << grid[{i, j}];
            if(j != size - 1)
                output_stream_text << ", ";
        }
        output_stream_text << "\n";
    }

    // remove the created files
    manager.remove(binary);
    manager.remove(text);

}

int main() {

    // -- generate a grid and store it --
    store();

    // -- load the grid, structure it and store it again --
    load();

    return EXIT_SUCCESS;
}
