add_module_library(api HEADER_ONLY)

target_link_libraries(api INTERFACE utils)

glob_executables(api_exes src)
foreach(exe ${api_exes})
	add_module_executable(api ${exe})
endforeach(exe)

glob_tests(api_tests test)
foreach(test ${api_tests})
	add_module_unittest(api ${test})
endforeach(test)
