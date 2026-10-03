if (WP_BUILD_C)
	add_subdirectory(./c)
endif()

if (WP_BUILD_CPP)
	add_subdirectory(./cpp)
endif()

if (WP_BUILD_CSHARP)
	add_subdirectory(./cpp_cli)
endif()

#add_subdirectory(./csharp)
#add_subdirectory(./python)



