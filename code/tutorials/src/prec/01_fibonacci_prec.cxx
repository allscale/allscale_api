#include <cstdlib>
#include <iostream>

#include "allscale/api/core/prec.h"

using namespace allscale::api::core;

long fibonacci(const unsigned int number) {
	auto f = prec(fun(
		[](const unsigned int& number) {
			return number < 2;
		},
		[](const unsigned int& number) { 
			return number;
		},
		[](const unsigned int& number, const auto& rec) {
			return run(rec(number - 1)).get() + run(rec(number - 2)).get();
		}
	));
	return f(number).get();
}

int main() {
	const int N = 12;

	long fib = fibonacci(N);

	std::cout << "fibonacci(" << N << ") = " << fib << std::endl;

	return EXIT_SUCCESS;
}
