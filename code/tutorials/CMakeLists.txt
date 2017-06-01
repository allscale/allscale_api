add_custom_target(tutorials)
add_custom_target(test_tutorials)

if(MSVC)
	set_target_properties(tutorials PROPERTIES FOLDER tutorials)
	set_target_properties(test_tutorials PROPERTIES FOLDER tutorials)
endif()

file(GLOB tutorials RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src src/*)
foreach(tutorial ${tutorials})
	add_custom_target(tutorials_${tutorial})
	add_dependencies(tutorials tutorials_${tutorial})

	add_custom_target(test_tutorials_${tutorial})
	add_dependencies(test_tutorials test_tutorials_${tutorial})

	if(MSVC)
		set_target_properties(tutorials_${tutorial} PROPERTIES FOLDER tutorials/${tutorial})
		set_target_properties(test_tutorials_${tutorial} PROPERTIES FOLDER tutorials/${tutorial})
	endif()

	glob_executables(tutorial_exes src/${tutorial})
	foreach(exe ${tutorial_exes})
		add_module_executable(tutorials ${exe} NO_LIB EXCLUDE_FROM_ALL OUTPUT_TARGET_NAME exe_tgt)

		# api dependency
		target_link_libraries(${exe_tgt} api)

		# pthread dependency
		target_link_libraries(${exe_tgt} Threads::Threads)

		# add to tutorials target
		add_dependencies(tutorials_${tutorial} ${exe_tgt})

		# setup as test
		if(NOT MSVC)
			add_custom_target(${exe_tgt}_test COMMAND $<TARGET_FILE:${exe_tgt}>)
		else()
			add_custom_target(${exe_tgt}_test COMMAND ${tutorial}/$(Configuration)/$<TARGET_FILE_NAME:${exe_tgt}>)
		endif()
		add_dependencies(${exe_tgt}_test ${exe_tgt})
		add_dependencies(test_tutorials_${tutorial} ${exe_tgt}_test)

		if(MSVC)
			set_target_properties(${exe_tgt} PROPERTIES
				FOLDER tutorials/${tutorial}
				RUNTIME_OUTPUT_DIRECTORY ${tutorial}
			)
			set_target_properties(${exe_tgt}_test PROPERTIES
				FOLDER tutorials/${tutorial}
				RUNTIME_OUTPUT_DIRECTORY ${tutorial}
			)
		endif()
	endforeach(exe)
endforeach(tutorial)
