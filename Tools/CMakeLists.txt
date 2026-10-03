if (WP_BUILD_C)
	#add_subdirectory(./c)
endif()

if (WP_BUILD_CPP)
	add_subdirectory(./cpp)
endif()

if (WIN32)
	if (CMAKE_CSharp_COMPILER)
		message(STATUS "C# compiler found: ${CMAKE_CSharp_COMPILER}")
		add_subdirectory(./csharp)
	else()
		message(STATUS "C# compiler not found. Skipping C# tools.")
	endif()
endif()



