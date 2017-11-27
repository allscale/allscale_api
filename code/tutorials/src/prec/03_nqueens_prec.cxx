#include <iostream>
#include "allscale/api/core/prec.h"
#include "allscale/api/user/arithmetic.h"

using namespace allscale::api::core;

template<typename Iter, typename Filter, typename Map, typename Reduce>
auto reduceIf(const Iter& a, const Iter& b, const Filter& filter, const Map& map, const Reduce& reduce) {
    using treeture_type = typename allscale::utils::lambda_traits<Map>::result_type::treeture_type;
    using result_type = typename allscale::utils::lambda_traits<Reduce>::result_type;

    // check that the interval is not empty
    if (a>b) return result_type(0);

    // spawn tasks and collect without heap allocation
    assert_lt(b-a,32);
    std::bitset<32> mask;
    std::array<treeture_type, 32> tasks;
    std::size_t j = 0;
    for(Iter i = a; i<b; ++i,++j) {
        if (!filter(i)) continue;
        mask.set(j);
        tasks[j] = map(i);
    }

    // collect results
    result_type res = 0;
    for(std::size_t j = 0; j < std::size_t(b-a); ++j) {
        if (!mask.test(j)) continue;
        res = reduce(res,std::move(tasks[j]).get());
    }
    return res;
}

template<typename Iter, typename Filter, typename Map>
auto sumIf(const Iter& a, const Iter& b, const Filter& filter, const Map& map) {
    return reduceIf(a,b,filter,map,std::plus<int>());
}

struct Assignment {
    
    int column;				// < the number of columns assigned so far
    int row;				// < the row this queen is placed
    const Assignment* rest;	// < the rest of this assignment

    Assignment()
        : column(-1), row(0), rest(nullptr) {}

    Assignment(int row, const Assignment& rest)
        : column(rest.column+1), row(row), rest(&rest) {}

    int size() const {
        return column + 1;
    }

    bool valid(int r) const {
        return valid(r,column+1);
    }

    bool valid(int r, int c) const {
        assert_lt(column,c);
        // check end of assignment
        if (column<0) return true;
        // if in same row => fail
        if (row == r) return false;
        // if on same diagonal => fail
        auto diff = c - column;
        if (row + diff == r || row - diff == r) return false;
        // check nested
        return rest->valid(r,c);
    }
};

int nqueens(int size) {
    // create the recursive version
    auto compute = prec(
        [size](const Assignment& args) {
            // check whether the assignment is complete
            return args.size() >= size;
        },
        [](const Assignment&) {
            // if a complete assignment is reached, we have a solution
            return 1;
        },
        [size](const Assignment& a, const auto& rec) {
            return sumIf(0,size,
                [&](int i){ return a.valid(i); },
                [&](int i){ return rec(Assignment(i,a)); }
            );
        }
    );

    // compute the result
    return compute(Assignment()).get();
}

int main() {
    const int N = 10;

    int solutions = nqueens(N);
    
    std::cout << "There are " << solutions << " solutions." << std::endl;
}