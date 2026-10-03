#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxScene.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>
#include <WPPhysx/WPPhysxManager.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxScene, PhysicsScene3 );

    using namespace physx;

    namespace
    {
        bool passesCollisionMask( const PxFilterData &filterData0, const PxFilterData &filterData1 )
        {
            return ( filterData0.word0 == 0 && filterData1.word0 == 0 ) ||
                   ( ( filterData0.word0 & filterData1.word1 ) != 0 ) ||
                   ( ( filterData1.word0 & filterData0.word1 ) != 0 );
        }
    } // namespace

    PhysxScene::PhysxScene() = default;

    PhysxScene::~PhysxScene()
    {
        unload( nullptr );
    }

    void PhysxScene::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager =
                workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
            auto physics = physicsManager->getPhysics();

            auto gravity = Vector3<physics_Num>( 0.0f, -9.81f, 0.0f );
            // properties->getPropertyAsVector3D("gravity",
            // Vector3<physics_Num>(0.0f, -9.8f, 0.0f));

            PxSceneDesc sceneDesc( physics->getTolerancesScale() );
            sceneDesc.gravity = PxVec3( gravity.X(), gravity.Y(), gravity.Z() );
            sceneDesc.broadPhaseType = PxBroadPhaseType::eMBP;
            sceneDesc.frictionOffsetThreshold = 0.05f;
            sceneDesc.maxNbContactDataBlocks = 1024;
            sceneDesc.ccdMaxPasses = 0;
            sceneDesc.solverBatchSize = 2;
            sceneDesc.dynamicStructure = PxPruningStructure::Enum::eNONE;
            sceneDesc.staticStructure = PxPruningStructure::Enum::eDYNAMIC_AABB_TREE;
            sceneDesc.dynamicTreeRebuildRateHint = 1000;

            PxSceneLimits limits;
            limits.maxNbActors = 256;
            limits.maxNbBodies = 256;
            limits.maxNbStaticShapes = 256;
            limits.maxNbDynamicShapes = 256;
            limits.maxNbAggregates = 256;
            limits.maxNbConstraints = 256;
            limits.maxNbRegions = 256;
            limits.maxNbObjectsPerRegion = 256;

            sceneDesc.limits = limits;

            auto minThreads = getMinThreads();
            auto maxThreads = getMaxThreads();

            auto numThreads = Thread::physical_concurrency();

            if( numThreads < minThreads )
            {
                numThreads = minThreads;
            }

            if( numThreads > maxThreads )
            {
                numThreads = maxThreads;
            }

            m_cpuDispatcher = PxDefaultCpuDispatcherCreate( numThreads );
            if( !m_cpuDispatcher )
            {
                WP_LOG_ERROR( "PxDefaultCpuDispatcherCreate failed!" );
            }

            sceneDesc.cpuDispatcher = m_cpuDispatcher;

            if( !sceneDesc.filterShader )
            {
                // sceneDesc.filterShader = PxDefaultSimulationFilterShader;
                sceneDesc.filterShader = simulationFilterShader;
            }

            sceneDesc.frictionType = PxFrictionType::ePATCH;

            m_collisionCallback = new CollisionCallback;
            sceneDesc.filterCallback = m_collisionCallback;

            m_simulationEventCallback = new SimulationEventCallback;
            sceneDesc.simulationEventCallback = m_simulationEventCallback;

            m_contactModificationCallback = new ContactModificationCallback;
            sceneDesc.contactModifyCallback = m_contactModificationCallback;

            sceneDesc.flags = PxSceneFlag::eENABLE_PCM;
            sceneDesc.frictionOffsetThreshold = 0.05f;

            sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

