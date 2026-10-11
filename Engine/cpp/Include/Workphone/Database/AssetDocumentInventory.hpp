#ifndef WP_ASSET_DOCUMENT_INVENTORY_HPP
#define WP_ASSET_DOCUMENT_INVENTORY_HPP
#include <Workphone/Database/AssetDocumentSave.hpp>
#include <memory>

namespace workphone
{
    struct AssetDocumentScanOptions
    {
        u32 maxDocuments = 10000, maxEntries = 100000, maxDepth = 64;
        u64 maxBytes = 256ull * 1024 * 1024;
    };
    struct AssetDocumentInventoryEntry
    {
        String path, digest, diagnostic;
        bool canReindex = false;
    };
    struct AssetDocumentInventorySnapshot;
    /** Detached report. The private snapshot binds apply to the original scan;
     * editing the displayed rows cannot authorize additional writes. No source
     * files are rewritten, and blocked/missing documents retain their old graph. */
    struct AssetDocumentInventory
    {
        Array<AssetDocumentInventoryEntry> documents;
        u64 bytesRead = 0;
        u32 visited = 0, excluded = 0, ready = 0, blocked = 0;
        bool succeeded = false;
        String error;

    private:
        std::shared_ptr<const AssetDocumentInventorySnapshot> m_snapshot;
        friend WPCore_API AssetDocumentInventory scanAssetDocuments( SmartPtr<AssetDatabaseManager>,
                                                                     const AssetDocumentScanOptions &,
                                                                     SmartPtr<AssetDatabaseManager> );
        friend WPCore_API bool reindexAssetDocuments( SmartPtr<AssetDatabaseManager>,
                                                      const AssetDocumentInventory &, u32 &, String & );
    };
    /** Discover known scene/material/prefab/resource extensions and all active
     * indexed documents. Excludes private/VCS storage, configured cache and the
     * mounted media root. Links/special files are reported, never traversed.
     * This is bounded synchronous IO; succeeded means traversal completed, not
     * that every document or every engine format has reference coverage. */
    WPCore_API AssetDocumentInventory scanAssetDocuments(
        SmartPtr<AssetDatabaseManager> catalog, const AssetDocumentScanOptions &options = {},
        SmartPtr<AssetDatabaseManager> media = nullptr );
    /** Revalidate the inventory and atomically replace all verifiable graphs.
     * Catalog/session/media/file/membership changes expire the preview. Any SQL
     * or validation failure rolls back the entire batch. Blocked rows remain
     * explicit and untouched; the count is zero on failure. */
    WPCore_API bool reindexAssetDocuments( SmartPtr<AssetDatabaseManager> catalog,
                                           const AssetDocumentInventory &inventory, u32 &indexed,
                                           String &error );
}  // namespace workphone
#endif
