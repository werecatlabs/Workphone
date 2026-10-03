project(luabind)

include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc/luabind )
include_directories( ./ )
include_directories( ../ )

file(GLOB_RECURSE HEADER_FILES "./inc/*.h" "./inc/*.hpp" "./inc/*.inl" )
file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.cpp")

add_library(luabind STATIC ${HEADER_FILES} ${SOURCE_FILES})