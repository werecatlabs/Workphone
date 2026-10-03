if (APPLE)
	project(nativefiledialog C CXX OBJC)
else()
	project(nativefiledialog C CXX)
endif()

file(GLOB_RECURSE HEADER_FILES "./include/*.h")

if (WIN32)
	file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.cpp")
elseif (APPLE)
	file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.m")
endif()

include_directories( include )

add_library(nativefiledialog ${HEADER_FILES} ${SOURCE_FILES})
