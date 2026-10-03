project(zlib C CXX)

file(GLOB_RECURSE HEADER_FILES "./include/*.h")
file(GLOB_RECURSE SOURCE_FILES "./source/*.c" "./source/*.cpp")

include_directories( ../ )
include_directories( include )

if (WIN32)
	include_directories( ../windows )
endif()

add_library(zlib ${HEADER_FILES} ${SOURCE_FILES})
