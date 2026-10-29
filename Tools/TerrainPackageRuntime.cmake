cmake_minimum_required(VERSION 3.21)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()
# Resolve the actual PE import closure, rather than copying an arbitrary build bin.
foreach(required IN ITEMS EXECUTABLE OUTPUT_DIRECTORY DUMPBIN)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM windows+pe)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL dumpbin)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${DUMPBIN}")
get_filename_component(runtime_directory "${EXECUTABLE}" DIRECTORY)
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${EXECUTABLE}"
    LIBRARIES ${ROOT_LIBRARIES}
    DIRECTORIES "${runtime_directory}"
    RESOLVED_DEPENDENCIES_VAR resolved
    UNRESOLVED_DEPENDENCIES_VAR unresolved
    PRE_EXCLUDE_REGEXES "^[Aa][Pp][Ii]-" "^[Ee][Xx][Tt]-"
    POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*")
if(unresolved)
    message(FATAL_ERROR "Unresolved package dependencies: ${unresolved}")
endif()
list(APPEND resolved ${ROOT_LIBRARIES})
list(REMOVE_DUPLICATES resolved)
list(SORT resolved)
foreach(dependency IN LISTS resolved)
    file(COPY "${dependency}" DESTINATION "${OUTPUT_DIRECTORY}")
endforeach()
foreach(plugin IN LISTS ROOT_LIBRARIES)
    if(NOT EXISTS "${plugin}")
        message(FATAL_ERROR "Missing explicit runtime plugin: ${plugin}")
    endif()
    file(COPY "${plugin}" DESTINATION "${OUTPUT_DIRECTORY}")
endforeach()
file(WRITE "${OUTPUT_DIRECTORY}/runtime-dependencies.txt" "${resolved}\n")
