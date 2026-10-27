#ifndef WPGraphicsResourceFormats_h__
#define WPGraphicsResourceFormats_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <ostream>
#include <Workphone/System/CompiledResource.hpp>
#include <WPGraphics/MaterialProgramBinding.hpp>
#include <array>
struct wp_renderer_dx11;

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
        std::vector<u8> ddsBytes;
        u32 storageFormat = 0; // v2 per-asset native encoding; target identity remains unchanged.
    };

    /** Exact versioned target profiles; zero denotes the existing portable BGRA
     * path. Unknown windows-dx11-dds-* profiles fail, never silently fall back. */
    WPGraphics_API u32 ddsFormatForTarget( const String &target );
    class ClawTexture;
    WPGraphics_API bool validateCookedTextureData( const CookedTextureData &data, String &error );
    WPGraphics_API bool uploadCookedDds( ClawTexture *texture, const CookedTextureData &data,
                                         wp_renderer_dx11 *renderer, String &error );

    enum class MaterialTextureSlot : u32
    {
        BaseColour = 0,
        Normal = 1,
        ORM = 2
    };

    struct CookedMaterialTextureReference
    {
        resource::ResourceID id;
        u64 sourceHash = 0;
        u64 payloadHash = 0;
        u64 compilerVersion = 0;
    };

    // Portable v3 material shader payload. Numeric values use explicit IEEE-754
    // or integer 32-bit words; persistent resource identities are ResourceIDs.
    // No process-local handles, native offsets or pointers are serialized.
    struct CookedMaterialShaderParameter
    {
        String name;
        MaterialShaderParameterType type = MaterialShaderParameterType::Scalar;
        std::array<u32,16> words{};
        CookedMaterialTextureReference texture;
    };
    struct CookedMaterialShaderData
    {
        String shaderID;
        u32 schemaVersion = 0;
        Array<CookedMaterialShaderParameter> parameters;
        std::vector<u8> vertexDXBC, pixelDXBC;
        MaterialProgramReflectionSelection vertexSelection, pixelSelection;
        bool present() const { return !shaderID.empty(); }
    };
    inline constexpr u32 graphicsMaterialShaderByteLimit = 4u * 1024u * 1024u;

    /** Normal maps require Normal filtering and linear storage; packed ORM maps
     * require Data filtering and linear storage. Base colour retains the legacy
     * contract for old materials, which did not restrict texture mip semantics. */
    WPGraphics_API bool validateMaterialTextureSemantic( const CookedTextureData &texture,
                                                         MaterialTextureSlot slot, String &error );

    struct CookedMaterialData
    {
        ColourF baseColour = ColourF::White;
        f32 metalness = 0.0f;
        f32 roughness = 1.0f;
        resource::ResourceID textureId;
        u64 textureSourceHash = 0;
        u64 texturePayloadHash = 0;
        u64 textureCompilerVersion = 0;
        CookedMaterialTextureReference normalTexture;
        CookedMaterialTextureReference ormTexture;
        CookedMaterialShaderData shader;
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
    // Cold installation/compiler helper. The supplied program must be empty.
    // Resource handles are generated locally from parameter indices, never read
    // from the cooked payload. Callers retain durable resource owners separately.
    WPGraphics_API bool populateCookedMaterialShader(const CookedMaterialShaderData &shader,
        MaterialShaderParametersInstance &values, wp_shader_program *program, String &error);
    // Validates authored shader fields before dependency cooking; durable texture
    // generation pins are added by the compiler and mandatory in cooked payloads.
    WPGraphics_API bool validateCookedMaterialShaderRecipe(const CookedMaterialShaderData &, String &error);
}  // namespace workphone::render

#endif
