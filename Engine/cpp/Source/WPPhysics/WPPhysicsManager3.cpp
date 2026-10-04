#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsBoxShape3.hpp>
#include <WPPhysics/WPPhysicsManager3.hpp>
#include <WPPhysics/WPPhysicsVehicle3.hpp>
#include <WPPhysics/WPPhysicsMaterial3.hpp>
#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <WPPhysics/WPPhysicsMeshShape3.hpp>
#include <WPPhysics/WPPhysicsPlaneShape3.hpp>
#include <WPPhysics/WPPhysicsRigidDynamic3.hpp>
#include <WPPhysics/WPPhysicsRigidStatic3.hpp>
#include <WPPhysics/WPPhysicsScene3.hpp>
#include <WPPhysics/WPPhysicsSphereShape3.hpp>
#include <WPPhysics/WPPhysicsTerrainShape3.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Physics/CapsuleController.hpp>
#include <Workphone/Physics/ConstraintD6.hpp>
#include <Workphone/Physics/ConstraintDrive.hpp>
#include <Workphone/Physics/ConstraintFixed3.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicleWheel3.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

extern "C"
{
#include <WorkphonePhysics/workphone_physics_constraint.h>
}

namespace workphone::physics
{
    namespace
    {
        template <class TBase>
        class WPPhysicsConstraint3Base : public TBase
        {
        public:
            explicit WPPhysicsConstraint3Base( wp_constraint_type type ) :
                m_constraint( wp_constraint_create( type ) )
            {
                if( !m_constraint )
                {
                    throw std::runtime_error( "Failed to create a WPPhysics constraint." );
                }
            }

            ~WPPhysicsConstraint3Base() override
            {
                wp_constraint_destroy( m_constraint );
                m_constraint = nullptr;
            }

            void load( SmartPtr<ISharedObject> ) override
            {
                this->setLoadingState( LoadingState::Loaded );
            }

            void unload( SmartPtr<ISharedObject> ) override
            {
                m_bodyA = nullptr;
                m_bodyB = nullptr;
                wp_constraint_set_body_a( m_constraint, nullptr );
                wp_constraint_set_body_b( m_constraint, nullptr );
                this->setLoadingState( LoadingState::Unloaded );
            }

            SmartPtr<IPhysicsBody3> getBodyA() const override
            {
                return m_bodyA;
            }

            void setBodyA( SmartPtr<IPhysicsBody3> body ) override
            {
                m_bodyA = body;
                wp_constraint_set_body_a( m_constraint, getNativeBody( body ) );
            }

            SmartPtr<IPhysicsBody3> getBodyB() const override
            {
                return m_bodyB;
            }

            void setBodyB( SmartPtr<IPhysicsBody3> body ) override
            {
                m_bodyB = body;
                wp_constraint_set_body_b( m_constraint, getNativeBody( body ) );
            }

            void setLocalPose( JointActorIndexEnum         actor,
                               const Transform3<real_Num> &localPose ) override
            {
                const auto index = static_cast<wp_s32>( actor );
                if( index < 0 || index >= 2 )
                {
                    WP_LOG_WARNING( "WPPhysicsConstraint3Base::setLocalPose: invalid actor index." );
                    return;
                }

                wp_constraint_set_local_position( m_constraint, index,
                                                  detail::toWp( localPose.getPosition() ) );
                wp_constraint_set_local_orientation( m_constraint, index,
                                                     detail::toWp( localPose.getOrientation() ) );
            }

            Transform3<real_Num> getLocalPose( JointActorIndexEnum actor ) const override
            {
                const auto index = static_cast<wp_s32>( actor );
                if( index < 0 || index >= 2 )
                {
                    return Transform3<real_Num>::identity();
                }

                return Transform3<real_Num>(
                    detail::fromWp( wp_constraint_get_local_position( m_constraint, index ) ),
                    detail::fromWp( wp_constraint_get_local_orientation( m_constraint, index ) ) );
            }

            void setConstraintFlag( ConstraintFlagEnum flag, bool value ) override
            {
                wp_constraint_set_flag( m_constraint, static_cast<wp_u32>( flag ), value ? 1 : 0 );
            }

            ConstraintFlagEnum getConstraintFlags() const override
            {
                return static_cast<ConstraintFlagEnum>( wp_constraint_get_flags( m_constraint ) );
            }

            void setBreakForce( real_Num force, real_Num torque ) override
            {
                wp_constraint_set_break_force( m_constraint, static_cast<wp_f32>( force ),
                                               static_cast<wp_f32>( torque ) );
            }

            void getBreakForce( real_Num &force, real_Num &torque ) const override
            {
                force = static_cast<real_Num>( wp_constraint_get_break_force( m_constraint ) );
                torque = static_cast<real_Num>( wp_constraint_get_break_torque( m_constraint ) );
            }

            void setProjectionLinearTolerance( real_Num tolerance ) override
            {
                wp_constraint_set_projection_linear_tolerance( m_constraint,
                                                               static_cast<wp_f32>( tolerance ) );
            }

            real_Num getProjectionLinearTolerance() const override
            {
                return static_cast<real_Num>(
                    wp_constraint_get_projection_linear_tolerance( m_constraint ) );
            }

            void setProjectionAngularTolerance( real_Num tolerance ) override
            {
                wp_constraint_set_projection_angular_tolerance( m_constraint,
                                                                static_cast<wp_f32>( tolerance ) );
            }

            real_Num getProjectionAngularTolerance() const override
            {
                return static_cast<real_Num>(
                    wp_constraint_get_projection_angular_tolerance( m_constraint ) );
            }

        protected:
            wp_constraint *getConstraint() const
            {
                return m_constraint;
            }

        private:
            static wp_rigidbody *getNativeBody( const SmartPtr<IPhysicsBody3> &body )
            {
                if( !body )
                {
                    return nullptr;
                }

                if( !dynamic_cast<WPPhysicsRigidDynamic3 *>( body.get() ) &&
                    !dynamic_cast<WPPhysicsRigidStatic3 *>( body.get() ) )
                {
                    return nullptr;
                }

                void *nativeBody = nullptr;
                body->_getObject( &nativeBody );
                return static_cast<wp_rigidbody *>( nativeBody );
            }

