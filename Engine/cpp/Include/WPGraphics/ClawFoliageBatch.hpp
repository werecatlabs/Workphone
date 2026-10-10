#ifndef WP_ClawFoliageBatch_hpp
#define WP_ClawFoliageBatch_hpp

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <memory>

namespace workphone::render
{
    class ClawMesh;
    class ClawRendererDX11;

    struct ClawFoliageInstance
    {
        Matrix4F transform = Matrix4F::identity();
        ColourF tint = ColourF::White;
    };

    /** Immutable instance population sharing one fixed-LOD mesh/material asset.
     * Build replacements on the render owner thread, then replace the scene batch.
     * No actors, mesh copies or GPU buffers are allocated per instance. */
    class WPGraphics_API ClawFoliageBatch final
    {
    public:
        static constexpr u32 maximumInstances = 65536;
        static std::shared_ptr<const ClawFoliageBatch> create(
            SmartPtr<ClawMesh> mesh, const Array<ClawFoliageInstance> &instances, String &error,
            u32 visibilityMask = ~u32( 0 ), bool castShadows = true );
        ~ClawFoliageBatch();
        ClawFoliageBatch( const ClawFoliageBatch & ) = delete;
        ClawFoliageBatch &operator=( const ClawFoliageBatch & ) = delete;
        u32 instanceCount() const;
        u32 visibilityMask() const;
        bool castsShadows() const;
        u64 render( ClawRendererDX11 &renderer ) const;

    private:
        ClawFoliageBatch();
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
#endif
