#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <memory>
#include <array>

struct wp_renderer_dx11;

namespace workphone::resource
{
    class ResourcePayloadMount;
    class ResourceLeafProviderFactory;
}  // namespace workphone::resource

namespace workphone::render
{
    struct CookedTextureData;
    /** Called only after full typed validation, while the complete resource is
     * available. Portable/DDS plans retain a bounded coarsest mip and sealed
     * per-mip offsets/hashes, never the complete input resource. WPZ1 and large
     * unmipped legacy payloads explicitly retain their ordinary complete node. */
    WPGraphics_API std::shared_ptr<const resource::RuntimeResource> snapshotRuntimeTexture(
        std::shared_ptr<const resource::RuntimeResource>,
        std::shared_ptr<resource::ResourcePayloadMount>, String &error );
    WPGraphics_API bool isRuntimeTextureSnapshot( const resource::RuntimeResource & );
    /** Strict private-plan semantic description; contains no pixel/DDS bytes. */
    WPGraphics_API bool describeRuntimeTextureSnapshot( const resource::RuntimeResource &,
                                                        CookedTextureData &, String &error );
    /** Strict private one-mip portable bootstrap sample; no file/device I/O. */
    WPGraphics_API bool sampleRuntimeTextureSnapshotBGRA( const resource::RuntimeResource &, u32 x,
                                                          u32 y, std::array<u8, 4> &, String &error );
    WPGraphics_API std::shared_ptr<resource::ResourceLeafProviderFactory>
    createRuntimeTextureLeafProviderFactory( std::shared_ptr<resource::ResourcePayloadMount>,
                                             String &error );

    struct RuntimeTextureStreamState
    {
        u32 firstMip = 0, requestedMip = 0, mipCount = 0;
        u64 residentCpuBytes = 0, residentGpuBytes = 0;
        bool pending = false, failed = false;
    };

    /** Native render-owner state. Calls require the existing graphics lifecycle
     * serialization. Final release/unload requires its owner or a paused render
     * owner; immutable chunk bytes never reference a native texture. */
    class WPGraphics_API ClawTextureStream final
    {
    public:
        static std::unique_ptr<ClawTextureStream> create(
            std::shared_ptr<const resource::RuntimeResource>,
            std::shared_ptr<resource::ResourcePayloadMount>, const String &target, bool packagedBuild,
            wp_renderer_dx11 *, String &error );
        ~ClawTextureStream();
        bool setDemand( u32 firstMip, s32 priority, s32 residencyPriority, String &error );
        bool process( wp_renderer_dx11 *, String &error );
        void *view( wp_renderer_dx11 *, s32 textureQuality, u32 maximumSize, String &error );
        void unload();
        Vector2I originalSize() const;
        Vector2I residentSize() const;
        RuntimeTextureStreamState state() const;

    private:
        struct Data;
        explicit ClawTextureStream( std::unique_ptr<Data> );
        std::unique_ptr<Data> m_data;
    };
}  // namespace workphone::render
