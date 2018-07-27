# AllScale API

The AllScale API comprises the AllScale Core API and AllScale User API.

The AllScale User API comprises an extendable set of parallel primitives and data constructs to support the effective implementation of efficient HPC applications.

The AllScale Core API defines the interface between the AllScale User API and the underlying implemenations, including the AllScale Compiler and Runtime environment itself.

For more detailed information visit our [wiki](https://github.com/allscale/allscale_api/wiki) or [project website](http://www.allscale.eu/home).

## Quickstart

This project requires C++14 features as offered by e.g. GCC >= 4.9 (GCC 5.2.1 is used for development and testing).
Furthermore CMake 3.2 (or later) is required for the build and testing process.
Simply execute the following commands to build the project and run all tests.

    $ mkdir build
    $ cd build
    $ cmake ../code
    $ make -j8
    $ ctest -j8

## Advanced Options

### Configuration

Following options can be supplied to CMake

| Option                  | Values          |
| ----------------------- | --------------- |
| -DCMAKE_BUILD_TYPE      | Release / Debug |
| -DBUILD_SHARED_LIBS     | ON / OFF        |
| -DBUILD_TESTS           | ON / OFF        |
| -DBUILD_DOCS            | ON / OFF        |
| -DBUILD_COVERAGE        | ON / OFF        |
| -DUSE_ASSERT            | ON / OFF        |
| -DALLSCALE_CHECK_BOUNDS | ON / OFF        |
| -DUSE_VALGRIND          | ON / OFF        |
| -DENABLE_PROFILING      | ON / OFF        |

The files `cmake/build_settings.cmake` and `code/CMakeLists.txt` state their
default value.

### Building / Testing

    $ mkdir build
    $ cd build
    $ cmake ../code
    $ make -j8
    $ ctest -j8

## Development

### Executable Bit

When working on Windows via SMB share, consider setting following Git setting.

    $ git config core.filemode false

### Licensor

A script, together with a Git hook, is provided to automatically add a license
header to each source file upon commit. See `scripts/license`.

### Eclipse Project

    $ cmake -G "Eclipse CDT4 - Unix Makefiles" /path/to/project

### Visual Studio Solution

    $ cmake -G "Visual Studio 15 2017 Win64" -DBUILD_SHARED_LIBS=OFF Z:\path\to\project

Add path for third-party libraries when needed.

### Coverage

Building the coverage us currently only supported on Linux, as Perl and Bash
are required. To build and view the coverage set the corresponding CMake flag
to `ON` and run:

    $ make
    $ make coverage
    $ xdg-open coverage/index.html

## Troubleshooting

### Getting GCC 5 / CMake 3.5 / Valgrind (for Testing)

The dependency installer can setup these required tools for you. Its README
(`scripts/dependencies/README.md`) holds the details.

It is preferred to use the operating system's package manager, if applicable.

### No Source Folder in Eclipse Project

Make sure your build folder is located outside the source folder. Eclipse is
not capable of dealing with such a setup correctly.
