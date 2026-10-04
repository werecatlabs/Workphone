#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsSphereShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsSphereShape3::WPPhysicsSphereShape3() : WPPhysicsShape3( WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
    }
    SmartPtr<IPhysicsMaterial3> WPPhysicsSphereShape3::getMaterial() const
    {
        return WPPhysicsShape3::getMaterial();
    }
    void WPPhysicsSphereShape3::setMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        WPPhysicsShape3::setMaterial( material );
    }
    void WPPhysicsSphereShape3::setLocalPose( const Transform3<real_Num> &pose )
    {
        WPPhysicsShape3::setLocalPose( pose );
    }
    Transform3<real_Num> WPPhysicsSphereShape3::getLocalPose() const
    {
        return WPPhysicsShape3::getLocalPose();
    }
    void WPPhysicsSphereShape3::setSimulationFilterData( const FilterData &data )
    {
        WPPhysicsShape3::setSimulationFilterData( data );
    }
    FilterData WPPhysicsSphereShape3::getSimulationFilterData() const
    {
        return WPPhysicsShape3::getSimulationFilterData();
    }
    void WPPhysicsSphereShape3::setActor( SmartPtr<IPhysicsBody3> body )
    {
        WPPhysicsShape3::setActor( body );
    }
    SmartPtr<IPhysicsBody3> WPPhysicsSphereShape3::getActor() const
    {
        return WPPhysicsShape3::getActor();
    }
    void WPPhysicsSphereShape3::_getObject( void **ppObject ) const
    {
        WPPhysicsShape3::_getObject( ppObject );
    }
    bool WPPhysicsSphereShape3::hasShapeData() const
    {
        return WPPhysicsShape3::hasShapeData();
    }
    SmartPtr<IPhysicsShape3> WPPhysicsSphereShape3::clone()
    {
        return WPPhysicsShape3::clone();
    }
    bool WPPhysicsSphereShape3::isAttached() const
    {
        return WPPhysicsShape3::isAttached();
    }
    void WPPhysicsSphereShape3::setEnabled( bool enabled )
    {
        WPPhysicsShape3::setEnabled( enabled );
    }
    bool WPPhysicsSphereShape3::isEnabled() const
    {
        return WPPhysicsShape3::isEnabled();
    }
    void WPPhysicsSphereShape3::setTrigger( bool trigger )
    {
        WPPhysicsShape3::setTrigger( trigger );
    }
    bool WPPhysicsSphereShape3::isTrigger() const
    {
        return WPPhysicsShape3::isTrigger();
    }
    void WPPhysicsSphereShape3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        WPPhysicsShape3::setStateContext( stateContext );
    }
    SmartPtr<IStateContext> WPPhysicsSphereShape3::getStateContext() const
    {
        return WPPhysicsShape3::getStateContext();
    }
    void WPPhysicsSphereShape3::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        WPPhysicsShape3::setStateListener( stateListener );
    }
    SmartPtr<IStateListener> WPPhysicsSphereShape3::getStateListener() const
    {
        return WPPhysicsShape3::getStateListener();
    }
    void WPPhysicsSphereShape3::setCollisionType( u32 mask )
    {
        WPPhysicsShape3::setCollisionType( mask );
    }
    u32 WPPhysicsSphereShape3::getCollisionType() const
    {
        return WPPhysicsShape3::getCollisionType();
    }
    void WPPhysicsSphereShape3::setCollisionMask( u32 mask )
    {
        WPPhysicsShape3::setCollisionMask( mask );
    }
    u32 WPPhysicsSphereShape3::getCollisionMask() const
    {
        return WPPhysicsShape3::getCollisionMask();
    }
    bool WPPhysicsSphereShape3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        return WPPhysicsShape3::handleStateChanged( message );
    }
    bool WPPhysicsSphereShape3::handleStateChanged( SmartPtr<IState> &state )
    {
        return WPPhysicsShape3::handleStateChanged( state );
    }
    void WPPhysicsSphereShape3::setRadius( real_Num radius )
    {
        WPPhysicsShape3::setRadius( radius );
    }
    real_Num WPPhysicsSphereShape3::getRadius() const
    {
        return WPPhysicsShape3::getRadius();
    }
} // namespace workphone::physics
