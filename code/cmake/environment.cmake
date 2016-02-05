#
# This file sets up some general variables and include paths for the build environment
#

# -------------------------------------------------------------- define some code locations

ENABLE_LANGUAGE(C)
ENABLE_LANGUAGE(CXX)

# setup std-include directories (to support some IDEs)
if (GCC_INCLUDE_DIR) 
	include_directories( ${GCC_INCLUDE_DIR} )
endif()

# get code root directory (based on current file name path)
get_filename_component( project_cmake_dir ${CMAKE_CURRENT_LIST_FILE} PATH )
get_filename_component( project_code_dir ${project_cmake_dir} PATH )
get_filename_component( project_root_dir ${project_code_dir} PATH )

include (${project_cmake_dir}/lookup_lib.cmake)
include (${project_cmake_dir}/add_unit_test.cmake)

set ( project_inc_dir 	            	${project_code_dir}/include )
set ( project_src_dir 	            	${project_code_dir}/src )

# -------------------------------------------------------------- determines project version

find_package(Git)
if(GIT_FOUND)
	# deduce the code version using git describe
	set ( project_version "`(cd ${project_code_dir}; ${GIT_EXECUTABLE} describe --dirty --always)`")
	#set ( project_version "shit" )
else()
	set ( project_version "unknown" )
endif()



# --------------------------------------------------------------------- including libraries

# set up third-part library home
if (NOT third_part_libs_home )
	if ( DEFINED LIBS_HOME ) 
		set ( third_part_libs_home ${LIBS_HOME} CACHE PATH "Third party library home" )
	else()
		set ( third_part_libs_home $ENV{LIBS_HOME} CACHE PATH "Third party library home" )
	endif()
endif()

# fix GTEST version
set(GTEST_VERSION 1.7.0)

# lookup pthread library
find_library(pthread_lib pthread)

# ------------------------------------------------------------- configuration for platforms

# enable C++0x support within gcc (if supported)
if (CMAKE_COMPILER_IS_GNUCXX)

	# add general flags
	add_definitions( -fshow-column )
	add_definitions( -fdiagnostics-show-option )
	add_definitions( -Wall )
	add_definitions( -fopenmp )
	add_definitions( -Wextra )
	add_definitions( -Werror )
	add_definitions( -pedantic )
	add_definitions( -fcilkplus )

	# add flag allowing arbitrary library ordering (not default in newer distributions)
	set (CMAKE_CXX_FLAGS "-Wl,--no-as-needed ${CMAKE_CXX_FLAGS}")

	# add flags for debug mode
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0")
  
	# add flags for release mode
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

endif()


# check for -std=c++14
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag( -std=c++14 CXX0X_Support )
if(CXX0X_Support)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
else()
	message( "WARNING: --std=c++14 not supported by your compiler!" )
endif()


# --------------------------------------------------------- Valgrind / GTest testing suite

# add -all-valgrind target
if(NOT TARGET valgrind)
	add_custom_target(valgrind)
endif()



