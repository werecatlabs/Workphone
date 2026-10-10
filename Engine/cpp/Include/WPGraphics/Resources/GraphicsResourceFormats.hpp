#ifndef WPGraphicsResourceFormats_h__
#define WPGraphicsResourceFormats_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <ostream>
#include <Workphone/System/CompiledResource.hpp>

namespace workphone::render
{
    inline constexpr u32 graphicsResourceFormatVersion = 1;
    inline constexpr u32 graphicsResourceDescriptorLimit = 64u * 1024u;
    inline constexpr u64 graphicsTextureByteLimit = 256ull * 1024ull * 1024ull;

    struct CookedTextureData
    {
        Array<TextureMipLevel> levels;
        TextureMipSettings mipSettings;
        String target;
        bool packagedBuild = false;
    };

    struct CookedMaterialData
    {
        ColourF baseColour = ColourF::White;
        f32 metalness = 0.0f;
        f32 roughness = 1.0f;
        resource::ResourceID textureId;
        u64 textureSourceHash = 0;
        u64 texturePayloadHash = 0;
        u64 textureCompilerVersion = 0;
        String target;
        bool packagedBuild = false;
    };

    /** Decode validated typed payloads. Material decoding also validates its exact
     * installed texture generation. Callers must compare target/build mode against
     * their consumer configuration before creating device resources.
     */
    WPGraphics_API bool decodeCookedTexture( const resource::RuntimeResource &resource,
                                             CookedTextureData &output, String &error );
    WPGraphics_API bool decodeCookedMaterial( const resource::RuntimeResource &resource,
                                              CookedMaterialData &output, String &error );

    /** Compiler payload writers; the ResourceSystem owns the outer container. */
    WPGraphics_API bool writeCookedTexture( const CookedTextureData &data, std::ostream &output,
                                            String &error );
    WPGraphics_API bool writeCookedMaterial( const CookedMaterialData &data, std::ostream &output,
                                             String &error );
}  // namespace workphone::render

#endif
