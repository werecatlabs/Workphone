project(WPNetwork C CXX)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPNetwork/*.h" "../../Include/WPNetwork/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPNetwork/*.cpp")

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ../../Include )
include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneNetwork )

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPPhysics ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})

        if (WP_BUILD_PHYSICS_DEBUG)
            link_directories(WPPhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
        endif()
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPNetwork STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPNetwork SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPNetwork Workphone.lib)
        target_link_libraries(WPNetwork WorkphoneCore.lib)
        target_link_libraries(WPNetwork WorkphoneNetwork.lib)
    endif()
endif()

target_compile_definitions(WPNetwork PRIVATE WPNetwork_EXPORTS)

target_link_libraries(WPNetwork WorkphoneNetwork)
add_dependencies(WPNetwork Workphone WorkphoneNetwork)

set_target_properties (WPNetwork PROPERTIES FOLDER Engine/Cpp/Network)
