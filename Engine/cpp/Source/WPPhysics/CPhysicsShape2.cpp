#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CPhysicsShape2.hpp>
#include <Workphone/Workphone.hpp>
#include <stdexcept>

namespace workphone::physics
{
    CPhysicsShape2::CPhysicsShape2( wp_collision_shape_type type ) :
        m_shape( wp_collision_shape_create( type ) )
    {
        if( !m_shape )
        {
            throw std::runtime_error( "Failed to create a WPPhysics 2D collision shape." );
        }
    }

    CPhysicsShape2::CPhysicsShape2( wp_collision_shape *shape ) : m_shape( shape )
    {
        WP_ASSERT( m_shape );
    }

    CPhysicsShape2::~CPhysicsShape2()
    {
        wp_collision_shape_destroy( m_shape );
        m_shape = nullptr;
    }

    void *CPhysicsShape2::getNativeObject() const
    {
        WP_ASSERT( m_shape );
        return m_shape;
    }

    wp_collision_shape *CPhysicsShape2::getShape() const
    {
        WP_ASSERT( m_shape );
        return m_shape;
    }

    bool CPhysicsShape2::isAttached() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_is_attached( m_shape ) != 0;
    }

    void CPhysicsShape2::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        if( ppObject )
        {
            *ppObject = getShape();
            WP_ASSERT( *ppObject );
        }
    }

    u8 CPhysicsShape2::getType() const
    {
        WP_ASSERT( m_shape );
        return static_cast<u8>( wp_collision_shape_get_type( m_shape ) );
    }

    bool CPhysicsShape2::isEnabled() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_is_enabled( m_shape ) != 0;
    }

    void CPhysicsShape2::setEnabled( bool enabled )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_enabled( m_shape, enabled );
        WP_ASSERT( isEnabled() == enabled );
    }

    bool CPhysicsShape2::isTrigger() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_is_trigger( m_shape ) != 0;
    }

    void CPhysicsShape2::setTrigger( bool trigger )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_trigger( m_shape, trigger );
        WP_ASSERT( isTrigger() == trigger );
    }

    void CPhysicsShape2::setCollisionType( u32 mask )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_collision_type( m_shape, mask );
        WP_ASSERT( getCollisionType() == mask );
    }

    u32 CPhysicsShape2::getCollisionType() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_get_collision_type( m_shape );
    }

    void CPhysicsShape2::setCollisionMask( u32 mask )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_collision_mask( m_shape, mask );
        WP_ASSERT( getCollisionMask() == mask );
    }

    u32 CPhysicsShape2::getCollisionMask() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_get_collision_mask( m_shape );
    }

    SmartPtr<IStateContext> CPhysicsShape2::getStateContext() const
    {
        return m_stateContext;
    }

    void CPhysicsShape2::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    SmartPtr<IStateListener> CPhysicsShape2::getStateListener() const
    {
        return m_stateListener;
    }

    void CPhysicsShape2::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    SmartPtr<Properties> CPhysicsShape2::getProperties() const
    {
        auto properties = m_properties ? workphone::make_ptr<Properties>( *m_properties )
                                       : workphone::make_ptr<Properties>();
        properties->setProperty( "enabled", isEnabled() );
        properties->setProperty( "trigger", isTrigger() );
        properties->setProperty( "collisionType", getCollisionType() );
        properties->setProperty( "collisionMask", getCollisionMask() );
        return properties;
    }

    void CPhysicsShape2::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "CPhysicsShape2::setProperties: properties are null." );
            return;
        }

        auto enabled = isEnabled();
        auto trigger = isTrigger();
        auto collisionType = getCollisionType();
        auto collisionMask = getCollisionMask();
        properties->getPropertyValue( "enabled", enabled );
        properties->getPropertyValue( "trigger", trigger );
        properties->getPropertyValue( "collisionType", collisionType );
        properties->getPropertyValue( "collisionMask", collisionMask );
        setEnabled( enabled );
        setTrigger( trigger );
        setCollisionType( collisionType );
        setCollisionMask( collisionMask );
        m_properties = workphone::make_ptr<Properties>( *properties );
    }
} // namespace workphone::physics
