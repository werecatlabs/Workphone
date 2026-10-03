#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ProjectManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Core/Path.hpp>

namespace workphone
{
    ProjectManager::ProjectManager()
    {
        m_projectName = "Plugin";
        m_enginePath = "Engine";
    }

    ProjectManager::~ProjectManager() = default;

    void ProjectManager::generateProject()
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto fileSystem = applicationManager->getFileSystemPtr();

        Array<String> defines;
        Array<String> includes;

        Array<String> libraryPaths;
        Array<String> macosLibraryPaths;
        Array<String> unixLibraryPaths;

        Array<String> libs;
        Array<String> macosLibs;
        Array<String> unixLibs;

        const auto capacity = 256;

        defines.reserve( capacity );
        includes.reserve( capacity );

        libraryPaths.reserve( capacity );
        macosLibraryPaths.reserve( capacity );
        unixLibraryPaths.reserve( capacity );

        libs.reserve( capacity );
        macosLibs.reserve( capacity );
        unixLibs.reserve( capacity );

#ifdef _WP_STATIC_LIB_
        defines.emplace_back( "_WP_STATIC_LIB_" );
#endif

        defines.emplace_back( "BOOST_ALL_NO_LIB" );
        defines.emplace_back( "__TBB_NO_IMPLICIT_LINKAGE=1" );
        defines.emplace_back( "__TBBMALLOC_NO_IMPLICIT_LINKAGE=1" );
        defines.emplace_back( "_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS" );
        defines.emplace_back( "_UNICODE" );
        defines.emplace_back( "WP_USE_BOOST" );

#if WP_USE_ONETBB
        defines.push_back( "WP_USE_ONETBB=1" );
#elif WP_USE_TBB
        defines.emplace_back( "WP_USE_TBB=1" );
#endif

        auto enginePath = getEnginePath();
        auto proprietaryLibraryPath = Path::lexically_normal( enginePath, "../lioncat_proprietary" );

        includes.push_back( enginePath + "/Dependencies/boost" );

#if WP_USE_ONETBB
        includes.push_back( enginePath + "/Dependencies/onetbb/include" );
#elif WP_USE_TBB
        includes.push_back( enginePath + "/Dependencies/tbb/include" );
#endif

        //includes.push_back( enginePath + "/Engine/cpp/Include/FBCPlusPlus" );
        //includes.push_back( enginePath + "/AdditionalLibraries/cppsqlite/include" );
        //includes.push_back( enginePath + "/AdditionalLibraries/sqlite/include" );
        // includes.push_back(enginePath + "/AdditionalLibraries/aurora");
        includes.push_back( enginePath + "/Engine/cpp/Include" );
        includes.emplace_back( "${WP_ROOT}/Plugin" );
        //includes.push_back( enginePath + "/FBEngine/cpp/Include/FBCPlusPlus" );

        auto additoinalIncludes = getIncludeFolders();
        includes.insert( includes.end(), additoinalIncludes.begin(), additoinalIncludes.end() );

        auto additionalLibraries = getLibraries();

        libs.emplace_back( "Workphone" );

#if _WP_STATIC_LIB_
        libs.emplace_back( "bcrypt" );
        libs.emplace_back( "libboost_thread" );
        libs.emplace_back( "libboost_date_time" );
        libs.emplace_back( "libboost_chrono" );
        libs.emplace_back( "libboost_timer" );
        libs.emplace_back( "libboost_filesystem" );
        libs.emplace_back( "libboost_json" );
        libs.emplace_back( "minizip" );
        libs.emplace_back( "nativefiledialog" );
        libs.emplace_back( "rttr" );
        libs.emplace_back( "tinyxml" );
        libs.emplace_back( "yaml-cpp" );
        libs.emplace_back( "zlib" );
        libs.emplace_back( "zzip" );
#endif

#if WP_USE_TBB
#    if WP_TBB_BUILD_STATIC
        libs.emplace_back( "tbb_static" );
        libs.emplace_back( "tbbmalloc_static" );
#    else
        libs.emplace_back( "tbb" );
        libs.emplace_back( "tbbmalloc" );
#    endif
#endif

        for( auto lib : additionalLibraries )
        {
            libs.push_back( lib );
        }