            wp_constraint          *m_constraint = nullptr;
            SmartPtr<IPhysicsBody3> m_bodyA;
            SmartPtr<IPhysicsBody3> m_bodyB;
        };

        class WPPhysicsNativeConstraintD6 final : public WPPhysicsConstraint3Base<ConstraintD6>
        {
        public:
            WPPhysicsNativeConstraintD6() : WPPhysicsConstraint3Base<ConstraintD6>( WORKPHONE_CONSTRAINT_D6 )
            {
            }

            void unload( SmartPtr<ISharedObject> data ) override
            {
                for( auto &drive : m_drives )
                {
                    drive = nullptr;
                }
                m_linearLimit = nullptr;
                WPPhysicsConstraint3Base<ConstraintD6>::unload( data );
            }

            void setDrivePosition( const Transform3<real_Num> &pose ) override
            {
                wp_constraint_set_drive_position( getConstraint(), detail::toWp( pose.getPosition() ) );
                wp_constraint_set_drive_orientation( getConstraint(),
                                                     detail::toWp( pose.getOrientation() ) );
            }

            Transform3<real_Num> getDrivePosition() const override
            {
                return Transform3<real_Num>(
                    detail::fromWp( wp_constraint_get_drive_position( getConstraint() ) ),
                    detail::fromWp( wp_constraint_get_drive_orientation( getConstraint() ) ) );
            }

            void setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive ) override
            {
                const auto i = static_cast<size_t>( index );
                if( i >= m_drives.size() )
                {
                    WP_LOG_WARNING( "WPPhysicsNativeConstraintD6::setDrive: invalid drive index." );
                    return;
                }

                m_drives[i] = drive;
                auto description = wp_constraint_drive_desc{};
                if( drive )
                {
                    description.stiffness = static_cast<wp_f32>( drive->getStiffness() );
                    description.damping = static_cast<wp_f32>( drive->getDamping() );
                    description.force_limit = static_cast<wp_f32>( drive->getForceLimit() );
                    description.is_acceleration = drive->isAcceleration() ? 1 : 0;
                }
                wp_constraint_set_drive( getConstraint(), static_cast<wp_d6_drive>( index ),
                                         description );
            }

            SmartPtr<IConstraintDrive> getDrive( D6DriveEnum index ) const override
            {
                const auto i = static_cast<size_t>( index );
                return i < m_drives.size() ? m_drives[i] : nullptr;
            }

            void setLinearLimit( SmartPtr<IConstraintLinearLimit> limit ) override
            {
                m_linearLimit = limit;
                auto description = wp_constraint_linear_limit{};
                if( limit )
                {
                    description.value = static_cast<wp_f32>( limit->getValue() );
                    description.restitution = static_cast<wp_f32>( limit->getRestitution() );
                    description.bounce_threshold = static_cast<wp_f32>( limit->getBounceThreshold() );
                    description.stiffness = static_cast<wp_f32>( limit->getStiffness() );
                    description.damping = static_cast<wp_f32>( limit->getDamping() );
                    description.contact_distance = static_cast<wp_f32>( limit->getContactDistance() );
                }
                wp_constraint_set_linear_limit( getConstraint(), description );
            }

            SmartPtr<IConstraintLinearLimit> getLinearLimit() const override
            {
                return m_linearLimit;
            }

            void setMotion( D6AxisEnum axis, D6MotionEnum type ) override
            {
                if( static_cast<u32>( axis ) >= static_cast<u32>( D6AxisEnum::eCOUNT ) )
                {
                    WP_LOG_WARNING( "WPPhysicsNativeConstraintD6::setMotion: invalid axis." );
                    return;
                }

                wp_constraint_set_motion( getConstraint(), static_cast<wp_d6_axis>( axis ),
                                          static_cast<wp_d6_motion>( type ) );
            }

            D6MotionEnum getMotion( D6AxisEnum axis ) const override
            {
                if( static_cast<u32>( axis ) >= static_cast<u32>( D6AxisEnum::eCOUNT ) )
                {
                    return D6MotionEnum::eLOCKED;
                }

                return static_cast<D6MotionEnum>(
                    wp_constraint_get_motion( getConstraint(), static_cast<wp_d6_axis>( axis ) ) );
            }

        private:
            std::array<SmartPtr<IConstraintDrive>, static_cast<size_t>( D6DriveEnum::eCOUNT )>
                                             m_drives{};
            SmartPtr<IConstraintLinearLimit> m_linearLimit;
        };

        class WPPhysicsConstraintFixed final : public WPPhysicsConstraint3Base<ConstraintFixed3>
        {
        public:
            WPPhysicsConstraintFixed() :
                WPPhysicsConstraint3Base<ConstraintFixed3>( WORKPHONE_CONSTRAINT_FIXED )
            {
            }
        };

        class WPPhysicsNativeConstraintLinearLimit final : public IConstraintLinearLimit
        {
        public:
            real_Num getValue() const override
            {
                return m_value;
            }

            void setValue( real_Num value ) override
            {
                m_value = std::max( value, static_cast<real_Num>( 0.0 ) );
            }

            real_Num getRestitution() const override
            {
                return m_restitution;
            }

            void setRestitution( real_Num restitution ) override
            {
                m_restitution = std::max( static_cast<real_Num>( 0.0 ),
                                          std::min( restitution, static_cast<real_Num>( 1.0 ) ) );
            }

            real_Num getBounceThreshold() const override
            {
                return m_bounceThreshold;
            }

            void setBounceThreshold( real_Num bounceThreshold ) override
            {
                m_bounceThreshold = std::max( bounceThreshold, static_cast<real_Num>( 0.0 ) );
            }

            real_Num getStiffness() const override
            {
                return m_stiffness;
            }

            void setStiffness( real_Num stiffness ) override
            {
                m_stiffness = std::max( stiffness, static_cast<real_Num>( 0.0 ) );
            }

