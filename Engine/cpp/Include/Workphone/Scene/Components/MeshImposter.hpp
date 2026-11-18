#pragma once

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/Systems/MeshImposterSystem.hpp>

namespace workphone::scene
{
    /** Runtime view selection for an orthographic mesh sprite sheet.
     * The owning actor contains a centred unit XY quad, with +Z normal and
     * top-left texture coordinates. Its parent defines the captured source space.
     * Each selected frame uses the baked projection basis and a front support
     * plane. Moving its quad along camera rays preserves the projected silhouette
     * while preventing terrain from slicing through the captured volume's centre.
     * Only its actor transform and unique material UV rectangle change.
     */
    class WPCore_API MeshImposter : public Component
    {
    public:
        MeshImposter();
        ~MeshImposter() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Stages a SourceCpu-admitted metadata copy before publication. Material
         * must belong exclusively to this imposter; texture resources may be shared.
         * Also installs frame zero, so pre-load bounds use the captured dimensions.
         */
        void configure( const Array<MeshImposterView> &views, const Vector3F &centre,
                        SmartPtr<render::IMaterial> material );

        /** Deterministic counterpart to the automatic active-LOD-camera update.
         * Returns false for invalid/degenerate camera or parent transforms without
         * changing the selected frame. Inside/behind the support plane, the safe
         * unscaled centre-plane fallback is used; near LOD detail must cover that
         * range. No detail mesh or atlas upload is performed.
         */
        bool updateForCamera( const Vector3<real_Num> &worldPosition );

        /** Uses parallel camera rays for orthographic views, independently of
         * camera/patch translation. An invalid direction preserves the frame.
         * Perspective views retain position-based selection.
         */
        bool updateForView( const LODSystem::View &view );

        Vector3F getLocalCentre() const;
        Vector3F getSelectedDirection() const;
        bool getSelectedFrame( MeshImposterView &view ) const;
        s32 getSelectedView() const;
        u32 getViewCount() const;
        u64 getSelectionChanges() const;

        WP_CLASS_REGISTER_DECL;

    private:
        bool applyView( s32 selected, const Vector3<real_Num> *localCamera = nullptr,
                        bool orthographic = true );
        bool updateFromCameraVector( const Vector3<real_Num> &worldVector, bool directionOnly );

        MeshImposterSystem::Handle m_storage;
    };
}
