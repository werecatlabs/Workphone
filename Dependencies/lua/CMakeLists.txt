project(lua)

include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ./ )
include_directories( ../ )

file(GLOB_RECURSE HEADER_FILES "./inc/*.h" "./inc/*.hpp" "./inc/*.inl" )
file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.cpp")

add_library(lua STATIC ${HEADER_FILES} ${SOURCE_FILES})
add_executable(luavm ${HEADER_FILES} ${SOURCE_FILES})