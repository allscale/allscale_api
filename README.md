# AllScale API
The AllScale API comprising the AllScale Core API and AllScale User API.

The AllScale User API comprises an extendable set of parallel primitives and data constructs to support the effective implementation of efficient HPC applications.

The AllScale Core API defining the interface between the AllScale User API and the underlying implemenations, including the AllScale Compiler and Runtime environment itself.


## Requirements
This project requires C++14 features as offered by e.g. GCC >= 4.9 (GCC 5.2.1 is used for development and testing).

For the build process, `cmake` (Version >=2.6) is required.



## Getting the Sources
Change to a working directory on your system and clone the repository using
```
git clone git@github.com:allscale/allscale_api.git
```
The command creates a sub-directory `allscale_api`.

## Build and Run
Change to your local `allscale_api` directory and create a debug build as follows:
```
mkdir build_debug
cd build_debug
. ../createDebug.sh
make -j8
```
The script `createDebug.sh` utilizes cmake to create a build environment. It uses `g++` as its default compiler and may be customized for other infastructures.

To run the test cases and demos within the project, run 
```
make test ARGS=-j8
```
within the build directory or execute the individual unit tests `ut_*` for targeting specific test suites or demos.

To build and run a release build, run the commands
```
mkdir build_release
cd build_release
. ../createRelease.sh
make -j8
```
within your local `allscale_api` directory.


