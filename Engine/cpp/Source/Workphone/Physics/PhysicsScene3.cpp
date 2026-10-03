//
// Created by Zane Desir on 31/10/2021.
//
#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/PhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/State/States/PhysicsSceneState.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysicsScene3, IPhysicsScene3 );
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysicsScene3::StateListener, IStateListener );

    PhysicsScene3::PhysicsScene3() = default;

    PhysicsScene3::~PhysicsScene3() = default;

    void PhysicsScene3::clear()
    {
    }

    void PhysicsScene3::addActor( SmartPtr<IPhysicsBody3> body )
    {
    }

    void PhysicsScene3::removeActor( SmartPtr<IPhysicsBody3> body )
    {
    }

    void PhysicsScene3::setSize( const Vector3<real_Num> &size )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->size = size;
            }
        }
    }

    Vector3<real_Num> PhysicsScene3::getSize() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->size;
            }
        }

        return Vector3<real_Num>::zero();
    }

    bool PhysicsScene3::rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                                 Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                 u32 collisionType /*= 0*/, u32 collisionMask /*= 0 */ )
    {
        return false;
    }

    bool PhysicsScene3::intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                    Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                    SmartPtr<ISharedObject> &object, u32 collisionType /*= 0*/,
                                    u32 collisionMask /*= 0 */ )
    {
        return false;
    }

    bool PhysicsScene3::castRay( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                                 Array<SmartPtr<IRaycastHit>> &hits )
    {
        return false;
    }

    bool PhysicsScene3::castRay( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit )
    {
        return false;
    }

    void PhysicsScene3::setGravity( const Vector3<real_Num> &gravity )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->gravity = gravity;
            }
        }
    }

    Vector3<real_Num> PhysicsScene3::getGravity() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->gravity;
            }
        }

        return Vector3<real_Num>::zero();
    }

    void PhysicsScene3::simulate( real_Num elapsedTime, void *scratchMemBlock, u32 scratchMemBlockSize,
                                  bool controlSimulation )
    {
    }

    bool PhysicsScene3::fetchResults( bool block, u32 *errorState )
    {
        return false;
    }

    SmartPtr<IStateContext> PhysicsScene3::getStateContext() const
    {
        return m_stateContext.load();
    }

    void PhysicsScene3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    bool PhysicsScene3::castRayDynamic( const Ray3<real_Num> &ray, SmartPtr<IRaycastHit> hit )
    {
        return false;
    }

    u32 PhysicsScene3::getMinThreads() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->minThreads;
            }
        }

        return 0;
    }

    void PhysicsScene3::setMinThreads( u32 minThreads )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->minThreads = minThreads;
            }
        }
    }

    u32 PhysicsScene3::getMaxThreads() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->maxThreads;
            }
        }

        return 0;
    }

    void PhysicsScene3::setMaxThreads( u32 maxThreads )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->maxThreads = maxThreads;
            }
        }
    }

    void PhysicsScene3::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            physicsManager->unlock();
        }
    }

    void PhysicsScene3::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            physicsManager->lock();
        }
    }

    bool PhysicsScene3::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
        {
            return physicsManager->try_lock();
        }

        return false;
    }

    void PhysicsScene3::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    SmartPtr<IStateListener> PhysicsScene3::getStateListener() const
    {
        return m_stateListener;
    }

    void PhysicsScene3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
    }

    void PhysicsScene3::handleStateChanged( SmartPtr<IState> &state )
    {
    }

    u32 PhysicsScene3::numStaticActors() const
    {
        u32 count = 0;
        for( auto &actor : m_rigidBodies )
        {
            if( actor && actor->isDerived<IRigidStatic3>() )
            {
                ++count;
            }
        }

        return count;
    }

    u32 PhysicsScene3::numDynamicActors() const
    {
        u32 count = 0;
        for( auto &actor : m_rigidBodies )
        {
            if( actor && actor->isDerived<IRigidDynamic3>() )
            {
                ++count;
            }
        }

        return count;
    }

    bool PhysicsScene3::hasActor( SmartPtr<IPhysicsBody3> body ) const
    {
        for( auto &actor : m_rigidBodies )
        {
            if( body == actor )
            {
                return true;
            }
        }

        return false;
    }

    Array<SmartPtr<IPhysicsBody3>> PhysicsScene3::getActors() const
    {
        auto bodies = m_rigidBodies.snapshot();
        return { bodies.begin(), bodies.end() };
    }

    PhysicsScene3::StateListener::StateListener() = default;

    PhysicsScene3::StateListener::~StateListener() = default;

    void PhysicsScene3::StateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    bool PhysicsScene3::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            owner->handleStateChanged( message );
        }

        return false;
    }

    bool PhysicsScene3::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            owner->handleStateChanged( state );
        }

        return false;
    }

    SmartPtr<PhysicsScene3> PhysicsScene3::StateListener::getOwner() const
    {
        auto owner = m_owner.load();
        return owner.lock();
    }

    void PhysicsScene3::StateListener::setOwner( SmartPtr<PhysicsScene3> owner )
    {
        m_owner = owner;
    }

    void PhysicsScene3::setSpatialPartitioning( SpatialPartitioningMethodEnum method )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->spatialPartitioning = method;
            }
        }
    }

    SpatialPartitioningMethodEnum PhysicsScene3::getSpatialPartitioning() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->spatialPartitioning;
            }
        }
        return SpatialPartitioningMethodEnum::None;
    }

    void PhysicsScene3::setSpatialOptions( const SpatialPartitioningOptions &options )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->spatialOptions = options;
            }
        }
    }

    SpatialPartitioningOptions PhysicsScene3::getSpatialOptions() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->spatialOptions;
            }
        }
        return SpatialPartitioningOptions();
    }

    void PhysicsScene3::setContactOptions( const ContactOptions &options )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->invalidateStateData<PhysicsSceneState>() )
            {
                physicsSceneState->contactOptions = options;
            }
        }
    }

    ContactOptions PhysicsScene3::getContactOptions() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto physicsSceneState = stateContext->getStateData<PhysicsSceneState>() )
            {
                return physicsSceneState->contactOptions;
            }
        }
        return ContactOptions();
    }

}  // namespace workphone::physics
