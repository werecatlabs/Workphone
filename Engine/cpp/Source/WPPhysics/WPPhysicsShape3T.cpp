#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    template <class T>
    WPPhysicsShape3T<T>::WPPhysicsShape3T()
    {
    }

    template <class T>
    WPPhysicsShape3T<T>::WPPhysicsShape3T( u32 type )
    {
        setType( type );
        load( nullptr );
    }

    template <class T>
    WPPhysicsShape3T<T>::~WPPhysicsShape3T()
    {
        unload( nullptr );
    }

    template <class T>
    void WPPhysicsShape3T<T>::load( SmartPtr<ISharedObject> data )
    {
        auto type = getType();
        if( m_shape ) return;
        m_shape = wp_collision_shape_create( static_cast<wp_collision_shape_type>( type ) );
        if( !m_shape )
        {
            throw std::runtime_error( "Failed to create a WPPhysics collision shape." );
        }
    }

    template <class T>
    void WPPhysicsShape3T<T>::unload( SmartPtr<ISharedObject> data )
    {
        wp_collision_shape_destroy( m_shape );
        m_shape = nullptr;
    }

    template <class T>
    SmartPtr<IPhysicsMaterial3> WPPhysicsShape3T<T>::getMaterial() const
    {
        return m_material;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        m_material = material;
        const auto backendMaterial = dynamic_cast<WPPhysicsMaterial3 *>( material.get() );
        wp_collision_shape_set_material(
            m_shape, backendMaterial ? backendMaterial->getMaterial() : nullptr );
    }

    template <class T>
    void WPPhysicsShape3T<T>::setLocalPose( const Transform3<real_Num> &pose )
    {
        wp_collision_shape_set_local_position( m_shape, WPPhysicsUtil::toWp( pose.getPosition() ) );
        wp_collision_shape_set_local_orientation( m_shape, WPPhysicsUtil::toWp( pose.getOrientation() ) );
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            // The native API takes dimensions, so bake actor scale into the geometry.
            // Keep the original extents to avoid compounding scale on later updates/clones.
            m_boxScale = pose.getScale();
            setExtents( m_boxExtents );
        }
    }

    template <class T>
    Transform3<real_Num> WPPhysicsShape3T<T>::getLocalPose() const
    {
        auto pose = Transform3<real_Num>(
            WPPhysicsUtil::fromWp( wp_collision_shape_get_local_position( m_shape ) ),
            WPPhysicsUtil::fromWp( wp_collision_shape_get_local_orientation( m_shape ) ) );
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
            pose.setScale( m_boxScale );
        return pose;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setSimulationFilterData( const FilterData &data )
    {
        wp_filter_data fd = { data.word0, data.word1, data.word2, data.word3 };
        wp_collision_shape_set_filter_data( m_shape, fd );
    }

    template <class T>
    FilterData WPPhysicsShape3T<T>::getSimulationFilterData() const
    {
        auto fd = wp_collision_shape_get_filter_data( m_shape );
        return FilterData( fd.word0, fd.word1, fd.word2, fd.word3 );
    }

    template <class T>
    void WPPhysicsShape3T<T>::setActor( SmartPtr<IPhysicsBody3> body )
    {
        m_actor = body;
    }

    template <class T>
    SmartPtr<IPhysicsBody3> WPPhysicsShape3T<T>::getActor() const
    {
        return m_actor.lock();
    }

    template <class T>
    void WPPhysicsShape3T<T>::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_shape;
        }
    }

    template <class T>
    bool WPPhysicsShape3T<T>::hasShapeData() const
    {
        return m_shape != nullptr;
    }

    template <class T>
    SmartPtr<IPhysicsShape3> WPPhysicsShape3T<T>::clone()
    {
        return nullptr;
    }

    template <class T>
    Vector3<real_Num> WPPhysicsShape3T<T>::getExtents() const
    {
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
            return m_boxExtents;
        return WPPhysicsUtil::fromWp( wp_collision_shape_get_box_half_extents( m_shape ) ) *
               static_cast<real_Num>( 2.0 );
    }

    template <class T>
    void WPPhysicsShape3T<T>::setExtents( const Vector3<real_Num> &extents )
    {
        auto safeExtents = WPPhysicsUtil::absoluteVector( extents );
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            m_boxExtents = safeExtents;
            safeExtents = WPPhysicsUtil::absoluteVector( m_boxExtents * m_boxScale );
        }
        wp_collision_shape_set_box_half_extents(
            m_shape, WPPhysicsUtil::toWp( safeExtents * static_cast<real_Num>( 0.5 ) ) );
    }

    template <class T>
    AABB3<real_Num> WPPhysicsShape3T<T>::getAABB() const
    {
        AABB3<real_Num> bounds;
        const auto type = wp_collision_shape_get_type( m_shape );
        if( type == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            // transformBounds applies the local scale exactly once.
            const auto halfExtents = m_boxExtents * static_cast<real_Num>( 0.5 );
            bounds = AABB3<real_Num>( -halfExtents, halfExtents );
        }
        else if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
        {
            const auto radius = getRadius();
            const auto halfExtents = Vector3<real_Num>( radius, radius, radius );
            bounds = AABB3<real_Num>( -halfExtents, halfExtents );
        }
        else if( type == WORKPHONE_COLLISION_SHAPE_CAPSULE )
        {
            const auto radius =
                static_cast<real_Num>( wp_collision_shape_get_capsule_radius( m_shape ) );
            const auto halfHeight =
                static_cast<real_Num>( wp_collision_shape_get_capsule_half_height( m_shape ) );
            const auto halfExtents = Vector3<real_Num>( radius, radius + halfHeight, radius );
            bounds = AABB3<real_Num>( -halfExtents, halfExtents );
        }
        else if( type == WORKPHONE_COLLISION_SHAPE_PLANE )
        {
            bounds.setInfinite();
            return bounds;
        }
        else if( type == WORKPHONE_COLLISION_SHAPE_MESH )
        {
            const auto mesh = wp_collision_shape_get_mesh_data( m_shape );
            if( !mesh || !mesh->vertices || mesh->vertex_count == 0 )
            {
                bounds.setNull();
                return bounds;
            }

            auto point = Vector3<real_Num>( static_cast<real_Num>( mesh->vertices[0] ),
                                            static_cast<real_Num>( mesh->vertices[1] ),
                                            static_cast<real_Num>( mesh->vertices[2] ) );
            bounds = AABB3<real_Num>( point );
            for( wp_u32 i = 1; i < mesh->vertex_count; ++i )
            {
                point = Vector3<real_Num>( static_cast<real_Num>( mesh->vertices[i * 3] ),
                                           static_cast<real_Num>( mesh->vertices[i * 3 + 1] ),
                                           static_cast<real_Num>( mesh->vertices[i * 3 + 2] ) );
                bounds.merge( point );
            }
        }
        else
        {
            bounds = m_aabb;
        }

        return WPPhysicsUtil::transformBounds( bounds, getLocalPose() );
    }

    template <class T>
    void WPPhysicsShape3T<T>::setAABB( const AABB3<real_Num> &box )
    {
        m_aabb = box;
        m_aabb.repair();
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX &&
            m_aabb.isFinite() )
        {
            auto pose = getLocalPose();
            pose.setPosition( m_aabb.getCenter() );
            setLocalPose( pose );
            setExtents( m_aabb.getExtent() );
        }
    }

    template <class T>
    void WPPhysicsShape3T<T>::setRadius( real_Num radius )
    {
        wp_collision_shape_set_sphere_radius( m_shape,
                                              static_cast<wp_f32>( Math<real_Num>::Abs( radius ) ) );
    }

    template <class T>
    real_Num WPPhysicsShape3T<T>::getRadius() const
    {
        return static_cast<real_Num>( wp_collision_shape_get_sphere_radius( m_shape ) );
    }

    template <class T>
    wp_collision_shape *WPPhysicsShape3T<T>::getShape() const
    {
        return m_shape;
    }

    template <class T>
    bool WPPhysicsShape3T<T>::isAttached() const
    {
        return wp_collision_shape_is_attached( m_shape ) != 0;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setEnabled( bool enabled )
    {
        wp_collision_shape_set_enabled( m_shape, enabled );
    }

    template <class T>
    bool WPPhysicsShape3T<T>::isEnabled() const
    {
        return wp_collision_shape_is_enabled( m_shape ) != 0;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setTrigger( bool trigger )
    {
        wp_collision_shape_set_trigger( m_shape, trigger );
    }

    template <class T>
    bool WPPhysicsShape3T<T>::isTrigger() const
    {
        return wp_collision_shape_is_trigger( m_shape ) != 0;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    template <class T>
    SmartPtr<IStateContext> WPPhysicsShape3T<T>::getStateContext() const
    {
        return m_stateContext;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    template <class T>
    SmartPtr<IStateListener> WPPhysicsShape3T<T>::getStateListener() const
    {
        return m_stateListener;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setCollisionType( u32 mask )
    {
        wp_collision_shape_set_collision_type( m_shape, mask );
    }

    template <class T>
    u32 WPPhysicsShape3T<T>::getCollisionType() const
    {
        return wp_collision_shape_get_collision_type( m_shape );
    }

    template <class T>
    void WPPhysicsShape3T<T>::setCollisionMask( u32 mask )
    {
        wp_collision_shape_set_collision_mask( m_shape, mask );
    }

    template <class T>
    u32 WPPhysicsShape3T<T>::getCollisionMask() const
    {
        return wp_collision_shape_get_collision_mask( m_shape );
    }

    template <class T>
    void WPPhysicsShape3T<T>::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            physicsManager->lock();
        }
    }

    template <class T>
    bool WPPhysicsShape3T<T>::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            return physicsManager->try_lock();
        }

        return false;
    }

    template <class T>
    void WPPhysicsShape3T<T>::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            physicsManager->unlock();
        }
    }

    template <class T>
    u32 WPPhysicsShape3T<T>::getType() const
    {
        return m_type;
    }

    template <class T>
    void WPPhysicsShape3T<T>::setType( u32 type )
    {
        m_type = type;
    }

    template class WPPhysicsShape3T<BoxShape3>;
    template class WPPhysicsShape3T<SphereShape>;
    template class WPPhysicsShape3T<PlaneShape>;
    template class WPPhysicsShape3T<MeshShape>;
    template class WPPhysicsShape3T<TerrainShape>;
}
