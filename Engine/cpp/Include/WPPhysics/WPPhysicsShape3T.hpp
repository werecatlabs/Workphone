#ifndef WPPHYSICSSHAPE3ADAPTER_HPP
#define WPPHYSICSSHAPE3ADAPTER_HPP

#include <WPPhysics/WPPhysicsUtil.hpp>

namespace workphone::physics
{
    /** Shares the backend-independent IPhysicsShape3 forwarding code used by concrete C shapes. */
    template <class T>
    class WPPhysicsShape3T : public T
    {
    public:
        WPPhysicsShape3T()
        {
        }

        WPPhysicsShape3T( u32 type )
        {
            setType( type );
        }

        ~WPPhysicsShape3T() override
        {
        }

        void load( SmartPtr<ISharedObject> data ) override
        {
            auto type = getType();
            m_shape( wp_collision_shape_create( type ) );
            if( !m_shape )
            {
                throw std::runtime_error( "Failed to create a WPPhysics collision shape." );
            }
        }

        void unload( SmartPtr<ISharedObject> data ) override
        {
            wp_collision_shape_destroy( m_shape );
            m_shape = nullptr;
        }
        SmartPtr<IPhysicsMaterial3> getMaterial() const
        {
            return m_material;
        }
        void setMaterial( SmartPtr<IPhysicsMaterial3> material )
        {
            m_material = material;
            const auto backendMaterial = dynamic_cast<WPPhysicsMaterial3 *>( material.get() );
            wp_collision_shape_set_material(
                m_shape, backendMaterial ? backendMaterial->getMaterial() : nullptr );
        }
        void setLocalPose( const Transform3<real_Num> &pose )
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
        Transform3<real_Num> getLocalPose() const
        {
            auto pose = Transform3<real_Num>(
                detail::fromWp( wp_collision_shape_get_local_position( m_shape ) ),
                detail::fromWp( wp_collision_shape_get_local_orientation( m_shape ) ) );
            if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
                pose.setScale( m_boxScale );
            return pose;
        }
        void setSimulationFilterData( const FilterData &data )
        {
            wp_filter_data fd = { data.word0, data.word1, data.word2, data.word3 };
            wp_collision_shape_set_filter_data( m_shape, fd );
        }
        FilterData getSimulationFilterData() const
        {
            auto fd = wp_collision_shape_get_filter_data( m_shape );
            return FilterData( fd.word0, fd.word1, fd.word2, fd.word3 );
        }
        void setActor( SmartPtr<IPhysicsBody3> body )
        {
            m_actor = body;
        }
        SmartPtr<IPhysicsBody3> getActor() const
        {
            return m_actor.lock();
        }
        void _getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = m_shape;
            }
        }

        bool hasShapeData() const
        {
            return m_shape != nullptr;
        }

        SmartPtr<IPhysicsShape3> clone()
        {
            SmartPtr<IPhysicsShape3> result;
            const auto type = wp_collision_shape_get_type( m_shape );
            if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
            {
                auto shape = workphone::make_ptr<WPPhysicsSphereShape3>();
                shape->setRadius( getRadius() );
                result = shape;
            }
            else if( type == WORKPHONE_COLLISION_SHAPE_BOX )
            {
                auto shape = workphone::make_ptr<WPPhysicsBoxShape3>();
                shape->setExtents( getExtents() );
                result = shape;
            }
            else
            {
                WP_LOG_ERROR( "clone: the concrete shape must provide a clone." );
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

        Vector3<real_Num> getExtents() const
        {
            if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
                return m_boxExtents;
            return detail::fromWp( wp_collision_shape_get_box_half_extents( m_shape ) ) *
                   static_cast<real_Num>( 2.0 );
        }

        void setExtents( const Vector3<real_Num> &extents )
        {
            auto safeExtents = WPPhysicsUtil::absoluteVector( extents );
            if( wp_collision_shape_get_type( m_shape ) == WORKPHONE_COLLISION_SHAPE_BOX )
            {
                m_boxExtents = safeExtents;
                safeExtents = WPPhysicsUtil::absoluteVector( m_boxExtents * m_boxScale );
            }
            wp_collision_shape_set_box_half_extents(
                m_shape, detail::toWp( safeExtents * static_cast<real_Num>( 0.5 ) ) );
        }

        AABB3<real_Num> getAABB() const
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

        void setAABB( const AABB3<real_Num> &box )
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

        void setRadius( real_Num radius )
        {
            wp_collision_shape_set_sphere_radius( m_shape,
                                                  static_cast<wp_f32>( Math<real_Num>::Abs( radius ) ) );
        }

        real_Num getRadius() const
        {
            return static_cast<real_Num>( wp_collision_shape_get_sphere_radius( m_shape ) );
        }

        wp_collision_shape *getShape() const
        {
            return m_shape;
        }

        bool isAttached() const
        {
            return wp_collision_shape_is_attached( m_shape ) != 0;
        }

        void setEnabled( bool enabled )
        {
            wp_collision_shape_set_enabled( m_shape, enabled );
        }

        bool isEnabled() const
        {
            return wp_collision_shape_is_enabled( m_shape ) != 0;
        }

        void setTrigger( bool trigger )
        {
            wp_collision_shape_set_trigger( m_shape, trigger );
        }

        bool isTrigger() const
        {
            return wp_collision_shape_is_trigger( m_shape ) != 0;
        }
        void setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        SmartPtr<IStateContext> getStateContext() const
        {
            return m_stateContext;
        }

        void setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        SmartPtr<IStateListener> getStateListener() const
        {
            return m_stateListener;
        }

        void setCollisionType( u32 mask )
        {
            wp_collision_shape_set_collision_type( m_shape, mask );
        }

        u32 getCollisionType() const
        {
            return wp_collision_shape_get_collision_type( m_shape );
        }

        void setCollisionMask( u32 mask )
        {
            wp_collision_shape_set_collision_mask( m_shape, mask );
        }

        u32 getCollisionMask() const
        {
            return wp_collision_shape_get_collision_mask( m_shape );
        }

        void lock() override
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                physicsManager->lock();
            }
        }

        bool try_lock() override
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                return physicsManager->try_lock();
            }

            return false;
        }

        void unlock() override
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                physicsManager->unlock();
            }
        }

        u32 getType() const
        {
            return m_type;
        }

        void setType( u32 type )
        {
            m_type = type;
        }

    protected:
        u32 m_type = 0;  ///< The type of the shape, as defined by the physics engine.
        wp_collision_shape *m_shape =
            nullptr;  ///< Internal pointer to the physics engine's collision shape.
        SmartPtr<IPhysicsMaterial3> m_material;  ///< The material assigned to this shape.
        WeakPtr<IPhysicsBody3> m_actor;  ///< Weak reference to the physics body owning this shape.
        SmartPtr<IStateContext> m_stateContext;    ///< Context for state management.
        SmartPtr<IStateListener> m_stateListener;  ///< Listener for state change notifications.
        AABB3<real_Num> m_aabb;                    ///< Cached axis-aligned bounding box.
        Vector3<real_Num> m_boxExtents = Vector3<real_Num>::unit();
        Vector3<real_Num> m_boxScale = Vector3<real_Num>::unit();
    };
}  // namespace workphone::physics

#endif
