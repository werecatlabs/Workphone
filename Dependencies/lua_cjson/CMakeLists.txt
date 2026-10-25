project(lua_cjson)

include_directories( ${WP_LUA_INCLUDE_DIR} )
include_directories( ${WP_ROOT}/Dependencies/lua_cjson/inc )

file(GLOB_RECURSE HEADER_FILES "./inc/*.h" "./inc/*.hpp" "./inc/*.inl" )
file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.cpp")

add_library(lua_cjson STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(lua_cjson PUBLIC lua)
