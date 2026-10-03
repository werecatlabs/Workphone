project(minizip C CXX)

file(GLOB_RECURSE HEADER_FILES "./include/*.h")
file(GLOB_RECURSE SOURCE_FILES "./source/*.c" "./source/*.cpp")

include_directories( include )
include_directories( include/zzip )
include_directories( ../zlib/include )

add_library(minizip ${HEADER_FILES} ${SOURCE_FILES})