#ifdef PX_WINDOWS
//            if( !sceneDesc.gpuDispatcher && mCudaContextManager )
//            {
//                sceneDesc.gpuDispatcher = mCudaContextManager->getGpuDispatcher();
//            }
#endif

            // sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;
            // sceneDesc.flags |= physx::PxSceneFlag::eENABLE_STABILIZATION;
            // sceneDesc.flags |= physx::PxSceneFlag::eADAPTIVE_FORCE;
            sceneDesc.flags |= PxSceneFlag::eENABLE_AVERAGE_POINT;

            // #if !WP_FINAL
            //		sceneDesc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
            // #endif

            sceneDesc.broadPhaseType = PxBroadPhaseType::eSAP;

            auto pScene = physics->createScene( sceneDesc );
            setScene( pScene );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxScene::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                if( auto scene = getScene() )
                {
                    auto numActors = scene->getNbActors( PxActorTypeSelectionFlag::eRIGID_DYNAMIC );
                    WP_ASSERT( numActors == 0 );

                    auto numStaticActors = scene->getNbActors( PxActorTypeSelectionFlag::eRIGID_STATIC );
                    WP_ASSERT( numStaticActors == 0 );
                }

                clear();

                if( auto scene = getScene() )
                {
                    auto numActors = scene->getNbActors( PxActorTypeSelectionFlag::eRIGID_DYNAMIC );
                    WP_ASSERT( numActors == 0 );

                    auto numStaticActors = scene->getNbActors( PxActorTypeSelectionFlag::eRIGID_STATIC );
                    WP_ASSERT( numStaticActors == 0 );

                    scene->release();
                    setScene( nullptr );
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxScene::setGravity( const Vector3<real_Num> &gravity )
    {
        ScopedLock lock( this, true );
        PhysicsScene3::setGravity( gravity );
        if( auto scene = getScene() )
        {
            scene->setGravity( PhysxUtil::toPx( gravity ) );
        }
    }

    Vector3<real_Num> PhysxScene::getGravity() const
    {
        ScopedLock lock( this );
        if( auto scene = getScene() )
        {
            return PhysxUtil::toFB( scene->getGravity() );
        }
        return PhysicsScene3::getGravity();
    }

    void PhysxScene::preUpdate()
    {
    }

    void PhysxScene::update()
    {
        TryLockGuard lock( this );
        if( lock.locked() && isLoaded() )
        {
            if( auto scene = getScene() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto timer = applicationManager->getTimer();
                WP_ASSERT( timer );

                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();

                auto physicsManager = applicationManager->getPhysicsManagerPtr();
                WP_ASSERT( physicsManager );

                WP_ASSERT( Math<time_interval>::isFinite( t ) );
                WP_ASSERT( Math<time_interval>::isFinite( dt ) );

                if( dt < Math<time_interval>::epsilon() || dt > m_maxDeltaTime )
                {
                    dt = m_maxDeltaTime;
                }

                if( applicationManager->isPlaying() )
                {
                    auto fDT = static_cast<f32>( dt );

                    scene->simulate( fDT );
                    scene->fetchResults( true );

#if !WP_USE_PHYSX_ACTIVE_TRANSFORMS
                    auto &rigidBodies = getActors();
                    for( auto rigidBody : rigidBodies )
                    {
                        auto pRigid = workphone::dynamic_pointer_cast<PhysxRigidDynamic>( rigidBody );
                        if( pRigid )
                        {
                            if( auto pxBody = pRigid->getActor() )
                            {
                                auto pxTransform = pxBody->getGlobalPose();
                                auto transform = PhysxUtil::toFB( pxTransform );
                                pRigid->setActiveTransform( transform );
                            }
                        }
                    }
#endif
                }
                else
                {
#if WP_PHYSX_DEBUG
                    scene->simulate( 1.0 / 100000.0 ); // todo work around for visualization in debug
                    scene->fetchResults();
#endif
                }

#if WP_USE_PHYSX_ACTIVE_TRANSFORMS
                if( applicationManager->isPlaying() )
                {
                    auto numTransforms = static_cast<u32>( 0 );
                    auto transforms = scene->getActiveTransforms( numTransforms );

                    for( size_t i = 0; i < numTransforms; ++i )
                    {
                        const auto &activeTransform = transforms[i];
                        auto rigidBody = static_cast<PhysxRigidDynamic *>( activeTransform.userData );
                        if( rigidBody )
                        {
                            auto &pxTransform = activeTransform.actor2World;
                            WP_ASSERT( pxTransform.isSane() );
                            WP_ASSERT( pxTransform.isValid() );

                            auto transform = PhysxUtil::toFB( pxTransform );
                            WP_ASSERT( transform.isSane() );
                            WP_ASSERT( transform.isValid() );

                            rigidBody->setActiveTransform( transform );
                        }
                    }
                }
#endif
            }
        }
    }

    void PhysxScene::postUpdate()
    {
        TryLockGuard lock( this );
        if( lock.locked() && isLoaded() )
        {
            if( auto scene = getScene() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto timer = applicationManager->getTimer();
                WP_ASSERT( timer );

                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();

                auto task = Thread::getCurrentTask();

                auto physicsManager = applicationManager->getPhysicsManager();
                WP_ASSERT( physicsManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                for( auto body : m_rigidBodies )
                {
                    if( body )
                    {
                        WP_ASSERT( body->isValid() );
                    }
                }
            }
        }
    }

    void PhysxScene::clear()
    {
        ScopedLock lock( this, true );

        if( auto scene = getScene() )
        {
            for( auto body : m_rigidBodies )
            {
                if( body )
                {
                    body->unload( nullptr );
                }
            }

            m_rigidBodies.clear();
        }
    }

    bool PhysxScene::rayTest( const Vector3F &start, const Vector3F &direction, Vector3F &hitPos,
                              Vector3F &hitNormal, u32 collisionType, u32 collisionMask )
    {
        return false;
    }

    bool PhysxScene::intersects( const Vector3F &start, const Vector3F &end, Vector3F &hitPos,
                                 Vector3F &hitNormal, SmartPtr<ISharedObject> &object, u32 collisionType,
                                 u32 collisionMask )
    {
        auto ray = Ray3F( start, end - start );
        auto hit = workphone::make_ptr<RaycastHit>();

        if( castRay( ray, hit ) )
        {
            hitPos = hit->getPoint();
            hitNormal = hit->getNormal();
            object = hit;
            return true;
        }

        return false;
    }

    PxScene *PhysxScene::getScene() const
    {
        return m_scene.load();
    }

    void PhysxScene::setScene( PxScene *scene )
    {
        m_scene = scene;
    }

    void PhysxScene::simulate( physics_Num elapsedTime, void *scratchMemBlock, u32 scratchMemBlockSize,
                               bool controlSimulation )
    {
        if( auto scene = getScene() )
        {
            ScopedLock lock( this, true );
            scene->simulate( elapsedTime, nullptr, scratchMemBlock, scratchMemBlockSize,
                             controlSimulation );
        }
    }

    bool PhysxScene::fetchResults( bool block, u32 *errorState )
    {
        if( auto scene = getScene() )
        {
            ScopedLock lock( this, true );
            return scene->fetchResults( block, errorState );
        }

        return false;
    }

    void PhysxScene::addActor( SmartPtr<IPhysicsBody3> body )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            ScopedLock lock( this, true );

            if( !body || hasActor( body ) )
            {
                return;
            }

            if( auto scene = getScene() )
            {
                WP_ASSERT( body );

                if( body )
                {
                    m_rigidBodies.push_back( body );

                    auto pRigidbody = workphone::dynamic_pointer_cast<PhysxRigidDynamic>( body );
                    if( pRigidbody )
                    {
                        if( !pRigidbody->isLoaded() )
                        {
                            pRigidbody->load( nullptr );
                        }

                        if( auto actor = pRigidbody->getActor() )
                        {
                            // WP_ASSERT( actor->getNbShapes() > 0 );

                            auto currentScene = actor->getScene();
                            if( currentScene == nullptr )
                            {
                                scene->addActor( *actor );
                            }
                        }
                    }

                    auto pRigidStatic = workphone::dynamic_pointer_cast<PhysxRigidStatic>( body );
                    if( pRigidStatic )
                    {
                        if( !pRigidStatic->isLoaded() )
                        {
                            pRigidStatic->load( nullptr );
                        }

                        auto shapes = pRigidStatic->getShapes();
                        for( auto shape : shapes )
                        {
                            if( !shape->isLoaded() )
                            {
                                shape->load( nullptr );
                            }
                        }

                        if( auto actor = pRigidStatic->getRigidStatic() )
                        {
                            // WP_ASSERT( actor->getNbShapes() > 0 );

                            auto currentScene = actor->getScene();
                            if( currentScene == nullptr )
                            {
                                if( actor->getNbShapes() > 0 )
                                {
                                    scene->addActor( *actor );
                                }
                            }
                        }
                    }
                }
            }

            body->setScene( this );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxScene::removeActor( SmartPtr<IPhysicsBody3> body )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            ScopedLock lock( this, true );

            if( body )
            {
                m_rigidBodies.erase( std::remove( m_rigidBodies.begin(), m_rigidBodies.end(), body ),
                                     m_rigidBodies.end() );

                auto pRigidbody = workphone::dynamic_pointer_cast<PhysxRigidDynamic>( body );
                if( pRigidbody )
                {
                    auto actor = pRigidbody->getActor();
                    if( actor )
                    {
                        if( auto scene = actor->getScene() )
                        {
                            scene->removeActor( *actor );
                        }
                    }
                }

                auto pRigidStatic = workphone::dynamic_pointer_cast<PhysxRigidStatic>( body );
                if( pRigidStatic )
                {
                    auto actor = pRigidStatic->getRigidStatic();
                    if( actor )
                    {
                        if( auto scene = actor->getScene() )
                        {
                            scene->removeActor( *actor );
                        }
                    }
                }
            }

            body->setScene( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool PhysxScene::castRay( const Vector3<physics_Num> &origin, const Vector3<physics_Num> &dir,
                              Array<SmartPtr<IRaycastHit>> &hits )
    {
        ScopedLock lock( this, true );

        using namespace physx;

        if( !origin.isFinite() || !dir.isFinite() || dir.lengthSquared() < Math<physics_Num>::epsilon() )
        {
            return false;
        }

        auto normalisedDir = dir;
        normalisedDir.normalise();

        auto   rorigin = PxVec3( origin.X(), origin.Y(), origin.Z() ); // [in] Ray origin
        auto   unitDir = PxVec3( normalisedDir.X(), normalisedDir.Y(),
                                 normalisedDir.Z() ); // [in] Normalized ray direction
        PxReal maxDistance = 100000;                  //-vec.y;       // [in] Raycast max distance

        RaycastCallback raycastCallback;

        PxHitFlags        hitFlags( PxHitFlag::eDEFAULT );
        PxQueryFilterData pxQueryFilterData( PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC );

        if( auto scene = getScene() )
        {
            bool status = scene->raycast( rorigin, unitDir, maxDistance, raycastCallback, hitFlags,
                                          pxQueryFilterData );
            if( status )
            {
                float distance = raycastCallback.m_hit.distance >= fabs( 100000.0f )
                                   ? 0.0f
                                   : raycastCallback.m_hit.distance;

                auto hitPos = origin + ( normalisedDir * distance );

                auto hit = workphone::make_ptr<RaycastHit>();
                hit->setDistance( distance );
                hit->setPoint( hitPos );
                hits.emplace_back( hit );

                return true;
            }
        }

        return false;
    }

    bool PhysxScene::castRay( const Ray3<physics_Num> &ray, SmartPtr<IRaycastHit> hit )
    {
        ScopedLock lock( this, true );

        using namespace physx;

        WP_ASSERT( MathUtil<physics_Num>::isFinite( ray.getOrigin() ) );
        WP_ASSERT( MathUtil<physics_Num>::isFinite( ray.getDirection() ) );

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();

        const Vector3<physics_Num> &origin = ray.getOrigin();
        const Vector3<physics_Num> &dir = ray.getDirection();

        auto      rorigin = PxVec3( origin.X(), origin.Y(), origin.Z() ); // [in] Ray origin
        auto      unitDir = PxVec3( dir.X(), dir.Y(), dir.Z() ); // [in] Normalized ray direction
        const f32 maxDistance = 100000.0f; //-vec.y;       // [in] Raycast max distance

        // const PxSceneQueryFlags outputFlags = PxSceneQueryFlag::eDISTANCE |
        // PxSceneQueryFlag::eIMPACT | PxSceneQueryFlag::eNORMAL; const PxSceneQueryFlags outputFlags
        // = PxSceneQueryFlag::eDISTANCE;

        // PxSceneQueryFilterData filterData(PxSceneQueryFilterFlag::eSTATIC);
        // filterData.flags |= PxQueryFlag::eANY_HIT;

        PxHitFlags        hitFlags( PxHitFlag::eDEFAULT );
        PxQueryFilterData pxQueryFilterData( PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC );
        // PxQueryFilterData pxQueryFilterData( PxQueryFlag::eSTATIC );

        if( auto scene = getScene() )
        {
            RaycastCallback raycastCallback;
            raycastCallback.m_checkStatic = true;
            raycastCallback.m_checkDynamic = false;

            bool status = scene->raycast( rorigin, unitDir, maxDistance, raycastCallback, hitFlags,
                                          pxQueryFilterData );
            if( status )
            {
                auto d = raycastCallback.m_hit.distance;
                if( d < maxDistance )
                {
                    const auto &n = raycastCallback.m_hit.normal;

                    hit->setDistance( d );
                    hit->setPoint( origin + ( dir * d ) );
                    hit->setNormal( Vector3<physics_Num>( n.x, n.y, n.z ) );

                    WP_ASSERT( MathF::isFinite( d ) );
                    WP_ASSERT( MathUtil<physics_Num>::isFinite( hit->getPoint() ) );
                    WP_ASSERT( MathUtil<physics_Num>::isFinite( hit->getNormal() ) );

                    return true;
                }
            }
        }

        return false;
    }

    bool PhysxScene::castRayDynamic( const Ray3<physics_Num> &ray, SmartPtr<IRaycastHit> hit )
    {
        ScopedLock lock( this, true );

        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();

        using namespace physx;

        auto origin = ray.getOrigin();
        auto dir = ray.getDirection();

        auto   rorigin = PxVec3( origin.X(), origin.Y(), origin.Z() ); // [in] Ray origin
        auto   unitDir = PxVec3( dir.X(), dir.Y(), dir.Z() );          // [in] Normalized ray direction
        PxReal maxDistance = 100000; //-vec.y;       // [in] Raycast max distance

        // const physx::PxSceneQueryFlags outputFlags = physx::PxSceneQueryFlag::eDISTANCE |
        // physx::PxSceneQueryFlag::eIMPACT | physx::PxSceneQueryFlag::eNORMAL; const
        // physx::PxSceneQueryFlags outputFlags = physx::PxSceneQueryFlag::eDISTANCE;

        // physx::PxSceneQueryFilterData filterData(physx::PxSceneQueryFilterFlag::eSTATIC);
        // filterData.flags |= physx::PxQueryFlag::eANY_HIT;

        RaycastCallback raycastCallback;

        PxHitFlags        hitFlags( PxHitFlag::eDEFAULT );
        PxQueryFilterData pxQueryFilterData( PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC );

        if( auto scene = getScene() )
        {
            bool status = scene->raycast( rorigin, unitDir, maxDistance, raycastCallback, hitFlags,
                                          pxQueryFilterData );
            if( status )
            {
                float distance = raycastCallback.m_hit.distance >= fabs( 100000.0f )
                                   ? 0.0f
                                   : raycastCallback.m_hit.distance;

                auto hitPos = origin + ( dir * distance );

                hit->setDistance( distance );
                hit->setPoint( hitPos );

                return true;
            }
        }

        return false;
    }

    void PhysxScene::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
    }

    void PhysxScene::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto physicsSceneState = state->getDataByType<PhysicsSceneState>() )
        {
            if( auto pxScene = getScene() )
            {
                auto gravity = PhysxUtil::toPx( physicsSceneState->gravity );
                pxScene->setGravity( gravity );
            }
        }
    }

    class QueryFilterCallback : public PxQueryFilterCallback
    {
    public:
        QueryFilterCallback() = default;

        ~QueryFilterCallback() override = default;

        PxQueryHitType::Enum preFilter( const PxFilterData &filterData, const PxShape *shape,
                                        const PxRigidActor *actor, PxHitFlags &queryFlags ) override
        {
            if( shape && !passesCollisionMask( filterData, shape->getQueryFilterData() ) )
            {
                return PxQueryHitType::eNONE;
            }

            return PxQueryHitType::eBLOCK;
        }

        PxQueryHitType::Enum postFilter( const PxFilterData &filterData, const PxQueryHit &hit ) override
        {
            return PxQueryHitType::eBLOCK;
        }
    };

    PxAgain PhysxScene::RaycastCallback::processTouches( const PxRaycastHit *buffer, u32 nbHits )
    {
        using namespace physx;

        for( u32 i = 0; i < nbHits; ++i )
        {
            const PxRaycastHit &hit = buffer[i];
            if( hit.distance < m_closestHit )
            {
                auto actor = hit.actor;
                auto shape = hit.shape;

                auto staticActor = actor->isRigidStatic();
                auto dynamicActor = actor->isRigidDynamic();

                if( m_checkStatic && staticActor )
                {
                    // if( shape->getFlags().isSet( PxShapeFlag::eSIMULATION_SHAPE ) )
                    {
                        m_hit = hit;
                        m_closestHit = hit.distance;
                    }
                }
                else if( m_checkDynamic && dynamicActor )
                {
                    // if( dynamicActor->getRigidDynamicFlags() & PxRigidDynamicFlag::eKINEMATIC )
                    {
                        m_hit = hit;
                        m_closestHit = hit.distance;
                    }
                }
            }
        }

        return false;
    }

    PhysxScene::RaycastCallback::RaycastCallback() :
        physx::PxRaycastCallback( buffer, 10 ),
        m_closestHit( static_cast<f32>( 1e10 ) )
    {
    }

    PxFilterFlags PhysxScene::simulationFilterShader( PxFilterObjectAttributes attributes0,
                                                      PxFilterData             filterData0,
                                                      PxFilterObjectAttributes attributes1,
                                                      PxFilterData filterData1, PxPairFlags &pairFlags,
                                                      const void *constantBlock,
                                                      PxU32       constantBlockSize )
    {
        // let triggers through
        if( PxFilterObjectIsTrigger( attributes0 ) || PxFilterObjectIsTrigger( attributes1 ) )
        {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return {};
        }

        if( !passesCollisionMask( filterData0, filterData1 ) )
        {
            return PxFilterFlag::eSUPPRESS;
        }

        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
        return PxFilterFlag::eDEFAULT | PxFilterFlag::eNOTIFY;
    }

    void PhysxScene::setMaxDeltaTime( time_interval maxDeltaTime )
    {
        m_maxDeltaTime = maxDeltaTime;
    }

    time_interval PhysxScene::getMaxDeltaTime() const
    {
        return m_maxDeltaTime;
    }

    PhysxScene::CollisionCallback::CollisionCallback() = default;

    PhysxScene::CollisionCallback::~CollisionCallback() = default;

    PxFilterFlags PhysxScene::CollisionCallback::pairFound(
        PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor *a0,
        const PxShape *s0, PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        const PxActor *a1, const PxShape *s1, PxPairFlags &pairFlags )
    {
        return PxFilterFlag::eDEFAULT | PxFilterFlag::eNOTIFY;
    }

    bool PhysxScene::CollisionCallback::statusChange( PxU32 &pairID, PxPairFlags &pairFlags,
                                                      PxFilterFlags &filterFlags )
    {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND |
                    PxPairFlag::eNOTIFY_TOUCH_LOST;
        filterFlags = PxFilterFlag::eDEFAULT | PxFilterFlag::eNOTIFY;

        return false;
    }

    void PhysxScene::CollisionCallback::pairLost( PxU32 pairID, PxFilterObjectAttributes attributes0,
                                                  PxFilterData             filterData0,
                                                  PxFilterObjectAttributes attributes1,
                                                  PxFilterData filterData1, bool objectDeleted )
    {
    }

    void PhysxScene::SimulationEventCallback::onConstraintBreak( PxConstraintInfo *constraints,
                                                                 PxU32             count )
    {
        int stop = 0;
        stop = 0;
    }

    void PhysxScene::SimulationEventCallback::onSleep( PxActor **actors, PxU32 count )
    {
        int stop = 0;
        stop = 0;
    }

    void PhysxScene::SimulationEventCallback::onWake( PxActor **actors, PxU32 count )
    {
        int stop = 0;
        stop = 0;
    }

    void PhysxScene::SimulationEventCallback::onContact( const PxContactPairHeader &pairHeader,
                                                         const PxContactPair *pairs, PxU32 nbPairs )
    {
    }

    void PhysxScene::SimulationEventCallback::onTrigger( PxTriggerPair *pairs, PxU32 count )
    {
    }

    void PhysxScene::SimulationEventCallback::checkBreakage( PxRigidDynamic *rb0, PxShape *shape )
    {
        int stop = 0;
        stop = 0;
    }

    PhysxScene::SimulationEventCallback::~SimulationEventCallback() = default;

    PhysxScene::SimulationEventCallback::SimulationEventCallback() = default;

    void PhysxScene::ContactModificationCallback::onContactModify( PxContactModifyPair *const pairs,
                                                                   PxU32                      count )
    {
    }
} // namespace workphone::physics
