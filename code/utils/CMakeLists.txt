add_module_library(utils HEADER_ONLY)

glob_executables(utils_exes src)
foreach(exe ${utils_exes})
	add_module_executable(utils ${exe})
endforeach(exe)

glob_tests(utils_tests test)
foreach(test ${utils_tests})
	add_module_unittest(utils ${test})
endforeach(test)
