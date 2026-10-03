project( HoudiniSceneCreator )

if (WIN32)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/WPEngine/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
elseif (APPLE)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/WPEngine/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(Tests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx)
    link_directories(Tests ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic)
elseif (UNIX)
endif (WIN32)

# Add a library with two source files.
set( library_name HoudiniSceneCreator )
add_executable( ${library_name} 
    createscene.cpp
)
# Link against the Houdini libraries, and add required include directories and compile definitions.
target_link_libraries( ${library_name} Houdini )
# Configure several common target properties, such as its output directory.
houdini_configure_target( ${library_name} )

if (WIN32)
    add_custom_command(TARGET  ${library_name} POST_BUILD
        COMMAND COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${library_name}> ${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/Debug
    )
endif ()


set_target_properties (HoudiniSceneCreator PROPERTIES FOLDER Tools/Cpp)