        macosLibs.emplace_back( "Workphone" );

#if _WP_STATIC_LIB_
        macosLibs.emplace_back( "libboost_thread" );
        macosLibs.emplace_back( "libboost_date_time" );
        macosLibs.emplace_back( "libboost_chrono" );
        macosLibs.emplace_back( "libboost_timer" );
        macosLibs.emplace_back( "libboost_filesystem" );
        macosLibs.emplace_back( "libboost_json" );
        macosLibs.emplace_back( "minizip" );
        macosLibs.emplace_back( "nativefiledialog" );
        macosLibs.emplace_back( "rttr" );
        macosLibs.emplace_back( "tinyxml" );
        macosLibs.emplace_back( "yaml-cpp" );
        macosLibs.emplace_back( "zlib" );
        macosLibs.emplace_back( "zzip" );
#endif

#if WP_USE_TBB
#    if WP_TBB_BUILD_STATIC
        macosLibs.emplace_back( "tbb_static" );
        macosLibs.emplace_back( "tbbmalloc_static" );
#    else
        macosLibs.emplace_back( "tbb" );
        macosLibs.emplace_back( "tbbmalloc" );
#    endif
#endif

        for( auto lib : additionalLibraries )
        {
            macosLibs.push_back( lib );
        }

        unixLibs.emplace_back( "Workphone" );

#if _WP_STATIC_LIB_
        unixLibs.emplace_back( "libboost_thread" );
        unixLibs.emplace_back( "libboost_date_time" );
        unixLibs.emplace_back( "libboost_chrono" );
        unixLibs.emplace_back( "libboost_timer" );
        unixLibs.emplace_back( "libboost_filesystem" );
        unixLibs.emplace_back( "libboost_json" );
        unixLibs.emplace_back( "minizip" );
        unixLibs.emplace_back( "nativefiledialog" );
        unixLibs.emplace_back( "rttr" );
        unixLibs.emplace_back( "tinyxml" );
        unixLibs.emplace_back( "yaml-cpp" );
        unixLibs.emplace_back( "zlib" );
        unixLibs.emplace_back( "zzip" );
#endif

#if WP_USE_TBB
#    if WP_TBB_BUILD_STATIC
        unixLibs.emplace_back( "tbb_static" );
        unixLibs.emplace_back( "tbbmalloc_static" );
#    else
        unixLibs.emplace_back( "tbb" );
        unixLibs.emplace_back( "tbbmalloc" );
#    endif
#endif

        for( auto lib : additionalLibraries )
        {
            unixLibs.push_back( lib );
        }

        auto toolset = String( "${WP_TOOLSET}" );
        auto arch = String( "${WP_ARCH}" );
        auto cmakeSourceStr = String( "${CMAKE_SOURCE_DIR}" );

#if WP_STATIC_LINK_CRT
        auto crtType = String( "MT" );
#else
        auto crtType = String( "MD" );
#endif

        libraryPaths.push_back( enginePath + "/libs/windows/" + toolset + "/" + arch + "/" + crtType +
                                "/${CMAKE_BUILD_TYPE}" );

        auto additionalLibraryPaths = getLibraryFolders();
        //libraryPaths.insert( libraryPaths.end(), additionalLibraryPaths.begin(),
        //                     additionalLibraryPaths.end() );
        for( auto lib : additionalLibraryPaths )
        {
            libraryPaths.push_back( lib + "/windows/" + toolset + "/" + arch + "/" + crtType +
                                    "/${CMAKE_BUILD_TYPE}" );
        }

        macosLibraryPaths.push_back( enginePath + "/libs/macos/${CMAKE_BUILD_TYPE}" );
        macosLibraryPaths.push_back( proprietaryLibraryPath + "/libs/macos/${CMAKE_BUILD_TYPE}" );

        unixLibraryPaths.push_back( enginePath + "/libs/unix/${CMAKE_BUILD_TYPE}" );
        unixLibraryPaths.push_back( proprietaryLibraryPath + "/libs/unix/${CMAKE_BUILD_TYPE}" );

