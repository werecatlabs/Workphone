#ifndef WP_MEDIA_ASSET_CATALOG_HPP
#define WP_MEDIA_ASSET_CATALOG_HPP

#include <Workphone/Database/AssetDatabaseManager.hpp>

namespace workphone
{

    /** Derived index of the bundled media tree, independent of project/domain DBs.
     * Explicit rebuilds retain UUIDs for unchanged relative paths. */
    WPCore_API bool buildMediaAssetCatalog( const String &mediaRoot, const String &catalogPath,
                                            String &error );
    /** Mount an existing catalog read-only, creating it only if absent. The
     * default destination is <mediaRoot>/.workphone/media-assets.db. */
    WPCore_API SmartPtr<AssetDatabaseManager> openMediaAssetCatalog( const String &mediaRoot,
                                                                     String &error,
                                                                     const String &catalogPath = {} );
}  // namespace workphone
#endif
