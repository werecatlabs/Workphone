#ifndef WPPHYSICSNATIVEBOXSHAPE2_HPP
#define WPPHYSICSNATIVEBOXSHAPE2_HPP

#include <WPPhysics/WPPhysicsShape2.hpp>
#include <Workphone/Interface/Physics/IBoxShape2.hpp>

namespace workphone::physics
{
    class WPPhysicsNativeBoxShape2 : public WPPhysicsShape2, public IBoxShape2
    {
    public:
        WPPhysicsNativeBoxShape2();
        ~WPPhysicsNativeBoxShape2() override;

        Sphere2<real_Num> getSphere() const override;
        AABB2<real_Num> getAABB() const override;
        void setAABB( const AABB2<real_Num> &box ) override;
        void getPoints( Array<Vector2<real_Num>> &points ) const override;
        void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

        bool isAttached() const override;
        void _getObject( void **ppObject ) const override;
        u8 getType() const override;
        bool isEnabled() const override;
        void setEnabled( bool enabled ) override;
        bool isTrigger() const override;
        void setTrigger( bool trigger ) override;
        void setCollisionType( u32 mask ) override;
        u32 getCollisionType() const override;
        void setCollisionMask( u32 mask ) override;
        u32 getCollisionMask() const override;
        SmartPtr<IStateContext> getStateContext() const override;
        void setStateContext( SmartPtr<IStateContext> stateContext ) override;
        SmartPtr<IStateListener> getStateListener() const override;
        void setStateListener( SmartPtr<IStateListener> stateListener ) override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;
        bool handleStateChanged( SmartPtr<IState> &state ) override;

    private:
        AABB2<real_Num> m_aabb;
    };
}  // namespace workphone::physics

#endif
