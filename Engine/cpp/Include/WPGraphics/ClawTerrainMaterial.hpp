#ifndef WP_CLAW_TERRAIN_MATERIAL_H
#define WP_CLAW_TERRAIN_MATERIAL_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <functional>

namespace workphone::render
{
    class ClawRendererDX11;

    /** Render-thread owner of a complete immutable four-layer GPU publication.
     * Stage never changes the active material. Publish only alongside matching tiles;
     * bind/unbind brackets all terrain draws, including the directional shadow pass.
     * This value helper is not a registered intrusive engine object. */
    class WPGraphics_API ClawTerrainMaterial final
    {
    public:
        struct Candidate;
        using TextureResolver = std::function<SmartPtr<ITexture>( const String &, String & )>;
        ClawTerrainMaterial();
        ~ClawTerrainMaterial();
        ClawTerrainMaterial( const ClawTerrainMaterial & ) = delete;
        ClawTerrainMaterial &operator=( const ClawTerrainMaterial & ) = delete;
        std::shared_ptr<const Candidate> stage( ClawRendererDX11 &renderer, TerrainSnapshot snapshot,
                                                String &error, TextureResolver resolver = {},
                                                SmartPtr<ITexture> legacyAlbedo = {} ) const;
        bool publish( ClawRendererDX11 &renderer, std::shared_ptr<const Candidate> candidate,
                      String &error );
        bool validateCandidate( ClawRendererDX11 &renderer,
                                const std::shared_ptr<const Candidate> &candidate, String &error ) const;
        /** Owning render thread only: caller must validateCandidate immediately before
         * entering its publication gate, with no intervening stage/clear/device change.
         * This allocation-free swap allows terrain/source/collision to commit together. */
        void commitPrevalidated( std::shared_ptr<const Candidate> candidate ) noexcept;
        /** As commitPrevalidated, retaining the retired publication in candidate so
         * its GPU references can be released after leaving the publication gate. */
        void swapPrevalidated( std::shared_ptr<const Candidate> &candidate ) noexcept;
        bool bind( ClawRendererDX11 &renderer, String &error ) const;
        static void unbind( ClawRendererDX11 &renderer );
        TerrainSnapshot snapshot() const;
        /** Active control/weather map storage; externally owned sampled maps are excluded. */
        u64 ownedGpuBytes() const;
        /** Lifecycle exclusion required. May release from the unloading thread;
         * the next stage claims its new render owner. Never race stage/bind. */
        void clear();

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}  // namespace workphone::render
#endif
