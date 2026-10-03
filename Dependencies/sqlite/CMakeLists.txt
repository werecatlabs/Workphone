if (APPLE)
    project(sqlite C CXX OBJC)
else()
    project(sqlite C CXX)
endif()

include_directories( ./include )

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_library(sqlite STATIC ${HEADER_FILES} ${SOURCE_FILES})