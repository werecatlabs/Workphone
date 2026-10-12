#ifndef WP_ASSET_REFERENCE_GRAPH_HPP
#define WP_ASSET_REFERENCE_GRAPH_HPP
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    struct AssetReferenceEdge
    {
        String sourcePath, slot, targetUuid, targetCatalog, expectedType;
        bool required = true;
        String sourceState, diagnostic;
    };

    /** A pending save is durable before file IO. Complete only after the exact
     * serialized bytes have been installed. Failed/abandoned saves block deletion. */
    struct AssetReferenceSave
    {
        AssetCatalogSession session;
        String sourcePath, sourceKey, revision, digest;
        Array<AssetReferenceEdge> edges;
    };

    WPCore_API bool beginAssetReferenceSave( SmartPtr<AssetDatabaseManager> catalog,
                                             const String &sourcePath, SmartPtr<Properties> serialized,
                                             const Array<u8> &bytes, AssetReferenceSave &save,
                                             String &error,
                                             SmartPtr<AssetDatabaseManager> media = nullptr );
    WPCore_API bool completeAssetReferenceSave( SmartPtr<AssetDatabaseManager> catalog,
                                                const AssetReferenceSave &save, String &error );

    struct AssetReferenceCursor
    {
        u64 afterId = 0, generation = 0, catalogInstance = 0;
        String value, targetCatalog;
        bool reverse = true;
    };

    struct AssetReferencePage
    {
        Array<AssetReferenceEdge> edges;
        AssetReferenceCursor next;
        bool succeeded = false, hasMore = false;
        String error;
    };

    /** Indexed forward/reverse queries. Reverse value is a target UUID; forward
     * value is a source path. No resources are constructed. Pages expire on any
     * catalog/reference revision. Quarantined documents are excluded. */
    WPCore_API AssetReferencePage queryAssetReferences( SmartPtr<AssetDatabaseManager> catalog,
                                                        const String &value, bool reverse = true,
                                                        const String &targetCatalog = "project",
                                                        u32 limit = 100,
                                                        const AssetReferenceCursor *cursor = nullptr );
}  // namespace workphone
#endif
