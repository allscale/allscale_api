macro(msvc_source_group label files)
	if(MSVC)
		set(one_value_args STRIP)
		cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})

		foreach(file ${files})
			get_filename_component(file_dir ${file} DIRECTORY)

			# Note: CMake 3.6 REGEX REPLACE misbehaves in that it continuously
			#       applies the replacement. Therefore REGEX MATCH and REPLACE
			#       is used.

			string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" file_dir "${file_dir}")

			string(REGEX MATCH "^${ARG_STRIP}/?" file_dir_prefix "${file_dir}")
			string(REPLACE "${file_dir_prefix}" "" file_dir "${file_dir}")

			string(REPLACE "/" "\\" file_dir "${file_dir}")

			source_group("${label}\\${file_dir}" FILES ${file})
		endforeach(file)
	endif()
endmacro()
