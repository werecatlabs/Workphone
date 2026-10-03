#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <fstream>
#include <algorithm>  // for std::for_each
#include <cassert>    // for assert
#include <cstddef>    // for std::size_t
#include <iostream>   // for std::cout
#include <ostream>    // for std::endl
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <system_error>

#if defined WP_PLATFORM_WIN32
#    include <direct.h>  // for _chdir
#    include <io.h>      // for _access
#    include <windows.h>
#elif defined WP_PLATFORM_APPLE
#    include <Workphone/Core/OSX/macUtils.hpp>
#endif

#if defined WP_PLATFORM_WIN32
#    include <filesystem>
#elif defined WP_PLATFORM_APPLE
#    include <filesystem>
#elif defined WP_PLATFORM_LINUX
#    include <experimental/filesystem>
#endif

namespace workphone
{

    namespace
    {
        // Thread-local error_code lvalue so std::filesystem directory iterators can use
        // the non-throwing error_code overloads. This avoids filesystem_error exceptions
        // escaping noexcept/destructor contexts (which would terminate the process) when
        // iterating paths that do not exist or cannot be accessed.
        inline std::error_code &_wpFsEc()
        {
            thread_local std::error_code ec;
            ec.clear();
            return ec;
        }
    }  // namespace

    template <>
    const BaseString<c8> BasePath<c8>::separatorBaseString = "/";

    template <>
    const BaseString<wchar_t> BasePath<wchar_t>::separatorBaseString = L"/";

    // Forward declarations for explicit template specializations
    template <>
    auto BasePath<c8>::getFolders( const String &path, bool recursive ) -> Array<String>;

    template <>
    auto BasePath<wchar_t>::getFolders( const StringW &path, bool recursive ) -> Array<StringW>;

    template <>
    auto BasePath<c8>::getFiles( const String &path, const String &extension ) -> Array<String>;

    template <>
    auto BasePath<wchar_t>::getFiles( const StringW &path, const StringW &extension ) -> Array<StringW>;

    template <>
    auto BasePath<c8>::getFilePath( const String &filePath ) -> String
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Find the last path separator ('/' or '\') and return the
        // substring before it. Behavior mirrors std::filesystem::path::remove_filename
        // for common cases: if no separator is found, return an empty string.
        auto pos = filePath.find_last_of( "/\\" );
        if( pos == String::npos )
        {
            return {};
        }

        // If the separator is at position 0, return single separator to represent root
        if( pos == 0 )
        {
            return BasePath<c8>::separatorBaseString;
        }

        return filePath.substr( 0, pos );
    }

    template <>
    auto BasePath<wchar_t>::getFilePath( const StringW &filePath ) -> StringW
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Find the last path separator (L'/' or L'\\') and return the
        // substring before it. If no separator is found, return empty string.
        auto pos = filePath.find_last_of( L"/\\" );
        if( pos == StringW::npos )
        {
            return {};
        }

        // If the separator is at position 0, return single separator to represent root
        if( pos == 0 )
        {
            return BasePath<wchar_t>::separatorBaseString;
        }

