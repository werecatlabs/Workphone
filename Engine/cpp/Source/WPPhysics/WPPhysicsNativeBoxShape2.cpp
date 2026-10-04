#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsNativeBoxShape2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone::physics
{
    namespace
    {
        wp_vec2f toWp( const Vector2<real_Num> &v )
        {
            wp_vec2f r = { static_cast<wp_f32>( v.X() ), static_cast<wp_f32>( v.Y() ) };
            return r;
        }

        Vector2<real_Num> fromWp( wp_vec2f v )
        {
            return Vector2<real_Num>( static_cast<real_Num>( v.x ), static_cast<real_Num>( v.y ) );
        }
    } // namespace

    WPPhysicsNativeBoxShape2::WPPhysicsNativeBoxShape2() : WPPhysicsShape2( WORKPHONE_COLLISION_SHAPE_BOX )
    {
        setAABB( AABB2<real_Num>( Vector2<real_Num>( -0.5, -0.5 ), Vector2<real_Num>( 0.5, 0.5 ) ) );
    }

    WPPhysicsNativeBoxShape2::~WPPhysicsNativeBoxShape2() = default;

    Sphere2<real_Num> WPPhysicsNativeBoxShape2::getSphere() const
    {
        const auto halfSize = m_aabb.getHalfSize();
        const auto radius = std::max( halfSize.X(), halfSize.Y() );
        return Sphere2<real_Num>( m_aabb.getCenter(), static_cast<f32>( radius ) );
    }

    AABB2<real_Num> WPPhysicsNativeBoxShape2::getAABB() const
    {
        return m_aabb;
    }

    void WPPhysicsNativeBoxShape2::setAABB( const AABB2<real_Num> &box )
    {
        m_aabb = box;
        m_aabb.repair();
        WP_ASSERT( m_aabb.isValid() );
        const auto halfSize = m_aabb.getHalfSize();
        WP_ASSERT( halfSize.X() > static_cast<real_Num>( 0 ) );
        WP_ASSERT( halfSize.Y() > static_cast<real_Num>( 0 ) );
        wp_collision_shape2_set_box_half_extents( getShape(), toWp( halfSize ) );
        auto     center = toWp( m_aabb.getCenter() );
        wp_vec3f localPosition = { center.x, center.y, 0.0f };
        wp_collision_shape_set_local_position( getShape(), localPosition );
        WP_ASSERT( fromWp( wp_collision_shape2_get_box_half_extents( getShape() ) ) == halfSize );
    }

    void WPPhysicsNativeBoxShape2::getPoints( Array<Vector2<real_Num>> &points ) const
    {
        points.clear();
        points.push_back( m_aabb.getMin() );
        points.push_back( Vector2<real_Num>( m_aabb.getMax().X(), m_aabb.getMin().Y() ) );
        points.push_back( m_aabb.getMax() );
        points.push_back( Vector2<real_Num>( m_aabb.getMin().X(), m_aabb.getMax().Y() ) );
    }

    void WPPhysicsNativeBoxShape2::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
    {
        WP_ASSERT( massData );
        WP_ASSERT( density >= static_cast<real_Num>( 0 ) );
        if( !massData )
        {
            return;
        }

        const auto size = m_aabb.getSize();
        const auto area = size.X() * size.Y();
        const auto mass = area * std::max( density, static_cast<real_Num>( 0 ) );
        const auto inertia =
            mass * ( size.X() * size.X() + size.Y() * size.Y() ) / static_cast<real_Num>( 12 );
        massData->setMass( static_cast<f32>( mass ) );
        massData->setCenter( m_aabb.getCenter() );
        massData->setInertia( static_cast<f32>( inertia ) );
        WP_ASSERT( massData->getMass() >= 0.0f );
    }

    bool WPPhysicsNativeBoxShape2::isAttached() const
    {
        return WPPhysicsShape2::isAttached();
    }
    void WPPhysicsNativeBoxShape2::_getObject( void **ppObject ) const
    {
        WPPhysicsShape2::_getObject( ppObject );
    }
    u8 WPPhysicsNativeBoxShape2::getType() const
    {
        return WPPhysicsShape2::getType();
    }
    bool WPPhysicsNativeBoxShape2::isEnabled() const
    {
        return WPPhysicsShape2::isEnabled();
    }
    void WPPhysicsNativeBoxShape2::setEnabled( bool enabled )
    {
        WPPhysicsShape2::setEnabled( enabled );
    }
    bool WPPhysicsNativeBoxShape2::isTrigger() const
    {
        return WPPhysicsShape2::isTrigger();
    }
    void WPPhysicsNativeBoxShape2::setTrigger( bool trigger )
    {
        WPPhysicsShape2::setTrigger( trigger );
    }
    void WPPhysicsNativeBoxShape2::setCollisionType( u32 mask )
    {
        WPPhysicsShape2::setCollisionType( mask );
    }
    u32 WPPhysicsNativeBoxShape2::getCollisionType() const
    {
        return WPPhysicsShape2::getCollisionType();
    }
    void WPPhysicsNativeBoxShape2::setCollisionMask( u32 mask )
    {
        WPPhysicsShape2::setCollisionMask( mask );
    }
    u32 WPPhysicsNativeBoxShape2::getCollisionMask() const
    {
        return WPPhysicsShape2::getCollisionMask();
    }
    SmartPtr<IStateContext> WPPhysicsNativeBoxShape2::getStateContext() const
    {
        return WPPhysicsShape2::getStateContext();
    }
    void WPPhysicsNativeBoxShape2::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        WPPhysicsShape2::setStateContext( stateContext );
    }
    SmartPtr<IStateListener> WPPhysicsNativeBoxShape2::getStateListener() const
    {
        return WPPhysicsShape2::getStateListener();
    }
    void WPPhysicsNativeBoxShape2::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        WPPhysicsShape2::setStateListener( stateListener );
    }
    SmartPtr<Properties> WPPhysicsNativeBoxShape2::getProperties() const
    {
        auto properties = WPPhysicsShape2::getProperties();
        properties->setProperty( "center", m_aabb.getCenter() );
        properties->setProperty( "size", m_aabb.getSize() );
        return properties;
    }
    void WPPhysicsNativeBoxShape2::setProperties( SmartPtr<Properties> properties )
    {
        WPPhysicsShape2::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto center = m_aabb.getCenter();
        auto size = m_aabb.getSize();
        properties->getPropertyValue( "center", center );
        properties->getPropertyValue( "size", size );
        size.X() = Math<real_Num>::Abs( size.X() );
        size.Y() = Math<real_Num>::Abs( size.Y() );
        const auto halfSize = size * static_cast<real_Num>( 0.5 );
        setAABB( AABB2<real_Num>( center - halfSize, center + halfSize ) );
    }
    bool WPPhysicsNativeBoxShape2::handleStateChanged( const SmartPtr<IStateMessage> & )
    {
        return false;
    }
    bool WPPhysicsNativeBoxShape2::handleStateChanged( SmartPtr<IState> & )
    {
        return false;
    }
} // namespace workphone::physics
