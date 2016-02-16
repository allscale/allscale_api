# AllScale API
The AllScale API comprising the AllScale Core API and AllScale User API.

The AllScale User API comprises an extendable set of parallel primitives and data constructs to support the effective implementation of efficient HPC applications.

The AllScale Core API defining the interface between the AllScale User API and the underlying implemenations, including the AllScale Compiler and Runtime environment itself.


## Requirements
This project requires C++14 features as offered by e.g. GCC >= 4.9 (GCC 5.2.1 is used for development and testing).

For the build process, `cmake` (Version >=2.6) is required.


## How to Build

```
mkdir build
cd build
cmake <path to local repo>
make -j && make test && make valgrind
```
