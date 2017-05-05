cmake_minimum_required(VERSION 3.2)
project(allscale LANGUAGES C CXX)

# -- Module Path
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)

# -- Prefix Path
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/../third_party CACHE STRING "Third Party Library Directory")
file(GLOB prefix_paths ${THIRD_PARTY_DIR}/*)
list(APPEND CMAKE_PREFIX_PATH ${prefix_paths})

# -- Project Settings
include(build_settings)
include(coverage)
include(doxygen)

# -- Dependency Settings
include(dependencies/pthread)
include(dependencies/googletest)
#include(dependencies/boost)
include(dependencies/valgrind)

# -- CMake Modules
include(add_module)
include(file_globs)
include(msvc_source_group)
include(nproc)

# -- Project Modules
add_subdirectory(api)
add_subdirectory(utils)
add_subdirectory(tutorials)
