#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsBoxShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsBoxShape3::WPPhysicsBoxShape3() : WPPhysicsShape3( WORKPHONE_COLLISION_SHAPE_BOX )
    {
    }
    SmartPtr<IPhysicsMaterial3> WPPhysicsBoxShape3::getMaterial() const
    {
        return WPPhysicsShape3::getMaterial();
    }
    void WPPhysicsBoxShape3::setMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        WPPhysicsShape3::setMaterial( material );
    }
    void WPPhysicsBoxShape3::setLocalPose( const Transform3<real_Num> &pose )
    {
        WPPhysicsShape3::setLocalPose( pose );
    }
    Transform3<real_Num> WPPhysicsBoxShape3::getLocalPose() const
    {
        return WPPhysicsShape3::getLocalPose();
    }
    void WPPhysicsBoxShape3::setSimulationFilterData( const FilterData &data )
    {
        WPPhysicsShape3::setSimulationFilterData( data );
    }
    FilterData WPPhysicsBoxShape3::getSimulationFilterData() const
    {
        return WPPhysicsShape3::getSimulationFilterData();
    }
    void WPPhysicsBoxShape3::setActor( SmartPtr<IPhysicsBody3> body )
    {
        WPPhysicsShape3::setActor( body );
    }
    SmartPtr<IPhysicsBody3> WPPhysicsBoxShape3::getActor() const
    {
        return WPPhysicsShape3::getActor();
    }
    void WPPhysicsBoxShape3::_getObject( void **ppObject ) const
    {
        WPPhysicsShape3::_getObject( ppObject );
    }
    bool WPPhysicsBoxShape3::hasShapeData() const
    {
        return WPPhysicsShape3::hasShapeData();
    }
    SmartPtr<IPhysicsShape3> WPPhysicsBoxShape3::clone()
    {
        return WPPhysicsShape3::clone();
    }
    bool WPPhysicsBoxShape3::isAttached() const
    {
        return WPPhysicsShape3::isAttached();
    }
    void WPPhysicsBoxShape3::setEnabled( bool enabled )
    {
        WPPhysicsShape3::setEnabled( enabled );
    }
    bool WPPhysicsBoxShape3::isEnabled() const
    {
        return WPPhysicsShape3::isEnabled();
    }
    void WPPhysicsBoxShape3::setTrigger( bool trigger )
    {
        WPPhysicsShape3::setTrigger( trigger );
    }
    bool WPPhysicsBoxShape3::isTrigger() const
    {
        return WPPhysicsShape3::isTrigger();
    }
    void WPPhysicsBoxShape3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        WPPhysicsShape3::setStateContext( stateContext );
    }
    SmartPtr<IStateContext> WPPhysicsBoxShape3::getStateContext() const
    {
        return WPPhysicsShape3::getStateContext();
    }
    void WPPhysicsBoxShape3::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        WPPhysicsShape3::setStateListener( stateListener );
    }
    SmartPtr<IStateListener> WPPhysicsBoxShape3::getStateListener() const
    {
        return WPPhysicsShape3::getStateListener();
    }
    void WPPhysicsBoxShape3::setCollisionType( u32 mask )
    {
        WPPhysicsShape3::setCollisionType( mask );
    }
    u32 WPPhysicsBoxShape3::getCollisionType() const
    {
        return WPPhysicsShape3::getCollisionType();
    }
    void WPPhysicsBoxShape3::setCollisionMask( u32 mask )
    {
        WPPhysicsShape3::setCollisionMask( mask );
    }
    u32 WPPhysicsBoxShape3::getCollisionMask() const
    {
        return WPPhysicsShape3::getCollisionMask();
    }
    bool WPPhysicsBoxShape3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        return WPPhysicsShape3::handleStateChanged( message );
    }
    bool WPPhysicsBoxShape3::handleStateChanged( SmartPtr<IState> &state )
    {
        return WPPhysicsShape3::handleStateChanged( state );
    }
    Vector3<real_Num> WPPhysicsBoxShape3::getExtents() const
    {
        return WPPhysicsShape3::getExtents();
    }
    void WPPhysicsBoxShape3::setExtents( const Vector3<real_Num> &extents )
    {
        WPPhysicsShape3::setExtents( extents );
    }
    AABB3<real_Num> WPPhysicsBoxShape3::getAABB() const
    {
        return WPPhysicsShape3::getAABB();
    }
    void WPPhysicsBoxShape3::setAABB( const AABB3<real_Num> &box )
    {
        WPPhysicsShape3::setAABB( box );
    }
} // namespace workphone::physics
