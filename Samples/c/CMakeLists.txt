set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)

if (WP_BUILD_C)
	add_subdirectory(./HelloWorld)
	#add_subdirectory(./Vehicle)
	#add_subdirectory(./Volume)
endif()



