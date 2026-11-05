#pragma once

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::render
{
    struct TextureProfileSettings
    {
        TextureMipSettings mip;
        String compression;  // Empty inherits the package target/project policy.
        u32 maximumSize = 0;  // Zero inherits the project policy.
    };

    struct TextureProfileDocument
    {
        String sourceRoot, descriptorPath, source, target, original;
        TextureProfileSettings defaults, effective;
        bool overridden = false;
    };

    enum class TexturePreviewChannel : u32 { Colour, Red, Green, Blue, Alpha, Error };

    struct TextureProfilePreview
    {
        u32 width = 0, height = 0, mipCount = 0, format = 0, mip = 0;
        u64 encodedBytes = 0, residentBytes = 0;
        u64 inputRevision = 0;
        u32 imageWidth = 0, imageHeight = 0;
        Array<u8> image;
        bool hasReference = false;
        double meanAbsoluteError = 0;
    };

    WPGraphics_API bool openTextureProfileDocument( const String &sourceRoot, const String &path,
        const String &target, TextureProfileDocument &document, String &error );

    // Optimistic save of one target. Other targets, source and descriptor defaults are
    // preserved. A null settings pointer clears the selected target's override.
    WPGraphics_API bool saveTextureProfileOverride( const TextureProfileDocument &document,
        const TextureProfileSettings *settings, String &error );

    // The actual cooker, with a 16 MiB decoded-input ceiling and a <=256x256 sampled
    // display image. The calling worker supplies its own ResultCpu admission.
    WPGraphics_API bool buildTextureProfilePreview( const TextureProfileDocument &document,
        const TextureProfileSettings &settings, u32 mip, TexturePreviewChannel channel,
        TextureProfilePreview &preview, String &error );

    WPGraphics_API bool isTextureProfilePreviewCurrent( const TextureProfileDocument &document,
        const TextureProfilePreview &preview, String &error );
}
