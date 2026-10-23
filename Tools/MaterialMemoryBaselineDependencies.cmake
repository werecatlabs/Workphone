# Test-only dependency imports for the material-memory baseline at 9572201fa.
# Configure the isolated baseline with WP_BUILD_DEPENDENCIES=OFF and pass this
# file through CMAKE_PROJECT_INCLUDE. Build only RelWithDebInfo with the shared
# /MD CRT. Copy the matching third-party libraries into the isolated checkout;
# do not share engine libraries, runtime binaries or output-directory junctions.
include_guard(GLOBAL)

if(WP_BUILD_DEPENDENCIES)
    message(FATAL_ERROR "Material memory baseline imports require WP_BUILD_DEPENDENCIES=OFF")
endif()

set(WP_MATERIAL_MEMORY_THIRDPARTY_DIR
    "${CMAKE_SOURCE_DIR}/libs/windows/v145/x64/MD/RelWithDebInfo"
    CACHE PATH "Isolated matching third-party libraries for material memory profiling")

foreach(_wp_material_profile_target IN ITEMS Meshopt FreeImage imgui)
    set(_wp_material_profile_library
        "${WP_MATERIAL_MEMORY_THIRDPARTY_DIR}/${_wp_material_profile_target}.lib")
    if(NOT EXISTS "${_wp_material_profile_library}")
        message(FATAL_ERROR "Missing isolated profiling dependency: ${_wp_material_profile_library}")
    endif()
    add_library(${_wp_material_profile_target} STATIC IMPORTED GLOBAL)
    set_target_properties(${_wp_material_profile_target} PROPERTIES
        IMPORTED_LOCATION "${_wp_material_profile_library}")
endforeach()

unset(_wp_material_profile_library)
unset(_wp_material_profile_target)
