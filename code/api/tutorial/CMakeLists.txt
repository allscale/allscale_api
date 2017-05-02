

# -- create new tutorial targets --

add_custom_target( tutorial )
add_custom_target( test_tutorials )


# for all tutorials

# find list of tutorials
set( tutorials "" )
file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach( child ${children} )
	if ( IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child} )
		list( APPEND tutorials ${child} )
	endif()
endforeach()


# create targets for each tutorial
foreach( tutorial ${tutorials} )
	
	# create build directory for this tutorial
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${tutorial})
	
	# -- tutorial build target --
	
	# create a target for the tutorial
	add_custom_target ( tutorial_${tutorial} )
	
	# add tutorial to list of all tutorials
	add_dependencies ( tutorial tutorial_${tutorial} )
	
	# -- tutorial test target --
	
	# create a test target for the tutorial
	add_custom_target ( tutorial_${tutorial}_test )
	
	# add tutorial to list of all tutorials
	add_dependencies ( test_tutorials tutorial_${tutorial}_test )
	
	
	# find all executable names
	glob_executables(tutorial_exes ${tutorial})
	foreach(exe ${tutorial_exes})
		get_filename_component(exe_name ${exe} NAME_WE)

		# --- building ---

		# The target name will be prefixed with the tutorial name so you can have
		# two identically named executables in different tutorials
		set ( target_name tutorial_${tutorial}_${exe_name} )
		add_executable(${target_name} EXCLUDE_FROM_ALL ${exe})
		
		# add the api dependency
		target_link_libraries(${target_name} api)
	
		# tuturials require pthread
		find_package(Threads REQUIRED)
		target_link_libraries(${target_name} ${CMAKE_THREAD_LIBS_INIT})
	
		# But the output name will not contain the module prefix, this is fine
		# inside the build directory since everything is organized in
		# subfolders. Just be aware when installing the executable, it may be
		# overwritten by one with the same output name.
		set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${tutorial}/${exe_name})
	
		if(MSVC)
			set_target_properties(${target_name} PROPERTIES FOLDER ${tutorial})
		endif()
	
		# add this executable to its tutorial
		add_dependencies ( tutorial_${tutorial} ${target_name} )
	
		# --- testing ---
	
		# also, make the executable a pseudo-test (since there is only one test target)
		add_custom_target( 
			${target_name}_test 
			${CMAKE_CURRENT_BINARY_DIR}/${tutorial}/${exe_name} 
			DEPENDS ${target_name}
		)

		# add this test 
		add_dependencies ( tutorial_${tutorial}_test ${target_name}_test )
	
	endforeach(exe)

endforeach()

