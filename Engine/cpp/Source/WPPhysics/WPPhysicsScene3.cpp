#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsScene3.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <stdexcept>

namespace workphone::physics
{

    WPPhysicsScene3::WPPhysicsScene3() : m_scene( wp_physics_scene_create() )
    {
        if( !m_scene )
        {
            throw std::runtime_error( "Failed to create a WPPhysics scene." );
        }
    }

    WPPhysicsScene3::~WPPhysicsScene3()
    {
        clear();
        wp_physics_scene_destroy( m_scene );
        m_scene = nullptr;
    }

    void WPPhysicsScene3::update()
    {
        ScopedLoadLock loadLock( this );
        if( !loadLock.isLoaded() )
        {
            return;
        }

        TryLockGuard lock( this );
        if( !lock.locked() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager || !applicationManager->isPlaying() )
        {
            return;
        }

        auto timer = applicationManager->getTimer();
        if( !timer )
        {
            return;
        }

        auto elapsedTime = timer->getDeltaTime();
        if( !Math<time_interval>::isFinite( elapsedTime ) ||
            elapsedTime <= Math<time_interval>::epsilon() )
        {
            return;
        }

        /*
         * Limit catch-up work to two 60 Hz substeps. A larger clamp creates a
         * feedback loop after a hitch: the longer step generates more collision
         * work, which delays the physics task again and makes the next step just
         * as expensive.
         */
        elapsedTime = std::min( elapsedTime, static_cast<time_interval>( 1.0 / 30.0 ) );
        simulate( static_cast<real_Num>( elapsedTime ), nullptr, 0, true );
        fetchResults( true, nullptr );
    }

    void WPPhysicsScene3::clear()
    {
        for( auto &actor : m_actors )
        {
            if( actor && actor->getScene().get() == this )
            {
                actor->setScene( nullptr );
            }
        }
        wp_physics_scene_clear( m_scene );
        m_actors.clear();
        m_lastPublishedTransforms.clear();
    }

    void WPPhysicsScene3::addActor( SmartPtr<IPhysicsBody3> body )
    {
        ScopedLock lock( this );
        if( !body || hasActor( body ) )
        {
            return;
        }
        if( auto currentScene = body->getScene() )
        {
            if( currentScene.get() != this )
            {
                WP_LOG_WARNING( "WPPhysicsScene3::addActor: actor already belongs to another scene." );
                return;
            }
        }

        void *raw = nullptr;
        body->_getObject( &raw );
        if( raw && wp_physics_scene_add_actor( m_scene, static_cast<wp_rigidbody *>( raw ) ) != 0 )
        {
            m_actors.push_back( body );
            m_lastPublishedTransforms.erase( body.get() );
            body->setScene( getSharedFromThis<WPPhysicsScene3>() );
        }
    }

    void WPPhysicsScene3::removeActor( SmartPtr<IPhysicsBody3> body )
    {
        if( !body )
        {
            return;
        }

        ScopedLock lock( this );

        void *raw = nullptr;
        body->_getObject( &raw );
        if( raw )
        {
            wp_physics_scene_remove_actor( m_scene, static_cast<wp_rigidbody *>( raw ) );
        }
        const auto it = std::find( m_actors.begin(), m_actors.end(), body );
        if( it != m_actors.end() )
        {
            m_lastPublishedTransforms.erase( body.get() );
            m_actors.erase( it );
            if( body->getScene().get() == this )
            {
                body->setScene( nullptr );
            }
        }
    }

    Array<SmartPtr<IPhysicsBody3>> WPPhysicsScene3::getActors() const
    {
        return m_actors.snapshot();
    }

    bool WPPhysicsScene3::hasActor( SmartPtr<IPhysicsBody3> body ) const
    {
        for( auto &actor : m_actors )
        {
            if( actor == body )
            {
                return true;
            }
        }

        return false;
    }

    u32 WPPhysicsScene3::numDynamicActors() const
    {
        u32 n = 0;
        for( auto &actor : m_actors )
        {
            void *raw = nullptr;
            actor->_getObject( &raw );
            if( raw && wp_rigidbody_get_type( static_cast<wp_rigidbody *>( raw ) ) !=
                           WORKPHONE_RIGIDBODY_STATIC )
                ++n;
        }

        return n;
    }

    u32 WPPhysicsScene3::numStaticActors() const
    {
        return static_cast<u32>( m_actors.size() ) - numDynamicActors();
    }

    void WPPhysicsScene3::setSize( const Vector3<real_Num> &size )
    {
        wp_physics_scene_set_size( m_scene, detail::toWp( size ) );
    }

    Vector3<real_Num> WPPhysicsScene3::getSize() const
    {
        return detail::fromWp( wp_physics_scene_get_size( m_scene ) );
    }