        /*
        libraryPaths.push_back( enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch +
                                "/" + crtType + "/physx/" );
        libraryPaths.push_back( enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch +
                                "/" + crtType + "/ogre/" );
        libraryPaths.push_back( enginePath + "/Dependencies/lib/windows/" + toolset + "/" + arch + "/" +
                                crtType + "/tbb/" );
        libraryPaths.push_back( enginePath + "/Dependencies/lib/windows/" + toolset + "/" + arch + "/" +
                                crtType + "/boost/" );
        libraryPaths.push_back( enginePath + "/Dependencies/lib/windows/" + toolset + "/" + arch + "/" +
                                crtType + "/tinybind/" );
        libraryPaths.push_back( enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch +
                                "/" + crtType + "/wildmagic/" );
        libraryPaths.push_back( enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch +
                                "/" + crtType + "/assimp/${CMAKE_BUILD_TYPE}" );
        libraryPaths.push_back( enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch +
                                "/" + crtType + "/ois/" );
        // libraryPaths.push_back(enginePath + "/AdditionalLibraries/aurora/x64/${CMAKE_BUILD_TYPE}");
        */

        const auto windowsLibraryExtendsionStr = String( ".lib" );
        const auto unixLibraryExtendsionStr = String( ".a" );
        const auto tabStr = String( " " );
        const auto newLineStr = String( "\n" );
        auto cmakeRequired = String( "cmake_minimum_required(VERSION 3.13)\n" );
        auto projectName = getProjectName();
        auto projectDeclare = "project(" + projectName + " C CXX)\n";

        auto compileOptions =
            String( "if (WIN32)" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /fp:fast\")" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /Qvec-report:2\")" ) + newLineStr +
            String( "" ) + newLineStr + String( "	if (NOT WP_PLATFORM_X64)" ) + newLineStr +
            String( "		set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /arch:SSE2\")" ) + newLineStr +
            String( "	endif()" ) + newLineStr + String( "" ) + newLineStr +

#if WP_STATIC_LINK_CRT
            String( "	set(CMAKE_CXX_FLAGS_DEBUG \"/D_DEBUG /MP /MTd /Zi /Ob0 /Od\")" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_MINSIZEREL \"/MT /MP /Zi /O2 /Ob2 /D NDEBUG\")" ) +
            newLineStr + String( "	set(CMAKE_CXX_FLAGS_RELEASE \"/MT /MP /O2 /Ob2 /Zi /D NDEBUG\")" ) +
            newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"/MT /MP /Zi /Od /Ob0 /D NDEBUG\")" ) +
            newLineStr +
#else
            String( "	set(CMAKE_CXX_FLAGS_DEBUG \"/D_DEBUG /MP /MDd /Zi /Ob0 /Od\")" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_MINSIZEREL \"/MD /MP /Zi /O2 /Ob2 /D NDEBUG\")" ) +
            newLineStr + String( "	set(CMAKE_CXX_FLAGS_RELEASE \"/MD /MP /O2 /Ob2 /Zi /D NDEBUG\")" ) +
            newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"/MD /MP /Zi /Od /Ob0 /D NDEBUG\")" ) +
            newLineStr +
