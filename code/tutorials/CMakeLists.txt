add_custom_target(tutorials)
add_custom_target(test_tutorials)

glob_executables(tutorial_exes src)
foreach(exe ${tutorial_exes})
	add_module_executable(tutorials ${exe} NO_LIB EXCLUDE_FROM_ALL OUTPUT_TARGET_NAME exe_tgt)

	# api dependenc
	target_link_libraries(${exe_tgt} api)

	# pthread dependency
	target_link_libraries(${exe_tgt} Threads::Threads)

	# add to tutorials target
	add_dependencies(tutorials ${exe_tgt})

	# setup as test
	add_custom_target(${exe_tgt}_test COMMAND $<TARGET_FILE:${exe_tgt}>)
	add_dependencies(${exe_tgt}_test ${exe_tgt})
	add_dependencies(test_tutorials ${exe_tgt}_test)
endforeach(exe)
