#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CSphereShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    CSphereShape3::CSphereShape3() : CPhysicsShape3( WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
    }
    SmartPtr<IPhysicsMaterial3> CSphereShape3::getMaterial() const
    {
        return CPhysicsShape3::getMaterial();
    }
    void CSphereShape3::setMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        CPhysicsShape3::setMaterial( material );
    }
    void CSphereShape3::setLocalPose( const Transform3<real_Num> &pose )
    {
        CPhysicsShape3::setLocalPose( pose );
    }
    Transform3<real_Num> CSphereShape3::getLocalPose() const
    {
        return CPhysicsShape3::getLocalPose();
    }
    void CSphereShape3::setSimulationFilterData( const FilterData &data )
    {
        CPhysicsShape3::setSimulationFilterData( data );
    }
    FilterData CSphereShape3::getSimulationFilterData() const
    {
        return CPhysicsShape3::getSimulationFilterData();
    }
    void CSphereShape3::setActor( SmartPtr<IPhysicsBody3> body )
    {
        CPhysicsShape3::setActor( body );
    }
    SmartPtr<IPhysicsBody3> CSphereShape3::getActor() const
    {
        return CPhysicsShape3::getActor();
    }
    void CSphereShape3::_getObject( void **ppObject ) const
    {
        CPhysicsShape3::_getObject( ppObject );
    }
    bool CSphereShape3::hasShapeData() const
    {
        return CPhysicsShape3::hasShapeData();
    }
    SmartPtr<IPhysicsShape3> CSphereShape3::clone()
    {
        return CPhysicsShape3::clone();
    }
    bool CSphereShape3::isAttached() const
    {
        return CPhysicsShape3::isAttached();
    }
    void CSphereShape3::setEnabled( bool enabled )
    {
        CPhysicsShape3::setEnabled( enabled );
    }
    bool CSphereShape3::isEnabled() const
    {
        return CPhysicsShape3::isEnabled();
    }
    void CSphereShape3::setTrigger( bool trigger )
    {
        CPhysicsShape3::setTrigger( trigger );
    }
    bool CSphereShape3::isTrigger() const
    {
        return CPhysicsShape3::isTrigger();
    }
    void CSphereShape3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        CPhysicsShape3::setStateContext( stateContext );
    }
    SmartPtr<IStateContext> CSphereShape3::getStateContext() const
    {
        return CPhysicsShape3::getStateContext();
    }
    void CSphereShape3::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        CPhysicsShape3::setStateListener( stateListener );
    }
    SmartPtr<IStateListener> CSphereShape3::getStateListener() const
    {
        return CPhysicsShape3::getStateListener();
    }
    void CSphereShape3::setCollisionType( u32 mask )
    {
        CPhysicsShape3::setCollisionType( mask );
    }
    u32 CSphereShape3::getCollisionType() const
    {
        return CPhysicsShape3::getCollisionType();
    }
    void CSphereShape3::setCollisionMask( u32 mask )
    {
        CPhysicsShape3::setCollisionMask( mask );
    }
    u32 CSphereShape3::getCollisionMask() const
    {
        return CPhysicsShape3::getCollisionMask();
    }
    bool CSphereShape3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        return CPhysicsShape3::handleStateChanged( message );
    }
    bool CSphereShape3::handleStateChanged( SmartPtr<IState> &state )
    {
        return CPhysicsShape3::handleStateChanged( state );
    }
    void CSphereShape3::setRadius( real_Num radius )
    {
        CPhysicsShape3::setRadius( radius );
    }
    real_Num CSphereShape3::getRadius() const
    {
        return CPhysicsShape3::getRadius();
    }
} // namespace workphone::physics
