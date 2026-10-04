#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsNativeSphereShape2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::physics
{
    WPPhysicsNativeSphereShape2::WPPhysicsNativeSphereShape2() : WPPhysicsShape2( WORKPHONE_COLLISION_SHAPE_SPHERE )
    {
        setRadius( static_cast<real_Num>( 0.5 ) );
    }

    WPPhysicsNativeSphereShape2::~WPPhysicsNativeSphereShape2() = default;

    void WPPhysicsNativeSphereShape2::setRadius( real_Num radius )
    {
        WP_ASSERT( radius > static_cast<real_Num>( 0 ) );
        const auto safeRadius = std::max( radius, static_cast<real_Num>( 0 ) );
        wp_collision_shape2_set_sphere_radius( getShape(), static_cast<wp_f32>( safeRadius ) );
        WP_ASSERT( getRadius() >= static_cast<real_Num>( 0 ) );
    }

    real_Num WPPhysicsNativeSphereShape2::getRadius() const
    {
        return static_cast<real_Num>( wp_collision_shape2_get_sphere_radius( getShape() ) );
    }

    Sphere2<real_Num> WPPhysicsNativeSphereShape2::getSphere() const
    {
        return Sphere2<real_Num>( Vector2<real_Num>::ZERO, static_cast<f32>( getRadius() ) );
    }

    AABB2<real_Num> WPPhysicsNativeSphereShape2::getAABB() const
    {
        const auto radius = getRadius();
        return AABB2<real_Num>( Vector2<real_Num>( -radius, -radius ),
                                Vector2<real_Num>( radius, radius ) );
    }

    void WPPhysicsNativeSphereShape2::getPoints( Array<Vector2<real_Num>> &points ) const
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

    void WPPhysicsNativeSphereShape2::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
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

    bool WPPhysicsNativeSphereShape2::isAttached() const
    {
        return WPPhysicsShape2::isAttached();
    }
    void WPPhysicsNativeSphereShape2::_getObject( void **ppObject ) const
    {
        WPPhysicsShape2::_getObject( ppObject );
    }
    u8 WPPhysicsNativeSphereShape2::getType() const
    {
        return WPPhysicsShape2::getType();
    }
    bool WPPhysicsNativeSphereShape2::isEnabled() const
    {
        return WPPhysicsShape2::isEnabled();
    }
    void WPPhysicsNativeSphereShape2::setEnabled( bool enabled )
    {
        WPPhysicsShape2::setEnabled( enabled );
    }
    bool WPPhysicsNativeSphereShape2::isTrigger() const
    {
        return WPPhysicsShape2::isTrigger();
    }
    void WPPhysicsNativeSphereShape2::setTrigger( bool trigger )
    {
        WPPhysicsShape2::setTrigger( trigger );
    }
    void WPPhysicsNativeSphereShape2::setCollisionType( u32 mask )
    {
        WPPhysicsShape2::setCollisionType( mask );
    }
    u32 WPPhysicsNativeSphereShape2::getCollisionType() const
    {
        return WPPhysicsShape2::getCollisionType();
    }
    void WPPhysicsNativeSphereShape2::setCollisionMask( u32 mask )
    {
        WPPhysicsShape2::setCollisionMask( mask );
    }
    u32 WPPhysicsNativeSphereShape2::getCollisionMask() const
    {
        return WPPhysicsShape2::getCollisionMask();
    }
    SmartPtr<IStateContext> WPPhysicsNativeSphereShape2::getStateContext() const
    {
        return WPPhysicsShape2::getStateContext();
    }
    void WPPhysicsNativeSphereShape2::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        WPPhysicsShape2::setStateContext( stateContext );
    }
    SmartPtr<IStateListener> WPPhysicsNativeSphereShape2::getStateListener() const
    {
        return WPPhysicsShape2::getStateListener();
    }
    void WPPhysicsNativeSphereShape2::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        WPPhysicsShape2::setStateListener( stateListener );
    }
    SmartPtr<Properties> WPPhysicsNativeSphereShape2::getProperties() const
    {
        auto properties = WPPhysicsShape2::getProperties();
        properties->setProperty( "radius", static_cast<f32>( getRadius() ) );
        return properties;
    }
    void WPPhysicsNativeSphereShape2::setProperties( SmartPtr<Properties> properties )
    {
        WPPhysicsShape2::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto radius = static_cast<f32>( getRadius() );
        properties->getPropertyValue( "radius", radius );
        setRadius( static_cast<real_Num>( radius ) );
    }
    bool WPPhysicsNativeSphereShape2::handleStateChanged( const SmartPtr<IStateMessage> & )
    {
        return false;
    }
    bool WPPhysicsNativeSphereShape2::handleStateChanged( SmartPtr<IState> & )
    {
        return false;
    }
} // namespace workphone::physics