#endif
            String( "elseif(APPLE)" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -msse -msse2 -msse3\")" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_DEBUG \"-g\")" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_MINSIZEREL \"-O3 -finline-functions -DNDEBUG\")" ) +
            newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_RELEASE \"-O3 -finline-functions -DNDEBUG\")" ) +
            newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"-O3 -g -finline-functions -DNDEBUG\")" ) +
            newLineStr + String( "else()" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -msse -msse2 -msse3\")" ) + newLineStr +
            String(
                "	#set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -Wno-everything\")" ) +
            newLineStr + String( "	set(CMAKE_CXX_FLAGS_DEBUG \"-g\")" ) + newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_MINSIZEREL \"-O3 -finline-functions -DNDEBUG\")" ) +
            newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_RELEASE \"-O3 -finline-functions -DNDEBUG\")" ) +
            newLineStr +
            String( "	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"-O3 -g -finline-functions -DNDEBUG\")" ) +
            newLineStr + String( "" ) + newLineStr + String( "	add_compile_definitions(NDEBUG)" ) +
            newLineStr + String( "endif()" );

        auto headerFilesStr =
            String( "file(GLOB_RECURSE HEADER_FILES \"Plugin/*.h\" \"Plugin/*.hpp\")\n" );
        auto sourceFilesStr = String( "file(GLOB_RECURSE SOURCE_FILES \"Plugin/*.cpp\")\n" );

        auto defineOpen = String( "add_compile_definitions(" );
        auto defineClose = String( ")" );

        auto includeDirOpen = String( "include_directories(" );
        auto includeDirClose = String( ")" );

        auto libraryDirOpen = String( "link_directories(" );
        auto libraryDirClose = String( ")" );

        auto libraryOpen = String( "target_link_libraries(" );
        auto libraryClose = String( ")" );

        auto cmakeStr = cmakeRequired + projectDeclare;
        cmakeStr += newLineStr;
        cmakeStr += "set(CMAKE_CXX_STANDARD 17)";
        cmakeStr += newLineStr;
        cmakeStr += "set(WP_ROOT \"${CMAKE_SOURCE_DIR}\")";
        cmakeStr += newLineStr + newLineStr;

        cmakeStr +=
            "if (WIN32)\n"
            "\tset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "
            "${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/"
            "${CMAKE_BUILD_TYPE})\n"
            "\tset(CMAKE_LIBRARY_OUTPUT_DIRECTORY "
            "${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/"
            "${CMAKE_BUILD_TYPE})\n"
            "\tset(CMAKE_RUNTIME_OUTPUT_DIRECTORY "
            "${WP_ROOT}/bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/"
            "${CMAKE_BUILD_TYPE})\n"
            "elseif (APPLE)\n"
            "    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/macOS/${CMAKE_BUILD_TYPE})\n"
            "    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/macOS/${CMAKE_BUILD_TYPE})\n"
            "    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/macOS/${CMAKE_BUILD_TYPE})\n"
            "elseif (UNIX)\n"
            "    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/linux/${CMAKE_BUILD_TYPE})\n"
            "    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/linux/${CMAKE_BUILD_TYPE})\n"
            "    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/linux/${CMAKE_BUILD_TYPE})\n"
            "endif ()";
        cmakeStr += newLineStr + newLineStr;

        cmakeStr += getPlatformOptions();
        cmakeStr += newLineStr + newLineStr;

        cmakeStr += getToolsetOptions();
        cmakeStr += newLineStr + newLineStr;

        cmakeStr += "set(WP_STATIC_LINK_CRT_TYPE \"" + crtType + "\")" + newLineStr;
        cmakeStr += newLineStr;

        cmakeStr += compileOptions + newLineStr;

        cmakeStr += newLineStr;

        cmakeStr +=
            "if (WP_PLATFORM_X64)\n"
            "\tadd_compile_definitions(_AMD64_)\n"
            "else()\n"
            "\tadd_compile_definitions(_X86_)\n"
            "endif()";

        cmakeStr += newLineStr + newLineStr;

        for( auto define : defines )
        {
            cmakeStr += defineOpen + define + defineClose + newLineStr;
        }

        cmakeStr += newLineStr + headerFilesStr + sourceFilesStr + newLineStr;

        for( auto includeDir : includes )
        {
            cmakeStr += includeDirOpen + includeDir + includeDirClose + newLineStr;
        }

        cmakeStr += newLineStr;

        std::stringstream linkDirectories;
        linkDirectories << String( "if (WIN32)\n" );

        for( auto libraryPath : libraryPaths )
        {
            linkDirectories << libraryDirOpen + projectName + " " + libraryPath + libraryDirClose +
                                   newLineStr;
        }

        linkDirectories << String( "elseif(APPLE)\n" );

        for( auto libraryPath : macosLibraryPaths )
        {
            linkDirectories << libraryDirOpen + projectName + " " + libraryPath + libraryDirClose +
                                   newLineStr;
        }

        linkDirectories << String( "elseif(UNIX)\n" );

        for( auto libraryPath : unixLibraryPaths )
        {
            linkDirectories << libraryDirOpen + projectName + " " + libraryPath + libraryDirClose +
                                   newLineStr;
        }

        linkDirectories << String( "endif()\n" );
        cmakeStr += linkDirectories.str();

        cmakeStr += newLineStr;

        auto filterStr =
            "set(WP_HEADER_REL_PATH \"${WP_ROOT}/Plugin\")\n"
            "set(WP_SOURCE_REL_PATH \"${WP_ROOT}/Plugin\")\n"
            "\n"
            "foreach(_source IN ITEMS ${HEADER_FILES})\n"
            "    get_filename_component(_source_file_path \"${_source}\" DIRECTORY)\n"
            "    string(REPLACE \"${WP_HEADER_REL_PATH}\" \"\" _group_file_path "
            "\"${_source_file_path}\")\n"
            "    string(REPLACE \"\\\\\" \"/\" _group_file_path \"${_group_file_path}\")\n"
            "    source_group(\"Header Files\\\\${_group_file_path}\" FILES \"${_source}\")\n"
            "endforeach()\n"
            "\n"
            "foreach(_source IN ITEMS ${SOURCE_FILES})\n"
            "    get_filename_component(_source_file_path \"${_source}\" DIRECTORY)\n"
            "    string(REPLACE \"${WP_SOURCE_REL_PATH}\" \"\" _group_file_path "
            "\"${_source_file_path}\")\n"
            "    string(REPLACE \"/\" \"\\\\\" _group_file_path \"${_group_file_path}\")\n"
            "    source_group(\"Source Files\\\\${_group_file_path}\" FILES \"${_source}\")\n"
            "endforeach()";

        cmakeStr += filterStr;
        cmakeStr += newLineStr + newLineStr;

        cmakeStr +=
            "file( GLOB_RECURSE LUA_GAME_SOURCES \"" + cmakeSourceStr + "/Assets/*.lua\" )" + newLineStr;
        cmakeStr += "source_group( \"Scripts\" FILES ${LUA_GAME_SOURCES} )" + newLineStr;
        cmakeStr += newLineStr;

        auto libraryType = String( "" );

        if( isSharedLibrary() )
        {
            libraryType = String( "SHARED" );
        }

        auto libraryDeclare =
            static_cast<String>( "add_library(" + projectName + " " + libraryType +
                                 " ${HEADER_FILES} ${SOURCE_FILES} ${LUA_GAME_SOURCES})\n" );
        cmakeStr += libraryDeclare + newLineStr;

        auto libraries = String( "if (WIN32)\n" );
        for( auto lib : libs )
        {
            libraries += tabStr + libraryOpen + projectName + " " + lib + windowsLibraryExtendsionStr +
                         libraryClose + newLineStr;
        }

        libraries += String( "elseif(APPLE)\n" );
        for( auto lib : macosLibs )
        {
            libraries += tabStr + libraryOpen + projectName + " " + lib + unixLibraryExtendsionStr +
                         libraryClose + newLineStr;
        }

        libraries += "target_link_libraries(" + projectName +
                     " stdc++ \"-framework Metal\" \"-framework OpenGL\" \"-framework Foundation\" "
                     "\"-framework QuartzCore\" \"-framework CoreGraphics\" \"-framework CoreServices\" "
                     "\"-framework Cocoa\" \"-framework Carbon\" \"-framework IOKit\" objc)" +
                     newLineStr;
        libraries += "set_target_properties(" + projectName +
                     " PROPERTIES LINK_FLAGS \"-Wl,-F/Library/Frameworks\")" + newLineStr;

        libraries += String( "elseif(UNIX)\n" );
        for( auto lib : unixLibs )
        {
            libraries += tabStr + libraryOpen + projectName + " " + lib + unixLibraryExtendsionStr +
                         libraryClose + newLineStr;
        }

        libraries += String( "endif()\n" );

        cmakeStr += libraries;
        cmakeStr += newLineStr;

        auto projectPath = applicationManager->getProjectPath();
        auto cmakeListsPath = projectPath + "/CMakeLists.txt";

        fileSystem->writeAllText( cmakeListsPath, cmakeStr );
    }

    auto ProjectManager::isSharedLibrary() const -> bool
    {
        return m_isSharedLibrary;
    }

    void ProjectManager::setSharedLibrary( bool sharedLibrary )
    {
        m_isSharedLibrary = sharedLibrary;
    }

    auto ProjectManager::getProjectName() const -> String
    {
        ScopedLock lock( this );
        return m_projectName;
    }

    void ProjectManager::setProjectName( const String &projectName )
    {
        ScopedLock lock( this );
        m_projectName = projectName;
    }

    auto ProjectManager::getEnginePath() const -> String
    {
        ScopedLock lock( this );
        return m_enginePath;
    }

    void ProjectManager::setEnginePath( const String &enginePath )
    {
        ScopedLock lock( this );
        m_enginePath = enginePath;
    }

    void ProjectManager::addIncludeFolder( const String &includeFolder )
    {
        ScopedLock lock( this );
        m_includeFolders.push_back( includeFolder );
    }

    void ProjectManager::removeIncludeFolder( const String &includeFolder )
    {
        ScopedLock lock( this );
        m_includeFolders.erase(
            std::remove( m_includeFolders.begin(), m_includeFolders.end(), includeFolder ),
            m_includeFolders.end() );
    }

    void ProjectManager::addLibraryFolder( const String &libraryFolder )
    {
        ScopedLock lock( this );
        m_libraryFolders.push_back( libraryFolder );
    }

    void ProjectManager::removeLibraryFolder( const String &libraryFolder )
    {
        ScopedLock lock( this );
        m_libraryFolders.erase(
            std::remove( m_libraryFolders.begin(), m_libraryFolders.end(), libraryFolder ),
            m_libraryFolders.end() );
    }

    auto ProjectManager::getIncludeFolders() const -> Array<String>
    {
        ScopedLock lock( this );
        return m_includeFolders;
    }

    void ProjectManager::setIncludeFolders( const Array<String> &includeFolders )
    {
        ScopedLock lock( this );
        m_includeFolders = includeFolders;
    }

    auto ProjectManager::getLibraryFolders() const -> Array<String>
    {
        ScopedLock lock( this );
        return m_libraryFolders;
    }

    void ProjectManager::setLibraryFolders( const Array<String> &libraryFolders )
    {
        ScopedLock lock( this );
        m_libraryFolders = libraryFolders;
    }

    void ProjectManager::addLibrary( const String &library )
    {
        ScopedLock lock( this );
        m_libraries.push_back( library );
    }

    void ProjectManager::removeLibrary( const String &library )
    {
        ScopedLock lock( this );
        m_libraries.erase( std::remove( m_libraries.begin(), m_libraries.end(), library ),
                           m_libraries.end() );
    }

    auto ProjectManager::getLibraries() const -> Array<String>
    {
        ScopedLock lock( this );
        return m_libraries;
    }

    void ProjectManager::setLibraries( const Array<String> &libraries )
    {
        ScopedLock lock( this );
        m_libraries = libraries;
    }

    void ProjectManager::clearLibraries()
    {
        ScopedLock lock( this );
        m_libraries.clear();
    }

    void ProjectManager::lock()
    {
        m_mutex.lock();
    }

    bool ProjectManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void ProjectManager::unlock()
    {
        m_mutex.unlock();
    }

    auto ProjectManager::getPlatformOptions() const -> String
    {
        return "include(CheckTypeSize)\n"
               "CHECK_TYPE_SIZE(\"void*\" WP_PTR_SIZE BUILTIN_TYPES_ONLY)\n"
               "\n"
               "if (WP_PTR_SIZE EQUAL 8)\n"
               "  set(WP_PLATFORM_X64 TRUE)\n"
               "  set(WP_ARCH \"x64\")\n"
               "else ()\n"
               "  set(WP_PLATFORM_X64 FALSE)\n"
               "  set(WP_ARCH \"x86\")\n"
               "endif ()";
    }

    auto ProjectManager::getToolsetOptions() const -> String
    {
        return "if(WIN32)\n"
               "\tif (MSVC_TOOLSET_VERSION EQUAL \"141\")\n"
               "\t\tset(WP_TOOLSET \"v141\")\n"
               "\telseif(MSVC_TOOLSET_VERSION EQUAL \"142\")\n"
               "\t\tset(WP_TOOLSET \"v142\")\n"
               "    elseif(MSVC_TOOLSET_VERSION EQUAL \"143\")\n"
               "\t\tset(WP_TOOLSET \"v143\")\n"
               "\tendif()\n"
               "elseif(APPLE)\n"
               "    set(WP_TOOLSET \"\")\n"
               "else()\n"
               "    set(WP_TOOLSET \"\")\n"
               "endif()";
    }
}  // namespace workphone
