#include <cstdlib>
#include <iostream>

long fibonacci(const unsigned int number) {
	if(number < 2)
		return number;
	return fibonacci(number - 1) + fibonacci(number - 2);
}

int main() {
	const int N = 12;

	long fib = fibonacci(N);

	std::cout << "fibonacci(" << N << ") = " << fib << std::endl;

	return EXIT_SUCCESS;
}
