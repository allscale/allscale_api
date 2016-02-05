# AllScale Core API
The core primitives of the AllScale API defining the interface between the AllScale User-Level API and the underlying implemenations, including the AllScale Compiler and Runtime environment itself.

## How to Build

```
mkdir build
cd build
LIBS_HOME=~/libs cmake <path to local repo>
make -j && make test && make valgrind
```
