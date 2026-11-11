#pragma once

#include <Workphone/Core/Path.hpp>
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <filesystem>

namespace workphone
{
    // Bundled assets belong to the installation, independently of the active
    // project and the directory chosen by a launcher or debugger.
    inline String resolveBundledMediaPath( const String &startupDirectory,
                                           const String &configuredMediaPath,
                                           const String &executableDirectory = {} )
    {
        namespace fs = std::filesystem;
        const auto configured = fs::u8path( configuredMediaPath.c_str() );
        if( configured.is_absolute() )
        {
            return configured.lexically_normal().generic_u8string().c_str();
        }
        const auto findMedia = [&]( const String &directory ) -> String {
            if( directory.empty() ) return {};
            auto root = fs::absolute( fs::u8path( directory.c_str() ) ).lexically_normal();
            std::error_code error;
            const auto explicitPath = root / configured;
            if( !configured.empty() && fs::is_directory( explicitPath, error ) )
            {
                return explicitPath.lexically_normal().generic_u8string().c_str();
            }
            while( !root.empty() )
            {
                for( const auto &candidate : { root / "Media", root / "bin/Media" } )
                {
                    error.clear();
                    if( fs::is_directory( candidate, error ) )
                    {
                        return candidate.lexically_normal().generic_u8string().c_str();
                    }
                }
                const auto parent = root.parent_path();
                if( parent == root ) break;
                root = parent;
            }
            return {};
        };
        auto media = findMedia( executableDirectory );
        if( media.empty() ) media = findMedia( startupDirectory );
        return media.empty() ? Path::getAbsolutePath( startupDirectory, configuredMediaPath ) : media;
    }

    inline String mountBundledMedia( SmartPtr<IFileSystem> fileSystem,
                                     const String &startupDirectory,
                                     const String &configuredMediaPath,
                                     const String &executableDirectory = {} )
    {
        const auto root = resolveBundledMediaPath( startupDirectory, configuredMediaPath,
                                                   executableDirectory );
        fileSystem->addFolder( root + "/Scripts", true );
        fileSystem->addFolder( root, false );
        return root;
    }

    inline bool mountBundledMediaCatalog( SmartPtr<IResourceDatabase> resources,
                                           const String &mediaRoot, String &error,
                                           const String &catalogPath = {} )
    {
        auto database = dynamic_pointer_cast<ResourceDatabase>( resources );
        if( !database )
        {
            error = "Resource database does not support bundled media catalogs";
            return false;
        }
        auto catalog = openMediaAssetCatalog( mediaRoot, error, catalogPath );
        if( !catalog ) return false;
        if( !database->setMediaCatalog( catalog ) )
        {
            error = "Resource resolver rejected the bundled media catalog";
            return false;
        }
        return true;
    }
}
