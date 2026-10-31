#pragma once
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <functional>
#include <vector>

namespace workphone
{
    struct ProjectAssetPolicy
    {
        s32 compression = 0, platform = 0, textureCompression = 0, textureQuality = 2;
        u32 maximumTextureSize = 16384;
        bool present = false;
    };
    struct TextureCompressionRequest
    {
        String format;
        s32 effort = 1;
        bool srgb = false;
        const render::TextureMipLevel *levels = nullptr;
        size_t levelCount = 0;
    };
    struct TextureCompressionOutput
    {
        String container; // dds, ktx2 or pvr; runtime platform plugins own installation.
        std::vector<u8> bytes;
    };
    using TextureCompressionEncoder = std::function<bool( const TextureCompressionRequest &,
                                                           TextureCompressionOutput &, String & )>;
    /** Provider registration is explicit; absent encoders fail without format substitution. */
    WPCore_API void registerTextureCompressionEncoder( const String &format, TextureCompressionEncoder encoder );
    WPCore_API bool hasTextureCompressionEncoder( const String &format );
    /** Release provider callbacks before their plugin modules are unloaded. */
    WPCore_API void releaseProjectAssetEncoders();
    WPCore_API bool encodeTextureCompression( const TextureCompressionRequest &request,
                                               TextureCompressionOutput &output, String &error );
    WPCore_API bool writeProjectAssetPolicy( const String &root, SmartPtr<Properties> settings, String &error );
    WPCore_API bool readProjectAssetPolicy( const String &path, ProjectAssetPolicy &policy, String &error );
    WPCore_API String projectTextureCompressionName( const ProjectAssetPolicy &policy, bool alpha );
    /** Versioned bounded deflate wrapper, independent of shader/texture payload formats. */
    WPCore_API Array<u8> compressProjectAssetPayload( const Array<u8> &payload, s32 effort );
    WPCore_API Array<u8> decompressProjectAssetPayload( const Array<u8> &payload, u64 maximumBytes );
}
