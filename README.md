# AllScale API
The AllScale API comprising the AllScale Core API and AllScale User API.

The AllScale User API comprises an extendable set of parallel primitives and data constructs to support the effective implementation of efficient HPC applications.

The AllScale Core API defining the interface between the AllScale User API and the underlying implemenations, including the AllScale Compiler and Runtime environment itself.

## How to Build

```
mkdir build
cd build
LIBS_HOME=~/libs cmake <path to local repo>
make -j && make test && make valgrind
```
