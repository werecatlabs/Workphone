#pragma once

#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <filesystem>

namespace workphone
{
    class FileSystem;

    /** Enable legacy authored path remapping for one packaged game file system. */
    void WPCore_API setPackagePathRemapping( const FileSystem &fileSystem, bool enabled );

    inline String packagePathAsciiLower( String value )
    {
        for( auto &character : value )
        {
            if( character >= 'A' && character <= 'Z' )
            {
                character = static_cast<char>( character + ( 'a' - 'A' ) );
            }
        }
        return value;
    }

    // Locate a legacy project-relative suffix. Callers must verify an exact match
    // in the package before replacing the authored reference.
    inline String packageRelativeAssetPath( const String &projectRoot, const String &filename )
    {
        namespace fs = std::filesystem;
        if( projectRoot.empty() || filename.empty() || !Path::isPathAbsolute( filename ) )
        {
            return {};
        }
        const auto path = fs::u8path( filename.c_str() );
        if( !path.is_absolute() || !path.has_filename() )
        {
            return {};
        }

        String root( fs::u8path( projectRoot.c_str() ).lexically_normal().generic_u8string().c_str() );
        if( root.empty() )
        {
            return {};
        }
        if( root.back() != '/' )
        {
            root += '/';
        }
        String absolute( path.lexically_normal().generic_u8string().c_str() );
#ifdef _WIN32
        root = packagePathAsciiLower( root );
        absolute = packagePathAsciiLower( absolute );
#endif
        if( absolute.compare( 0, root.size(), root ) == 0 )
        {
            return {};
        }

        fs::path candidate;
        for( const auto &component : path.relative_path() )
        {
            if( component == "." || component == ".." )
            {
                return {};
            }
            if( candidate.empty() )
            {
                const auto name = packagePathAsciiLower( component.generic_u8string().c_str() );
                if( name != "assets" && name != "cache" && name != "settingscache" && name != "media" )
                {
                    continue;
                }
            }
            candidate /= component;
        }
        return candidate.has_parent_path() ? String( candidate.generic_u8string().c_str() ) : String();
    }
}  // namespace workphone
