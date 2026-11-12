#pragma once
#include <Workphone/Interface/Procedural/ProceduralTextureData.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>

namespace workphone::render
{
    // Save RGBA8 procedural pixels as immutable content-addressed PNG/.texres
    // sources beneath projectRoot. Reuses identical files, never replaces assets.
    WPCore_API bool saveGeneratedTextureSource( const procedural::TextureBuffer &rgba,
                                                const String &projectRoot, const String &assetDirectory,
                                                const String &name, TextureMipFilter filter,
                                                String &textureId, String &error );
}  // namespace workphone::render
