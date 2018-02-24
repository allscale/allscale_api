#include <iostream>
#include <cstdlib>
#include <vector>

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
		//assert_lt(column,c);
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

int n_queens(const Assignment& a, int size) {
	if(a.size() >= size)
		return 1;

	int solution = 0;

	for(int i = 0; i < size; i++) {
		if(a.valid(i))
			solution += n_queens(Assignment(i, a), size);
	}
	
	return solution;
}

int main() {
	const int N = 10;

	int solutions = n_queens(Assignment(), N);


	std::cout << "There are " << solutions << " solutions." << std::endl;

	return EXIT_SUCCESS;
}