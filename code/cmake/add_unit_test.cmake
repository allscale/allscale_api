
# define macro for adding tests
macro ( add_unit_test case_name ut_prefix )

	#check if target for test suite already exists
	if(NOT TARGET ${ut_prefix})
		add_custom_target(${ut_prefix})
	endif()

	add_dependencies(${ut_prefix} ${case_name})

	# lookup pthread library
	find_package(Threads REQUIRED)

	#With CMAKE 3.2 external projets support byproducts -> this is needed to support ninja as
	#generator
	if(NOT TARGET googletest) 
		include(ExternalProject)
		set(GTEST_PREFIX ${CMAKE_BINARY_DIR}/ep-gtest-${GTEST_VERSION})
		#set(GTEST_PREFIX ${THIRD_PARTY_LIBS_HOME}/ep-gtest-${GTEST_VERSION}/)
		#ugly but necessary, in future versions one can use ${BINARY_DIR} in BUILD_BYPRODUCTS
		set(gtest_lib ${GTEST_PREFIX}/src/googletest-build/googlemock/gtest/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
		set(gtest_main_lib ${GTEST_PREFIX}/src/googletest-build/googlemock/gtest/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})

		# for now, always dynamically link in Windows
		if(MSVC)
			set(googletest_BUILD_SHARED_LIBS ON)
		endif()

		# NOTE: We need to "forward" the C(XX) compiler _and_ it's first argument here in CMAKE_ARGS in order to compile googletest with the same compiler and settings.
		# This is rather hacky as maybe we should already forward some more variables which we don't know of yet until they might actually be needed and things will break.
		# For now this works and we haven't really found a nicer way to do this.
		ExternalProject_Add(googletest
			URL http://insieme-compiler.org/ext_libs/gtest-1.8.0.tar.gz
			URL_HASH SHA256=58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8
			PREFIX ${GTEST_PREFIX} 
			INSTALL_COMMAND "" #make gtest gtest_main
			CMAKE_ARGS -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_C_COMPILER_ARG1=${CMAKE_C_COMPILER_ARG1} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_CXX_COMPILER_ARG1=${CMAKE_CXX_COMPILER_ARG1} -Dgtest_force_shared_crt=${MSVC_SHARED_RUNTIME} -DBUILD_SHARED_LIBS=${googletest_BUILD_SHARED_LIBS}
			BUILD_BYPRODUCTS
				${gtest_lib}
				${gtest_main_lib}
			DOWNLOAD_NO_PROGRESS 1
		)
	
		ExternalProject_Get_Property(googletest source_dir binary_dir)
		set(GTEST_LIBRARY_PATH ${gtest_lib})
		set(GTEST_MAIN_LIBRARY_PATH ${gtest_main_lib})

		set(GTEST_LIBRARY gtest)
		set(GTEST_MAIN_LIBRARY gtest_main)
		add_library(${GTEST_LIBRARY} STATIC IMPORTED)
		add_library(${GTEST_MAIN_LIBRARY} STATIC IMPORTED)
		set_property(TARGET ${GTEST_LIBRARY} PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
		set_property(TARGET ${GTEST_MAIN_LIBRARY} PROPERTY IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY_PATH})
		add_dependencies(${GTEST_LIBRARY} googletest) 
		add_dependencies(${GTEST_MAIN_LIBRARY} googletest) 
	else()
		ExternalProject_Get_Property(googletest source_dir binary_dir)
		set(GTEST_LIBRARY_PATH ${binary_dir}/googlemock/gtest/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
		set(GTEST_MAIN_LIBRARY_PATH ${binary_dir}/googlemock/gtest/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
		#set(GTEST_LIBRARY gtest)
		#set(GTEST_MAIN_LIBRARY gtest_main)
	endif()

	add_dependencies(${case_name} googletest)
	target_include_directories(${case_name} SYSTEM PRIVATE ${source_dir}/googletest/include)
	# add dependency to google test libraries
	target_link_libraries(${case_name} ${GTEST_LIBRARY_PATH})
	target_link_libraries(${case_name} ${GTEST_MAIN_LIBRARY_PATH})

	# add dependency to pthread (TODO check gtest if depends on pthread?)
	target_link_libraries(${case_name} ${CMAKE_THREAD_LIBS_INIT})

	# add normal test
    add_test(NAME ${case_name} COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${case_name}")

endmacro(add_unit_test)


# utility that encodes the sub-path (starting from CMAKE_CURRENT_SOURCE_DIR) into the name of targets
function( fix_case_name output case_file )


        get_filename_component( case_dir ${case_file} DIRECTORY )
        string( CONCAT current_dir ${CMAKE_CURRENT_SOURCE_DIR} "/test" )
        string( LENGTH ${current_dir} current_dir_length)
        string( SUBSTRING ${case_dir} ${current_dir_length} -1 case_stripped_dir )
        string( REPLACE "/" "_" case_stripped_dir "${case_stripped_dir}" )

	set(${output} ${case_stripped_dir} PARENT_SCOPE )

endfunction( fix_case_name )

