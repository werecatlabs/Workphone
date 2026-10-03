project(luajit)

include_directories( ${WP_ROOT}/Dependencies/luajit/src )
include_directories( ./ )
include_directories( ../ )

file(GLOB_RECURSE HEADER_FILES "./inc/*.h" "./inc/*.hpp" "./inc/*.inl" )
file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.cpp")

add_library(luajit STATIC ${HEADER_FILES} ${SOURCE_FILES})