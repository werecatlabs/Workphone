#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CBoxShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    CBoxShape3::CBoxShape3() : CPhysicsShape3( WORKPHONE_COLLISION_SHAPE_BOX )
    {
    }
    SmartPtr<IPhysicsMaterial3> CBoxShape3::getMaterial() const
    {
        return CPhysicsShape3::getMaterial();
    }
    void CBoxShape3::setMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        CPhysicsShape3::setMaterial( material );
    }
    void CBoxShape3::setLocalPose( const Transform3<real_Num> &pose )
    {
        CPhysicsShape3::setLocalPose( pose );
    }
    Transform3<real_Num> CBoxShape3::getLocalPose() const
    {
        return CPhysicsShape3::getLocalPose();
    }
    void CBoxShape3::setSimulationFilterData( const FilterData &data )
    {
        CPhysicsShape3::setSimulationFilterData( data );
    }
    FilterData CBoxShape3::getSimulationFilterData() const
    {
        return CPhysicsShape3::getSimulationFilterData();
    }
    void CBoxShape3::setActor( SmartPtr<IPhysicsBody3> body )
    {
        CPhysicsShape3::setActor( body );
    }
    SmartPtr<IPhysicsBody3> CBoxShape3::getActor() const
    {
        return CPhysicsShape3::getActor();
    }
    void CBoxShape3::_getObject( void **ppObject ) const
    {
        CPhysicsShape3::_getObject( ppObject );
    }
    bool CBoxShape3::hasShapeData() const
    {
        return CPhysicsShape3::hasShapeData();
    }
    SmartPtr<IPhysicsShape3> CBoxShape3::clone()
    {
        return CPhysicsShape3::clone();
    }
    bool CBoxShape3::isAttached() const
    {
        return CPhysicsShape3::isAttached();
    }
    void CBoxShape3::setEnabled( bool enabled )
    {
        CPhysicsShape3::setEnabled( enabled );
    }
    bool CBoxShape3::isEnabled() const
    {
        return CPhysicsShape3::isEnabled();
    }
    void CBoxShape3::setTrigger( bool trigger )
    {
        CPhysicsShape3::setTrigger( trigger );
    }
    bool CBoxShape3::isTrigger() const
    {
        return CPhysicsShape3::isTrigger();
    }
    void CBoxShape3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        CPhysicsShape3::setStateContext( stateContext );
    }
    SmartPtr<IStateContext> CBoxShape3::getStateContext() const
    {
        return CPhysicsShape3::getStateContext();
    }
    void CBoxShape3::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        CPhysicsShape3::setStateListener( stateListener );
    }
    SmartPtr<IStateListener> CBoxShape3::getStateListener() const
    {
        return CPhysicsShape3::getStateListener();
    }
    void CBoxShape3::setCollisionType( u32 mask )
    {
        CPhysicsShape3::setCollisionType( mask );
    }
    u32 CBoxShape3::getCollisionType() const
    {
        return CPhysicsShape3::getCollisionType();
    }
    void CBoxShape3::setCollisionMask( u32 mask )
    {
        CPhysicsShape3::setCollisionMask( mask );
    }
    u32 CBoxShape3::getCollisionMask() const
    {
        return CPhysicsShape3::getCollisionMask();
    }
    bool CBoxShape3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        return CPhysicsShape3::handleStateChanged( message );
    }
    bool CBoxShape3::handleStateChanged( SmartPtr<IState> &state )
    {
        return CPhysicsShape3::handleStateChanged( state );
    }
    Vector3<real_Num> CBoxShape3::getExtents() const
    {
        return CPhysicsShape3::getExtents();
    }
    void CBoxShape3::setExtents( const Vector3<real_Num> &extents )
    {
        CPhysicsShape3::setExtents( extents );
    }
    AABB3<real_Num> CBoxShape3::getAABB() const
    {
        return CPhysicsShape3::getAABB();
    }
    void CBoxShape3::setAABB( const AABB3<real_Num> &box )
    {
        CPhysicsShape3::setAABB( box );
    }
} // namespace workphone::physics
