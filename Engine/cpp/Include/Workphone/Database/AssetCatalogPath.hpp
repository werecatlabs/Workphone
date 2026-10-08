#ifndef WP_ASSET_CATALOG_PATH_HPP
#define WP_ASSET_CATALOG_PATH_HPP

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    struct AssetCatalogPath
    {
        String path;
        String key;
    };

    /** Capture an absolute UTF-8 project root, resolving existing filesystem prefixes.
     *  Relative roots are interpreted once against the caller's current directory.
     */
    WPCore_API bool canonicalAssetCatalogRoot( const String &input, String &output, String &error );

    /** Resolve a file path under an absolute captured project root.
     *  The returned path is project-relative UTF-8 with '/' separators. The key folds ASCII
     *  case on Windows only; non-ASCII case folding is unsupported. Existing absolute root
     *  aliases, including Windows short names, resolve to the captured root before suffix checks.
     *  Existing symlink/junction prefixes are resolved, but missing source files are allowed.
     *  This validates catalog identity, not access security: filesystem changes can race resolution.
     *  On failure output is cleared and error describes the rejected path.
     */
    WPCore_API bool canonicalAssetCatalogPath( const String &projectRoot, const String &input,
                                               AssetCatalogPath &output, String &error );
}  // namespace workphone

#endif
