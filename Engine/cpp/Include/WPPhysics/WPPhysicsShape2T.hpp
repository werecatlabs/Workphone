#ifndef WPPHYSICSSHAPE2T_HPP
#define WPPHYSICSSHAPE2T_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Physics/BoxShape2.hpp>
#include <Workphone/Physics/SphereShape2.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

extern "C" {
#include <WorkphonePhysics/workphone_physics_2d.h>
}

namespace workphone::physics
{
    template <class T>
    class WPPhysicsShape2T : public T
    {
    public:
        explicit WPPhysicsShape2T(wp_collision_shape_type type);
        explicit WPPhysicsShape2T(wp_collision_shape *shape);
        ~WPPhysicsShape2T() override;

        void *getNativeObject() const;
        wp_collision_shape *getShape() const;

        bool isAttached() const;
        void _getObject(void **ppObject) const;
        u8 getType() const;
        bool isEnabled() const;
        void setEnabled(bool enabled);
        bool isTrigger() const;
        void setTrigger(bool trigger);
        void setCollisionType(u32 mask);
        u32 getCollisionType() const;
        void setCollisionMask(u32 mask);
        u32 getCollisionMask() const;
        SmartPtr<IStateContext> getStateContext() const;
        void setStateContext(SmartPtr<IStateContext> stateContext);
        SmartPtr<IStateListener> getStateListener() const;
        void setStateListener(SmartPtr<IStateListener> stateListener);
        SmartPtr<Properties> getProperties() const;
        void setProperties(SmartPtr<Properties> properties);

    private:
        wp_collision_shape *m_shape = nullptr;
        SmartPtr<IStateContext> m_stateContext;
        SmartPtr<IStateListener> m_stateListener;
        SmartPtr<Properties> m_properties;
    };

    extern template class WPPhysicsShape2T<BoxShape2>;
    extern template class WPPhysicsShape2T<SphereShape2>;
} // namespace workphone::physics

#endif
