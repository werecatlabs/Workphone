if (WP_BUILD_TESTS)
    if(TARGET WorkphoneNetwork)
        add_subdirectory(NetworkProduction)
    endif()
    add_subdirectory(GraphicsProduction)
	if (WP_BUILD_C)
		add_subdirectory(./c)
	endif()
	
	if (WP_BUILD_CPP)
		add_subdirectory(./cpp)
	endif()
	
	#add_subdirectory(./cpp_cli)
	
	if (WP_CSHARP)
		if (CMAKE_CSharp_COMPILER)
			add_subdirectory(./csharp)	
		endif()
	endif ()
endif()


