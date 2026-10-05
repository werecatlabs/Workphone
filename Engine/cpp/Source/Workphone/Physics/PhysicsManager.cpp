//
// Created by Zane Desir on 31/10/2021.
//
#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Physics/IConstraintD6.hpp>
#include <Workphone/Interface/Physics/IConstraintFixed3.hpp>
#include <Workphone/Interface/Physics/IConstraintDrive.hpp>
#include <Workphone/Interface/Physics/IConstraintLinearLimit.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicle3.hpp>
#include <Workphone/Interface/Physics/IPhysicsSoftBody3.hpp>
#include <Workphone/Interface/Physics/ICharacterController3.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Math/OBB3.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Core/UnorderedMap.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysicsManager, IPhysicsManager );

    PhysicsManager::PhysicsManager() = default;

    PhysicsManager::~PhysicsManager() = default;

    void PhysicsManager::load( SmartPtr<ISharedObject> data )
    {
    }

    void PhysicsManager::unload( SmartPtr<ISharedObject> data )
    {
        if( auto physicsScene = getPhysicsScene() )
        {
            physicsScene->clear();
            physicsScene->unload( nullptr );
            setPhysicsScene( nullptr );
        }

        if( auto controlsScene = getControlsScene() )
        {
            controlsScene->unload( nullptr );
            setControlsScene( nullptr );
        }

        if( auto raycastScene = getRaycastScene() )
        {
            raycastScene->unload( nullptr );
            setRaycastScene( nullptr );
        }

        if( auto objectsScene = getObjectsScene() )
        {
            objectsScene->unload( nullptr );
            setObjectsScene( nullptr );
        }
    }

    bool PhysicsManager::getEnableDebugDraw() const
    {
        return m_debugDrawEnabled.load();
    }

    void PhysicsManager::setEnableDebugDraw( bool enableDebugDraw )
    {
        m_debugDrawEnabled = enableDebugDraw;

        if( !enableDebugDraw )
        {
            DebugForceCommand command;
            while( m_debugForces.try_pop( command ) )
            {
            }
        }
    }

    void PhysicsManager::debugDraw()
    {
        if( !getEnableDebugDraw() )
        {
            return;
        }

        if( auto debug = getDebugRenderer() )
        {
            drawQueuedDebugForces( *debug );
        }
        else
        {
            DebugForceCommand command;
            while( m_debugForces.try_pop( command ) )
            {
            }
        }
    }

    void PhysicsManager::queueDebugForce( hash_type bodyId, const Vector3<real_Num> &position,
                                          const Vector3<real_Num> &force )
    {
        if( !getEnableDebugDraw() || !position.isFinite() || !force.isFinite() ||
            force.dotProduct( force ) <= Math<real_Num>::epsilon() )
        {
            return;
        }

        m_debugForces.push( DebugForceCommand{ bodyId, position, force } );
    }

    real_Num PhysicsManager::getDebugForceScale() const
    {
        return static_cast<real_Num>( m_debugForceScale );
    }

    void PhysicsManager::setDebugForceScale( real_Num scale )
    {
        if( Math<real_Num>::isFinite( scale ) )
        {
            m_debugForceScale =
                static_cast<f32>( Math<real_Num>::max( scale, static_cast<real_Num>( 0.0 ) ) );
        }
    }

    SmartPtr<render::IDebug> PhysicsManager::getDebugRenderer() const
    {
        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
            {
                return graphicsSystem->getDebug();
            }
        }

        return nullptr;
    }

    void PhysicsManager::drawDebugBody( render::IDebug &debug, const IRigidBody3 &body ) const
    {
        const auto localBounds = body.getLocalAABB();
        const auto worldBounds = body.getWorldAABB();
        if( !localBounds.isFinite() || !localBounds.isValid() || !worldBounds.isFinite() ||
            !worldBounds.isValid() )
        {
            return;
        }

        const auto id = body.getId() ^ static_cast<hash_type>( 0x50485953u );
        debug.drawAABB( id, worldBounds, 0x00ff00ffu );
        debug.drawOBB( id, OBB3<real_Num>( localBounds, body.getTransform() ), 0x00ffffffu );
    }

    void PhysicsManager::drawQueuedDebugForces( render::IDebug &debug )
    {
        struct AccumulatedForce
        {
            Vector3<real_Num> position = Vector3<real_Num>::zero();
            Vector3<real_Num> force = Vector3<real_Num>::zero();
        };

        UnorderedMap<hash_type, AccumulatedForce> forces;
        DebugForceCommand command;
        while( m_debugForces.try_pop( command ) )
        {
            auto &entry = forces[command.bodyId];
            entry.position = command.position;
            entry.force += command.force;
        }

        const auto forceScale = getDebugForceScale();
        for( const auto &entry : forces )
        {
            const auto id = entry.first ^ static_cast<hash_type>( 0x464f5243u );
            debug.drawArrow( id, entry.second.position, entry.second.force * forceScale, 0xff0000ffu );
        }
    }

    SmartPtr<IPhysicsMaterial3> PhysicsManager::addMaterial()
    {
        return nullptr;
    }

    void PhysicsManager::removeMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
    }

    SmartPtr<IPhysicsScene3> PhysicsManager::addScene()
    {
        return nullptr;
    }

    void PhysicsManager::removeScene( SmartPtr<IPhysicsScene3> scene )
    {
    }

    bool PhysicsManager::removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape )
    {
        return false;
    }

    bool PhysicsManager::removePhysicsBody( SmartPtr<IRigidBody3> body )
    {
        return false;
    }

    SmartPtr<ICharacterController3> PhysicsManager::addCharacter()
    {
        return nullptr;
    }

    SmartPtr<IRigidStatic3> PhysicsManager::addRigidStatic( const Transform3<real_Num> &transform )
    {
        return nullptr;
    }

    SmartPtr<IRigidStatic3> PhysicsManager::addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape )
    {
        return nullptr;
    }

    SmartPtr<IRigidStatic3> PhysicsManager::addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                                            SmartPtr<Properties> properties )
    {
        return nullptr;
    }

    bool PhysicsManager::rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                                  Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                  u32 collisionType /*= 0*/, u32 collisionMask /*= 0 */ )
    {
        return false;
    }

    bool PhysicsManager::intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                     Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                     SmartPtr<ISharedObject> &object, u32 collisionType /*= 0*/,
                                     u32 collisionMask /*= 0 */ )
    {
        return false;
    }

    SmartPtr<IConstraintD6> PhysicsManager::addConstraintD6( SmartPtr<IPhysicsBody3> actor0,
                                                             const Transform3<real_Num> &localFrame0,
                                                             SmartPtr<IPhysicsBody3> actor1,
                                                             const Transform3<real_Num> &localFrame1 )
    {
        return nullptr;
    }

    SmartPtr<IConstraintFixed3> PhysicsManager::addFixedConstraint(
        SmartPtr<IPhysicsBody3> actor0, const Transform3<real_Num> &localFrame0,
        SmartPtr<IPhysicsBody3> actor1, const Transform3<real_Num> &localFrame1 )
    {
        return nullptr;
    }

    void PhysicsManager::removeConstraint( SmartPtr<IPhysicsConstraint3> constraint )
    {
        ScopedLock lock( this );

        unloadObject( constraint );
        m_constraints.erase( std::remove( m_constraints.begin(), m_constraints.end(), constraint ),
                             m_constraints.end() );
    }

    SmartPtr<IConstraintDrive> PhysicsManager::addConstraintDrive()
    {
        return nullptr;
    }

    SmartPtr<IConstraintLinearLimit> PhysicsManager::addConstraintLinearLimit( real_Num extent,
                                                                               real_Num contactDist )
    {
        return nullptr;
    }

    SmartPtr<IRaycastHit> PhysicsManager::addRaycastHitData()
    {
        return nullptr;
    }

    void PhysicsManager::removeRaycastHitData( SmartPtr<IRaycastHit> raycastHitData )
    {
    }

    TaskId PhysicsManager::getStateTask() const
    {
        return static_cast<TaskId>( 0 );
    }

    TaskId PhysicsManager::getPhysicsTask() const
    {
        return static_cast<TaskId>( 0 );
    }

    void PhysicsManager::loadObject( SmartPtr<ISharedObject> object, bool forceQueue /*= false */ )
    {
    }

    void PhysicsManager::unloadObject( SmartPtr<ISharedObject> object, bool forceQueue /*= false */ )
    {
    }

    SmartPtr<IPhysicsScene3> PhysicsManager::getPhysicsScene() const
    {
        return m_physicsScene;
    }

    void PhysicsManager::setPhysicsScene( SmartPtr<IPhysicsScene3> physicsScene )
    {
        m_physicsScene = physicsScene;
    }

    SmartPtr<IPhysicsScene3> PhysicsManager::getObjectsScene() const
    {
        return m_objectsScene;
    }

    void PhysicsManager::setObjectsScene( SmartPtr<IPhysicsScene3> objectsScene )
    {
        m_objectsScene = objectsScene;
    }

    SmartPtr<IPhysicsScene3> PhysicsManager::getRaycastScene() const
    {
        return m_raycastScene;
    }

    void PhysicsManager::setRaycastScene( SmartPtr<IPhysicsScene3> raycastScene )
    {
        m_raycastScene = raycastScene;
    }

    SmartPtr<IPhysicsScene3> PhysicsManager::getControlsScene() const
    {
        return m_controlsScene;
    }

    void PhysicsManager::setControlsScene( SmartPtr<IPhysicsScene3> controlsScene )
    {
        m_controlsScene = controlsScene;
    }

    void PhysicsManager::lock()
    {
        m_mutex.lock();
    }

    bool PhysicsManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void PhysicsManager::unlock()
    {
        m_mutex.unlock();
    }

    SmartPtr<IRigidDynamic3> PhysicsManager::addRigidDynamic( const Transform3<real_Num> &transform )
    {
        return nullptr;
    }
}  // namespace workphone::physics
