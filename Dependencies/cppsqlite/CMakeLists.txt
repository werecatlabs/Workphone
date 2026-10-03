if (APPLE)
    project(cppsqlite C CXX OBJC)
else()
    project(cppsqlite C CXX)
endif()

include_directories( ./include )
include_directories( ../sqlite/include )

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_library(cppsqlite STATIC ${HEADER_FILES} ${SOURCE_FILES})