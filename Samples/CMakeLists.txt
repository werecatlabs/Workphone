if (WP_BUILD_C)
	source_group(./c c)
	add_subdirectory(./c)
endif()

if (WP_BUILD_CPP)
	source_group(./cpp cpp)
	add_subdirectory(./cpp)
endif()

if (WP_BUILD_CSHARP)
	#add_subdirectory(./cpp_cli)
endif()

#add_subdirectory(./csharp)
#add_subdirectory(./python)



