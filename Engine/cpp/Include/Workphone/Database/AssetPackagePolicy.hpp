#ifndef WP_ASSET_PACKAGE_POLICY_HPP
#define WP_ASSET_PACKAGE_POLICY_HPP
#include <Workphone/Database/AssetDatabaseManager.hpp>

namespace workphone
{
    enum class AssetPackageUsage
    {
        Runtime,
        EditorOnly,
        Excluded
    };
    struct AssetPackagePolicy
    {
        AssetDatabaseManager::EntrySnapshot entry;
        AssetPackageUsage usage = AssetPackageUsage::Runtime;
        bool explicitPolicy = false;
    };
    /** Read-only inspection; an absent project policy defaults to runtime.
     * Bundled media uses its existing explicit usage and fails on missing policy.
     * No schema, catalog entries or resources are constructed by inspection.
     */
    WPCore_API bool getAssetPackagePolicy( SmartPtr<AssetDatabaseManager> catalog, const String &uuid,
                                           AssetPackagePolicy &policy, String &error );
    /** Persist project usage in a separately versioned schema. Snapshot must be
     * current; media/read-only/unsupported schemas fail without writes. Revision
     * triggers also invalidate DTOs after raw/independent-connection policy writes.
     */
    WPCore_API bool setAssetPackagePolicy( SmartPtr<AssetDatabaseManager> catalog,
                                           const AssetDatabaseManager::EntrySnapshot &entry,
                                           AssetPackageUsage usage, String &error );
}  // namespace workphone
#endif
