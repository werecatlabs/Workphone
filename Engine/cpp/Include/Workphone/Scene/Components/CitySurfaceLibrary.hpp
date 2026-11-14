#pragma once
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Procedural/TextureForgeTypes.hpp>

namespace workphone::scene
{
    enum class CitySurface : u32 { Asphalt, Paving, Brick, Plaster, Concrete, Ground, Count };

    WPCore_API const char *citySurfaceAssetId( CitySurface surface ) noexcept;
    WPCore_API float citySurfaceTileMetres( CitySurface surface ) noexcept;

    // Load a complete bundled photographic surface from the application's
    // mounted Media/Procedural/City/Textures directory. Output is 256, 512 or 1024
    // pixels; source headers must be <=1024 before codec pixels are allocated.
    // JPEG inputs use a single baseline scan; PNG compressed ancillary metadata
    // is unsupported. Both containers admit at most 64 KiB of metadata.
    // JPEG descriptive metadata is removed from the transient decoder input;
    // original files remain unchanged and embedded thumbnails are never decoded.
    // Albedo stays sRGB; normal/ORM stay linear. Source OpenGL normal green is
    // flipped to the engine's increasing-image-V tangent convention. ORM is
    // AO/Roughness/zero-metalness. Height remains empty (appearance only).
    // Missing/corrupt inputs and budget denial preserve output and return false
    // with a diagnostic, allowing the caller's existing procedural fallback.
    WPCore_API bool loadCitySurface( CitySurface surface, u32 size,
                                     procedural::SurfaceBakeResult &output, String &error );

    // Explicit directory containing the four original Poly Haven image files.
    // Useful for importing/validating packs independently of an application.
    WPCore_API bool loadCitySurfaceFromDirectory( const String &directory, CitySurface surface,
                                                  u32 size, procedural::SurfaceBakeResult &output,
                                                  String &error );
}  // namespace workphone::scene
