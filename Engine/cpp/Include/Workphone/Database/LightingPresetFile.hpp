#ifndef WP_LIGHTING_PRESET_FILE_HPP
#define WP_LIGHTING_PRESET_FILE_HPP

#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Scene/Directors/LightingDirector.hpp>

namespace workphone
{
    inline String registerLightingPresetFile( AssetDatabaseManager &catalog, IFileSystem &files,
                                              const String &filePath, String &error )
    {
        error.clear();
        const auto extension = StringUtil::make_lower( Path::getFileExtension( filePath ) );
        if( extension != ".resource" && extension != ".lightingpreset" ) return {};

        AssetCatalogPath source;
        const auto root = catalog.getProjectRoot();
        if( !canonicalAssetCatalogPath( root, filePath, source, error ) ) return {};

        const auto sourcePath = root + "/" + source.path;
        auto data = make_ptr<Properties>();
        DataUtil::parse( files.readAllText( sourcePath ), data.get() );
        const String type = TypeManager::instance()->getName( scene::LightingDirector::typeInfo() );
        const auto authoredType = data->getProperty( Director::objectTypeStr );
        // Generic director files must identify their type. Older preset files can have only values.
        const bool legacyPreset = extension == ".lightingpreset" && authoredType.empty() &&
                                  ( data->hasProperty( scene::LightingDirector::ambientColourStr ) ||
                                    data->hasProperty( scene::LightingDirector::upperHemisphereStr ) ||
                                    data->hasProperty( scene::LightingDirector::lowerHemisphereStr ) ||
                                    data->hasProperty( scene::LightingDirector::hemisphereDirStr ) ||
                                    data->hasProperty( scene::LightingDirector::envmapScaleStr ) );
        if( authoredType != type && !legacyPreset ) return {};

        const auto findRegistered = [&]() -> String {
            auto entry = dynamic_pointer_cast<scene::ResourceDirector>(
                catalog.getResourceEntryFromPath( sourcePath ) );
            if( !entry ) return {};

            const auto uuid = entry->getResourceUUID();
            AssetDatabaseManager::EntrySnapshot snapshot;
            if( catalog.tryGetEntry( uuid, snapshot ) && snapshot.type == type ) return uuid;
            error = "Existing catalog entry is not a lighting resource: " + source.path;
            return {};
        };

        const auto registered = findRegistered();
        if( !registered.empty() || !error.empty() ) return registered;

        const auto uuid = StringUtil::getUUID();
        if( catalog.registerFileEntries( { { uuid, sourcePath, type } }, error ) ) return uuid;

        // A concurrent import can publish the same path while a drop is being handled.
        const auto concurrent = findRegistered();
        if( !concurrent.empty() ) error.clear();
        return concurrent;
    }
}  // namespace workphone

#endif
