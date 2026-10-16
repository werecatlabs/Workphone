# Decoder sources from meshoptimizer v0.22 (MIT); see LICENSE.md.
if(NOT TARGET Meshopt)
    add_library(Meshopt STATIC allocator.cpp indexcodec.cpp vertexcodec.cpp vertexfilter.cpp)
    target_include_directories(Meshopt PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    target_compile_features(Meshopt PUBLIC cxx_std_11)
    set_target_properties(Meshopt PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(Meshopt PRIVATE /Y-)
        # Workphone's legacy PCH helper adds /Yu outside CMake's PCH machinery.
        # Per-source flags come last and keep this third-party decoder independent.
        set_source_files_properties(allocator.cpp indexcodec.cpp vertexcodec.cpp vertexfilter.cpp
            PROPERTIES COMPILE_FLAGS "/Y-" SKIP_PRECOMPILE_HEADERS ON)
    endif()
endif()
