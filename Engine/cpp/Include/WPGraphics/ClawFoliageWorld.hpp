#ifndef WPClawFoliageWorld_h__
#define WPClawFoliageWorld_h__
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/FoliagePageStream.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <functional>

namespace workphone::render
{
    class ClawRendererDX11;
    struct ClawFoliageView
    {
        u64 key = 1;
        Matrix4F worldToClip = Matrix4F::identity();
        Vector3F position = Vector3F::zero();
        f32 projectionScale = 1, lodBias = 1;
        bool orthographic = false, cull = true, shadow = false;
        s32 forcedLod = -1;
        u32 visibilityMask = ~u32(0);
    };
    struct ClawFoliageInteraction
    {
        Vector3F position = Vector3F::zero();
        f32 radius = 1, strength = 1;
    };
    struct ClawFoliageDiagnostics
    {
        bool currentGenerationReady = false;
        u64 sourceRevision = 0, viewKey = 0;
        u32 residentPages = 0, pendingPages = 0, failedPages = 0;
        u64 residentInstances = 0, testedInstances = 0, culledInstances = 0;
        std::array<u64,3> lodInstances{};
        u64 groups = 0, predictedDraws = 0, actualDraws = 0, submittedInstances = 0;
        u64 geometryBytes = 0, uploadBytes = 0;
    };
    /** Render-owner sparse foliage world. Pages own residency, not draw boundaries.
     * All visible cells are grouped by shared species/LOD/section. Each view retains
     * independent hard-switch+hysteresis history; grass keeps a stable density subset.
     * Source replacement publishes only complete demanded pages and device assets.
     */
    class WPGraphics_API ClawFoliageWorld final
    {
    public:
        using TextureResolver = std::function<SmartPtr<ITexture>(const String &,String &)>;
        // The gate must invoke commit synchronously exactly once on success and
        // never on failure. All allocating preparation/retirement stays outside.
        using PublicationGate = std::function<bool(FoliageSnapshot,const std::function<void()> &,String &)>;
        ClawFoliageWorld();
        ~ClawFoliageWorld();
        bool setSource(FoliageSnapshot, TextureResolver, String &error);
        bool setSource(FoliageStreamSource, TextureResolver, String &error);
        bool setPlacement(const Matrix4F &,String &error);
        bool setInteractions(const Array<ClawFoliageInteraction> &,String &error);
        bool setPublicationGate(PublicationGate,String &error);
        void setVisibilityMask(u32 mask);
        /** Successful preparation may still be pending; inspect currentGenerationReady.
         * Native residency is bounded to 30 local page widths from each view. */
        bool prepare(ClawRendererDX11 &,const Array<ClawFoliageView> &,f32 seconds,String &error);
        bool render(ClawRendererDX11 &,const ClawFoliageView &,String &error);
        FoliageSnapshot source() const;
        ClawFoliageDiagnostics diagnostics() const;
        String lastError() const;
        void clear();
        ClawFoliageWorld(const ClawFoliageWorld &) = delete;
        ClawFoliageWorld &operator=(const ClawFoliageWorld &) = delete;
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
#endif
