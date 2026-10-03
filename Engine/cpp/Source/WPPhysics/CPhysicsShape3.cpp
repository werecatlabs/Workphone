#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CBoxShape3.hpp>
#include <WPPhysics/CPhysicsMaterial3.hpp>
#include <WPPhysics/CPhysicsShape3.hpp>
#include <WPPhysics/CSphereShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    namespace
    {
        AABB3<real_Num> transformBounds( const AABB3<real_Num>      &bounds,
                                         const Transform3<real_Num> &transform )
        {
            if( bounds.isInfinite() )
            {
                auto result = AABB3<real_Num>();
                result.setInfinite();
                return result;
            }
            if( bounds.isNull() )
            {
                auto result = AABB3<real_Num>();
                result.setNull();
                return result;
            }

            Vector3<real_Num> corners[8];
            bounds.getEdges( corners );
            auto result = AABB3<real_Num>( transform.transformPoint( corners[0] ) );
            for( u32 i = 1; i < 8; ++i )
            {
                result.merge( transform.transformPoint( corners[i] ) );
            }
            return result;
        }

        Vector3<real_Num> absoluteVector( const Vector3<real_Num> &value )
        {
            return Vector3<real_Num>( Math<real_Num>::Abs( value.X() ), Math<real_Num>::Abs( value.Y() ),
                                      Math<real_Num>::Abs( value.Z() ) );
        }
    } // namespace

    CPhysicsShape3::CPhysicsShape3( wp_collision_shape_type type ) :
        m_shape( wp_collision_shape_create( type ) )
    {
        if( !m_shape )
        {
            throw std::runtime_error( "Failed to create a WPPhysics collision shape." );
        }
    }
    CPhysicsShape3::~CPhysicsShape3()
    {
        wp_collision_shape_destroy( m_shape );
        m_shape = nullptr;
    }
    SmartPtr<IPhysicsMaterial3> CPhysicsShape3::getMaterial() const
    {
        return m_material;
    }
    void CPhysicsShape3::setMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        m_material = material;
        const auto backendMaterial = dynamic_cast<CPhysicsMaterial3 *>( material.get() );
        wp_collision_shape_set_material( m_shape,
                                         backendMaterial ? backendMaterial->getMaterial() : nullptr );
    }
    void CPhysicsShape3::setLocalPose( const Transform3<real_Num> &pose )
    {
        wp_collision_shape_set_local_position( m_shape, detail::toWp( pose.getPosition() ) );
        wp_collision_shape_set_local_orientation( m_shape, detail::toWp( pose.getOrientation() ) );
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            // The native API takes dimensions, so bake actor scale into the geometry.
            // Keep the original extents to avoid compounding scale on later updates/clones.
            m_boxScale = pose.getScale();
            setExtents( m_boxExtents );
        }
    }
    Transform3<real_Num> CPhysicsShape3::getLocalPose() const
    {
        auto pose = Transform3<real_Num>(
            detail::fromWp( wp_collision_shape_get_local_position( m_shape ) ),
            detail::fromWp( wp_collision_shape_get_local_orientation( m_shape ) ) );
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
            pose.setScale( m_boxScale );
        return pose;
    }
    void CPhysicsShape3::setSimulationFilterData( const FilterData &data )
    {
        wp_filter_data fd = { data.word0, data.word1, data.word2, data.word3 };
        wp_collision_shape_set_filter_data( m_shape, fd );
    }
    FilterData CPhysicsShape3::getSimulationFilterData() const
    {
        auto fd = wp_collision_shape_get_filter_data( m_shape );
        return FilterData( fd.word0, fd.word1, fd.word2, fd.word3 );
    }
    void CPhysicsShape3::setActor( SmartPtr<IPhysicsBody3> body )
    {
        m_actor = body;
    }
    SmartPtr<IPhysicsBody3> CPhysicsShape3::getActor() const
    {
        return m_actor.lock();
    }
    void CPhysicsShape3::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_shape;
        }
    }
    bool CPhysicsShape3::hasShapeData() const
    {
        return m_shape != nullptr;
    }
    SmartPtr<IPhysicsShape3> CPhysicsShape3::clone()
    {
        SmartPtr<IPhysicsShape3> result;
        const auto               type = wp_collision_shape_get_type( m_shape );
        if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
        {
            auto shape = workphone::make_ptr<CSphereShape3>();
            shape->setRadius( getRadius() );
            result = shape;
        }
        else if( type == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            auto shape = workphone::make_ptr<CBoxShape3>();
            shape->setExtents( getExtents() );
            result = shape;
        }
        else
        {
            WP_LOG_ERROR( "CPhysicsShape3::clone: the concrete shape must provide a clone." );
            return nullptr;
        }

        result->setLocalPose( getLocalPose() );
        result->setSimulationFilterData( getSimulationFilterData() );
        result->setCollisionType( getCollisionType() );
        result->setCollisionMask( getCollisionMask() );
        result->setMaterial( m_material );
        result->setEnabled( isEnabled() );
        result->setTrigger( isTrigger() );
        return result;
    }
    Vector3<real_Num> CPhysicsShape3::getExtents() const
    {
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
            return m_boxExtents;
        return detail::fromWp( wp_collision_shape_get_box_half_extents( m_shape ) ) *
               static_cast<real_Num>( 2.0 );
    }
    void CPhysicsShape3::setExtents( const Vector3<real_Num> &extents )
    {
        auto safeExtents = absoluteVector( extents );
        if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            m_boxExtents = safeExtents;
            safeExtents = absoluteVector( m_boxExtents * m_boxScale );
        }
        wp_collision_shape_set_box_half_extents(
            m_shape, detail::toWp( safeExtents * static_cast<real_Num>( 0.5 ) ) );
    }
    AABB3<real_Num> CPhysicsShape3::getAABB() const
    {
        AABB3<real_Num> bounds;
        const auto      type = wp_collision_shape_get_type( m_shape );
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

        return transformBounds( bounds, getLocalPose() );
    }
    void CPhysicsShape3::setAABB( const AABB3<real_Num> &box )
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
    void CPhysicsShape3::setRadius( real_Num radius )
    {
        wp_collision_shape_set_sphere_radius( m_shape,
                                              static_cast<wp_f32>( Math<real_Num>::Abs( radius ) ) );
    }
    real_Num CPhysicsShape3::getRadius() const
    {
        return static_cast<real_Num>( wp_collision_shape_get_sphere_radius( m_shape ) );
    }
    wp_collision_shape *CPhysicsShape3::getShape() const
    {
        return m_shape;
    }
    bool CPhysicsShape3::isAttached() const
    {
        return wp_collision_shape_is_attached( m_shape ) != 0;
    }
    void CPhysicsShape3::setEnabled( bool enabled )
    {
        wp_collision_shape_set_enabled( m_shape, enabled );
    }
    bool CPhysicsShape3::isEnabled() const
    {
        return wp_collision_shape_is_enabled( m_shape ) != 0;
    }
    void CPhysicsShape3::setTrigger( bool trigger )
    {
        wp_collision_shape_set_trigger( m_shape, trigger );
    }
    bool CPhysicsShape3::isTrigger() const
    {
        return wp_collision_shape_is_trigger( m_shape ) != 0;
    }
    void CPhysicsShape3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }
    SmartPtr<IStateContext> CPhysicsShape3::getStateContext() const
    {
        return m_stateContext;
    }
    void CPhysicsShape3::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }
    SmartPtr<IStateListener> CPhysicsShape3::getStateListener() const
    {
        return m_stateListener;
    }
    void CPhysicsShape3::setCollisionType( u32 mask )
    {
        wp_collision_shape_set_collision_type( m_shape, mask );
    }
    u32 CPhysicsShape3::getCollisionType() const
    {
        return wp_collision_shape_get_collision_type( m_shape );
    }
    void CPhysicsShape3::setCollisionMask( u32 mask )
    {
        wp_collision_shape_set_collision_mask( m_shape, mask );
    }
    u32 CPhysicsShape3::getCollisionMask() const
    {
        return wp_collision_shape_get_collision_mask( m_shape );
    }
    bool CPhysicsShape3::handleStateChanged( const SmartPtr<IStateMessage> & )
    {
        return false;
    }
    bool CPhysicsShape3::handleStateChanged( SmartPtr<IState> & )
    {
        return false;
    }
} // namespace workphone::physics
