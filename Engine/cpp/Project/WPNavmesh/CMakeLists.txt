project(WPNavmesh)

file(GLOB_RECURSE HEADER_FILES "../../Include/WPNavmesh/*.h" "../../Include/WPNavmesh/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPNavmesh/*.cpp" "../../Source/WPNavmesh/*.c")

add_library(WPNavmesh STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(WPNavmesh 
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/Engine/cpp/Include
        ${CMAKE_SOURCE_DIR}/Engine/cpp/Include/WPNavmesh
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPNavmesh
)

# Precompiled headers
if(WP_USE_PRECOMPILED_HEADERS)
    target_compile_options(WPNavmesh PRIVATE /Yu"WPNavmeshPCH.hpp")
    set_source_files_properties(WPNavmeshPCH.cpp PROPERTIES COMPILE_FLAGS /Yc"WPNavmesh/WPNavmeshPCH.hpp")
endif()

# Export
set_target_properties(WPNavmesh PROPERTIES
    FOLDER "Engine"
    OUTPUT_NAME "WPNavmesh"
)