            real_Num getDamping() const override
            {
                return m_damping;
            }

            void setDamping( real_Num damping ) override
            {
                m_damping = std::max( damping, static_cast<real_Num>( 0.0 ) );
            }

            real_Num getContactDistance() const override
            {
                return m_contactDistance;
            }

            void setContactDistance( real_Num contactDistance ) override
            {
                m_contactDistance = std::max( contactDistance, static_cast<real_Num>( 0.0 ) );
            }

        private:
            real_Num m_value = static_cast<real_Num>( 0.0 );
            real_Num m_restitution = static_cast<real_Num>( 0.0 );
            real_Num m_bounceThreshold = static_cast<real_Num>( 0.0 );
            real_Num m_stiffness = static_cast<real_Num>( 0.0 );
            real_Num m_damping = static_cast<real_Num>( 0.0 );
            real_Num m_contactDistance = static_cast<real_Num>( 0.0 );
        };




        SmartPtr<IPhysicsScene3> selectQueryScene( const SmartPtr<IPhysicsScene3>        &raycastScene,
                                                   const SmartPtr<IPhysicsScene3>        &physicsScene,
                                                   const Array<SmartPtr<IPhysicsScene3>> &scenes )
        {
            if( raycastScene )
            {
                return raycastScene;
            }
            if( physicsScene )
            {
                return physicsScene;
            }
            return scenes.empty() ? nullptr : scenes.front();
        }

        void applyRigidBodyProperties( SmartPtr<IRigidBody3>       body,
                                       const SmartPtr<Properties> &properties )
        {
            if( !body || !properties )
            {
                return;
            }

            auto transform = body->getTransform();
            if( properties->getPropertyValue( "transform", transform ) )
            {
                body->setTransform( transform );
            }

            auto position = body->getTransform().getPosition();
            if( properties->getPropertyValue( "position", position ) )
            {
                transform = body->getTransform();
                transform.setPosition( position );
                body->setTransform( transform );
            }

            auto orientation = body->getTransform().getOrientation();
            if( properties->getPropertyValue( "orientation", orientation ) )
            {
                transform = body->getTransform();
                transform.setOrientation( orientation );
                body->setTransform( transform );
            }

            auto mass = static_cast<f32>( body->getMass() );
            if( properties->getPropertyValue( "mass", mass ) && mass > 0.0f )
            {
                body->setMass( static_cast<real_Num>( mass ) );
            }

            auto collisionType = body->getCollisionType();
            if( properties->getPropertyValue( "collisionType", collisionType ) )
            {
                body->setCollisionType( collisionType );
            }

            auto collisionMask = body->getCollisionMask();
            if( properties->getPropertyValue( "collisionMask", collisionMask ) )
            {
                body->setCollisionMask( collisionMask );
            }

            auto enabled = body->isEnabled();
            if( properties->getPropertyValue( "enabled", enabled ) )
            {
                body->setEnabled( enabled );
            }
        }

