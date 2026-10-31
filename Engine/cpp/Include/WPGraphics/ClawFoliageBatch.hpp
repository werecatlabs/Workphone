#ifndef WP_ClawFoliageBatch_hpp
#define WP_ClawFoliageBatch_hpp
#include <Workphone/Memory/SmartPtr.hpp>

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <memory>

namespace workphone::scene
{
    class TerrainGroundPopulation;
}

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
        /** Uses terrain-owned stable placements; holes/outside anchors are omitted.
         * The caller replaces its existing scene batch after a population update.
         */
        static std::shared_ptr<const ClawFoliageBatch> createTerrainGrounded(
            SmartPtr<ClawMesh> mesh, const scene::TerrainGroundPopulation &population, String &error,
            const ColourF &tint = ColourF::White, u32 visibilityMask = ~u32( 0 ),
            bool castShadows = true );
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
        static std::shared_ptr<const ClawFoliageBatch> createPrepared(
            SmartPtr<ClawMesh> mesh, const Array<ClawFoliageInstance> &instances, String &error,
            u32 visibilityMask, bool castShadows, bool allowEmpty );
        std::unique_ptr<Impl> m_impl;
    };
}  // namespace workphone::render
#endif
