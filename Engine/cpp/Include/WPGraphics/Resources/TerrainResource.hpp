#ifndef WP_TERRAIN_RESOURCE_H
#define WP_TERRAIN_RESOURCE_H

#include <WPGraphics/ClawTerrainMaterial.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/TerrainTiles.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

namespace workphone::render
{
    inline constexpr u64 terrainResourceByteLimit = 256ull * 1024ull * 1024ull;

    /** Compiler for .terrain descriptors serialized by the existing terrain editor.
     * Nonempty layer map names must be data://...texres IDs. The exact immutable
     * texture generations are install dependencies, not source/runtime file paths. */
    class WPGraphics_API TerrainResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies( const resource::CompileContext &, resource::DependencySet &,
                              String &error ) const override;
        resource::CompilationStatus compile( const resource::CompileContext &, std::ostream &,
                                             Array<String> &messages ) const override;
    };

    struct CookedTerrainData
    {
        TerrainData terrain;
        Array<TerrainTileInfo> tiles;
        String target;
        bool packagedBuild = false;
    };

    WPGraphics_API bool decodeCookedTerrain( const resource::RuntimeResource &resource,
                                             CookedTerrainData &output, String &error );

    class DecodedCookedTerrain;
    class CookedTerrainCandidate;
    WPGraphics_API std::shared_ptr<const DecodedCookedTerrain> decodeCookedTerrainResource(
        std::shared_ptr<const resource::RuntimeResource>, const String &target, bool packagedBuild,
        String &error );

    struct CookedTerrainDecodeOptions
    {
        u64 key = 0;
        s32 priority = 0;
        u64 maximumSourceBytes = 256ull << 20;
        u64 maximumWorkingBytes = 128ull << 20;
        u64 maximumResultBytes = 64ull << 20;
    };

    /** Replace a dedicated producer's demand with a bounded, source-free CPU decode.
     * Input storage is leased from the producer's scheduler; conservative parser/result
     * bounds are checked before submission. Cancellation discards obsolete decodes
     * at callback boundaries; parsing an admitted payload is not preempted mid-call.
     * Prefer WorldStreamingScheduler::shared(), then take the producer's result
     * and static_pointer_cast<const DecodedCookedTerrain>(result.payload). This alias
     * retains the result budget; the decoded resource retains the input budget even
     * after uploaded materials take ownership. No GPU work runs on workers. */
    WPGraphics_API bool queueCookedTerrainDecode( WorldStreamProducer &,
                                                  std::shared_ptr<const resource::RuntimeResource>,
                                                  const String &target, bool packagedBuild,
                                                  const CookedTerrainDecodeOptions &, String &error );
    /** GPU-only owner stage; consumes a sealed worker decode without repeating
     * height/tile description or texture payload decoding on the render thread. */
    WPGraphics_API std::shared_ptr<const CookedTerrainCandidate> stageCookedTerrainMaterial(
        ClawRendererDX11 &, std::shared_ptr<const DecodedCookedTerrain>, String &error );

    /** Immutable validated CPU preparation. May be decoded on a worker; no device,
     * application services or source tree are required. */
    class WPGraphics_API DecodedCookedTerrain final
    {
    public:
        const CookedTerrainData data;
        const std::shared_ptr<const resource::RuntimeResource> resource;
        const Array<CookedTextureData> textures;

    private:
        DecodedCookedTerrain( CookedTerrainData, std::shared_ptr<const resource::RuntimeResource>,
                              Array<CookedTextureData> );
        friend std::shared_ptr<const DecodedCookedTerrain> decodeCookedTerrainResource(
            std::shared_ptr<const resource::RuntimeResource>, const String &, bool, String & );
    };

    class WPGraphics_API CookedTerrainCandidate final
    {
    public:
        const std::shared_ptr<const DecodedCookedTerrain> decoded;
        const TerrainData &terrain;
        const Array<TerrainTileInfo> &tiles;
        const ClawTerrainMaterial::TextureResolver resolver;
        const std::shared_ptr<const resource::RuntimeResource> resource;

    private:
        CookedTerrainCandidate( std::shared_ptr<const DecodedCookedTerrain>,
                                ClawTerrainMaterial::TextureResolver );
        friend std::shared_ptr<const CookedTerrainCandidate> stageCookedTerrainMaterial(
            ClawRendererDX11 &, std::shared_ptr<const DecodedCookedTerrain>, String & );
    };

    /** Source-free, render-owner staging. Decode and upload the entire layer closure
     * before modifying any terrain. Retain the resolver via applyCookedTerrain();
     * it owns the uploaded ClawTextures and immutable cooked dependency handles. */
    WPGraphics_API std::shared_ptr<const CookedTerrainCandidate> stageCookedTerrainMaterial(
        ClawRendererDX11 &renderer, std::shared_ptr<const resource::RuntimeResource> resource,
        const String &target, bool packagedBuild, String &error );
}  // namespace workphone::render
#endif
