#ifndef WPPHYSICSSHAPE2_HPP
#define WPPHYSICSSHAPE2_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/INativePhysicsObject2.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

extern "C" {
#include <WorkphonePhysics/workphone_physics_2d.h>
}

namespace workphone::physics
{
    class WPPhysicsShape2 : public INativePhysicsObject2
    {
    public:
        explicit WPPhysicsShape2( wp_collision_shape_type type );
        explicit WPPhysicsShape2( wp_collision_shape *shape );
        ~WPPhysicsShape2() override;

        void *getNativeObject() const override;
        wp_collision_shape *getShape() const;

        bool isAttached() const;
        void _getObject( void **ppObject ) const;
        u8 getType() const;
        bool isEnabled() const;
        void setEnabled( bool enabled );
        bool isTrigger() const;
        void setTrigger( bool trigger );
        void setCollisionType( u32 mask );
        u32 getCollisionType() const;
        void setCollisionMask( u32 mask );
        u32 getCollisionMask() const;
        SmartPtr<IStateContext> getStateContext() const;
        void setStateContext( SmartPtr<IStateContext> stateContext );
        SmartPtr<IStateListener> getStateListener() const;
        void setStateListener( SmartPtr<IStateListener> stateListener );
        SmartPtr<Properties> getProperties() const;
        void setProperties( SmartPtr<Properties> properties );

    private:
        wp_collision_shape *m_shape = nullptr;
        SmartPtr<IStateContext> m_stateContext;
        SmartPtr<IStateListener> m_stateListener;
        SmartPtr<Properties> m_properties;
    };
}  // namespace workphone::physics

#endif