        return filePath.substr( 0, pos );
    }

    template <>
    auto BasePath<c8>::getFileName( const String &filePath ) -> String
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Trim any trailing separators, then return the substring after the
        // last remaining separator. If the path consists only of separators,
        // return the separator string to represent root. If there are no
        // separators, return the whole input.
        if( filePath.empty() )
        {
            return {};
        }

        // Find last character that is not a separator
        auto endPos = filePath.find_last_not_of( "/\\" );
        if( endPos == String::npos )
        {
            // path is all separators (e.g. "/" or "\\\\") -> return separator
            return BasePath<c8>::separatorBaseString;
        }

        // Find last separator before endPos
        auto sepPos = filePath.find_last_of( "/\\", endPos );
        if( sepPos == String::npos )
        {
            // No separator found -> entire trimmed string is filename
            return filePath.substr( 0, endPos + 1 );
        }

        // Return substring after separator up to endPos (inclusive)
        return filePath.substr( sepPos + 1, endPos - sepPos );
    }

    template <>
    auto BasePath<wchar_t>::getFileName( const StringW &filePath ) -> StringW
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Trim any trailing separators, then return the substring after the
        // last remaining separator. If the path consists only of separators,
        // return the separator string to represent root. If there are no
        // separators, return the whole input.
        if( filePath.empty() )
        {
            return {};
        }

        // Find last character that is not a separator
        auto endPos = filePath.find_last_not_of( L"/\\" );
        if( endPos == StringW::npos )
        {
            // path is all separators (e.g. L"/" or L"\\\\") -> return separator
            return BasePath<wchar_t>::separatorBaseString;
        }

        // Find last separator before endPos
        auto sepPos = filePath.find_last_of( L"/\\", endPos );
        if( sepPos == StringW::npos )
        {
            // No separator found -> entire trimmed string is filename
            return filePath.substr( 0, endPos + 1 );
        }

        // Return substring after separator up to endPos (inclusive)
        return filePath.substr( sepPos + 1, endPos - sepPos );
    }

    template <>
    auto BasePath<c8>::getFileExtension( const String &filePath ) -> String
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Extract filename (trim trailing separators), then return substring
        // from last '.' in the filename (including the dot). Matches
        // std::filesystem::path::extension semantics for common cases.
        if( filePath.empty() )
        {
            return {};
        }

        // Trim trailing separators
        auto endPos = filePath.find_last_not_of( "/\\" );
        if( endPos == String::npos )
        {
            return {};
        }

        // Find start of filename (after last separator)
        auto sepPos = filePath.find_last_of( "/\\", endPos );
        String filename;
        if( sepPos == String::npos )
        {
            filename = filePath.substr( 0, endPos + 1 );
        }
        else
        {
            filename = filePath.substr( sepPos + 1, endPos - sepPos );
        }

        // Special cases: "." and ".." have no extension
        if( filename == "." || filename == ".." )
        {
            return {};
        }

        auto dotPos = filename.find_last_of( '.' );
        if( dotPos == String::npos )
        {
            return {};
        }

        return filename.substr( dotPos );
    }

    template <>
    auto BasePath<wchar_t>::getFileExtension( const StringW &filePath ) -> StringW
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Extract filename (trim trailing separators), then return substring
        // from last L'.' in the filename (including the dot). Matches
        // std::filesystem::path::extension semantics for common cases.
        if( filePath.empty() )
        {
            return {};
        }

        // Trim trailing separators
        auto endPos = filePath.find_last_not_of( L"/\\" );
        if( endPos == StringW::npos )
        {
            return {};
        }

        // Find start of filename (after last separator)
        auto sepPos = filePath.find_last_of( L"/\\", endPos );
        StringW filename;
        if( sepPos == StringW::npos )
        {
            filename = filePath.substr( 0, endPos + 1 );
        }
        else
        {
            filename = filePath.substr( sepPos + 1, endPos - sepPos );
        }

        // Special cases: L"." and L".." have no extension
        if( filename == L"." || filename == L".." )
        {
            return {};
        }

        auto dotPos = filename.find_last_of( L'.' );
        if( dotPos == StringW::npos )
        {
            return {};
        }

        return filename.substr( dotPos );
    }

    template <>
    auto BasePath<c8>::getFileNameWithoutExtension( const String &filePath ) -> String
    {
        // last extension (substring from last '.') if present.
        if( filePath.empty() )
        {
            return {};
        }

        // Trim trailing separators
        auto endPos = filePath.find_last_not_of( "/\\" );
        if( endPos == String::npos )
        {
            // path is all separators -> return separator
            return BasePath<c8>::separatorBaseString;
        }

        // Find start of filename (after last separator)
        auto sepPos = filePath.find_last_of( "/\\", endPos );
        String filename;
        if( sepPos == String::npos )
        {
            filename = filePath.substr( 0, endPos + 1 );
        }
        else
        {
            filename = filePath.substr( sepPos + 1, endPos - sepPos );
        }

        // Special cases: "." and ".." -> return as-is
        if( filename == "." || filename == ".." )
        {
            return filename;
        }

        auto dotPos = filename.find_last_of( '.' );
        if( dotPos == String::npos )
        {
            return filename;
        }

        return filename.substr( 0, dotPos );
    }

    template <>
    auto BasePath<wchar_t>::getFileNameWithoutExtension( const StringW &filePath ) -> StringW
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Extract filename (trim trailing separators), then remove the
        // last extension (substring from last L'.') if present.
        if( filePath.empty() )
        {
            return {};
        }

        // Trim trailing separators
        auto endPos = filePath.find_last_not_of( L"/\\" );
        if( endPos == StringW::npos )
        {
            // path is all separators -> return separator
            return BasePath<wchar_t>::separatorBaseString;
        }

        // Find start of filename (after last separator)
        auto sepPos = filePath.find_last_of( L"/\\", endPos );
        StringW filename;
        if( sepPos == StringW::npos )
        {
            filename = filePath.substr( 0, endPos + 1 );
        }
        else
        {
            filename = filePath.substr( sepPos + 1, endPos - sepPos );
        }

        // Special cases: L"." and L".." -> return as-is
        if( filename == L"." || filename == L".." )
        {
            return filename;
        }

        auto dotPos = filename.find_last_of( L'.' );
        if( dotPos == StringW::npos )
        {
            return filename;
        }

        return filename.substr( 0, dotPos );
    }

    template <>
    auto BasePath<c8>::getFilePathWithoutExtension( const String &filePath ) -> String
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Compose: <directory path> '/' <filename without extension>
        // Use existing helpers which are already filesystem-free.
        String dir = getFilePath( filePath );
        String nameNoExt = getFileNameWithoutExtension( filePath );

        if( dir.empty() )
        {
            return nameNoExt;
        }

        // If dir equals the root separator, don't duplicate separators
        if( dir == BasePath<c8>::separatorBaseString )
        {
            return dir + nameNoExt;
        }

        return dir + '/' + nameNoExt;
    }

    template <>
    auto BasePath<wchar_t>::getFilePathWithoutExtension( const StringW &filePath ) -> StringW
    {
        // Manual implementation to avoid dependency on std::filesystem.
        // Compose: <directory path> '/' <filename without extension>
        // Use existing helpers which are already filesystem-free.
        StringW dir = getFilePath( filePath );
        StringW nameNoExt = getFileNameWithoutExtension( filePath );

        if( dir.empty() )
        {
            return nameNoExt;
        }

        // If dir equals the root separator, don't duplicate separators
        if( dir == BasePath<wchar_t>::separatorBaseString )
        {
            return dir + nameNoExt;
        }

        return dir + L'/' + nameNoExt;
    }

    template <>
    void BasePath<c8>::setWorkingDirectory( const BaseString<c8> &dir )
    {
#if defined WP_PLATFORM_WIN32
        if( _chdir( dir.c_str() ) != 0 )
        {
            WP_LOG_ERROR( "Failed to change working directory to " + dir );
        }
#elif defined WP_PLATFORM_APPLE
        std::filesystem::path path( dir.str() );
        current_path( path );
#elif defined WP_PLATFORM_LINUX
        if( chdir( dir.c_str() ) != 0 )
        {
            WP_LOG_ERROR( "Failed to change working directory to " + dir );
        }
#endif
    }

    template <>
    void BasePath<wchar_t>::setWorkingDirectory( const BaseString<wchar_t> &dir )
    {
#if defined WP_PLATFORM_WIN32
        if( _wchdir( dir.c_str() ) != 0 )
        {
            WP_LOG_ERROR( L"Failed to change working directory to " + dir );
        }
#elif defined WP_PLATFORM_APPLE
        std::filesystem::path path( dir.str() );
        current_path( path );
#elif defined WP_PLATFORM_LINUX
        if( chdir( dir.c_str() ) != 0 )
        {
            WP_LOG_ERROR( L"Failed to change working directory to " + dir );
        }
#endif
    }

    template <>
    auto BasePath<c8>::getWorkingDirectory() -> String
    {
#if defined WP_PLATFORM_WIN32
        char buffer[MAX_PATH];
        if( _getcwd( buffer, MAX_PATH ) != nullptr )
        {
            return StringUtil::cleanupPath( buffer );
        }

        WP_LOG_ERROR( "Failed to get current working directory" );
        return {};
#elif defined WP_PLATFORM_APPLE
        char buffer[PATH_MAX];
#elif defined WP_PLATFORM_LINUX
        auto str = std::filesystem::current_path().string();
        return StringUtil::cleanupPath( str );
#endif
    }

    template <>
    auto BasePath<wchar_t>::getWorkingDirectory() -> StringW
    {
#if defined WP_PLATFORM_WIN32
        wchar_t buffer[MAX_PATH];
        if( _wgetcwd( buffer, MAX_PATH ) != nullptr )
        {
            return StringUtilW::cleanupPath( buffer );
        }

        WP_LOG_ERROR( "Failed to get current working directory" );
        return {};
#elif defined WP_PLATFORM_APPLE
        auto str = std::filesystem::current_path().wstring();
        return StringUtilW::cleanupPath( str );
#elif defined WP_PLATFORM_LINUX
        wchar_t buffer[PATH_MAX];
        if( getcwd( buffer, PATH_MAX ) != nullptr )
        {
            return StringUtilW::cleanupPath( buffer );
        }
        WP_LOG_ERROR( "Failed to get current working directory" );
        return {};
#endif
    }

    template <>
    auto BasePath<c8>::getFolders( const String &path, bool recursive ) -> Array<String>
    {
        Array<String> files;

        try
        {
            std::filesystem::path dir( StringUtil::str( path ) );

            if( recursive )
            {
                for( const auto &entry :
                     std::filesystem::recursive_directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( StringUtil::cleanupPath( entry.path().u8string() ) );
                    }
                }
            }
            else
            {
                for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( StringUtil::cleanupPath( entry.path().u8string() ) );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<wchar_t>::getFolders( const StringW &path, bool recursive ) -> Array<StringW>
    {
        Array<StringW> files;

        try
        {
            std::filesystem::path dir( StringUtilW::str( path ) );

            if( recursive )
            {
                for( const auto &entry :
                     std::filesystem::recursive_directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( entry.path().wstring() );
                    }
                }
            }
            else
            {
                for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( entry.path().wstring() );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<c8>::getPaths( const String &path, bool recursive ) -> Array<String>
    {
        Array<String> files;
        try
        {
            std::filesystem::path dir = StringUtil::str( path );

            if( recursive )
            {
                for( const auto &entry :
                     std::filesystem::recursive_directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( entry.path().u8string() );
                    }
                }
            }
            else
            {
                for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( entry.path().u8string() );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<wchar_t>::getPaths( const StringW &path, bool recursive ) -> Array<StringW>
    {
        Array<StringW> files;
        try
        {
            std::filesystem::path dir = StringUtilW::str( path );

            if( recursive )
            {
                for( const auto &entry :
                     std::filesystem::recursive_directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( entry.path().wstring() );
                    }
                }
            }
            else
            {
                for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
                {
                    if( std::filesystem::is_directory( entry ) )
                    {
                        files.push_back( entry.path().wstring() );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<c8>::getFiles( const String &path ) -> Array<String>
    {
        try
        {
            Array<String> files;
            files.reserve( 4096 );

            std::filesystem::path dir( StringUtil::str( path ) );
            for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
            {
                if( !std::filesystem::is_directory( entry ) )
                {
                    try
                    {
                        auto entryPath = entry.path();
                        auto entryPathStr = entryPath.u8string();
                        auto fileName = StringUtil::cleanupPath( entryPathStr );
                        files.push_back( fileName );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }

            return files;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<wchar_t>::getFiles( const StringW &path ) -> Array<StringW>
    {
        try
        {
            Array<StringW> files;
            files.reserve( 8 );

            std::filesystem::path dir = StringUtilW::str( path );
            for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
            {
                if( !std::filesystem::is_directory( entry ) )
                {
                    auto filePath = StringUtilW::cleanupPath( entry.path().wstring() );
                    files.push_back( filePath );
                }
            }

            return files;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<wchar_t>::getFilesAsAbsolutePaths( const StringW &path, bool recursive )
        -> Array<StringW>
    {
        Array<StringW> files;
        files.reserve( 8 );

        try
        {
            std::filesystem::path dir = StringUtilW::str( path );

            if( !std::filesystem::exists( dir, _wpFsEc() ) ||
                !std::filesystem::is_directory( dir, _wpFsEc() ) )
            {
                return files;
            }

            if( recursive )
            {
                for( const auto &entry :
                     std::filesystem::recursive_directory_iterator( dir, _wpFsEc() ) )
                {
                    if( !std::filesystem::is_directory( entry ) )
                    {
                        auto fileName = entry.path().wstring();
                        files.push_back( fileName );
                    }
                }
            }
            else
            {
                for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
                {
                    if( !std::filesystem::is_directory( entry ) )
                    {
                        auto fileName = entry.path().wstring();
                        files.push_back( fileName );
                    }
                }
            }
        }
        catch( const std::filesystem::filesystem_error &ex )
        {
            WP_LOG_ERROR( ex.what() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }
    template <>
    auto BasePath<c8>::getFiles( const String &path, const String &extension ) -> Array<String>
    {
        Array<String> fileList;

#if defined WP_PLATFORM_WIN32
        if( ( GetFileAttributesA( path.c_str() ) & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
        {
            WIN32_FIND_DATA fileData;
            auto handle = INVALID_HANDLE_VALUE;

            String searchPath = path + "/*";
            handle = FindFirstFileA( searchPath.c_str(), &fileData );

            if( handle != INVALID_HANDLE_VALUE )
            {
                while( FindNextFileA( handle, &fileData ) )
                {
                    if( strcmp( fileData.cFileName, ".." ) != 0 )
                    {
                        String filePath = path + String( fileData.cFileName );
                        if( ( fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
                        {
                            String fileName( fileData.cFileName );
                            if( getFileExtension( fileName ) == extension )
                            {
                                fileList.push_back( fileName );
                            }
                        }
                    }
                }

                FindClose( handle );
            }
        }
#endif

        return fileList;
    }

    template <>
    auto BasePath<c8>::getFilesAsAbsolutePaths( const String &path, bool recursive ) -> Array<String>
    {
        Array<String> files;
        try
        {
            files.reserve( 8 );

            std::filesystem::path dir = StringUtil::str( path );
            for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
            {
                if( !std::filesystem::is_directory( entry ) )
                {
                    auto fileName = entry.path().u8string();
                    files.push_back( fileName );
                }
                else
                {
                    if( recursive )
                    {
                        auto subFolderPath = entry.path().u8string();
                        auto subFolderFiles = getFilesAsAbsolutePaths( subFolderPath, recursive );
                        files.insert( files.end(), subFolderFiles.begin(), subFolderFiles.end() );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<wchar_t>::getFiles( const StringW &path, const StringW &extension ) -> Array<StringW>
    {
        Array<StringW> fileList;
        fileList.reserve( 8 );

        try
        {
            std::filesystem::path dir = StringUtilW::str( path );

            if( !std::filesystem::exists( dir, _wpFsEc() ) ||
                !std::filesystem::is_directory( dir, _wpFsEc() ) )
            {
                return fileList;
            }

            for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
            {
                if( !std::filesystem::is_directory( entry ) )
                {
                    auto fileName = entry.path().filename().wstring();
                    if( getFileExtension( fileName ) == extension )
                    {
                        fileList.push_back( fileName );
                    }
                }
            }
        }
        catch( const std::filesystem::filesystem_error &ex )
        {
            WP_LOG_ERROR( ex.what() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return fileList;
    }

    template <>
    auto BasePath<c8>::getFilesAsAbsolutePaths( const String &path, const String &extension,
                                                bool recursive ) -> Array<String>
    {
        Array<String> files;
        try
        {
            files.reserve( 8 );

            std::filesystem::path dir = StringUtil::str( path );
            for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
            {
                if( !std::filesystem::is_directory( entry ) )
                {
                    auto fileName = entry.path().u8string();
                    if( getFileExtension( fileName ) == extension )
                    {
                        files.push_back( fileName );
                    }
                }
                else
                {
                    if( recursive )
                    {
                        auto subFolderPath = entry.path().u8string();
                        auto subFolderFiles =
                            getFilesAsAbsolutePaths( subFolderPath, extension, recursive );
                        for( auto &file : subFolderFiles )
                        {
                            if( getFileExtension( file ) == extension )
                            {
                                files.push_back( file );
                            }
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<wchar_t>::getFilesAsAbsolutePaths( const StringW &path, const StringW &extension,
                                                     bool recursive ) -> Array<StringW>
    {
        Array<StringW> files;
        try
        {
            files.reserve( 8 );

            std::filesystem::path dir = StringUtilW::str( path );
            for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
            {
                if( !std::filesystem::is_directory( entry ) )
                {
                    auto fileName = entry.path().wstring();
                    if( getFileExtension( fileName ) == extension )
                    {
                        files.push_back( fileName );
                    }
                }
                else
                {
                    if( recursive )
                    {
                        auto subFolderPath = entry.path().wstring();
                        auto subFolderFiles =
                            getFilesAsAbsolutePaths( subFolderPath, extension, recursive );

                        for( auto &file : subFolderFiles )
                        {
                            if( getFileExtension( file ) == extension )
                            {
                                files.push_back( file );
                            }
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return files;
    }

    template <>
    auto BasePath<c8>::getLeaf( const String &filePath ) -> String
    {
        std::filesystem::path path( StringUtil::str( filePath ) );
        auto fileName = path.filename();
        return fileName.string();
    }

    template <>
    auto BasePath<wchar_t>::getLeaf( const StringW &filePath ) -> StringW
    {
        std::filesystem::path path( StringUtilW::str( filePath ) );
        auto fileName = path.filename();
        return fileName.wstring();
    }

    template <class T>
    auto BasePath<T>::isFolder( const BaseString<T> &path ) -> bool
    {
        std::filesystem::path p( StringUtility<T>::str( path ) );
        return std::filesystem::is_directory( p );
    }

    template <class T>
    auto BasePath<T>::isFile( const BaseString<T> &filePath ) -> bool
    {
        std::filesystem::path path( StringUtility<T>::str( filePath ) );
        return std::filesystem::is_regular_file( path );
    }

    template <class T>
    auto BasePath<T>::isExistingFolder( const BaseString<T> &path ) -> bool
    {
        std::filesystem::path p( StringUtility<T>::str( path ) );
        return std::filesystem::exists( p );
    }

    template <class T>
    auto BasePath<T>::isExistingFile( const BaseString<T> &filePath ) -> bool
    {
        std::filesystem::path p( StringUtility<T>::str( filePath ) );
        return std::filesystem::exists( p );
    }

    template <>
    auto BasePath<c8>::getFilePathFolders( [[maybe_unused]] const String &filePath ) -> Array<String>
    {
        Array<String> folders;
        std::filesystem::path path( StringUtil::str( filePath ) );
        while( !path.empty() )
        {
            auto fileName = path.filename();
            auto folder = fileName.string();
            if( !folder.empty() )
                folders.push_back( folder );
            path = path.parent_path();
        }
        std::reverse( folders.begin(), folders.end() );
        return folders;
    }

    template <>
    auto BasePath<wchar_t>::getFilePathFolders( [[maybe_unused]] const StringW &filePath )
        -> Array<StringW>
    {
        Array<StringW> folders;
        std::filesystem::path path( StringUtilW::str( filePath ) );
        while( !path.empty() )
        {
            auto fileName = path.filename();
            auto folder = fileName.wstring();
            if( !folder.empty() )
                folders.push_back( folder );
            path = path.parent_path();
        }
        std::reverse( folders.begin(), folders.end() );
        return folders;
    }

    template <class T>
    auto BasePath<T>::isFilesEqual( const BaseString<T> &lFilePath, const BaseString<T> &rFilePath )
        -> bool
    {
#if defined WP_PLATFORM_WIN32
        std::ifstream lFile( lFilePath, std::ios::in | std::ios::binary );
        std::ifstream rFile( rFilePath, std::ios::in | std::ios::binary );

        if( !lFile.good() || !rFile.good() )
        {
            return false;
        }

        std::streamsize lReadBytesCount = 0;
        std::streamsize rReadBytesCount = 0;

        const s32 bufferSize = 1048576 * 10;
        auto p_lBuffer = new char[bufferSize];
        auto p_rBuffer = new char[bufferSize];

        do
        {
            lFile.read( p_lBuffer, bufferSize );
            rFile.read( p_rBuffer, bufferSize );
            lReadBytesCount = lFile.gcount();
            rReadBytesCount = rFile.gcount();

            if( lReadBytesCount != rReadBytesCount ||
                std::memcmp( p_lBuffer, p_rBuffer, static_cast<size_t>( lReadBytesCount ) ) != 0 )
            {
                return false;
            }
        } while( lFile.good() || rFile.good() );

        delete[] p_lBuffer;
        delete[] p_rBuffer;

        return true;
#else
        return false;
#endif
    }

    template <class T>
    auto BasePath<T>::getFileSize( const BaseString<T> &filename ) -> size_t
    {
        std::ifstream in( filename.c_str(), std::ios::in | std::ios::binary );
        in.seekg( 0, std::ifstream::end );
        return (size_t)in.tellg();
    }

    template <>
    void BasePath<c8>::copyFolder( const String &srcPath, const String &dstPath )
    {
        std::filesystem::path sourcePath( StringUtil::str( srcPath ) );
        std::filesystem::path destinationPath( StringUtil::str( dstPath ) );

        try
        {
            // Check if the source path exists
            if( std::filesystem::exists( sourcePath ) )
            {
                // Check if the source is a directory
                if( std::filesystem::is_directory( sourcePath ) )
                {
                    // Create the destination directory if it doesn't exist
                    if( !std::filesystem::exists( destinationPath ) )
                    {
                        std::filesystem::create_directories( destinationPath );
                    }

                    // Recursively copy the contents of the source directory to the destination
                    auto flags = std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing;
                    std::filesystem::copy( sourcePath, destinationPath, flags );
                }
                else
                {
                    // Source is not a directory
                    std::cerr << "Source is not a directory." << std::endl;
                }
            }
            else
            {
                // Source path doesn't exist
                std::cerr << "Source path does not exist." << std::endl;
            }
        }
        catch( const std::filesystem::filesystem_error &ex )
        {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
    }

    template <>
    void BasePath<wchar_t>::copyFolder( const StringW &srcPath, const StringW &dstPath )
    {
        std::filesystem::path sourcePath( StringUtilW::str( srcPath ) );
        std::filesystem::path destinationPath( StringUtilW::str( dstPath ) );

        try
        {
            // Check if the source path exists
            if( std::filesystem::exists( sourcePath ) )
            {
                // Check if the source is a directory
                if( std::filesystem::is_directory( sourcePath ) )
                {
                    // Create the destination directory if it doesn't exist
                    if( !std::filesystem::exists( destinationPath ) )
                    {
                        std::filesystem::create_directories( destinationPath );
                    }

                    // Recursively copy the contents of the source directory to the destination
                    auto flags = std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing;
                    std::filesystem::copy( sourcePath, destinationPath, flags );
                }
                else
                {
                    // Source is not a directory
                    std::cerr << "Source is not a directory." << std::endl;
                }
            }
            else
            {
                // Source path doesn't exist
                std::cerr << "Source path does not exist." << std::endl;
            }
        }
        catch( const std::filesystem::filesystem_error &ex )
        {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
    }

    template <>
    void BasePath<c8>::copyFile( const String &srcPath, const String &dstPath )
    {
        auto newPath = getFilePath( dstPath );
        auto cleanDst = StringUtility<c8>::cleanupPath( newPath );

        auto bSuccess = true;
        std::error_code ec;

        auto retries = 0;
        std::filesystem::path dir( StringUtil::str( cleanDst ) );
        while( !std::filesystem::is_directory( dir, _wpFsEc() ) && retries++ < 100 )
        {
            bSuccess = false;

            if( std::filesystem::create_directories( StringUtil::str( cleanDst ), ec ) )
            {
                bSuccess = true;
                break;
            }

            Thread::yield();
        }

        if( !bSuccess && ec.value() != 0 )
        {
            auto message = "Cannot create directory." + StringUtil::str( cleanDst ) +
                           std::string( " " ) + ec.message();
            throw Exception( message );
        }

#if 0
		std::ifstream  src(srcPath, std::ios::binary);
		std::ofstream  dst(cleanDst, std::ios::trunc | std::ios::binary);

		dst << src.rdbuf();
#else
        std::filesystem::copy_file( StringUtil::str( srcPath ), StringUtil::str( dstPath ),
                                    std::filesystem::copy_options::overwrite_existing );
#endif
    }

    template <>
    void BasePath<wchar_t>::copyFile( const StringW &srcPath, const StringW &dstPath )
    {
        StringW newPath = getFilePath( dstPath );
        StringW cleanDst = StringUtility<wchar_t>::replaceAll( newPath, L"\\", L"/" );
        cleanDst = StringUtility<wchar_t>::replaceAll( cleanDst, L"//", L"/" );

        bool bSuccess = true;
        std::error_code ec;

        int retries = 0;
        std::filesystem::path dir( StringUtilW::str( cleanDst ) );
        while( !std::filesystem::is_directory( dir, _wpFsEc() ) && retries++ < 100 )
        {
            bSuccess = false;

            if( std::filesystem::create_directories( dir, ec ) )
            {
                bSuccess = true;
                break;
            }

            // Thread::sleep(10.0);
        }

        if( !bSuccess && ec.value() != 0 )
        {
            auto message = String( "Cannot create directory." );
            throw Exception( message );
        }

#if 0
		std::ifstream  src(srcPath, std::ios::binary);
		std::ofstream  dst(cleanDst, std::ios::trunc | std::ios::binary);

		dst << src.rdbuf();
#else
        std::filesystem::copy_file( StringUtilW::str( srcPath ), StringUtilW::str( dstPath ),
                                    std::filesystem::copy_options::overwrite_existing );
#endif
    }

    template <>
    void BasePath<c8>::createDirectories( const String &dst )
    {
        try
        {
            auto cleanDst = StringUtility<c8>::cleanupPath( dst );

            auto bSuccess = true;
            std::error_code ec;

            auto retries = 0;
            std::filesystem::path dir( StringUtil::str( cleanDst ) );
            while( !std::filesystem::is_directory( dir, _wpFsEc() ) && retries++ < 100 )
            {
                bSuccess = false;

                if( std::filesystem::create_directories( dir, ec ) )
                {
                    bSuccess = true;
                    break;
                }

                Thread::yield();
            }

            if( !bSuccess && ec.value() != 0 )
            {
                auto message =
                    "Cannot create directory." + StringUtil::str( cleanDst ) + " " + ec.message();
                throw Exception( message );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    template <>
    void BasePath<wchar_t>::createDirectories( const StringW &dst )
    {
        try
        {
            auto cleanDst = StringUtility<wchar_t>::cleanupPath( dst );

            auto bSuccess = true;
            std::error_code ec;

            auto retries = 0;
            std::filesystem::path dir( StringUtilW::str( cleanDst ) );
            while( !std::filesystem::is_directory( dir, _wpFsEc() ) && retries++ < 100 )
            {
                bSuccess = false;

                if( std::filesystem::create_directories( dir, ec ) )
                {
                    bSuccess = true;
                    break;
                }

                Thread::yield();
            }

            if( !bSuccess && ec.value() != 0 )
            {
                auto message = StringW( L"Cannot create directory." ) + cleanDst + L" " +
                               StringUtil::toUTF8to16( ec.message() );
                throw Exception( StringUtil::toUTF16to8( message ) );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    template <>
    auto BasePath<c8>::getMD5( [[maybe_unused]] const String &filePath ) -> String
    {
#if WP_USE_OPENSSL
        if( !BasePath<c8>::isExistingFile( filePath ) )
        {
            return String( "" );
        }

        unsigned char c[MD5_DIGEST_LENGTH + 1];
        memset( c, 0, sizeof( c ) );

        const char *filename = filePath;
        int i;
        FILE *inFile = fopen( filename, "rb" );
        MD5_CTX mdContext;
        int bytes;

        const int bufferSize = 1048576 * 10;
        unsigned char *data = new unsigned char[bufferSize];

        if( inFile == NULL )
        {
            delete[] data;
            return String( "" );
        }

        MD5_Init( &mdContext );
        while( ( bytes = fread( data, 1, bufferSize, inFile ) ) != 0 )
            MD5_Update( &mdContext, data, bytes );

        MD5_Final( c, &mdContext );

        c[MD5_DIGEST_LENGTH] = 0;
        String retVal = (char *)c;

        fclose( inFile );

        delete[] data;
        return retVal;
#else
        if( isExistingFile( filePath ) )
        {
            std::fstream stream( StringUtil::str( filePath ), std::fstream::in );
            stream.seekg( 0, std::ios_base::end );
            auto size = static_cast<size_t>( stream.tellg() );
            stream.seekg( 0, std::ios_base::beg );

            auto data = new unsigned char[size];
            stream.read( reinterpret_cast<char *>( data ), size );

            return getMD5( data, size );
        }

        return String( "" );
#endif
    }

    template <>
    auto BasePath<wchar_t>::getMD5( [[maybe_unused]] const StringW &filePath ) -> StringW
    {
#if WP_USE_OPENSSL
        if( !BasePath<wchar_t>::isExistingFile( filePath ) )
        {
            return String( "" );
        }

        std::fstream stream( filePath, std::fstream::in | std::fstream::binary );
        if( !stream.is_open() )
        {
            throw Exception( "Cannot open file." );
        }

        const int bufferSize = 1048576 * 10;
        unsigned char *data = new unsigned char[bufferSize];
        size_t bytesRead = 0;
        size_t totalBytesRead = 0;

        unsigned char c[MD5_DIGEST_LENGTH + 1];
        memset( c, 0, sizeof( c ) );

        MD5_CTX mdContext;

        MD5_Init( &mdContext );

        while( !stream.eof() )
        {
            stream.read( (char *)data, bufferSize );
            bytesRead = (size_t)stream.gcount();
            totalBytesRead += bytesRead;

            MD5_Update( &mdContext, data, bytesRead );
        }

        MD5_Final( c, &mdContext );

        c[MD5_DIGEST_LENGTH] = 0;
        String retVal = (char *)c;

        delete[] data;
        return retVal;
#else
        if( isExistingFile( filePath ) )
        {
#    if defined WP_PLATFORM_WIN32
            std::fstream stream( StringUtilW::str( filePath ), std::fstream::in );
#    else
            std::fstream stream( StringUtil::toUTF16to8( filePath ), std::fstream::in );
#    endif

            stream.seekg( 0, std::ios_base::end );
            auto size = static_cast<size_t>( stream.tellg() );
            stream.seekg( 0, std::ios_base::beg );

            auto data = new unsigned char[size];
            stream.read( reinterpret_cast<char *>( data ), size );

            return getMD5( data, size );
        }

        return {};
#endif
    }

    template <class T>
    auto BasePath<T>::getMD5( [[maybe_unused]] void *data, [[maybe_unused]] size_t size )
        -> BaseString<T>
    {
#if WP_USE_OPENSSL
        unsigned char c[MD5_DIGEST_LENGTH + 1];
        memset( c, 0, sizeof( c ) );

        MD5_CTX mdContext;

        MD5_Init( &mdContext );
        MD5_Update( &mdContext, data, size );
        MD5_Final( c, &mdContext );

        c[MD5_DIGEST_LENGTH] = 0;
        String retVal = (char *)c;

        return retVal;
#else
        // Fallback: produce a deterministic hex string using std::hash when OpenSSL
        // is not available. This is not MD5 but provides a stable hashed string.
        const auto ptr = reinterpret_cast<const unsigned char *>( data );
        std::size_t h = 0;
        // Hash the data using std::hash on string_view (stable for given implementation)
        {
            std::string_view sv( reinterpret_cast<const char *>( ptr ), size );
            h = std::hash<std::string_view>{}( sv );
        }

        std::ostringstream oss;
        oss << std::hex << std::setfill( '0' ) << std::setw( sizeof( std::size_t ) * 2 ) << h;
        auto hex = oss.str();

        std::basic_string<T> result;
        result.reserve( hex.size() );
        if constexpr( std::is_same_v<T, char> )
        {
            result.assign( hex, hex.size() );
        }
        else
        {
            for( char c : hex )
                result.push_back( static_cast<T>( c ) );
        }

        return result;
#endif
    }

    template <>
    void BasePath<c8>::deleteFiles( [[maybe_unused]] const String &path )
    {
        auto files = getFiles( path );
        for( auto &fileName : files )
        {
            auto filePath = path + String( "/" ) + fileName;

            std::filesystem::path dir( StringUtil::str( filePath ) );
            std::filesystem::remove( dir );
        }

        auto folders = getFolders( path, false );
        for( auto &folder : folders )
        {
            try
            {
                auto folderPath = path + "/" + folder + "/";
                // deleteFiles(folderPath);

                std::filesystem::path dir( StringUtil::str( folderPath ) );
                if( std::filesystem::is_directory( dir, _wpFsEc() ) )
                {
                    std::filesystem::remove_all( dir );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_ERROR( e.what() );
            }
        }
    }

    template <class T>
    void BasePath<T>::deleteFolder( const BaseString<T> &filePath )
    {
        try
        {
            std::filesystem::path path( StringUtility<T>::str( filePath ) );
            if( std::filesystem::exists( path ) )
            {
                std::filesystem::remove( path );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( e.what() );
        }
    }

    template <class T>
    void BasePath<T>::deleteFile( const BaseString<T> &filePath )
    {
        try
        {
            std::filesystem::path path( StringUtility<T>::str( filePath ) );
            if( std::filesystem::exists( path ) )
            {
                std::filesystem::remove( path );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( e.what() );
        }
    }

    template <>
    void BasePath<wchar_t>::deleteFiles( [[maybe_unused]] const StringW &path )
    {
        auto files = getFiles( path );
        for( auto fileName : files )
        {
            StringW filePath = path + StringW( L"/" ) + fileName;

            std::filesystem::path dir( StringUtilW::str( filePath ) );
            std::filesystem::remove( dir );
        }

        auto folders = getFolders( path, false );
        for( auto folder : folders )
        {
            try
            {
                StringW folderPath = path + StringW( L"/" ) + folder + StringW( L"/" );
                // deleteFiles(folderPath);

                std::filesystem::path dir( StringUtilW::str( folderPath ) );
                if( std::filesystem::is_directory( dir, _wpFsEc() ) )
                {
                    std::filesystem::remove_all( dir );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    template <>
    auto BasePath<c8>::getAbsolutePath( const String &path ) -> String
    {
        try
        {
            std::filesystem::path p( StringUtil::str( path ) );
            std::error_code ec;

            std::filesystem::path result;

            // If the path exists, try to get its canonical form. If it doesn't exist
            // fall back to absolute (which doesn't resolve symlinks).
            if( std::filesystem::exists( p, ec ) && !ec )
            {
                result = std::filesystem::canonical( p, ec );
                if( ec )
                {
                    // canonical failed (e.g., insufficient permissions), fall back
                    result = std::filesystem::absolute( p, ec );
                }
            }
            else
            {
                result = std::filesystem::absolute( p, ec );
            }

            result = result.lexically_normal();
            auto absolutePath = result.string();
            return StringUtil::cleanupPath( absolutePath );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<wchar_t>::getAbsolutePath( const StringW &path ) -> StringW
    {
        try
        {
            std::filesystem::path p( StringUtilW::str( path ) );
            std::error_code ec;

            std::filesystem::path result;

            if( std::filesystem::exists( p, ec ) && !ec )
            {
                result = std::filesystem::canonical( p, ec );
                if( ec )
                {
                    result = std::filesystem::absolute( p, ec );
                }
            }
            else
            {
                result = std::filesystem::absolute( p, ec );
            }

            result = result.lexically_normal();
            auto absolutePath = StringW( result.wstring() );
            return StringUtilW::cleanupPath( absolutePath );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<c8>::getAbsolutePath( const String &path, const String &relativePath ) -> String
    {
        try
        {
            if( StringUtil::isNullOrEmpty( relativePath ) )
            {
                return {};
            }

            std::filesystem::path basePath( StringUtil::str( path ) );
            std::filesystem::path relPath( StringUtil::str( relativePath ) );

            // Combine base and relative then resolve to canonical/absolute
            auto combined = basePath / relPath;
            std::error_code ec;
            std::filesystem::path result;

            if( std::filesystem::exists( combined, ec ) && !ec )
            {
                result = std::filesystem::canonical( combined, ec );
                if( ec )
                {
                    result = std::filesystem::absolute( combined, ec );
                }
            }
            else
            {
                result = std::filesystem::absolute( combined, ec );
            }

            result = result.lexically_normal();
            auto absolutePath = static_cast<String>( result.string() );
            return StringUtil::cleanupPath( absolutePath );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<wchar_t>::getAbsolutePath( const StringW &path, const StringW &relativePath )
        -> StringW
    {
        try
        {
            if( StringUtilW::isNullOrEmpty( relativePath ) )
            {
                return {};
            }

            std::filesystem::path basePath( StringUtilW::str( path ) );
            std::filesystem::path relPath( StringUtilW::str( relativePath ) );

            auto combined = basePath / relPath;
            std::error_code ec;
            std::filesystem::path result;

            if( std::filesystem::exists( combined, ec ) && !ec )
            {
                result = std::filesystem::canonical( combined, ec );
                if( ec )
                {
                    result = std::filesystem::absolute( combined, ec );
                }
            }
            else
            {
                result = std::filesystem::absolute( combined, ec );
            }

            result = result.lexically_normal();
            auto absolutePath = StringW( result.wstring() );
            return StringUtilW::cleanupPath( absolutePath );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<c8>::lexically_normal( const String &path ) -> String
    {
        auto p = std::filesystem::path( StringUtil::str( path ) );
        auto l = p.lexically_normal();
        return StringUtil::cleanupPath( l.string() );
    }

    template <>
    auto BasePath<wchar_t>::lexically_normal( const StringW &path ) -> StringW
    {
        auto p = std::filesystem::path( StringUtilW::str( path ) );
        auto l = p.lexically_normal();
        return StringUtilW::cleanupPath( l.wstring() );
    }

    template <>
    auto BasePath<c8>::lexically_normal( const String &path, const String &relativePath ) -> String
    {
        auto p = std::filesystem::path( StringUtil::str( path + String( "/" ) + relativePath ) );
        auto l = p.lexically_normal();
        return StringUtil::cleanupPath( l.string() );
    }

    template <>
    auto BasePath<wchar_t>::lexically_normal( const StringW &path, const StringW &relativePath )
        -> StringW
    {
        auto p = std::filesystem::path( StringUtilW::str( path + L"/" + relativePath ) );
        auto l = p.lexically_normal();
        return StringUtilW::cleanupPath( l.wstring() );
    }

    template <>
    auto BasePath<c8>::lexically_relative( const String &path, const String &relativePath ) -> String
    {
        auto p = std::filesystem::path( StringUtil::str( path ) );
        auto r = std::filesystem::path( StringUtil::str( relativePath ) );
        auto l = r.lexically_relative( p );
        return StringUtil::cleanupPath( l.string() );
    }

    template <>
    auto BasePath<wchar_t>::lexically_relative( const StringW &path, const StringW &relativePath )
        -> StringW
    {
        auto p = std::filesystem::path( StringUtilW::str( path ) );
        auto r = std::filesystem::path( StringUtilW::str( relativePath ) );
        auto l = r.lexically_relative( p );
        return StringUtilW::cleanupPath( l.wstring() );
    }

    template <>
    auto BasePath<c8>::lexically_proximate( const String &path, const String &relativePath ) -> String
    {
        auto p = std::filesystem::path( StringUtil::str( path ) );
        auto r = std::filesystem::path( StringUtil::str( relativePath ) );
        auto l = p.lexically_proximate( r );
        return StringUtil::cleanupPath( l.string() );
    }

    template <>
    auto BasePath<wchar_t>::lexically_proximate( const StringW &path, const StringW &relativePath )
        -> StringW
    {
        auto p = std::filesystem::path( StringUtilW::str( path ) );
        auto r = std::filesystem::path( StringUtilW::str( relativePath ) );
        auto l = p.lexically_proximate( r );
        return StringUtilW::cleanupPath( l.wstring() );
    }

    template <class T>
    void _append( std::filesystem::path &p, std::filesystem::path::iterator begin,
                  std::filesystem::path::iterator end )
    {
        for( ; begin != end; ++begin )
        {
            p /= *begin;
        }
    }

    // Return path when appended to a_From will resolve to same as a_To
    auto make_relative( std::filesystem::path a_From, std::filesystem::path a_To )
        -> std::filesystem::path
    {
        a_From = absolute( a_From );
        a_To = absolute( a_To );
        std::filesystem::path ret;
        auto itrFrom( a_From.begin() ), itrTo( a_To.begin() );
        // Find common base
        for( auto toEnd( a_To.end() ), fromEnd( a_From.end() );
             itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo )
        {
            ;
        }
        // Navigate backwards in directory to reach previously found base
        for( auto fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
        {
            if( ( *itrFrom ) != "." )
            {
                ret /= "..";
            }
        }
        // Now navigate down the directory branch
        _append<std::filesystem::path::iterator>( ret, itrTo, a_To.end() );
        return ret;
    }

    template <>
    auto BasePath<c8>::getRelativePath( const String &path, const String &relativePath ) -> String
    {
        try
        {
            std::filesystem::path basePath( StringUtil::str( path ) );
            std::filesystem::path relPath( StringUtil::str( relativePath ) );

            auto sRelativePath = make_relative( basePath, relPath ).string();
            return StringUtil::cleanupPath( sRelativePath );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    auto BasePath<wchar_t>::getRelativePath( const StringW &path, const StringW &relativePath )
        -> StringW
    {
        try
        {
            std::filesystem::path basePath( StringUtilW::str( path ) );
            std::filesystem::path relPath( StringUtilW::str( relativePath ) );

            auto sRelativePath = make_relative( basePath, relPath ).wstring();
            return StringUtilW::cleanupPath( sRelativePath );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <class T>
    void BasePath<T>::rename( const BaseString<T> &pathOld, const BaseString<T> &pathNew )
    {
        try
        {
            std::filesystem::path pathOldObject( StringUtility<T>::str( pathOld ) );
            std::filesystem::path pathNewObject( StringUtility<T>::str( pathNew ) );
            std::filesystem::rename( pathOldObject, pathNewObject );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    template <>
    auto BasePath<c8>::macBundlePath() -> String
    {
#if defined WP_PLATFORM_APPLE
        return workphone::macBundlePath();
#else
        return {};
#endif
    }

    template <>
    auto BasePath<wchar_t>::macBundlePath() -> StringW
    {
#if defined WP_PLATFORM_APPLE
        auto str = workphone::macBundlePath();
        return StringUtil::toUTF8to16( str );
#else
        return {};
#endif
    }

    template <class T>
    auto BasePath<T>::isPathAbsolute( const BaseString<T> &basePath ) -> bool
    {
        if( basePath.empty() )
        {
            return false;
        }

        if( basePath[0] == static_cast<T>( '/' ) || basePath[0] == static_cast<T>( '\\' ) )
        {
            return true;
        }

        return basePath.size() > 2 && basePath[1] == static_cast<T>( ':' ) &&
               ( basePath[2] == static_cast<T>( '/' ) || basePath[2] == static_cast<T>( '\\' ) );
    }

    template <class T>
    auto BasePath<T>::isPathRelative( [[maybe_unused]] const BaseString<T> &basePath,
                                      [[maybe_unused]] const BaseString<T> &relativePath ) -> bool
    {
        std::filesystem::path path( StringUtility<T>::str( relativePath ) );
        return path.is_relative();
    }

    template <class T>
    auto BasePath<T>::hasFileName( const BaseString<T> &path ) -> bool
    {
        if( path.empty() )
        {
            return false;
        }

        const auto last = path.back();
        return last != static_cast<T>( '/' ) && last != static_cast<T>( '\\' );
    }

    template <>
    auto BasePath<c8>::readAllText( const String &path ) -> String
    {
        std::fstream fs;
        fs.open( StringUtil::str( path ), std::fstream::in );

        const size_t bufSize = 4096;
        char pBuf[bufSize];

        String result;
        result.reserve( bufSize );

        size_t nr = 1;
        while( !fs.eof() && nr > 0 )
        {
            fs.read( pBuf, bufSize );
            nr = static_cast<size_t>( fs.gcount() );

            result.append( pBuf, nr );
        }

        fs.close();

        return result;
    }

    template <>
    void BasePath<c8>::writeAllText( const String &path, const String &contents )
    {
        std::fstream fs;
        fs.open( StringUtil::str( path ), std::fstream::out );

        fs << contents;

        fs.close();
    }

    template <>
    auto BasePath<wchar_t>::readAllText( const StringW &path ) -> StringW
    {
        std::wfstream fs;

#if defined WP_PLATFORM_WIN32
        fs.open( StringUtilW::str( path ), std::wfstream::in );
#else
        std::string sPath = StringUtil::toUTF16to8( path );
        fs.open( sPath, std::wfstream::in );
#endif

        const size_t bufSize = 4096;
        wchar_t pBuf[bufSize];

        StringW result;
        result.reserve( bufSize );

        size_t nr = 1;
        while( !fs.eof() && nr > 0 )
        {
            fs.read( pBuf, bufSize );
            nr = static_cast<size_t>( fs.gcount() );

            result.append( pBuf, nr );
        }

        fs.close();

        return result;
    }

    template <>
    void BasePath<wchar_t>::writeAllText( const StringW &path, const StringW &contents )
    {
        std::wfstream fs;

#if defined WP_PLATFORM_WIN32
        fs.open( StringUtilW::str( path ), std::wfstream::out );
#else
        std::string sPath = StringUtil::toUTF16to8( path );
        fs.open( sPath, std::wfstream::out );
#endif

        fs << contents;

        fs.close();
    }

    template <>
    bool BasePath<c8>::endsWith( const String &path, const String &ending )
    {
        if( path.length() >= ending.length() )
        {
            return ( 0 == path.compare( path.length() - ending.length(), ending.length(), ending ) );
        }

        return false;
    }

    template <>
    bool BasePath<wchar_t>::endsWith( const StringW &path, const StringW &ending )
    {
        if( path.length() >= ending.length() )
        {
            return ( 0 == path.compare( path.length() - ending.length(), ending.length(), ending ) );
        }

        return false;
    }

    template <>
    auto BasePath<c8>::getParentPath( const String &path ) -> String
    {
        std::filesystem::path p( StringUtil::str( path ) );
        if( p.has_parent_path() )
        {
            return StringUtil::cleanupPath( p.parent_path().string() );
        }

        return {};
    }

    template <>
    auto BasePath<wchar_t>::getParentPath( const StringW &path ) -> StringW
    {
        std::filesystem::path p( StringUtilW::str( path ) );
        if( p.has_parent_path() )
        {
            return StringUtilW::cleanupPath( p.parent_path().wstring() );
        }

        return {};
    }

    // explicit instantiation for common character types
    template class BasePath<c8>;
    template class BasePath<wchar_t>;
}  // namespace workphone
