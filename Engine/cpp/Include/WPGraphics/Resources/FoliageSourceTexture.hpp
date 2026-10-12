#ifndef WPFoliageSourceTexture_h__
#define WPFoliageSourceTexture_h__
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <memory>
struct wp_renderer_dx11;

namespace workphone::render
{
    // Editor/source workflow for the same strict .texres descriptor consumed by
    // the offline compiler. Returned objects own scheduler memory reservations.
    // No global cache or source-file dependency is added to installed packages.
    WPGraphics_API SmartPtr<ITexture> loadFoliageSourceTexture( const String &sourceRoot,
                                                                const String &resourceId,
                                                                wp_renderer_dx11 *, String &error );
    WPGraphics_API std::shared_ptr<const TextureMipLevel> readFoliageSourceTexture(
        const String &sourceRoot, const String &resourceId, String &error );
}  // namespace workphone::render
#endif
