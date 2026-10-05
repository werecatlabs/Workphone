#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsShape2T.hpp>
#include <Workphone/Workphone.hpp>
#include <stdexcept>

namespace workphone::physics
{
    template <class T>
    WPPhysicsShape2T<T>::WPPhysicsShape2T( wp_collision_shape_type type ) :
        m_shape( wp_collision_shape_create( type ) )
    {
        if( !m_shape )
        {
            throw std::runtime_error( "Failed to create a WPPhysics 2D collision shape." );
        }
    }

    template <class T>
    WPPhysicsShape2T<T>::WPPhysicsShape2T( wp_collision_shape *shape ) : m_shape( shape )
    {
        WP_ASSERT( m_shape );
    }

    template <class T>
    WPPhysicsShape2T<T>::~WPPhysicsShape2T()
    {
        wp_collision_shape_destroy( m_shape );
        m_shape = nullptr;
    }

    template <class T>
    void *WPPhysicsShape2T<T>::getNativeObject() const
    {
        WP_ASSERT( m_shape );
        return m_shape;
    }

    template <class T>
    wp_collision_shape *WPPhysicsShape2T<T>::getShape() const
    {
        WP_ASSERT( m_shape );
        return m_shape;
    }

    template <class T>
    bool WPPhysicsShape2T<T>::isAttached() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_is_attached( m_shape ) != 0;
    }

    template <class T>
    void WPPhysicsShape2T<T>::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        if( ppObject )
        {
            *ppObject = getShape();
            WP_ASSERT( *ppObject );
        }
    }

    template <class T>
    u8 WPPhysicsShape2T<T>::getType() const
    {
        WP_ASSERT( m_shape );
        return static_cast<u8>( wp_collision_shape_get_type( m_shape ) );
    }

    template <class T>
    bool WPPhysicsShape2T<T>::isEnabled() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_is_enabled( m_shape ) != 0;
    }

    template <class T>
    void WPPhysicsShape2T<T>::setEnabled( bool enabled )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_enabled( m_shape, enabled );
        WP_ASSERT( isEnabled() == enabled );
    }

    template <class T>
    bool WPPhysicsShape2T<T>::isTrigger() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_is_trigger( m_shape ) != 0;
    }

    template <class T>
    void WPPhysicsShape2T<T>::setTrigger( bool trigger )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_trigger( m_shape, trigger );
        WP_ASSERT( isTrigger() == trigger );
    }

    template <class T>
    void WPPhysicsShape2T<T>::setCollisionType( u32 mask )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_collision_type( m_shape, mask );
        WP_ASSERT( getCollisionType() == mask );
    }

    template <class T>
    u32 WPPhysicsShape2T<T>::getCollisionType() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_get_collision_type( m_shape );
    }

    template <class T>
    void WPPhysicsShape2T<T>::setCollisionMask( u32 mask )
    {
        WP_ASSERT( m_shape );
        wp_collision_shape_set_collision_mask( m_shape, mask );
        WP_ASSERT( getCollisionMask() == mask );
    }

    template <class T>
    u32 WPPhysicsShape2T<T>::getCollisionMask() const
    {
        WP_ASSERT( m_shape );
        return wp_collision_shape_get_collision_mask( m_shape );
    }

    template <class T>
    SmartPtr<IStateContext> WPPhysicsShape2T<T>::getStateContext() const
    {
        return m_stateContext;
    }

    template <class T>
    void WPPhysicsShape2T<T>::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    template <class T>
    SmartPtr<IStateListener> WPPhysicsShape2T<T>::getStateListener() const
    {
        return m_stateListener;
    }

    template <class T>
    void WPPhysicsShape2T<T>::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    template <class T>
    SmartPtr<Properties> WPPhysicsShape2T<T>::getProperties() const
    {
        auto properties = m_properties ? workphone::make_ptr<Properties>( *m_properties )
                                       : workphone::make_ptr<Properties>();
        properties->setProperty( "enabled", isEnabled() );
        properties->setProperty( "trigger", isTrigger() );
        properties->setProperty( "collisionType", getCollisionType() );
        properties->setProperty( "collisionMask", getCollisionMask() );
        return properties;
    }

    template <class T>
    void WPPhysicsShape2T<T>::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "WPPhysicsShape2T::setProperties: properties are null." );
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
    template class WPPhysicsShape2T<BoxShape2>;
    template class WPPhysicsShape2T<SphereShape2>;
} // namespace workphone::physics
