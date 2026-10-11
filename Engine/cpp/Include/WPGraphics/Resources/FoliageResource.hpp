#ifndef WPFoliageResource_h__
#define WPFoliageResource_h__
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/FoliagePageStream.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <map>

namespace workphone::render
{
    class ClawRendererDX11;
    using FoliageTextureResolver = std::function<SmartPtr<ITexture>( const String &, String & )>;
    class WPGraphics_API FoliageResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies( const resource::CompileContext &, resource::DependencySet &, String & ) const override;
        resource::CompilationStatus compile( const resource::CompileContext &, std::ostream &, Array<String> & ) const override;
    };
    struct FoliageDecodeOptions
    {
        u64 key = 0;
        s32 priority = 0;
        u64 maximumSourceBytes = 256ull << 20;
        u64 maximumWorkingBytes = 128ull << 20;
        u64 maximumResultBytes = 64ull << 20;
    };
    /** CPU preparation retains shared species and texture closure, but reads no
     * page payload until the page loader is requested by FoliagePageStream. */
    class WPGraphics_API DecodedFoliageWorld final
    {
    public:
        Array<StreamBudget> retainedBudgets; // Released after the retained CPU buffers.
        FoliageStreamSource source;
        std::map<String, CookedTextureData> textures;
        Array<std::shared_ptr<const resource::RuntimeResource>> retainedResources;
        u64 resultBytes = 0;
    };
    /** Write immutable generation-named species/pages, then atomically replace the
     * world JSON descriptor. Texture resource IDs are existing data:// dependencies.
     * A failed export leaves the previously published world descriptor untouched. */
    WPGraphics_API bool writeFoliageSources( FoliageSnapshot, const String &sourceRoot,
        const resource::ResourceID &worldId, String &error );
    struct CookedFoliageCandidate
    {
        FoliageStreamSource source;
        FoliageTextureResolver resolver;
    };
    WPGraphics_API std::shared_ptr<const DecodedFoliageWorld> decodeCookedFoliageWorld(
        std::shared_ptr<resource::ResourceSystem>, const resource::ResourceID &, const String &target,
        bool packagedBuild, String &error, const FoliageDecodeOptions &options = {} );
    WPGraphics_API bool queueCookedFoliageWorld( WorldStreamProducer &,
        std::shared_ptr<resource::ResourceSystem>, const resource::ResourceID &, const String &target,
        bool packagedBuild, const FoliageDecodeOptions &, String &error );
    WPGraphics_API std::shared_ptr<const CookedFoliageCandidate> stageCookedFoliageWorld(
        ClawRendererDX11 &, std::shared_ptr<const DecodedFoliageWorld>, String &error );
}
#endif
