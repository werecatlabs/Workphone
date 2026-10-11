#ifndef WP_ASSET_DOCUMENT_SAVE_HPP
#define WP_ASSET_DOCUMENT_SAVE_HPP
#include <Workphone/Database/AssetReferenceGraph.hpp>

namespace workphone
{
    /** Publish complete bytes through an exclusive sibling temporary file. Project
     * documents also publish a pending/ready reference revision. The installed
     * path is returned only on success. Exports and cache scratch are not indexed.
     * Bundled media and private catalog storage cannot be overwritten. */
    WPCore_API bool saveAssetDocument( const String &path, SmartPtr<Properties> serialized,
                                       const Array<u8> &bytes, String &installedPath, String &error );
    WPCore_API bool saveAssetDocumentText( const String &path, SmartPtr<Properties> serialized,
                                           const String &text, String &error );

    /** Index existing bytes without writing the document or constructing resources.
     * JSON/XML require the versioned property metadata emitted by current saves;
     * portable binary Properties are also supported. Ambiguous legacy text and
     * unsupported formats fail without replacing the previous graph. */
    WPCore_API bool reindexAssetDocument( SmartPtr<AssetDatabaseManager> catalog, const String &path,
                                          String &error,
                                          SmartPtr<AssetDatabaseManager> media = nullptr );
}  // namespace workphone
#endif