        void applyShapeProperties( SmartPtr<IPhysicsShape3> shape, const SmartPtr<ISharedObject> &data )
        {
            auto properties = workphone::dynamic_pointer_cast<Properties>( data );
            if( !shape || !properties )
            {
                return;
            }

            auto localPose = shape->getLocalPose();
            if( properties->getPropertyValue( "localPose", localPose ) )
            {
                shape->setLocalPose( localPose );
            }

            auto enabled = shape->isEnabled();
            if( properties->getPropertyValue( "enabled", enabled ) )
            {
                shape->setEnabled( enabled );
            }

            auto trigger = shape->isTrigger();
            if( properties->getPropertyValue( "trigger", trigger ) )
            {
                shape->setTrigger( trigger );
            }

            auto collisionType = shape->getCollisionType();
            if( properties->getPropertyValue( "collisionType", collisionType ) )
            {
                shape->setCollisionType( collisionType );
            }

            auto collisionMask = shape->getCollisionMask();
            if( properties->getPropertyValue( "collisionMask", collisionMask ) )
            {
                shape->setCollisionMask( collisionMask );
            }

            if( auto sphere = workphone::dynamic_pointer_cast<ISphereShape3>( shape ) )
            {
                auto radius = static_cast<f32>( sphere->getRadius() );
                if( properties->getPropertyValue( "radius", radius ) && radius > 0.0f )
                {
                    sphere->setRadius( static_cast<real_Num>( radius ) );
                }
            }
            else if( auto box = workphone::dynamic_pointer_cast<IBoxShape3>( shape ) )
            {
                auto extents = box->getExtents();
                if( properties->getPropertyValue( "extents", extents ) && extents.X() > 0 &&
                    extents.Y() > 0 && extents.Z() > 0 )
                {
                    box->setExtents( extents );
                }
            }
            else if( auto plane = workphone::dynamic_pointer_cast<IPlaneShape3>( shape ) )
            {
                auto normal = plane->getNormal();
                if( properties->getPropertyValue( "normal", normal ) &&
                    normal.lengthSquared() > Math<real_Num>::epsilon() )
                {
                    plane->setNormal( normal );
                }

                auto distance = static_cast<f32>( plane->getDistance() );
                if( properties->getPropertyValue( "distance", distance ) )
                {
                    plane->setDistance( static_cast<real_Num>( distance ) );
                }
            }
            else if( auto mesh = workphone::dynamic_pointer_cast<IMeshShape>( shape ) )
            {
                auto convex = mesh->isConvex();
                if( properties->getPropertyValue( "convex", convex ) )
                {
                    mesh->setConvex( convex );
                }
            }
        }
    } // namespace

    WPPhysicsManager3::WPPhysicsManager3() : m_system( wp_physics_system_create() )
    {
        if( !m_system )
        {
            throw std::runtime_error( "Failed to create the WPPhysics system." );
        }

        wp_physics_system_set_user_data( m_system, this );
    }

    WPPhysicsManager3::~WPPhysicsManager3()
    {
        unload( nullptr );
        wp_physics_system_set_user_data( m_system, nullptr );
        wp_physics_system_destroy( m_system );
        m_system = nullptr;
    }

    void WPPhysicsManager3::load( SmartPtr<ISharedObject> )
    {
        ScopedLock lock( this );
        if( getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void WPPhysicsManager3::unload( SmartPtr<ISharedObject> data )
    {
        Array<SmartPtr<IPhysicsConstraint3>>   constraints;
        Array<SmartPtr<IPhysicsVehicle3>>      vehicles;
        Array<SmartPtr<ICharacterController3>> characters;
        Array<SmartPtr<IRaycastHit>>           raycastHits;
        Array<SmartPtr<IRigidBody3>>           bodies;
        Array<SmartPtr<IPhysicsShape3>>        shapes;
        Array<SmartPtr<IPhysicsMaterial3>>     materials;
        Array<SmartPtr<IPhysicsScene3>>        scenes;

        {
            ScopedLock lock( this );
            if( getLoadingState() == LoadingState::Unloading )
            {
                return;
            }
            if( getLoadingState() == LoadingState::Unloaded && m_constraints.empty() &&
                m_vehicles.empty() && m_characters.empty() && m_raycastHits.empty() &&
                m_bodies.empty() && m_shapes.empty() && m_materials.empty() && m_scenes.empty() )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            constraints.swap( m_constraints );
            vehicles.swap( m_vehicles );
            characters.swap( m_characters );
            raycastHits.swap( m_raycastHits );
            bodies.swap( m_bodies );
            shapes.swap( m_shapes );
            materials.swap( m_materials );
            scenes.swap( m_scenes );

            m_physicsScene = nullptr;
            m_objectsScene = nullptr;
            m_raycastScene = nullptr;
            m_controlsScene = nullptr;
        }

        for( auto &constraint : constraints )
        {
            if( constraint )
            {
                constraint->unload( data );
            }
        }

        for( auto &vehicle : vehicles )
        {
            if( vehicle )
            {
                vehicle->unload( data );
            }
        }

        for( auto &character : characters )
        {
            if( character )
            {
                character->unload( data );
            }
        }

        for( auto &scene : scenes )
        {
            if( !scene )
            {
                continue;
            }

            const auto actors = scene->getActors();
            for( const auto &actor : actors )
            {
                if( actor && scene->hasActor( actor ) )
                {
                    scene->removeActor( actor );
                }
            }
            scene->clear();
            scene->setLoadingState( LoadingState::Unloaded );
        }

        for( auto &body : bodies )
        {
            if( body )
            {
                body->setScene( nullptr );
                body->setLoadingState( LoadingState::Unloaded );
            }
        }

        for( auto &shape : shapes )
        {
            if( shape )
            {
                shape->setActor( nullptr );
                shape->setLoadingState( LoadingState::Unloaded );
            }
        }

        for( auto &material : materials )
        {
            if( material )
            {
                material->setLoadingState( LoadingState::Unloaded );
            }
        }

        for( auto &hit : raycastHits )
        {
            if( hit )
            {
                hit->setLoadingState( LoadingState::Unloaded );
            }
        }

        setLoadingState( LoadingState::Unloaded );
    }

    bool WPPhysicsManager3::getEnableDebugDraw() const
    {
        ScopedLock lock( this );
        return m_system && wp_physics_system_get_debug_draw( m_system ) != 0;
    }

    void WPPhysicsManager3::setEnableDebugDraw( bool enableDebugDraw )
    {
        ScopedLock lock( this );
        if( m_system )
        {
            wp_physics_system_set_debug_draw( m_system, enableDebugDraw ? 1 : 0 );
        }

        PhysicsManager::setEnableDebugDraw( enableDebugDraw );
    }

    void WPPhysicsManager3::debugDraw()
    {
        if( !getEnableDebugDraw() )
        {
            return;
        }

        if( auto debug = getDebugRenderer() )
        {
            Array<SmartPtr<IRigidBody3>> bodies;
            {
                ScopedLock lock( this );
                bodies = m_bodies;
            }

            for( const auto &body : bodies )
            {
                if( body && body->isEnabled() )
                {
                    drawDebugBody( *debug, *body );
                }
            }
        }

        PhysicsManager::debugDraw();
    }

    void WPPhysicsManager3::postUpdate()
    {
        if( getEnableDebugDraw() )
        {
            debugDraw();
        }
    }

    SmartPtr<IPhysicsMaterial3> WPPhysicsManager3::addMaterial()
    {
        try
        {
            auto material = workphone::make_ptr<WPPhysicsMaterial3>();
            if( !material->getMaterial() )
            {
                WP_LOG_ERROR( "WPPhysicsManager3::addMaterial: native allocation failed." );
                return nullptr;
            }

            material->setLoadingState( LoadingState::Loaded );
            ScopedLock lock( this );
            m_materials.push_back( material );
            return material;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    void WPPhysicsManager3::removeMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        if( !material )
        {
            return;
        }

        ScopedLock lock( this );
        const auto it = std::find( m_materials.begin(), m_materials.end(), material );
        if( it != m_materials.end() )
        {
            ( *it )->setLoadingState( LoadingState::Unloaded );
            m_materials.erase( it );
        }
    }

    SmartPtr<IPhysicsScene3> WPPhysicsManager3::addScene()
    {
        try
        {
            auto scene = workphone::make_ptr<WPPhysicsScene3>();
            if( !scene->getScene() )
            {
                WP_LOG_ERROR( "WPPhysicsManager3::addScene: native allocation failed." );
                return nullptr;
            }

            scene->setLoadingState( LoadingState::Loaded );
            ScopedLock lock( this );
            m_scenes.push_back( scene );
            if( !m_physicsScene )
            {
                m_physicsScene = scene;
            }
            return scene;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    void WPPhysicsManager3::removeScene( SmartPtr<IPhysicsScene3> scene )
    {
        if( !scene )
        {
            return;
        }

        {
            ScopedLock lock( this );
            const auto it = std::find( m_scenes.begin(), m_scenes.end(), scene );
            if( it == m_scenes.end() )
            {
                return;
            }

            m_scenes.erase( it );
            if( m_physicsScene == scene )
            {
                m_physicsScene = m_scenes.empty() ? nullptr : m_scenes.front();
            }
            if( m_objectsScene == scene )
            {
                m_objectsScene = nullptr;
            }
            if( m_raycastScene == scene )
            {
                m_raycastScene = nullptr;
            }
            if( m_controlsScene == scene )
            {
                m_controlsScene = nullptr;
            }
        }

        const auto actors = scene->getActors();
        for( const auto &actor : actors )
        {
            if( actor && scene->hasActor( actor ) )
            {
                scene->removeActor( actor );
            }
        }
        scene->clear();
        scene->setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<IPhysicsShape3> WPPhysicsManager3::addCollisionShapeByType( hash64                  type,
                                                                        SmartPtr<ISharedObject> data )
    {
        if( type == 0 )
        {
            WP_LOG_ERROR( "WPPhysicsManager3::addCollisionShapeByType: type hash is zero." );
            return nullptr;
        }

        try
        {
            auto       typeManager = TypeManager::instance();
            const auto sphereType = typeManager ? typeManager->getHash( ISphereShape3::typeInfo() ) : 0;
            const auto boxType = typeManager ? typeManager->getHash( IBoxShape3::typeInfo() ) : 0;
            const auto planeType = typeManager ? typeManager->getHash( IPlaneShape3::typeInfo() ) : 0;
            const auto meshType = typeManager ? typeManager->getHash( IMeshShape::typeInfo() ) : 0;
            const auto terrainType = typeManager ? typeManager->getHash( ITerrainShape::typeInfo() ) : 0;

            SmartPtr<IPhysicsShape3> shape;
            if( type == sphereType || type == ISphereShape3::typeInfo() )
            {
                shape = workphone::make_ptr<WPPhysicsSphereShape3>();
            }
            else if( type == boxType || type == IBoxShape3::typeInfo() )
            {
                shape = workphone::make_ptr<WPPhysicsBoxShape3>();
            }
            else if( type == planeType || type == IPlaneShape3::typeInfo() )
            {
                shape = workphone::make_ptr<WPPhysicsPlaneShape3>();
            }
            else if( type == meshType || type == IMeshShape::typeInfo() )
            {
                shape = workphone::make_ptr<WPPhysicsMeshShape3>();
            }
            else if( type == terrainType || type == ITerrainShape::typeInfo() )
            {
                shape = workphone::make_ptr<WPPhysicsTerrainShape3>();
            }
            else
            {
                WP_LOG_WARNING(
                    "WPPhysicsManager3::addCollisionShapeByType: unsupported shape type hash " +
                    StringUtil::toString( type ) + "." );
                return nullptr;
            }

            if( !shape || !shape->hasShapeData() )
            {
                WP_LOG_ERROR(
                    "WPPhysicsManager3::addCollisionShapeByType: failed to allocate native shape data." );
                return nullptr;
            }

            shape->load( data );
            applyShapeProperties( shape, data );
            shape->setLoadingState( LoadingState::Loaded );

            {
                ScopedLock lock( this );
                m_shapes.push_back( shape );
            }

            return shape;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( ... )
        {
            WP_LOG_ERROR(
                "WPPhysicsManager3::addCollisionShapeByType: unknown exception while creating shape." );
        }

        return nullptr;
    }

    bool WPPhysicsManager3::removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape )
    {
        if( !collisionShape )
        {
            return false;
        }

        Array<SmartPtr<IRigidBody3>> bodies;
        {
            ScopedLock lock( this );
            const auto shapeIt = std::find( m_shapes.begin(), m_shapes.end(), collisionShape );
            if( shapeIt == m_shapes.end() )
            {
                return false;
            }

            bodies = m_bodies;
            m_shapes.erase( shapeIt );
        }

        for( auto &body : bodies )
        {
            if( !body )
            {
                continue;
            }

            const auto bodyShapes = body->getShapes();
            if( std::find( bodyShapes.begin(), bodyShapes.end(), collisionShape ) != bodyShapes.end() )
            {
                body->removeShape( collisionShape );
            }
        }

        collisionShape->setActor( nullptr );
        collisionShape->setLoadingState( LoadingState::Unloaded );
        return true;
    }

    bool WPPhysicsManager3::removePhysicsBody( SmartPtr<IRigidBody3> body )
    {
        if( !body )
        {
            return false;
        }

        Array<SmartPtr<IPhysicsConstraint3>> constraintsToRemove;
        Array<SmartPtr<IPhysicsVehicle3>>    vehiclesToRemove;
        {
            ScopedLock lock( this );
            const auto bodyIt = std::find( m_bodies.begin(), m_bodies.end(), body );
            if( bodyIt == m_bodies.end() )
            {
                return false;
            }

            m_bodies.erase( bodyIt );
            for( const auto &constraint : m_constraints )
            {
                if( constraint && ( constraint->getBodyA().get() == body.get() ||
                                    constraint->getBodyB().get() == body.get() ) )
                {
                    constraintsToRemove.push_back( constraint );
                }
            }

            for( const auto &constraint : constraintsToRemove )
            {
                m_constraints.erase(
                    std::remove( m_constraints.begin(), m_constraints.end(), constraint ),
                    m_constraints.end() );
            }

            for( const auto &vehicle : m_vehicles )
            {
                const auto backendVehicle = dynamic_cast<WPPhysicsVehicle3 *>( vehicle.get() );
                if( backendVehicle && backendVehicle->getChassis().get() == body.get() )
                {
                    vehiclesToRemove.push_back( vehicle );
                }
            }
            for( const auto &vehicle : vehiclesToRemove )
            {
                m_vehicles.erase( std::remove( m_vehicles.begin(), m_vehicles.end(), vehicle ),
                                  m_vehicles.end() );
            }
        }

        for( auto &constraint : constraintsToRemove )
        {
            constraint->unload( nullptr );
        }

        for( auto &vehicle : vehiclesToRemove )
        {
            vehicle->unload( nullptr );
        }

        if( auto scene = body->getScene() )
        {
            if( scene->hasActor( body ) )
            {
                scene->removeActor( body );
            }
            else
            {
                body->setScene( nullptr );
            }
        }

        body->setLoadingState( LoadingState::Unloaded );
        return true;
    }

    SmartPtr<ICharacterController3> WPPhysicsManager3::addCharacter()
    {
        try
        {
            auto character = workphone::make_ptr<CapsuleController>();
            character->load( nullptr );
            if( character->getLoadingState() != LoadingState::Loaded )
            {
                character->setLoadingState( LoadingState::Loaded );
            }

            ScopedLock lock( this );
            m_characters.push_back( character );
            return character;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    SmartPtr<IRigidStatic3> WPPhysicsManager3::addRigidStatic( const Transform3<real_Num> &transform )
    {
        try
        {
            auto body = workphone::make_ptr<WPPhysicsRigidStatic3>();
            body->setTransform( transform );
            body->setLoadingState( LoadingState::Loaded );

            ScopedLock lock( this );
            m_bodies.push_back( body );
            return body;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    SmartPtr<IRigidDynamic3> WPPhysicsManager3::addRigidDynamic( const Transform3<real_Num> &transform )
    {
        try
        {
            auto body = workphone::make_ptr<WPPhysicsRigidDynamic3>( WORKPHONE_RIGIDBODY_DYNAMIC );
            if( !body->getBody() )
            {
                WP_LOG_ERROR( "WPPhysicsManager3::addRigidDynamic: native allocation failed." );
                return nullptr;
            }

            body->setTransform( transform );
            body->setLoadingState( LoadingState::Loaded );
            ScopedLock lock( this );
            m_bodies.push_back( body );
            return body;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    SmartPtr<IRigidStatic3> WPPhysicsManager3::addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape )
    {
        return addRigidStatic( collisionShape, nullptr );
    }

    SmartPtr<IRigidStatic3> WPPhysicsManager3::addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                                              SmartPtr<Properties>     properties )
    {
        ScopedLock lock( this );
        if( collisionShape && !dynamic_cast<WPPhysicsShape3Backend *>( collisionShape.get() ) )
        {
            WP_LOG_ERROR(
                "WPPhysicsManager3::addRigidStatic: shape belongs to another physics backend." );
            return nullptr;
        }

        auto body = addRigidStatic( Transform3<real_Num>::identity() );
        if( !body )
        {
            return nullptr;
        }

        if( collisionShape )
        {
            body->addShape( collisionShape );
        }

        applyRigidBodyProperties( body, properties );
        return body;
    }
    SmartPtr<IPhysicsVehicle3> WPPhysicsManager3::addVehicle( SmartPtr<IRigidBody3> chassis )
    {
        return addVehicle( chassis, nullptr );
    }

    bool WPPhysicsManager3::removeVehicle( SmartPtr<IPhysicsVehicle3> vehicle )
    {
        if( !vehicle )
        {
            return false;
        }

        {
            ScopedLock lock( this );
            const auto it = std::find( m_vehicles.begin(), m_vehicles.end(), vehicle );
            if( it == m_vehicles.end() )
            {
                return false;
            }
            m_vehicles.erase( it );
        }
        vehicle->unload( nullptr );
        return true;
    }

    SmartPtr<IPhysicsVehicle3> WPPhysicsManager3::addVehicle( SmartPtr<IRigidBody3>       chassis,
                                                             const SmartPtr<Properties> &properties )
    {
        if( !chassis )
        {
            WP_LOG_ERROR( "WPPhysicsManager3::addVehicle: chassis is null." );
            return nullptr;
        }
        if( !dynamic_cast<WPPhysicsRigidDynamic3 *>( chassis.get() ) )
        {
            WP_LOG_ERROR(
                "WPPhysicsManager3::addVehicle: chassis must be a WPPhysics dynamic rigid body." );
            return nullptr;
        }

        try
        {
            ScopedLock lock( this );
            if( std::none_of( m_bodies.begin(), m_bodies.end(),
                              [&chassis]( const SmartPtr<IRigidBody3> &body )
                              { return body.get() == chassis.get(); } ) )
            {
                WP_LOG_ERROR( "WPPhysicsManager3::addVehicle: chassis is not managed by this manager." );
                return nullptr;
            }
            if( std::any_of( m_vehicles.begin(), m_vehicles.end(),
                             [&chassis]( const SmartPtr<IPhysicsVehicle3> &vehicle )
                             {
                                 const auto backend = dynamic_cast<WPPhysicsVehicle3 *>( vehicle.get() );
                                 return backend && backend->getChassis().get() == chassis.get();
                             } ) )
            {
                WP_LOG_ERROR( "WPPhysicsManager3::addVehicle: chassis already belongs to a vehicle." );
                return nullptr;
            }

            auto vehicle = workphone::make_ptr<WPPhysicsVehicle3>( chassis );
            u32  wheelCount = 0;
            f32  wheelRadius = 0.35f;
            f32  wheelWidth = 0.25f;
            f32  suspensionTravelCm = 20.0f;
            f32  suspensionForce = 6000.0f;
            f32  suspensionStiffness = 35.0f;
            f32  suspensionDamping = 4.5f;
            f32  frictionSlip = 1.0f;
            u32  materialId = 0;
            bool enabled = true;
            bool finalize = false;

            if( properties )
            {
                properties->getPropertyValue( "wheelCount", wheelCount );
                properties->getPropertyValue( "wheelRadius", wheelRadius );
                properties->getPropertyValue( "wheelWidth", wheelWidth );
                properties->getPropertyValue( "suspensionTravelCm", suspensionTravelCm );
                properties->getPropertyValue( "suspensionForce", suspensionForce );
                properties->getPropertyValue( "suspensionStiffness", suspensionStiffness );
                properties->getPropertyValue( "suspensionDamping", suspensionDamping );
                properties->getPropertyValue( "frictionSlip", frictionSlip );
                properties->getPropertyValue( "materialId", materialId );
                properties->getPropertyValue( "enabled", enabled );
                properties->getPropertyValue( "finalize", finalize );
            }

            wheelCount = std::min<u32>( wheelCount, 32u );
            for( u32 i = 0; i < wheelCount; ++i )
            {
                auto wheel = vehicle->addWheel();
                if( !wheel )
                {
                    break;
                }
                wheel->setRadius( static_cast<real_Num>( wheelRadius ) );
                wheel->setWidth( static_cast<real_Num>( wheelWidth ) );
                wheel->setMaxSuspensionTravelCm( static_cast<real_Num>( suspensionTravelCm ) );
                wheel->setMaxSuspensionForce( static_cast<real_Num>( suspensionForce ) );
                wheel->setSuspensionStiffness( static_cast<real_Num>( suspensionStiffness ) );
                wheel->setSuspensionDamping( static_cast<real_Num>( suspensionDamping ) );
                wheel->setFrictionSlip( static_cast<real_Num>( frictionSlip ) );
            }
            vehicle->setMaterialId( materialId );
            vehicle->setEnabled( enabled );
            if( finalize )
            {
                vehicle->finalize();
            }

            m_vehicles.push_back( vehicle );
            return vehicle;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    bool WPPhysicsManager3::rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                                    Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                    u32 collisionType, u32 collisionMask )
    {
        hitPos = Vector3<real_Num>::zero();
        hitNormal = Vector3<real_Num>::zero();
        if( direction.lengthSquared() <= Math<real_Num>::epsilon() )
        {
            return false;
        }

        SmartPtr<IPhysicsScene3> scene;
        {
            ScopedLock lock( this );
            scene = selectQueryScene( m_raycastScene, m_physicsScene, m_scenes );
        }

        if( !scene )
        {
            return false;
        }

        try
        {
            return scene->rayTest( start, direction, hitPos, hitNormal, collisionType, collisionMask );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            hitPos = Vector3<real_Num>::zero();
            hitNormal = Vector3<real_Num>::zero();
            return false;
        }
    }

    bool WPPhysicsManager3::intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                       Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                       SmartPtr<ISharedObject> &object, u32 collisionType,
                                       u32 collisionMask )
    {
        hitPos = Vector3<real_Num>::zero();
        hitNormal = Vector3<real_Num>::zero();
        object = nullptr;
        if( ( end - start ).lengthSquared() <= Math<real_Num>::epsilon() )
        {
            return false;
        }

        SmartPtr<IPhysicsScene3> scene;
        {
            ScopedLock lock( this );
            scene = selectQueryScene( m_raycastScene, m_physicsScene, m_scenes );
        }

        if( !scene )
        {
            return false;
        }

        try
        {
            return scene->intersects( start, end, hitPos, hitNormal, object, collisionType,
                                      collisionMask );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            hitPos = Vector3<real_Num>::zero();
            hitNormal = Vector3<real_Num>::zero();
            object = nullptr;
            return false;
        }
    }

    SmartPtr<IConstraintD6> WPPhysicsManager3::addConstraintD6( SmartPtr<IPhysicsBody3>     actor0,
                                                               const Transform3<real_Num> &localFrame0,
                                                               SmartPtr<IPhysicsBody3>     actor1,
                                                               const Transform3<real_Num> &localFrame1 )
    {
        if( !actor0 && !actor1 )
        {
            WP_LOG_ERROR( "WPPhysicsManager3::addConstraintD6: at least one actor is required." );
            return nullptr;
        }
        if( actor0 && actor0 == actor1 )
        {
            WP_LOG_ERROR( "WPPhysicsManager3::addConstraintD6: an actor cannot constrain itself." );
            return nullptr;
        }

        try
        {
            ScopedLock lock( this );
            const auto isManagedActor = [this]( const SmartPtr<IPhysicsBody3> &actor )
            {
                if( !actor )
                {
                    return true;
                }
                return std::any_of( m_bodies.begin(), m_bodies.end(),
                                    [&actor]( const SmartPtr<IRigidBody3> &body )
                                    { return body.get() == actor.get(); } );
            };
            if( !isManagedActor( actor0 ) || !isManagedActor( actor1 ) )
            {
                WP_LOG_ERROR( "WPPhysicsManager3::addConstraintD6: actors must belong to this manager." );
                return nullptr;
            }

            auto constraint = workphone::make_ptr<WPPhysicsNativeConstraintD6>();
            constraint->setBodyA( actor0 );
            constraint->setBodyB( actor1 );
            constraint->setLocalPose( JointActorIndexEnum::eACTOR0, localFrame0 );
            constraint->setLocalPose( JointActorIndexEnum::eACTOR1, localFrame1 );
            constraint->load( nullptr );

            m_constraints.push_back( constraint );
            return constraint;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    SmartPtr<IConstraintFixed3> WPPhysicsManager3::addFixedConstraint(
        SmartPtr<IPhysicsBody3> actor0, const Transform3<real_Num> &localFrame0,
        SmartPtr<IPhysicsBody3> actor1, const Transform3<real_Num> &localFrame1 )
    {
        if( !actor0 && !actor1 )
        {
            WP_LOG_ERROR( "WPPhysicsManager3::addFixedConstraint: at least one actor is required." );
            return nullptr;
        }
        if( actor0 && actor0 == actor1 )
        {
            WP_LOG_ERROR( "WPPhysicsManager3::addFixedConstraint: an actor cannot constrain itself." );
            return nullptr;
        }

        try
        {
            ScopedLock lock( this );
            const auto isManagedActor = [this]( const SmartPtr<IPhysicsBody3> &actor )
            {
                if( !actor )
                {
                    return true;
                }
                return std::any_of( m_bodies.begin(), m_bodies.end(),
                                    [&actor]( const SmartPtr<IRigidBody3> &body )
                                    { return body.get() == actor.get(); } );
            };
            if( !isManagedActor( actor0 ) || !isManagedActor( actor1 ) )
            {
                WP_LOG_ERROR(
                    "WPPhysicsManager3::addFixedConstraint: actors must belong to this manager." );
                return nullptr;
            }

            auto constraint = workphone::make_ptr<WPPhysicsConstraintFixed>();
            constraint->setBodyA( actor0 );
            constraint->setBodyB( actor1 );
            constraint->setLocalPose( JointActorIndexEnum::eACTOR0, localFrame0 );
            constraint->setLocalPose( JointActorIndexEnum::eACTOR1, localFrame1 );
            constraint->load( nullptr );

            m_constraints.push_back( constraint );
            return constraint;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    void WPPhysicsManager3::removeConstraint( SmartPtr<IPhysicsConstraint3> constraint )
    {
        if( !constraint )
        {
            return;
        }

        {
            ScopedLock lock( this );
            const auto it = std::find( m_constraints.begin(), m_constraints.end(), constraint );
            if( it == m_constraints.end() )
            {
                return;
            }
            m_constraints.erase( it );
        }
        constraint->unload( nullptr );
    }

    SmartPtr<IConstraintDrive> WPPhysicsManager3::addConstraintDrive()
    {
        try
        {
            auto drive = workphone::make_ptr<ConstraintDrive>();
            drive->setLoadingState( LoadingState::Loaded );
            return drive;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    SmartPtr<IConstraintLinearLimit> WPPhysicsManager3::addConstraintLinearLimit( real_Num extent,
                                                                                 real_Num contactDist )
    {
        if( !std::isfinite( static_cast<double>( extent ) ) || extent < 0 )
        {
            WP_LOG_ERROR(
                "WPPhysicsManager3::addConstraintLinearLimit: extent must be finite and non-negative." );
            return nullptr;
        }
        if( !std::isfinite( static_cast<double>( contactDist ) ) )
        {
            WP_LOG_ERROR(
                "WPPhysicsManager3::addConstraintLinearLimit: contact distance must be finite." );
            return nullptr;
        }

        auto limit = workphone::make_ptr<WPPhysicsNativeConstraintLinearLimit>();
        limit->setValue( extent );
        limit->setContactDistance( contactDist < 0 ? static_cast<real_Num>( 0.0 ) : contactDist );
        limit->setLoadingState( LoadingState::Loaded );
        return limit;
    }

    SmartPtr<IRaycastHit> WPPhysicsManager3::addRaycastHitData()
    {
        try
        {
            auto hit = workphone::make_ptr<RaycastHit>();
            hit->setTriangleIndex( -1 );
            hit->setLoadingState( LoadingState::Loaded );

            ScopedLock lock( this );
            m_raycastHits.push_back( hit );
            return hit;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return nullptr;
        }
    }

    void WPPhysicsManager3::removeRaycastHitData( SmartPtr<IRaycastHit> raycastHitData )
    {
        if( !raycastHitData )
        {
            return;
        }

        ScopedLock lock( this );
        const auto it = std::find( m_raycastHits.begin(), m_raycastHits.end(), raycastHitData );
        if( it != m_raycastHits.end() )
        {
            ( *it )->setLoadingState( LoadingState::Unloaded );
            m_raycastHits.erase( it );
        }
    }
    TaskId WPPhysicsManager3::getStateTask() const
    {
        return TaskId::Physics;
    }
    TaskId WPPhysicsManager3::getPhysicsTask() const
    {
        return TaskId::Physics;
    }
    void WPPhysicsManager3::loadObject( SmartPtr<ISharedObject> object, bool )
    {
        if( !object || object->getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        try
        {
            ScopedLock lock( this );
            object->load( nullptr );
            if( object->getLoadingState() != LoadingState::Loaded &&
                object->getLoadingState() != LoadingState::LoadingQueued )
            {
                object->setLoadingState( LoadingState::Loaded );
            }
        }
        catch( const std::exception &e )
        {
            object->setLoadingState( LoadingState::Unloaded );
            WP_LOG_EXCEPTION( e );
        }
    }

    void WPPhysicsManager3::unloadObject( SmartPtr<ISharedObject> object, bool )
    {
        if( !object || object->getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        try
        {
            ScopedLock lock( this );

            // The lightweight WP rigid-body wrappers own their native resources for
            // their entire C++ lifetime. Their inherited unload implementation expects
            // a global application state manager, which this standalone backend does not
            // require, so a loading-state transition is the appropriate teardown here.
            if( dynamic_cast<WPPhysicsRigidDynamic3 *>( object.get() ) ||
                dynamic_cast<WPPhysicsRigidStatic3 *>( object.get() ) )
            {
                object->setLoadingState( LoadingState::Unloaded );
                return;
            }

            object->unload( nullptr );
            if( object->getLoadingState() != LoadingState::Unloaded )
            {
                object->setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( const std::exception &e )
        {
            object->setLoadingState( LoadingState::Unloaded );
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IPhysicsScene3> WPPhysicsManager3::getPhysicsScene() const
    {
        ScopedLock lock( this );
        return m_physicsScene;
    }

    void WPPhysicsManager3::setPhysicsScene( SmartPtr<IPhysicsScene3> physicsScene )
    {
        ScopedLock lock( this );
        m_physicsScene = physicsScene;
    }

    SmartPtr<IPhysicsScene3> WPPhysicsManager3::getObjectsScene() const
    {
        ScopedLock lock( this );
        return m_objectsScene;
    }

    void WPPhysicsManager3::setObjectsScene( SmartPtr<IPhysicsScene3> objectsScene )
    {
        ScopedLock lock( this );
        m_objectsScene = objectsScene;
    }

    SmartPtr<IPhysicsScene3> WPPhysicsManager3::getRaycastScene() const
    {
        ScopedLock lock( this );
        return m_raycastScene;
    }

    void WPPhysicsManager3::setRaycastScene( SmartPtr<IPhysicsScene3> raycastScene )
    {
        ScopedLock lock( this );
        m_raycastScene = raycastScene;
    }

    SmartPtr<IPhysicsScene3> WPPhysicsManager3::getControlsScene() const
    {
        ScopedLock lock( this );
        return m_controlsScene;
    }

    void WPPhysicsManager3::setControlsScene( SmartPtr<IPhysicsScene3> controlsScene )
    {
        ScopedLock lock( this );
        m_controlsScene = controlsScene;
    }
} // namespace workphone::physics
