#pragma once

#include <Workphone/Core/StringTypes.hpp>
#include <algorithm>
#include <filesystem>
#include <string>

namespace workphone
{
    // Resource mounts include caches and test fixtures. Those files remain
    // explicitly loadable, but must not execute merely because a project opens.
    inline bool shouldAutoLoadProjectScript( const String &scriptPath, const String &projectRoot )
    {
        if( scriptPath.empty() || projectRoot.empty() )
        {
            return true;
        }

        namespace fs = std::filesystem;
        auto portablePath = []( const String &value ) {
            std::string path( value.c_str() );
            std::replace( path.begin(), path.end(), '\\', '/' );
            return fs::u8path( path );
        };
        std::error_code error;
        const auto rootPath = fs::absolute( portablePath( projectRoot ), error ).lexically_normal();
        if( error )
        {
            return true;
        }
        auto path = portablePath( scriptPath );
        if( !path.is_absolute() )
        {
            path = rootPath / path;
        }
        auto root = rootPath.generic_u8string();
        auto candidate = path.lexically_normal().generic_u8string();
#ifdef _WIN32
        auto lower = []( std::string &value ) {
            for( auto &character : value )
            {
                if( character >= 'A' && character <= 'Z' )
                {
                    character = static_cast<char>( character + ( 'a' - 'A' ) );
                }
            }
        };
        lower( root );
        lower( candidate );
#endif
        if( root.empty() )
        {
            return true;
        }
        if( root.back() != '/' )
        {
            root += '/';
        }
        // Preserve bundled Media and other explicitly mounted external folders.
        if( candidate.compare( 0, root.size(), root ) != 0 )
        {
            return true;
        }
        const auto separator = candidate.find( '/', root.size() );
        const auto directory = candidate.substr( root.size(), separator - root.size() );
#ifdef _WIN32
        return directory != "cache" && directory != "settingscache" && directory != "tests" &&
               directory != ".workphone";
#else
        return directory != "Cache" && directory != "SettingsCache" && directory != "Tests" &&
               directory != ".workphone";
#endif
    }

    inline void filterAutoDiscoveredScripts( Array<String> &scripts, const String &projectRoot )
    {
        scripts.erase( std::remove_if( scripts.begin(), scripts.end(), [&]( const String &path ) {
                           return !shouldAutoLoadProjectScript( path, projectRoot );
                       } ),
                       scripts.end() );
    }
}  // namespace workphone
