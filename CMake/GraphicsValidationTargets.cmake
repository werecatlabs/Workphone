# CTest commands can invoke an executable through a Python driver. Export all
# executable paths after Tools/Tests have been declared so validation builds can
# resolve both the command and its executable arguments, including missing files.
function(wp_collect_validation_executables directory)
    get_property(_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_target IN LISTS _targets)
        get_target_property(_type "${_target}" TYPE)
        if(_type STREQUAL "EXECUTABLE")
            set_property(GLOBAL APPEND PROPERTY WP_VALIDATION_EXECUTABLES "${_target}")
        endif()
    endforeach()
    get_property(_children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_child IN LISTS _children)
        wp_collect_validation_executables("${_child}")
    endforeach()
endfunction()
wp_collect_validation_executables("${CMAKE_SOURCE_DIR}")
get_property(_executables GLOBAL PROPERTY WP_VALIDATION_EXECUTABLES)
set(_mapping "")
foreach(_target IN LISTS _executables)
    string(APPEND _mapping "${_target}|$<TARGET_FILE:${_target}>\n")
endforeach()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/graphics-validation/$<CONFIG>/targets.txt"
    CONTENT "${_mapping}")
