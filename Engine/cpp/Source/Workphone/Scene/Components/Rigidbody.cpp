#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/RigidbodyListener.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/Constraint.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

#include <array>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Rigidbody, Component );

    const String Rigidbody::MassStr = String( "mass" );
    const String Rigidbody::KinematicStr = String( "kinematic" );
    const String Rigidbody::MassSpaceInertiaTensorStr = String( "massSpaceInertiaTensor" );
    const String Rigidbody::MaxLinearVelocityStr = String( "maxLinearVelocity" );
    const String Rigidbody::MaxAngularVelocityStr = String( "maxAngularVelocity" );
    const String Rigidbody::LinearDampingStr = String( "linearDamping" );
    const String Rigidbody::AngularDampingStr = String( "angularDamping" );
    const String Rigidbody::UseGravityStr = String( "useGravity" );
    const String Rigidbody::ContinuousCollisionDetectionStr = String( "continuousCollisionDetection" );
    const String Rigidbody::SleepThresholdStr = String( "sleepThreshold" );
    const String Rigidbody::StabilizationThresholdStr = String( "stabilizationThreshold" );
    const String Rigidbody::SolverPositionIterationsStr = String( "solverPositionIterations" );
    const String Rigidbody::SolverVelocityIterationsStr = String( "solverVelocityIterations" );
    const String Rigidbody::ContactReportThresholdStr = String( "contactReportThreshold" );
    const String Rigidbody::GroupMaskStr = String( "groupMask" );
    const String Rigidbody::CollisionMaskStr = String( "collisionMask" );
    const String Rigidbody::CollisionMaskOverrideStr = String( "collisionMaskOverride" );
    const String Rigidbody::LocalBoundsStr = String( "localBounds" );
    const String Rigidbody::LinearVelocityStr = String( "linearVelocity" );
    const String Rigidbody::AngularVelocityStr = String( "angularVelocity" );
    const String Rigidbody::HasRigidDynamicStr = String( "hasRigidDynamic" );
    const String Rigidbody::HasRigidStaticStr = String( "hasRigidStatic" );
    const String Rigidbody::HasPhysicsBodyStr = String( "hasPhysicsBody" );
    const String Rigidbody::NumShapesStr = String( "numShapes" );
    const String Rigidbody::NumConstraintsStr = String( "numConstraints" );
    const String Rigidbody::HasConstraintsStr = String( "hasConstraints" );
    const String Rigidbody::IsSleepingStr = String( "isSleeping" );

    namespace
    {
        constexpr auto MinimumDynamicMass = static_cast<real_Num>( 1.0e-6 );
        constexpr auto UnlimitedAngularVelocity = static_cast<real_Num>( 1.0e5 );

        real_Num clampNonNegative( real_Num value )
        {
            return value >= static_cast<real_Num>( 0.0 ) ? value : static_cast<real_Num>( 0.0 );
        }

        real_Num clampMass( real_Num mass )
        {
            return mass >= MinimumDynamicMass ? mass : MinimumDynamicMass;
        }

        Vector3<real_Num> clampNonNegative( const Vector3<real_Num> &value )
        {
            return Vector3<real_Num>( clampNonNegative( value.x ), clampNonNegative( value.y ),
                                      clampNonNegative( value.z ) );
        }

        real_Num getPhysicsMaxAngularVelocity( real_Num maxAngularVelocity )
        {
            return maxAngularVelocity > static_cast<real_Num>( 0.0 ) ? maxAngularVelocity
                                                                     : UnlimitedAngularVelocity;
        }

        struct RealPropertyDescriptor
        {
            const String &name;
            real_Num ( Rigidbody::*getter )() const;
            void ( Rigidbody::*setter )( real_Num );
        };

        struct BoolPropertyDescriptor
        {
            const String &name;
            bool ( Rigidbody::*getter )() const;
            void ( Rigidbody::*setter )( bool );
        };

        struct U32PropertyDescriptor
        {
            const String &name;
            u32 ( Rigidbody::*getter )() const;
            void ( Rigidbody::*setter )( u32 );
        };

        struct Vector3PropertyDescriptor
        {
            const String &name;
            Vector3<real_Num> ( Rigidbody::*getter )() const;
            void ( Rigidbody::*setter )( const Vector3<real_Num> & );
        };

        struct AABBPropertyDescriptor
        {
            const String &name;
            AABB3<real_Num> ( Rigidbody::*getter )() const;
            void ( Rigidbody::*setter )( const AABB3<real_Num> & );
        };

        struct ReadOnlyBoolPropertyDescriptor
        {
            const String &name;
            bool ( Rigidbody::*getter )() const;
        };

        struct ReadOnlyU32PropertyDescriptor
        {
            const String &name;
            u32 ( Rigidbody::*getter )() const;
        };

        const auto &getRealPropertyDescriptors()
        {
            static const std::array<RealPropertyDescriptor, 8> descriptors = {
                { { Rigidbody::MassStr, &Rigidbody::getMass, &Rigidbody::setMass },
                  { Rigidbody::MaxLinearVelocityStr, &Rigidbody::getMaxLinearVelocity,
                    &Rigidbody::setMaxLinearVelocity },
                  { Rigidbody::MaxAngularVelocityStr, &Rigidbody::getMaxAngularVelocity,
                    &Rigidbody::setMaxAngularVelocity },
                  { Rigidbody::LinearDampingStr, &Rigidbody::getLinearDamping,
                    &Rigidbody::setLinearDamping },
                  { Rigidbody::AngularDampingStr, &Rigidbody::getAngularDamping,
                    &Rigidbody::setAngularDamping },
                  { Rigidbody::SleepThresholdStr, &Rigidbody::getSleepThreshold,
                    &Rigidbody::setSleepThreshold },
                  { Rigidbody::StabilizationThresholdStr, &Rigidbody::getStabilizationThreshold,
                    &Rigidbody::setStabilizationThreshold },
                  { Rigidbody::ContactReportThresholdStr, &Rigidbody::getContactReportThreshold,
                    &Rigidbody::setContactReportThreshold } }
            };

            return descriptors;
        }

        const auto &getBoolPropertyDescriptors()
        {
            static const std::array<BoolPropertyDescriptor, 4> descriptors = {
                { { Rigidbody::KinematicStr, &Rigidbody::isKinematic, &Rigidbody::setKinematic },
                  { Rigidbody::CollisionMaskOverrideStr, &Rigidbody::hasCollisionMaskOverride,
                    &Rigidbody::setCollisionMaskOverride },
                  { Rigidbody::UseGravityStr, &Rigidbody::getUseGravity, &Rigidbody::setUseGravity },
                  { Rigidbody::ContinuousCollisionDetectionStr,
                    &Rigidbody::getContinuousCollisionDetection,
                    &Rigidbody::setContinuousCollisionDetection } }
            };

            return descriptors;
        }

        const auto &getU32PropertyDescriptors()
        {
            static const std::array<U32PropertyDescriptor, 4> descriptors = {
                { { Rigidbody::GroupMaskStr, &Rigidbody::getGroupMask, &Rigidbody::setGroupMask },
                  { Rigidbody::CollisionMaskStr, &Rigidbody::getCollisionMask,
                    &Rigidbody::setCollisionMask },
                  { Rigidbody::SolverPositionIterationsStr, &Rigidbody::getSolverPositionIterations,
                    &Rigidbody::setSolverPositionIterations },
                  { Rigidbody::SolverVelocityIterationsStr, &Rigidbody::getSolverVelocityIterations,
                    &Rigidbody::setSolverVelocityIterations } }
            };

            return descriptors;
        }

        const auto &getVector3PropertyDescriptors()
        {
            static const std::array<Vector3PropertyDescriptor, 3> descriptors = {
                { { Rigidbody::MassSpaceInertiaTensorStr, &Rigidbody::getMassSpaceInertiaTensor,
                    &Rigidbody::setMassSpaceInertiaTensor },
                  { Rigidbody::LinearVelocityStr, &Rigidbody::getLinearVelocity,
                    &Rigidbody::setLinearVelocity },
                  { Rigidbody::AngularVelocityStr, &Rigidbody::getAngularVelocity,
                    &Rigidbody::setAngularVelocity } }
            };

            return descriptors;
        }

        const auto &getAABBPropertyDescriptors()
        {
            static const std::array<AABBPropertyDescriptor, 1> descriptors = {
                { { Rigidbody::LocalBoundsStr, &Rigidbody::getLocalBounds, &Rigidbody::setLocalBounds } }
            };

            return descriptors;
        }

        const auto &getReadOnlyBoolPropertyDescriptors()
        {
            static const std::array<ReadOnlyBoolPropertyDescriptor, 5> descriptors = {
                { { Rigidbody::HasRigidDynamicStr, &Rigidbody::hasRigidDynamic },
                  { Rigidbody::HasRigidStaticStr, &Rigidbody::hasRigidStatic },
                  { Rigidbody::HasPhysicsBodyStr, &Rigidbody::hasPhysicsBody },
                  { Rigidbody::HasConstraintsStr, &Rigidbody::hasConstraints },
                  { Rigidbody::IsSleepingStr, &Rigidbody::isSleeping } }
            };

            return descriptors;
        }

        const auto &getReadOnlyU32PropertyDescriptors()
        {
            static const std::array<ReadOnlyU32PropertyDescriptor, 2> descriptors = {
                { { Rigidbody::NumShapesStr, &Rigidbody::getNumShapes },
                  { Rigidbody::NumConstraintsStr, &Rigidbody::getNumConstraints } }
            };

            return descriptors;
        }

        struct PropertyEditorMetadata
        {
            const String &name;
            const char *label;
            const char *category;
            const char *description;
            const char *minimum;
            const char *maximum;
            const char *step;
        };

        const auto &getPropertyEditorMetadata()
        {
            static const PropertyEditorMetadata metadata[] = {
                { Rigidbody::MassStr, "Mass", "Body", "Mass of a dynamic body.", "0.000001", "", "0.1" },
                { Rigidbody::KinematicStr, "Kinematic", "Body",
                  "Drive the body from the actor transform instead of simulation.", "", "", "" },
                { Rigidbody::UseGravityStr, "Use Gravity", "Body",
                  "Allow scene gravity to accelerate this body.", "", "", "" },
                { Rigidbody::MassSpaceInertiaTensorStr, "Inertia Tensor", "Body",
                  "Principal moments of inertia in mass space.", "0", "", "0.1" },
                { Rigidbody::LinearDampingStr, "Linear Damping", "Damping & Limits",
                  "Damping applied to linear velocity.", "0", "", "0.01" },
                { Rigidbody::AngularDampingStr, "Angular Damping", "Damping & Limits",
                  "Damping applied to angular velocity.", "0", "", "0.01" },
                { Rigidbody::MaxLinearVelocityStr, "Max Linear Speed", "Damping & Limits",
                  "Application-side speed limit; zero means unlimited.", "0", "", "0.1" },
                { Rigidbody::MaxAngularVelocityStr, "Max Angular Speed", "Damping & Limits",
                  "Angular speed limit; zero means unlimited.", "0", "", "0.1" },
                { Rigidbody::SleepThresholdStr, "Sleep Threshold", "Damping & Limits",
                  "Energy threshold below which the body can sleep.", "0", "", "0.001" },
                { Rigidbody::StabilizationThresholdStr, "Stabilization Threshold", "Damping & Limits",
                  "Energy threshold used to stabilize slow bodies.", "0", "", "0.001" },
                { Rigidbody::GroupMaskStr, "Collision Group", "Collision",
                  "Category bits identifying this body.", "0", "", "1" },
                { Rigidbody::CollisionMaskStr, "Collides With", "Collision",
                  "Category bits this body is allowed to collide with.", "0", "", "1" },
                { Rigidbody::CollisionMaskOverrideStr, "Override Actor Mask", "Collision",
                  "Use this component's collision mask instead of inheriting the actor mask.", "", "",
                  "" },
                { Rigidbody::ContinuousCollisionDetectionStr, "Continuous Collision Detection",
                  "Collision", "Use swept collision detection for fast dynamic bodies.", "", "", "" },
                { Rigidbody::ContactReportThresholdStr, "Contact Report Threshold", "Collision",
                  "Minimum contact impulse reported to listeners.", "0", "", "0.1" },
                { Rigidbody::SolverPositionIterationsStr, "Position Iterations", "Solver",
                  "Minimum position iterations used by the constraint solver.", "1", "255", "1" },
                { Rigidbody::SolverVelocityIterationsStr, "Velocity Iterations", "Solver",
                  "Minimum velocity iterations used by the constraint solver.", "1", "255", "1" },
                { Rigidbody::LinearVelocityStr, "Linear Velocity", "Runtime",
                  "Current or initial world-space linear velocity.", "", "", "0.1" },
                { Rigidbody::AngularVelocityStr, "Angular Velocity", "Runtime",
                  "Current or initial world-space angular velocity.", "", "", "0.1" },
                { Rigidbody::LocalBoundsStr, "Local Bounds", "Runtime",
                  "Cached bounds of the attached collision geometry.", "", "", "" },
                { Rigidbody::HasRigidDynamicStr, "Has Dynamic Body", "Runtime",
                  "Whether a dynamic physics body is allocated.", "", "", "" },
                { Rigidbody::HasRigidStaticStr, "Has Static Body", "Runtime",
                  "Whether a static physics body is allocated.", "", "", "" },
                { Rigidbody::HasPhysicsBodyStr, "Has Physics Body", "Runtime",
                  "Whether any physics body is allocated.", "", "", "" },
                { Rigidbody::NumShapesStr, "Shape Count", "Runtime",
                  "Number of shapes attached to the live body.", "", "", "" },
                { Rigidbody::NumConstraintsStr, "Constraint Count", "Runtime",
                  "Number of constraints attached to this component.", "", "", "" },
                { Rigidbody::HasConstraintsStr, "Has Constraints", "Runtime",
                  "Whether constraints are attached to this component.", "", "", "" },
                { Rigidbody::IsSleepingStr, "Sleeping", "Runtime",
                  "Whether the live dynamic body is sleeping.", "", "", "" },
            };

            return metadata;
        }

        void applyPropertyEditorMetadata( SmartPtr<Properties> properties )
        {
            for( const auto &metadata : getPropertyEditorMetadata() )
            {
                if( !properties->hasProperty( metadata.name ) )
                {
                    continue;
                }

                auto &property = properties->getPropertyObject( metadata.name );
                property.setAttribute( "label", metadata.label );
                property.setAttribute( "category", metadata.category );
                property.setAttribute( "description", metadata.description );

                if( metadata.minimum[0] != '\0' )
                {
                    property.setAttribute( "min", metadata.minimum );
                }

                if( metadata.maximum[0] != '\0' )
                {
                    property.setAttribute( "max", metadata.maximum );
                }

                if( metadata.step[0] != '\0' )
                {
                    property.setAttribute( "step", metadata.step );
                }
            }
        }
    }  // namespace

    Rigidbody::Rigidbody() = default;

    Rigidbody::~Rigidbody() = default;

    void Rigidbody::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            if( m_componentFSM )
            {
                m_componentFSM->setPriority( 5000 );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Rigidbody::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            if( physicsManager )
            {
                if( auto rigidDynamic = getRigidDynamic() )
                {
                    if( auto listener = getRigidbodyListener() )
                    {
                        listener->setOwner( nullptr );
                        rigidDynamic->removeObjectListener( listener );
                        setRigidbodyListener( nullptr );
                    }
                }

                if( auto rigidStatic = getRigidStatic() )
                {
                    if( auto listener = getRigidbodyListener() )
                    {
                        listener->setOwner( nullptr );
                        rigidStatic->removeObjectListener( listener );
                        setRigidbodyListener( nullptr );
                    }
                }

                destroyRigidbodyObject();
            }

            m_scene = nullptr;

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Rigidbody::updatePhysicsState()
    {
        if( isLoaded() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            if( physicsManager )
            {
                if( auto actor = getActor() )
                {
                    auto enabled = isEnabled() && actor->isEnabledInScene();

                    if( auto rigidDynamic = getRigidDynamic() )
                    {
                        rigidDynamic->setMass( m_mass );
                        rigidDynamic->setMassSpaceInertiaTensor( m_massSpaceInertiaTensor );
                        rigidDynamic->setLinearDamping( m_linearDamping );
                        rigidDynamic->setAngularDamping( m_angularDamping );
                        rigidDynamic->setMaxAngularVelocity(
                            getPhysicsMaxAngularVelocity( m_maxAngularVelocity ) );
                        rigidDynamic->setSleepThreshold( m_sleepThreshold );
                        rigidDynamic->setStabilizationThreshold( m_stabilizationThreshold );
                        rigidDynamic->setSolverIterationCounts( m_solverPositionIterations,
                                                                m_solverVelocityIterations );
                        rigidDynamic->setContactReportThreshold( m_contactReportThreshold );
                        rigidDynamic->setActorFlag( physics::ActorFlagEnum::eDISABLE_GRAVITY,
                                                    !m_useGravity );
                        rigidDynamic->setRigidBodyFlag( physics::RigidBodyFlagEnum::eENABLE_CCD,
                                                        m_continuousCollisionDetection );
                        rigidDynamic->setKinematic( m_isKinematic );
                        rigidDynamic->setEnabled( enabled );
                        rigidDynamic->setCollisionType( m_groupMask );
                        rigidDynamic->setCollisionMask( m_collisionMask );
                    }

                    if( auto rigidStatic = getRigidStatic() )
                    {
                        rigidStatic->setMass( m_mass );
                        rigidStatic->setMassSpaceInertiaTensor( m_massSpaceInertiaTensor );
                        rigidStatic->setActorFlag( physics::ActorFlagEnum::eDISABLE_GRAVITY,
                                                   !m_useGravity );
                        rigidStatic->setEnabled( enabled );
                        rigidStatic->setCollisionType( m_groupMask );
                        rigidStatic->setCollisionMask( m_collisionMask );
                    }

                    if( enabled )
                    {
                        if( auto rigidDynamic = getRigidDynamic() )
                        {
                            auto physicsScene = rigidDynamic->getScene();
                            if( !physicsScene )
                            {
                                attachShape();

                                if( rigidDynamic->getNumShapes() > 0 )
                                {
                                    addToScene();
                                }
                            }
                        }

                        if( auto rigidStatic = getRigidStatic() )
                        {
                            auto physicsScene = rigidStatic->getScene();
                            if( !physicsScene )
                            {
                                attachShape();

                                if( rigidStatic->getNumShapes() > 0 )
                                {
                                    addToScene();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void Rigidbody::updateConstraints()
    {
        for( auto &constraint : m_constraints )
        {
            constraint->updatePhysicsState();
        }
    }

    void Rigidbody::createRigidbodyObject()
    {
        if( auto actor = getActorPtr() )
        {
            if( actor->isStatic() )
            {
                if( m_rigidStatic )
                {
                    return;
                }
            }
            else
            {
                if( m_rigidDynamic )
                {
                    return;
                }
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto physicsMgr = applicationManager->getPhysicsManagerPtr();
            if( physicsMgr )
            {
                if( auto rigidStatic = getRigidStatic() )
                {
                    physicsMgr->removePhysicsBody( rigidStatic );
                    setRigidStatic( nullptr );
                }

                if( auto rigidDynamic = getRigidDynamic() )
                {
                    physicsMgr->removePhysicsBody( rigidDynamic );
                    setRigidDynamic( nullptr );
                }

                SmartPtr<physics::IPhysicsBody3> body;
                auto transform = actor->getWorldTransform();

                if( actor->isStatic() )
                {
                    body = physicsMgr->addRigidStatic( transform );

                    auto listener = factoryManager->make_ptr<RigidbodyListener>();
                    listener->setOwner( this );
                    body->addObjectListener( listener );
                    setRigidbodyListener( listener );

                    setRigidStatic( body );
                }
                else
                {
                    auto rigidDynamic = physicsMgr->addRigidDynamic( transform );

                    auto listener = factoryManager->make_ptr<RigidbodyListener>();
                    listener->setOwner( this );
                    rigidDynamic->addObjectListener( listener );
                    setRigidbodyListener( listener );

                    if( rigidDynamic )
                    {
                        rigidDynamic->setMassSpaceInertiaTensor( m_massSpaceInertiaTensor );
                        rigidDynamic->setLinearDamping( m_linearDamping );
                        rigidDynamic->setAngularDamping( m_angularDamping );
                        rigidDynamic->setMaxAngularVelocity(
                            getPhysicsMaxAngularVelocity( m_maxAngularVelocity ) );
                        rigidDynamic->setSleepThreshold( m_sleepThreshold );
                        rigidDynamic->setStabilizationThreshold( m_stabilizationThreshold );
                        rigidDynamic->setSolverIterationCounts( m_solverPositionIterations,
                                                                m_solverVelocityIterations );
                        rigidDynamic->setContactReportThreshold( m_contactReportThreshold );
                        rigidDynamic->setRigidBodyFlag( physics::RigidBodyFlagEnum::eENABLE_CCD,
                                                        m_continuousCollisionDetection );
                        rigidDynamic->setKinematic( m_isKinematic );
                        rigidDynamic->setLinearVelocity( getLinearVelocity() );
                        rigidDynamic->setAngularVelocity( getAngularVelocity() );
                    }

                    body = rigidDynamic;
                    setRigidDynamic( body );
                }

                body->setMass( m_mass );
                body->setActorFlag( physics::ActorFlagEnum::eDISABLE_GRAVITY, !m_useGravity );
                body->setCollisionType( m_groupMask );
                body->setCollisionMask( m_collisionMask );
            }
        }
    }

    void Rigidbody::updateShapes()
    {
        // Detach all currently attached shapes first so stale shapes are not left behind
        // when the collision component changes or is removed.
        if( auto rigidDynamic = getRigidDynamic() )
        {
            for( auto shape : rigidDynamic->getShapes() )
            {
                if( shape )
                {
                    rigidDynamic->removeShape( shape, false );
                }
            }
        }

        if( auto rigidStatic = getRigidStatic() )
        {
            for( auto shape : rigidStatic->getShapes() )
            {
                if( shape )
                {
                    rigidStatic->removeShape( shape, false );
                }
            }
        }

        removeFromScene();
        attachShape();
        addToScene();
    }

    void Rigidbody::addConstraint( SmartPtr<Constraint> constraint )
    {
        m_constraints.push_back( constraint );
    }

    void Rigidbody::removeConstraint( SmartPtr<Constraint> constraint )
    {
        m_constraints.erase( std::remove( m_constraints.begin(), m_constraints.end(), constraint ),
                             m_constraints.end() );
    }

    bool Rigidbody::hasConstraint( SmartPtr<Constraint> constraint ) const
    {
        return std::find( m_constraints.begin(), m_constraints.end(), constraint ) !=
               m_constraints.end();
    }

    bool Rigidbody::hasConstraints() const
    {
        return !m_constraints.empty();
    }

    u32 Rigidbody::getNumConstraints() const
    {
        return static_cast<u32>( m_constraints.size() );
    }

    Array<SmartPtr<Constraint>> Rigidbody::getConstraints() const
    {
        Array<SmartPtr<Constraint>> constraints;
        constraints.reserve( m_constraints.size() );

        for( auto &constraint : m_constraints )
        {
            constraints.push_back( constraint.lock() );
        }

        return constraints;
    }

    void Rigidbody::setConstraints( const Array<SmartPtr<Constraint>> &constraints )
    {
        m_constraints.clear();

        for( auto &constraint : constraints )
        {
            m_constraints.push_back( constraint );
        }
    }

    void Rigidbody::removeFromScene()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            if( auto physicsScene = physicsManager->getPhysicsScene() )
            {
                if( auto rigidDynamic = getRigidDynamic() )
                {
                    if( physicsScene )
                    {
                        physicsScene->removeActor( rigidDynamic );
                    }
                }

                if( auto rigidStatic = getRigidStatic() )
                {
                    if( physicsScene )
                    {
                        physicsScene->removeActor( rigidStatic );
                    }
                }
            }
        }
    }

    void Rigidbody::addToScene()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManagerPtr();
        if( !physicsManager )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        if( auto physicsScene = physicsManager->getPhysicsScene() )
        {
            if( auto rigidDynamic = getRigidDynamic() )
            {
                physicsScene->addActor( rigidDynamic );
            }

            if( auto rigidStatic = getRigidStatic() )
            {
                physicsScene->addActor( rigidStatic );
            }
        }
        else
        {
            WP_LOG_ERROR( "Physics scene is not available." );
        }
    }

    void Rigidbody::destroyRigidbodyObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            return;
        }

        auto physicsScene = physicsManager->getPhysicsScene();
        if( physicsScene )
        {
            if( auto rigidDynamic = getRigidDynamic() )
            {
                physicsScene->removeActor( rigidDynamic );
            }

            if( auto rigidStatic = getRigidStatic() )
            {
                physicsScene->removeActor( rigidStatic );
            }
        }

        if( auto rigidDynamic = getRigidDynamic() )
        {
            for( auto &shape : rigidDynamic->getShapes() )
            {
                rigidDynamic->removeShape( shape );
            }
            physicsManager->removePhysicsBody( rigidDynamic );
            setRigidDynamic( nullptr );
        }

        if( auto rigidStatic = getRigidStatic() )
        {
            for( auto &shape : rigidStatic->getShapes() )
            {
                rigidStatic->removeShape( shape );
            }
            physicsManager->removePhysicsBody( rigidStatic );
            setRigidStatic( nullptr );
        }

        if( m_material )
        {
            physicsManager->removeMaterial( m_material );
            m_material = nullptr;
        }
    }

    void Rigidbody::attachShape()
    {
        try
        {
            if( auto actor = getActorPtr() )
            {
                auto root = actor->getSceneRoot();
                if( root )
                {
                    root->updateTransform();
                }

                auto transform = actor->getTransform();
                if( !transform )
                {
                    return;
                }

                auto collision = actor->getComponent<Collision>();
                if( collision )
                {
                    if( collision->isLoaded() )
                    {
                        auto shape = collision->getShape();
                        WP_ASSERT( shape );

                        if( shape && !shape->isAttached() )
                        {
                            shape->setCollisionType( m_groupMask );
                            shape->setCollisionMask( m_collisionMask );

                            auto localTransform = transform->getWorldTransform();
                            localTransform.setPosition( Vector3<real_Num>::zero() );
                            localTransform.setOrientation( Quaternion<real_Num>::identity() );

                            shape->setLocalPose( localTransform );

                            if( auto rigidDynamic = getRigidDynamic() )
                            {
                                rigidDynamic->addShape( shape );
                            }

                            if( auto rigidStatic = getRigidStatic() )
                            {
                                rigidStatic->addShape( shape );
                            }
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Rigidbody::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActor() )
        {
            auto state = getState();
            switch( state )
            {
            case State::Edit:
            case State::Play:
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto physicsManager = applicationManager->getPhysicsManager();

                auto enabled = isEnabled() && actor->isEnabledInScene();

                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
                {
                    if( enabled )
                    {
                        createRigidbodyObject();
                    }
                    else
                    {
                        destroyRigidbodyObject();
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    if( enabled )
                    {
                        createRigidbodyObject();
                    }
                    else
                    {
                        destroyRigidbodyObject();
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagStatic ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagStatic ) )
                {
                    if( enabled )
                    {
                        destroyRigidbodyObject();
                        createRigidbodyObject();
                    }
                    else
                    {
                        destroyRigidbodyObject();
                    }
                }

                if( enabled )
                {
                    attachShape();

                    if( auto rigidDynamic = getRigidDynamic() )
                    {
                        if( rigidDynamic->getNumShapes() > 0 )
                        {
                            addToScene();
                        }
                    }

                    if( auto rigidStatic = getRigidStatic() )
                    {
                        if( rigidStatic->getNumShapes() > 0 )
                        {
                            addToScene();
                        }
                    }
                }

                updateSmoothTransformState();
            }
            break;
            default:
            {
            }
            }
        }
    }

    FSMReturnType Rigidbody::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                createRigidbodyObject();
                attachShape();
                addToScene();
                updateConstraints();
                updatePhysicsState();

                updateSmoothTransformState();
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                removeFromScene();
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Pending:
        {
        }
        break;
        case FSMEvent::Complete:
        {
        }
        break;
        case FSMEvent::NewState:
        {
        }
        break;
        case FSMEvent::WaitForChange:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    Parameter Rigidbody::handleEvent( EventType eventType, hash_type eventValue,
                                      const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                      SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == componentLoaded )
        {
            if( object )
            {
                if( object->isDerived<Collision>() )
                {
                    auto collision = workphone::static_pointer_cast<Collision>( object );
                    auto componentState = collision->getState();

                    if( componentState == State::Edit || componentState == State::Play )
                    {
                        if( collision->getActorPtr() == getActorPtr() )
                        {
                            attachShape();
                        }
                    }
                }
            }
        }

        return Component::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    void Rigidbody::updateKinematicState( bool kinematicState )
    {
        m_isKinematic = kinematicState;

        if( m_rigidDynamic )
        {
            m_rigidDynamic->setKinematic( kinematicState );
        }
    }

    SmartPtr<physics::IRigidDynamic3> Rigidbody::getRigidDynamic() const
    {
        return m_rigidDynamic;
    }

    void Rigidbody::setRigidDynamic( SmartPtr<physics::IRigidDynamic3> rigidDynamic )
    {
        m_rigidDynamic = rigidDynamic;
    }

    SmartPtr<physics::IRigidStatic3> Rigidbody::getRigidStatic() const
    {
        return m_rigidStatic;
    }

    void Rigidbody::setRigidStatic( SmartPtr<physics::IRigidStatic3> rigidStatic )
    {
        m_rigidStatic = rigidStatic;
    }

    bool Rigidbody::isStatic() const
    {
        if( auto actor = getActor() )
        {
            return actor->isStatic();
        }

        return false;
    }

    bool Rigidbody::isKinematic() const
    {
        return m_isKinematic;
    }

    void Rigidbody::setKinematic( bool kinematic )
    {
        m_isKinematic = kinematic;
        updateKinematicState( kinematic );
    }

    u32 Rigidbody::getGroupMask() const
    {
        return m_groupMask;
    }

    void Rigidbody::setGroupMask( u32 groupMask )
    {
        m_groupMask = groupMask;
        applyCollisionFiltering();
    }

    u32 Rigidbody::getCollisionMask() const
    {
        return m_collisionMask;
    }

    void Rigidbody::setCollisionMask( u32 collisionMask )
    {
        setCollisionMaskInternal( collisionMask, true );
    }

    bool Rigidbody::hasCollisionMaskOverride() const
    {
        return m_hasCollisionMaskOverride;
    }

    void Rigidbody::setCollisionMaskOverride( bool collisionMaskOverride )
    {
        m_hasCollisionMaskOverride = collisionMaskOverride;

        if( !collisionMaskOverride )
        {
            if( auto actor = getActor() )
            {
                applyActorCollisionMask( actor->getCollisionMask() );
            }
            else
            {
                applyCollisionFiltering();
            }
        }
    }

    void Rigidbody::applyActorCollisionMask( u32 collisionMask )
    {
        setCollisionMaskInternal( collisionMask, false );
    }

    bool Rigidbody::hasRigidDynamic() const
    {
        return getRigidDynamic() != nullptr;
    }

    bool Rigidbody::hasRigidStatic() const
    {
        return getRigidStatic() != nullptr;
    }

    bool Rigidbody::hasPhysicsBody() const
    {
        return hasRigidDynamic() || hasRigidStatic();
    }

    u32 Rigidbody::getNumShapes() const
    {
        if( auto rigidDynamic = getRigidDynamic() )
        {
            return rigidDynamic->getNumShapes();
        }

        if( auto rigidStatic = getRigidStatic() )
        {
            return rigidStatic->getNumShapes();
        }

        return 0;
    }

    void Rigidbody::setCollisionMaskInternal( u32 collisionMask, bool explicitOverride )
    {
        m_collisionMask = collisionMask;
        if( explicitOverride )
        {
            m_hasCollisionMaskOverride = true;
        }

        applyCollisionFiltering();
    }

    void Rigidbody::applyCollisionFiltering()
    {
        const auto groupMask = getGroupMask();
        const auto collisionMask = getCollisionMask();

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setCollisionMask( collisionMask );
            rigidDynamic->setCollisionType( groupMask );
        }

        if( auto rigidStatic = getRigidStatic() )
        {
            rigidStatic->setCollisionMask( collisionMask );
            rigidStatic->setCollisionType( groupMask );
        }

        if( auto actor = getActorPtr() )
        {
            if( auto collision = actor->getComponent<Collision>() )
            {
                if( auto shape = collision->getShape() )
                {
                    shape->setCollisionType( groupMask );
                    shape->setCollisionMask( collisionMask );
                }
            }
        }
    }

    AABB3<real_Num> Rigidbody::getLocalBounds() const
    {
        return m_bounds;
    }

    void Rigidbody::setLocalBounds( const AABB3<real_Num> &bounds )
    {
        m_bounds = bounds;
    }

    SmartPtr<physics::IPhysicsScene3> Rigidbody::getScene() const
    {
        return m_scene;
    }

    void Rigidbody::setScene( SmartPtr<physics::IPhysicsScene3> scene )
    {
        m_scene = scene;
    }

    SmartPtr<physics::IPhysicsMaterial3> Rigidbody::getMaterial() const
    {
        return m_material;
    }

    void Rigidbody::setMaterial( SmartPtr<physics::IPhysicsMaterial3> material )
    {
        m_material = material;
    }

    SmartPtr<physics::IPhysicsBody3> Rigidbody::getClonedActor() const
    {
        return m_clonedActor;
    }

    void Rigidbody::setClonedActor( SmartPtr<physics::IPhysicsBody3> clonedActor )
    {
        m_clonedActor = clonedActor;
    }

    s32 Rigidbody::getTransformReferences() const
    {
        return m_transformReferences;
    }

    void Rigidbody::setTransformReferences( s32 transformReferences )
    {
        m_transformReferences = transformReferences;
    }

    void Rigidbody::updateComponents()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();

        auto typeInfo = Rigidbody::typeInfo();
        auto components = sceneManager->getComponentsByType<Rigidbody>( typeInfo );
        for( auto &component : components )
        {
            component->preUpdate();
        }
    }

    void Rigidbody::preUpdate()
    {
        if( auto rigidDynamic = getRigidDynamic() )
        {
            if( m_isKinematic )
            {
                if( auto actor = getActor() )
                {
                    auto p = actor->getPosition();
                    auto r = actor->getOrientation();
                    auto transform = Transform3<real_Num>( p, r );
                    rigidDynamic->setKinematicTarget( transform );
                }
            }

            auto maxLinear = m_maxLinearVelocity;
            if( maxLinear > static_cast<real_Num>( 0.0 ) )
            {
                auto velocity = rigidDynamic->getLinearVelocity();
                auto speedSq = velocity.lengthSquared();
                if( speedSq > maxLinear * maxLinear )
                {
                    velocity = velocity.normaliseCopy() * maxLinear;
                    m_linearVelocity = velocity;
                    rigidDynamic->setLinearVelocity( velocity, false );
                }
            }

            auto maxAngular = m_maxAngularVelocity;
            if( maxAngular > static_cast<real_Num>( 0.0 ) )
            {
                auto velocity = rigidDynamic->getAngularVelocity();
                auto speedSq = velocity.lengthSquared();
                if( speedSq > maxAngular * maxAngular )
                {
                    velocity = velocity.normaliseCopy() * maxAngular;
                    m_angularVelocity = velocity;
                    rigidDynamic->setAngularVelocity( velocity, false );
                }
            }

            m_linearVelocity = rigidDynamic->getLinearVelocity();
            m_angularVelocity = rigidDynamic->getAngularVelocity();
        }
    }

    void Rigidbody::update()
    {
        if( auto rigidDynamic = getRigidDynamic() )
        {
            m_linearVelocity = rigidDynamic->getLinearVelocity();
            m_angularVelocity = rigidDynamic->getAngularVelocity();

            if( !m_isKinematic )
            {
                auto transform = rigidDynamic->getTransform();
                if( auto actor = getActor() )
                {
                    auto actorTransform = actor->getTransform();
                    if( actorTransform && !actor->isSmoothMotion() )
                    {
                        actor->setPosition( transform.getPosition() );
                        actor->setOrientation( transform.getOrientation() );
                    }
                }
            }
        }
    }

    void Rigidbody::postUpdate()
    {
        updateConstraints();
    }

    Array<SmartPtr<ISharedObject>> Rigidbody::getChildObjects() const
    {
        auto objects = Component::getChildObjects();

        if( auto rigidDynamic = m_rigidDynamic.load() )
            objects.emplace_back( rigidDynamic );
        if( auto rigidStatic = m_rigidStatic.load() )
            objects.emplace_back( rigidStatic );
        if( auto clonedActor = m_clonedActor.load() )
            objects.emplace_back( clonedActor );
        if( auto scene = m_scene.load() )
            objects.emplace_back( scene );
        if( auto material = m_material.load() )
            objects.emplace_back( material );

        return objects;
    }

    SmartPtr<Properties> Rigidbody::getProperties() const
    {
        try
        {
            auto properties = Component::getProperties();
            if( !properties )
            {
                return nullptr;
            }

            for( const auto &descriptor : getRealPropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )() );
            }

            for( const auto &descriptor : getBoolPropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )() );
            }

            for( const auto &descriptor : getU32PropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )() );
            }

            for( const auto &descriptor : getVector3PropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )() );
            }

            for( const auto &descriptor : getAABBPropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )() );
            }

            for( const auto &descriptor : getReadOnlyBoolPropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )(), true );
            }

            for( const auto &descriptor : getReadOnlyU32PropertyDescriptors() )
            {
                properties->setProperty( descriptor.name, ( this->*descriptor.getter )(), true );
            }

            applyPropertyEditorMetadata( properties );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void Rigidbody::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( !properties )
            {
                return;
            }

            Component::setProperties( properties );

            for( const auto &descriptor : getRealPropertyDescriptors() )
            {
                auto value = ( this->*descriptor.getter )();
                if( properties->getPropertyValue( descriptor.name, value ) )
                {
                    ( this->*descriptor.setter )( value );
                }
            }

            for( const auto &descriptor : getBoolPropertyDescriptors() )
            {
                auto value = ( this->*descriptor.getter )();
                if( properties->getPropertyValue( descriptor.name, value ) )
                {
                    ( this->*descriptor.setter )( value );
                }
            }

            for( const auto &descriptor : getU32PropertyDescriptors() )
            {
                auto value = ( this->*descriptor.getter )();
                if( properties->getPropertyValue( descriptor.name, value ) )
                {
                    ( this->*descriptor.setter )( value );
                }
            }

            for( const auto &descriptor : getVector3PropertyDescriptors() )
            {
                auto value = ( this->*descriptor.getter )();
                if( properties->getPropertyValue( descriptor.name, value ) )
                {
                    ( this->*descriptor.setter )( value );
                }
            }

            for( const auto &descriptor : getAABBPropertyDescriptors() )
            {
                auto value = ( this->*descriptor.getter )();
                if( properties->getPropertyValue( descriptor.name, value ) )
                {
                    ( this->*descriptor.setter )( value );
                }
            }

            auto collisionMaskOverride = hasCollisionMaskOverride();
            if( properties->getPropertyValue( CollisionMaskOverrideStr, collisionMaskOverride ) )
            {
                setCollisionMaskOverride( collisionMaskOverride );
            }

            updatePhysicsState();
            updateSmoothTransformState();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Rigidbody::updateTransform()
    {
        auto state = getState();
        switch( state )
        {
        case State::Edit:
        {
            if( auto actor = getActor() )
            {
                auto transform = actor->getTransform();
                if( transform && transform->getTransformReferences() > 0 )
                {
                    return;
                }

                auto p = actor->getPosition();
                auto r = actor->getOrientation();

                auto t = Transform3<real_Num>( p, r );

                if( m_rigidDynamic )
                {
                    m_rigidDynamic->setTransform( t );
                }

                if( m_rigidStatic )
                {
                    m_rigidStatic->setTransform( t );
                }
            }
        }
        break;
        case State::Play:
        {
            if( auto actor = getActor() )
            {
                auto transform = actor->getTransform();
                if( transform && transform->getTransformReferences() > 0 )
                {
                    return;
                }

                auto p = actor->getPosition();
                auto r = actor->getOrientation();

                auto t = Transform3<real_Num>( p, r );

                if( m_rigidDynamic )
                {
                    auto scene = m_rigidDynamic->getScene();
                    if( !scene )
                    {
                        m_rigidDynamic->setTransform( t );
                    }
                }

                if( m_rigidStatic )
                {
                    m_rigidStatic->setTransform( t );
                }
            }
        }
        break;
        default:
        {
        }
        }
    }

    Vector3<real_Num> Rigidbody::getLinearVelocity() const
    {
        if( m_rigidDynamic )
        {
            return m_rigidDynamic->getLinearVelocity();
        }

        return m_linearVelocity;
    }

    void Rigidbody::setLinearVelocity( const Vector3<real_Num> &linearVelocity )
    {
        m_linearVelocity = linearVelocity;

        if( m_rigidDynamic )
        {
            m_rigidDynamic->setLinearVelocity( linearVelocity );
        }
    }

    Vector3<real_Num> Rigidbody::getAngularVelocity() const
    {
        if( m_rigidDynamic )
        {
            return m_rigidDynamic->getAngularVelocity();
        }

        return m_angularVelocity;
    }

    Vector3<real_Num> Rigidbody::getPointVelocity( const Vector3<real_Num> &point )
    {
        if( m_rigidDynamic )
        {
            const auto globalPose = m_rigidDynamic->getTransform();
            const auto cmassLocalPose = m_rigidDynamic->getCMassLocalPose();
            const auto centerOfMass = globalPose.transformPoint( cmassLocalPose.getPosition() );
            const auto rpoint = point - centerOfMass;

            const auto linearVelocity = getLinearVelocity();
            const auto angularVelocity = getAngularVelocity();
            return linearVelocity + angularVelocity.crossProduct( rpoint );
        }

        return Vector3<real_Num>::zero();
    }

    void Rigidbody::setAngularVelocity( const Vector3<real_Num> &angularVelocity )
    {
        m_angularVelocity = angularVelocity;

        if( m_rigidDynamic )
        {
            m_rigidDynamic->setAngularVelocity( angularVelocity );
        }
    }

    real_Num Rigidbody::getMaxLinearVelocity() const
    {
        return m_maxLinearVelocity;
    }

    void Rigidbody::setMaxLinearVelocity( real_Num maxLinearVelocity )
    {
        m_maxLinearVelocity = clampNonNegative( maxLinearVelocity );
    }

    real_Num Rigidbody::getMaxAngularVelocity() const
    {
        return m_maxAngularVelocity;
    }

    void Rigidbody::setMaxAngularVelocity( real_Num maxAngularVelocity )
    {
        m_maxAngularVelocity = clampNonNegative( maxAngularVelocity );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setMaxAngularVelocity( getPhysicsMaxAngularVelocity( m_maxAngularVelocity ) );
        }
    }

    real_Num Rigidbody::getLinearDamping() const
    {
        return m_linearDamping;
    }

    void Rigidbody::setLinearDamping( real_Num linearDamping )
    {
        m_linearDamping = clampNonNegative( linearDamping );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setLinearDamping( m_linearDamping );
        }
    }

    real_Num Rigidbody::getAngularDamping() const
    {
        return m_angularDamping;
    }

    void Rigidbody::setAngularDamping( real_Num angularDamping )
    {
        m_angularDamping = clampNonNegative( angularDamping );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setAngularDamping( m_angularDamping );
        }
    }

    bool Rigidbody::getUseGravity() const
    {
        return m_useGravity;
    }

    void Rigidbody::setUseGravity( bool useGravity )
    {
        m_useGravity = useGravity;

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setActorFlag( physics::ActorFlagEnum::eDISABLE_GRAVITY, !useGravity );
        }

        if( auto rigidStatic = getRigidStatic() )
        {
            rigidStatic->setActorFlag( physics::ActorFlagEnum::eDISABLE_GRAVITY, !useGravity );
        }
    }

    bool Rigidbody::getContinuousCollisionDetection() const
    {
        return m_continuousCollisionDetection;
    }

    void Rigidbody::setContinuousCollisionDetection( bool enabled )
    {
        m_continuousCollisionDetection = enabled;

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setRigidBodyFlag( physics::RigidBodyFlagEnum::eENABLE_CCD, enabled );
        }
    }

    real_Num Rigidbody::getSleepThreshold() const
    {
        return m_sleepThreshold;
    }

    void Rigidbody::setSleepThreshold( real_Num threshold )
    {
        m_sleepThreshold = clampNonNegative( threshold );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setSleepThreshold( m_sleepThreshold );
        }
    }

    real_Num Rigidbody::getStabilizationThreshold() const
    {
        return m_stabilizationThreshold;
    }

    void Rigidbody::setStabilizationThreshold( real_Num threshold )
    {
        m_stabilizationThreshold = clampNonNegative( threshold );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setStabilizationThreshold( m_stabilizationThreshold );
        }
    }

    u32 Rigidbody::getSolverPositionIterations() const
    {
        return m_solverPositionIterations;
    }

    void Rigidbody::setSolverPositionIterations( u32 iterations )
    {
        m_solverPositionIterations = std::min( std::max( iterations, 1u ), 255u );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setSolverIterationCounts( m_solverPositionIterations,
                                                    m_solverVelocityIterations );
        }
    }

    u32 Rigidbody::getSolverVelocityIterations() const
    {
        return m_solverVelocityIterations;
    }

    void Rigidbody::setSolverVelocityIterations( u32 iterations )
    {
        m_solverVelocityIterations = std::min( std::max( iterations, 1u ), 255u );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setSolverIterationCounts( m_solverPositionIterations,
                                                    m_solverVelocityIterations );
        }
    }

    real_Num Rigidbody::getContactReportThreshold() const
    {
        return m_contactReportThreshold;
    }

    void Rigidbody::setContactReportThreshold( real_Num threshold )
    {
        m_contactReportThreshold = clampNonNegative( threshold );

        if( auto rigidDynamic = getRigidDynamic() )
        {
            rigidDynamic->setContactReportThreshold( m_contactReportThreshold );
        }
    }

    bool Rigidbody::isSleeping() const
    {
        if( auto rigidDynamic = getRigidDynamic() )
        {
            return rigidDynamic->isSleeping();
        }

        return false;
    }

    Vector3<real_Num> Rigidbody::getLocalAngularVelocity() const
    {
        if( m_rigidDynamic )
        {
            auto av = getAngularVelocity();
            auto transform = m_rigidDynamic->getTransform();
            auto q = transform.getOrientation();
            return q.rotateInv( av );
        }

        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> Rigidbody::getLocalLinearVelocity() const
    {
        if( m_rigidDynamic )
        {
            auto linearVelocity = getLinearVelocity();
            auto transform = m_rigidDynamic->getTransform();
            auto q = transform.getOrientation();
            return q.rotateInv( linearVelocity );
        }

        return Vector3<real_Num>::zero();
    }

    void Rigidbody::addForce( const Vector3<real_Num> &force )
    {
        if( m_rigidDynamic )
        {
            m_rigidDynamic->addForce( force );
        }
    }

    void Rigidbody::addTorque( const Vector3<real_Num> &torque )
    {
        if( m_rigidDynamic )
        {
            m_rigidDynamic->addTorque( torque );
        }
    }

    void Rigidbody::setMass( real_Num mass )
    {
        m_mass = clampMass( mass );

        if( m_rigidDynamic )
        {
            m_rigidDynamic->setMass( m_mass );
        }

        if( m_rigidStatic )
        {
            m_rigidStatic->setMass( m_mass );
        }
    }

    real_Num Rigidbody::getMass() const
    {
        return m_mass;
    }

    void Rigidbody::setMassProps( real_Num mass, const Vector3<real_Num> &moi )
    {
        setMass( mass );
        setMassSpaceInertiaTensor( moi );
    }

    Vector3<real_Num> Rigidbody::getLocalPointVelocity( const Vector3<real_Num> &point )
    {
        if( !m_rigidDynamic )
        {
            return Vector3<real_Num>::zero();
        }

        const auto globalPose = m_rigidDynamic->getTransform();

        const auto cmassLocalPose = m_rigidDynamic->getCMassLocalPose();
        const auto centerOfMass = globalPose.transformPoint( cmassLocalPose.getPosition() );
        const auto rpoint = point - centerOfMass;

        const auto linearVelocity = getLinearVelocity();
        const auto angularVelocity = getAngularVelocity();

        return globalPose.inverseTransformPoint( linearVelocity +
                                                 angularVelocity.crossProduct( rpoint ) );
    }

    Vector3<real_Num> Rigidbody::getMassSpaceInertiaTensor() const
    {
        return m_massSpaceInertiaTensor;
    }

    void Rigidbody::setMassSpaceInertiaTensor( const Vector3<real_Num> &massSpaceInertiaTensor )
    {
        m_massSpaceInertiaTensor = clampNonNegative( massSpaceInertiaTensor );

        if( m_rigidDynamic )
        {
            m_rigidDynamic->setMassSpaceInertiaTensor( m_massSpaceInertiaTensor );
        }

        if( m_rigidStatic )
        {
            m_rigidStatic->setMassSpaceInertiaTensor( m_massSpaceInertiaTensor );
        }
    }

    Transform3<real_Num> Rigidbody::getTransform() const
    {
        if( m_rigidDynamic )
        {
            return m_rigidDynamic->getTransform();
        }

        if( m_rigidStatic )
        {
            return m_rigidStatic->getTransform();
        }

        return {};
    }

    void Rigidbody::setRigidbodyListener( SmartPtr<RigidbodyListener> rigidbodyListener )
    {
        m_rigidbodyListener = rigidbodyListener;
    }

    SmartPtr<RigidbodyListener> Rigidbody::getRigidbodyListener() const
    {
        return m_rigidbodyListener;
    }

    void Rigidbody::updateSmoothTransformState()
    {
        if( auto actor = getActor() )
        {
            if( auto transform = actor->getTransform() )
            {
                auto enabled = isEnabled() && actor->isEnabledInScene();
                if( enabled )
                {
                    if( actor->isSmoothMotion() )
                    {
                        transform->setTask( TaskId::Physics );
                    }
                    else
                    {
                        transform->setTask( TaskId::None );
                    }
                }
                else
                {
                    transform->setTask( TaskId::None );
                }
            }
        }
    }

}  // namespace workphone::scene
