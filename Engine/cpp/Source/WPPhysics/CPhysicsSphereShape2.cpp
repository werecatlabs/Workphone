#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CPhysicsSphereShape2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::physics
{
    CPhysicsSphereShape2::CPhysicsSphereShape2() : CPhysicsShape2( WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        setRadius( static_cast<real_Num>( 0.5 ) );
    }

    CPhysicsSphereShape2::~CPhysicsSphereShape2() = default;

    void CPhysicsSphereShape2::setRadius( real_Num radius )
    {
        WP_ASSERT( radius > static_cast<real_Num>( 0 ) );
        const auto safeRadius = std::max( radius, static_cast<real_Num>( 0 ) );
        wp_collision_shape2_set_sphere_radius( getShape(), static_cast<wp_f32>( safeRadius ) );
        WP_ASSERT( getRadius() >= static_cast<real_Num>( 0 ) );
    }

    real_Num CPhysicsSphereShape2::getRadius() const
    {
        return static_cast<real_Num>( wp_collision_shape2_get_sphere_radius( getShape() ) );
    }

    Sphere2<real_Num> CPhysicsSphereShape2::getSphere() const
    {
        return Sphere2<real_Num>( Vector2<real_Num>::ZERO, static_cast<f32>( getRadius() ) );
    }

    AABB2<real_Num> CPhysicsSphereShape2::getAABB() const
    {
        const auto radius = getRadius();
        return AABB2<real_Num>( Vector2<real_Num>( -radius, -radius ),
                                Vector2<real_Num>( radius, radius ) );
    }

    void CPhysicsSphereShape2::getPoints( Array<Vector2<real_Num>> &points ) const
    {
        points.clear();
        constexpr u32 numPoints = 16;
        points.reserve( numPoints );
        const auto radius = getRadius();
        const auto twoPi = static_cast<real_Num>( 6.28318530717958647692 );
        for( u32 i = 0; i < numPoints; ++i )
        {
            const auto angle = twoPi * static_cast<real_Num>( i ) / static_cast<real_Num>( numPoints );
            points.emplace_back( static_cast<real_Num>( std::cos( angle ) ) * radius,
                                 static_cast<real_Num>( std::sin( angle ) ) * radius );
        }
    }

    void CPhysicsSphereShape2::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
    {
        WP_ASSERT( massData );
        WP_ASSERT( density >= static_cast<real_Num>( 0 ) );
        if( !massData )
        {
            return;
        }

        const auto radius = getRadius();
        const auto pi = static_cast<real_Num>( 3.14159265358979323846 );
        const auto mass = pi * radius * radius * std::max( density, static_cast<real_Num>( 0 ) );
        const auto inertia = static_cast<real_Num>( 0.5 ) * mass * radius * radius;
        massData->setMass( static_cast<f32>( mass ) );
        massData->setCenter( Vector2<real_Num>::ZERO );
        massData->setInertia( static_cast<f32>( inertia ) );
        WP_ASSERT( massData->getMass() >= 0.0f );
    }

    bool CPhysicsSphereShape2::isAttached() const
    {
        return CPhysicsShape2::isAttached();
    }
    void CPhysicsSphereShape2::_getObject( void **ppObject ) const
    {
        CPhysicsShape2::_getObject( ppObject );
    }
    u8 CPhysicsSphereShape2::getType() const
    {
        return CPhysicsShape2::getType();
    }
    bool CPhysicsSphereShape2::isEnabled() const
    {
        return CPhysicsShape2::isEnabled();
    }
    void CPhysicsSphereShape2::setEnabled( bool enabled )
    {
        CPhysicsShape2::setEnabled( enabled );
    }
    bool CPhysicsSphereShape2::isTrigger() const
    {
        return CPhysicsShape2::isTrigger();
    }
    void CPhysicsSphereShape2::setTrigger( bool trigger )
    {
        CPhysicsShape2::setTrigger( trigger );
    }
    void CPhysicsSphereShape2::setCollisionType( u32 mask )
    {
        CPhysicsShape2::setCollisionType( mask );
    }
    u32 CPhysicsSphereShape2::getCollisionType() const
    {
        return CPhysicsShape2::getCollisionType();
    }
    void CPhysicsSphereShape2::setCollisionMask( u32 mask )
    {
        CPhysicsShape2::setCollisionMask( mask );
    }
    u32 CPhysicsSphereShape2::getCollisionMask() const
    {
        return CPhysicsShape2::getCollisionMask();
    }
    SmartPtr<IStateContext> CPhysicsSphereShape2::getStateContext() const
    {
        return CPhysicsShape2::getStateContext();
    }
    void CPhysicsSphereShape2::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        CPhysicsShape2::setStateContext( stateContext );
    }
    SmartPtr<IStateListener> CPhysicsSphereShape2::getStateListener() const
    {
        return CPhysicsShape2::getStateListener();
    }
    void CPhysicsSphereShape2::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        CPhysicsShape2::setStateListener( stateListener );
    }
    SmartPtr<Properties> CPhysicsSphereShape2::getProperties() const
    {
        auto properties = CPhysicsShape2::getProperties();
        properties->setProperty( "radius", static_cast<f32>( getRadius() ) );
        return properties;
    }
    void CPhysicsSphereShape2::setProperties( SmartPtr<Properties> properties )
    {
        CPhysicsShape2::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto radius = static_cast<f32>( getRadius() );
        properties->getPropertyValue( "radius", radius );
        setRadius( static_cast<real_Num>( radius ) );
    }
    bool CPhysicsSphereShape2::handleStateChanged( const SmartPtr<IStateMessage> & )
    {
        return false;
    }
    bool CPhysicsSphereShape2::handleStateChanged( SmartPtr<IState> & )
    {
        return false;
    }
} // namespace workphone::physics
