#pragma once
#include <Workphone/Vehicle/CarVisual.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>

namespace workphone::vehiclevisual
{
    /// Native actor/material/mesh adapter. Generated children are transient and
    /// owned by this object; imported mesh and material prototypes are immutable.
    class WPCore_API CarVisualScene
    {
    public:
        CarVisualScene();
        ~CarVisualScene();
        CarVisualScene( const CarVisualScene & ) = delete;
        CarVisualScene &operator=( const CarVisualScene & ) = delete;
        void build( SmartPtr<scene::IGameActor> parent, scene::IComponent::State state, const Spec &spec,
                    const String &chassisPath, const String &wheelPath, Colour paint,
                    const String &liveryPath );
        // The physics/client owns the parent pose. This applies local body/wheel/FX
        // poses without moving the parent twice. Camera is optional, in car-local space.
        void sync( const Frame &frame, double seconds, const Vec3 *camera = nullptr );
        void setState( scene::IComponent::State state );
        void unload();
        bool ready() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}  // namespace workphone::vehiclevisual