    bool WPPhysicsScene3::rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                                  Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                  u32 collisionType, u32 collisionMask )
    {
        wp_vec3f p, n;
        auto hit = wp_physics_scene_ray_test( m_scene, detail::toWp( start ), detail::toWp( direction ),
                                              &p, &n, collisionType, collisionMask ) != 0;
        hitPos = hit ? detail::fromWp( p ) : Vector3<real_Num>::zero();
        hitNormal = hit ? detail::fromWp( n ) : Vector3<real_Num>::zero();
        return hit;
    }

    bool WPPhysicsScene3::intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                     Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                     SmartPtr<ISharedObject> &object, u32 collisionType,
                                     u32 collisionMask )
    {
        object = nullptr;
        wp_vec3f      p, n;
        wp_rigidbody *hitBody = nullptr;
        auto          hit =
            wp_physics_scene_intersects_ex( m_scene, detail::toWp( start ), detail::toWp( end ), &p, &n,
                                            &hitBody, nullptr, collisionType, collisionMask ) != 0;
        hitPos = hit ? detail::fromWp( p ) : Vector3<real_Num>::zero();
        hitNormal = hit ? detail::fromWp( n ) : Vector3<real_Num>::zero();

        if( hitBody )
        {
            for( auto &actor : m_actors )
            {
                void *nativeActor = nullptr;
                actor->_getObject( &nativeActor );
                if( nativeActor == hitBody )
                {
                    object = actor;
                    break;
                }
            }
        }
        return hit;
    }

    bool WPPhysicsScene3::castRay( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                                  Array<SmartPtr<IRaycastHit>> &hits )
    {
        ScopedLock lock( this );

        hits.clear();
        auto hit = workphone::make_ptr<RaycastHit>();
        if( castRay( Ray3<real_Num>( origin, dir ), hit ) )
        {
            hits.push_back( hit );
            return true;
        }

        return false;
    }

    bool WPPhysicsScene3::castRay( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit )
    {
        ScopedLock lock( this );

        if( !hit || !ray.isValid() || ray.getDirection().lengthSquared() <= Math<real_Num>::epsilon() )
        {
            return false;
        }

        auto actorTypes = static_cast<wp_u32>( 0 );
        if( hit->getCheckStatic() )
        {
            actorTypes |= WORKPHONE_SCENE_QUERY_STATIC;
        }
        if( hit->getCheckDynamic() )
        {
            actorTypes |= WORKPHONE_SCENE_QUERY_DYNAMIC | WORKPHONE_SCENE_QUERY_KINEMATIC;
        }

        hit->setCollider( nullptr );
        hit->setRigidBody( nullptr );
        hit->setPoint( Vector3<real_Num>::zero() );
        hit->setNormal( Vector3<real_Num>::zero() );
        hit->setDistance( static_cast<real_Num>( 0 ) );
        hit->setTriangleIndex( -1 );
        if( actorTypes == 0u )
        {
            return false;
        }

        auto direction = ray.getDirection();
        direction.normalise();
        const auto          end = ray.getOrigin() + direction * static_cast<real_Num>( 100000.0 );
        wp_vec3f            point;
        wp_vec3f            normal;
        wp_rigidbody       *nativeBody = nullptr;
        wp_collision_shape *nativeShape = nullptr;
        const auto          collisionMask = hit->getCollisionMask();
        const auto          didHit = wp_physics_scene_intersects_actor_types_ex(
                                         m_scene, detail::toWp( ray.getOrigin() ), detail::toWp( end ), &point,
                                         &normal, &nativeBody, &nativeShape, 0, collisionMask, actorTypes ) != 0;
        if( !didHit )
        {
            return false;
        }

        hit->setPoint( detail::fromWp( point ) );
        hit->setNormal( detail::fromWp( normal ) );
        hit->setDistance( ( hit->getPoint() - ray.getOrigin() ).length() );

        for( const auto &actor : m_actors )
        {
            if( !actor )
            {
                continue;
            }

            void *candidateBody = nullptr;
            actor->_getObject( &candidateBody );
            if( candidateBody != nativeBody )
            {
                continue;
            }

            if( auto rigidBody = workphone::dynamic_pointer_cast<IRigidBody3>( actor ) )
            {
                hit->setRigidBody( rigidBody );
                for( const auto &shape : rigidBody->getShapes() )
                {
                    void *candidateShape = nullptr;
                    if( shape )
                    {
                        shape->_getObject( &candidateShape );
                    }
                    if( candidateShape == nativeShape )
                    {
                        hit->setCollider( shape );
                        break;
                    }
                }
            }
            break;
        }
        return true;
    }

    bool WPPhysicsScene3::castRayDynamic( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit )
    {
        if( !hit )
        {
            return false;
        }

        ScopedLock lock( this );

        const auto checkStatic = hit->getCheckStatic();
        const auto checkDynamic = hit->getCheckDynamic();
        hit->setCheckStatic( false );
        hit->setCheckDynamic( true );
        const auto result = castRay( ray, hit );
        hit->setCheckStatic( checkStatic );
        hit->setCheckDynamic( checkDynamic );
        return result;
    }

    void WPPhysicsScene3::setGravity( const Vector3<real_Num> &vec )
    {
        wp_physics_scene_set_gravity( m_scene, detail::toWp( vec ) );
    }

    Vector3<real_Num> WPPhysicsScene3::getGravity() const
    {
        return detail::fromWp( wp_physics_scene_get_gravity( m_scene ) );
    }

    void WPPhysicsScene3::simulate( real_Num elapsedTime, void *, u32, bool )
    {
        wp_physics_scene_simulate( m_scene, static_cast<wp_f32>( elapsedTime ) );
    }

    bool WPPhysicsScene3::fetchResults( bool block, u32 *errorState )
    {
        if( errorState )
        {
            *errorState = 0;
        }

        const auto fetched = wp_physics_scene_fetch_results( m_scene, block ) != 0;
        if( fetched )
        {
            publishActiveTransforms();
        }
        return fetched;
    }

    u32 WPPhysicsScene3::getMinThreads() const
    {
        return wp_physics_scene_get_min_threads( m_scene );
    }

    void WPPhysicsScene3::setMinThreads( u32 minThreads )
    {
        wp_physics_scene_set_min_threads( m_scene, minThreads );
    }

    u32 WPPhysicsScene3::getMaxThreads() const
    {
        return wp_physics_scene_get_max_threads( m_scene );
    }

    void WPPhysicsScene3::setMaxThreads( u32 maxThreads )
    {
        wp_physics_scene_set_max_threads( m_scene, maxThreads );
    }

    wp_physics_scene *WPPhysicsScene3::getScene() const
    {
        return m_scene;
    }

    void WPPhysicsScene3::publishActiveTransforms()
    {
        /*
         * Listeners are user code and may remove actors. Iterate a snapshot
         * so those callbacks cannot invalidate the traversal.
         */
        const auto actors = m_actors;
        for( const auto &actor : actors )
        {
            if( !actor )
            {
                continue;
            }

            void *nativeActor = nullptr;
            actor->_getObject( &nativeActor );
            if( !nativeActor )
            {
                continue;
            }

            const auto *body = static_cast<const wp_rigidbody *>( nativeActor );
            if( wp_rigidbody_get_type( body ) == WORKPHONE_RIGIDBODY_STATIC ||
                !wp_rigidbody_has_flag( body, WORKPHONE_RIGIDBODY_FLAG_ENABLED ) )
            {
                continue;
            }

            const auto transform = actor->getTransform();
            const auto published = m_lastPublishedTransforms.find( actor.get() );
            if( published != m_lastPublishedTransforms.end() && published->second == transform )
            {
                continue;
            }
            m_lastPublishedTransforms[actor.get()] = transform;

            if( auto stateContext = actor->getStateContext() )
            {
                if( auto state = stateContext->getStateData<PhysicsBodyState>() )
                {
                    state->transform = transform;
                }
            }

            auto arguments = Parameters();
            arguments.resize( 2 );
            arguments[0].setVector3( transform.getPosition() );
            arguments[1].setQuaternion( transform.getOrientation() );

            auto listeners = actor->getObjectListeners();
            for( auto &listener : listeners )
            {
                if( listener )
                {
                    listener->handleEvent( EventType::Scene, IEvent::transform, arguments, actor,
                                           nullptr, nullptr );
                }
            }
        }
    }

    void WPPhysicsScene3::setSpatialPartitioning( SpatialPartitioningMethodEnum method )
    {
        wp_physics_scene_set_spatial_partitioning(
            m_scene, static_cast<wp_spatial_partitioning_method>( method ) );
    }

    SpatialPartitioningMethodEnum WPPhysicsScene3::getSpatialPartitioning() const
    {
        return static_cast<SpatialPartitioningMethodEnum>(
            wp_physics_scene_get_spatial_partitioning( m_scene ) );
    }

    void WPPhysicsScene3::setSpatialOptions( const SpatialPartitioningOptions &options )
    {
        // Note: C API doesn't have wp_physics_scene_set_spatial_options
        // This would need to be implemented in the C library
        (void)options;
    }

    SpatialPartitioningOptions WPPhysicsScene3::getSpatialOptions() const
    {
        // Note: C API doesn't have a getter for spatial options
        return SpatialPartitioningOptions();
    }

    void WPPhysicsScene3::setContactOptions( const ContactOptions &options )
    {
        // Note: C API doesn't have wp_physics_scene_set_contact_options
        // This would need to be implemented in the C library
        (void)options;
    }

    ContactOptions WPPhysicsScene3::getContactOptions() const
    {
        // Note: C API doesn't have wp_physics_scene_get_contact_options
        // This would need to be implemented in the C library
        return ContactOptions();
    }
} // namespace workphone::physics
