#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <WPPhysx/WPPhysxAllocator.hpp>
#include <WPPhysx/WPPhysxPoolAllocator.hpp>
#include <WPPhysx/WPPhysxErrorOutput.hpp>
#include <WPPhysx/WPPhysxBoxShape.hpp>
#include <WPPhysx/WPPhysxPlaneShape.hpp>
#include <WPPhysx/WPPhysxCharacterController.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <WPPhysx/WPPhysxTerrain.hpp>
#include <WPPhysx/WPPhysxMeshShape.hpp>
#include <WPPhysx/WPPhysxSphereShape.hpp>
#include <WPPhysx/WPPhysxScene.hpp>
#include <WPPhysx/WPPhysxConstraintD6.hpp>
#include <WPPhysx/WPPhysxConstraintFixed3.hpp>
#include <WPPhysx/WPPhysxCooker.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <WPPhysx/WPPhysxConstraintDrive.hpp>
#include <WPPhysx/WPPhysxConstraintLimit.hpp>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>
#include <PxExtensionsAPI.h>

#ifdef WP_PHYSX_DEBUG
#    include "extensions/PxVisualDebuggerExt.h"
#endif

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxManager, PhysicsManager );

    PxVehicleTelemetryData *mTelemetryData4W;

    ///////////////////////////////////////////////////////////////////////////////

    enum Word3
    {
        SWEPT_INTEGRATION_LINEAR = 1,
    };

    namespace
    {
        template <class FilterData0, class FilterData1>
        bool passesCollisionMask( const FilterData0 &filterData0, const FilterData1 &filterData1 )
        {
            return ( filterData0.word0 == 0 && filterData1.word0 == 0 ) ||
                   ( ( filterData0.word0 & filterData1.word1 ) != 0 ) ||
                   ( ( filterData1.word0 & filterData0.word1 ) != 0 );
        }

        class CollisionMaskQueryFilter : public PxQueryFilterCallback
        {
        public:
            PxQueryHitType::Enum preFilter( const PxFilterData &filterData, const PxShape *shape,
                                            const PxRigidActor *actor, PxHitFlags &queryFlags ) override
            {
                if( shape && !passesCollisionMask( filterData, shape->getQueryFilterData() ) )
                {
                    return PxQueryHitType::eNONE;
                }

                return PxQueryHitType::eBLOCK;
            }

            PxQueryHitType::Enum postFilter( const PxFilterData &filterData,
                                             const PxQueryHit   &hit ) override
            {
                return PxQueryHitType::eBLOCK;
            }
        };
    } // namespace

    auto SampleVehicleFilterShader( PxFilterObjectAttributes attributes0, FilterData filterData0,
                                    PxFilterObjectAttributes attributes1, FilterData filterData1,
                                    PxPairFlags &pairFlags, const void *constantBlock,
                                    PxU32 constantBlockSize ) -> PxFilterFlags
    {
        // let triggers through
        if( PxFilterObjectIsTrigger( attributes0 ) || PxFilterObjectIsTrigger( attributes1 ) )
        {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return {};
        }

        // use a group-based mechanism for all other pairs:
        // - Objects within the default group (mask 0) always collide
        // - By default, objects of the default group do not collide
        //   with any other group. If they should collide with another
        //   group then this can only be specified through the filter
        //   data of the default group objects (objects of a different
        //   group can not choose to do so)
        // - For objects that are not in the default group, a bitmask
        //   is used to define the groups they should collide with
        if( !passesCollisionMask( filterData0, filterData1 ) )
        {
            return PxFilterFlag::eSUPPRESS;
        }

        pairFlags = PxPairFlag::eCONTACT_DEFAULT;

        // enable CCD stuff -- for now just for everything or nothing.
        // if((filterData0.word3|filterData1.word3) & SWEPT_INTEGRATION_LINEAR)
        //	pairFlags |= PxPairFlag::eSWEPT_INTEGRATION_LINEAR;

        // The pairFlags for each object are stored in word2 of the filter data. Combine them.
        pairFlags |= PxPairFlags( static_cast<PxU16>( filterData0.word2 | filterData1.word2 ) );
        return {};
    }

    PhysxManager::PhysxManager()
    {
        m_physics = nullptr;
        m_cooking = nullptr;
        m_defaultMaterial = nullptr;
    }

    PhysxManager::~PhysxManager()
    {
        unload( nullptr );
    }

    void PhysxManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_characters.reserve( 10 );

            m_allocator = workphone::make_shared<PhysxAllocator>();

            // Create a custom allocator callback object with a pool size of 512 MB
            // m_allocator = workphone::make_shared<PhysxPoolAllocator>( 512 * 1024 * 1024 );

            m_errorOutput = workphone::make_shared<PhysxErrorOutput>();

            m_foundation = PxCreateFoundation( PX_PHYSICS_VERSION, *m_allocator, *m_errorOutput );
            if( !m_foundation )
            {
                auto &foundation = PxGetFoundation();
                m_foundation = &foundation;
                WP_LOG( "Getting existing px foundation." );
            }

            auto scale = PxTolerancesScale();
            auto physics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_foundation, scale );
            if( !physics )
            {
                WP_LOG_INFO( "Error: SDK initialisation failed." );
            }

            auto params = PxCookingParams( scale );
            m_cooking = PxCreateCooking( PX_PHYSICS_VERSION, *m_foundation, params );
            if( !m_cooking )
            {
                WP_LOG_INFO( "[OgrePhysX] Error: Cooking initialisation failed." );
            }

            if( !PxInitExtensions( *physics ) )
            {
                WP_LOG_INFO( "PxInitExtensions failed!" );
            }

            // if (mPxPhysics->getPvdConnectionManager())
            //	PxExtensionVisualDebugger::connect(mPxPhysics->getPvdConnectionManager(),
            //"localhost", 5425, 500, true);

            // create default material
            m_defaultMaterial = physics->createMaterial( 0.5f, 0.5f, 0.1f );

            setPhysics( physics );

#if WP_PHYSX_DEBUG
            WP_LOG( "PhysicsManager::initialise PvdConnectionManager" );

            // connect to PVD
            auto cm = physics->getPvdConnectionManager();
            if( cm )
            {
                if( cm->isConnected() )
                {
                    cm->disconnect();
                }
                else
                {
                    bool mUseFullPvdConnection = true;
                    // The connection flags state overall what data is to be sent to PVD.  Currently
                    // the Debug connection flag requires support from the implementation (don't send
                    // the data when debug isn't set) but the other two flags, profile and memory
                    // are taken care of by the PVD SDK.

                    // Use these flags for a clean profile trace with minimal overhead
                    // PVD::TConnectionFlagsType theConnectionFlags( PVD::PvdConnectionType::Profile
                    // )
                    debugger::TConnectionFlagsType theConnectionFlags(
                        debugger::PvdConnectionType::eDEBUG | debugger::PvdConnectionType::ePROFILE |
                        debugger::PvdConnectionType::eMEMORY );
                    if( !mUseFullPvdConnection )
                    {
                        theConnectionFlags =
                            debugger::TConnectionFlagsType( debugger::PvdConnectionType::ePROFILE );
                    }

                    // Create a pvd connection that writes data straight to the filesystem.  This is
                    // the fastest connection on windows for various reasons.  First, the transport
                    // is quite fast as pvd writes data in blocks and filesystems work well with that
                    // abstraction. Second, you don't have the PVD application parsing data and using
                    // CPU and memory bandwidth while your application is running.
                    // PxExtensionVisualDebugger::connect(mPhysics->getPvdConnectionManager(),"c:\\temp.pxd2",
                    // PxDebuggerConnectionFlags( (PxU32)theConnectionFlags));

                    // The normal way to connect to pvd.  PVD needs to be running at the time this
                    // function is called. We don't worry about the return value because we are
                    // already registered as a listener for connections and thus our onPvdConnected
                    // call will take care of setting up our basic connection state.
                    // PxExtensionVisualDebugger::connect(pPhysics->getPvdConnectionManager(),
                    // "127.0.0.1", 5425, 10, PxDebuggerConnectionFlags( (PxU32)theConnectionFlags)
                    // );

                    PxVisualDebuggerExt::createConnection( cm, "localhost", 5425, 500 );
                }
            }

            // connect to PVD
#endif

            setLoadingState( LoadingState::Loaded );
            WP_LOG_INFO( "PhysX physics created." );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                m_loadQueue.clear();

                for( auto box : m_boxShapes )
                {
                    if( box )
                    {
                        box->unload( data );
                    }
                }

                for( auto mesh : m_meshShapes )
                {
                    if( mesh )
                    {
                        mesh->unload( data );
                    }
                }

                for( auto plane : m_planeShapes )
                {
                    if( plane )
                    {
                        plane->unload( data );
                    }
                }

                for( auto constraint : m_constraints )
                {
                    if( constraint )
                    {
                        constraint->unload( data );
                    }
                }

                for( auto body : m_staticBodies )
                {
                    if( body )
                    {
                        body->unload( data );
                    }
                }

                for( auto body : m_rigidBodies )
                {
                    if( body )
                    {
                        body->unload( data );
                    }
                }

                m_constraints.clear();
                m_boxShapes.clear();
                m_meshShapes.clear();
                m_planeShapes.clear();

                m_rigidBodies.clear();
                m_staticBodies.clear();

                for( auto scene : m_scenes )
                {
                    scene->clear();
                    scene->unload( nullptr );
                }

                m_scenes.clear();

                if( m_cpuDispatcher )
                {
                    m_cpuDispatcher->release();
                    m_cpuDispatcher = nullptr;
                }

                if( m_cooking )
                {
                    m_cooking->release();
                    m_cooking = nullptr;
                }

                if( auto physics = getPhysics() )
                {
                    PxCloseExtensions();

                    physics->release();
                    setPhysics( nullptr );
                }

                if( m_foundation )
                {
                    m_foundation->release();
                    m_foundation = nullptr;
                }

                m_allocator = nullptr;
                m_errorOutput = nullptr;

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxManager::setDefaultMaterial( PxMaterial *defaultMaterial )
    {
        m_defaultMaterial = defaultMaterial;
    }

    auto PhysxManager::getVehicleManager() const -> PhysxVehicleManager *
    {
        return m_vehicleManager;
    }

    void PhysxManager::setVehicleManager( PhysxVehicleManager *vehicleManager )
    {
        m_vehicleManager = vehicleManager;
    }

    auto PhysxManager::getCooker() const -> PhysxCooker *
    {
        return m_cooker;
    }

    void PhysxManager::setCooker( PhysxCooker *cooker )
    {
        m_cooker = cooker;
    }

    auto PhysxManager::addMaterial() -> SmartPtr<IPhysicsMaterial3>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto material = factoryManager->make_ptr<PhysxMaterial>();

        if( auto physics = getPhysics() )
        {
            auto m = physics->createMaterial( 0.5f, 0.5f, 0.1f );
            material->setMaterial( m );
            m_materials.emplace_back( material );
        }

        return material;
    }

    void PhysxManager::removeMaterial( SmartPtr<IPhysicsMaterial3> material )
    {
        m_materials.erase( std::remove( m_materials.begin(), m_materials.end(), material ),
                           m_materials.end() );
    }

    auto PhysxManager::createSphere() -> SmartPtr<ISphereShape3>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            if( auto sphere = factoryManager->make_ptr<PhysxSphereShape>() )
            {
                auto material = addMaterial();
                sphere->setMaterial( material );

                loadObject( sphere );
                m_sphereShapes.emplace_back( sphere );
                return sphere;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto PhysxManager::createBox() -> SmartPtr<IBoxShape3>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            if( auto box = factoryManager->make_ptr<PhysxBoxShape>() )
            {
                auto material = addMaterial();
                box->setMaterial( material );

                loadObject( box );
                m_boxShapes.emplace_back( box );
                return box;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto PhysxManager::createPlane() -> SmartPtr<IPlaneShape3>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            if( auto plane = factoryManager->make_ptr<PhysxPlaneShape>() )
            {
                auto material = addMaterial();
                plane->setMaterial( material );

                loadObject( plane );
                m_planeShapes.emplace_back( plane );
                return plane;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto PhysxManager::createMesh() -> SmartPtr<IMeshShape>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            if( auto mesh = factoryManager->make_ptr<PhysxMeshShape>() )
            {
                loadObject( mesh );
                m_meshShapes.emplace_back( mesh );
                return mesh;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto PhysxManager::createTerrain() -> SmartPtr<ITerrainShape>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            if( auto terrain = factoryManager->make_ptr<PhysxTerrain>() )
            {
                loadObject( terrain );
                m_terrainShapes.emplace_back( terrain );
                return terrain;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void PhysxManager::preUpdate()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager->getTimerPtr();

        auto       task = Thread::getCurrentTask();
        auto       t = timer->getTime();
        auto       dt = timer->getDeltaTime();
        const auto maxDT = 1.0 / 15.0;

        ScopedLock lock( this );

        if( dt < MathF::epsilon() || dt > maxDT )
        {
            dt = 1.0 / 60.0;
        }

        if( !m_loadQueue.empty() )
        {
            SmartPtr<ISharedObject> loadObject;
            while( m_loadQueue.try_pop( loadObject ) )
            {
                if( loadObject )
                {
                    if( !loadObject->isLoaded() )
                    {
                        loadObject->load( nullptr );
                    }
                }
            }
        }

        if( m_vehicleManager )
        {
            // if (m_scene)
            //{
            //	m_vehicleManager->suspensionRaycasts(m_scene);
            // }
        }

        // for (auto c : m_constraints)
        //{
        //	if (c)
        //	{
        //		c->update();
        //	}
        // }

        // for (u32 i = 0; i < m_rigidBodies.size(); ++i)
        //{
        //	auto& rigidBody = m_rigidBodies[i];
        //	if (rigidBody)
        //	{
        //		rigidBody->update();
        //	}
        // }

        // for (u32 i = 0; i < m_characters.size(); ++i)
        //{
        //	auto& character = m_characters[i];
        //	if (character)
        //	{
        //		character->update();
        //	}
        // }
    }

    void PhysxManager::update()
    {
        // Some callers drive the physics manager through update() without a distinct
        // preUpdate() phase. Ensure objects queued for the physics task still become usable.
        if( !m_loadQueue.empty() )
        {
            ScopedLock lock( this );

            SmartPtr<ISharedObject> object;
            while( m_loadQueue.try_pop( object ) )
            {
                if( object && !object->isLoaded() )
                {
                    object->load( nullptr );
                }
            }
        }
    }

    void PhysxManager::postUpdate()
    {
        if( getEnableDebugDraw() )
        {
            debugDraw();
        }
    }

    auto PhysxManager::addScene() -> SmartPtr<IPhysicsScene3>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto scene = factoryManager->make_ptr<PhysxScene>();
            scene->load( nullptr );

            m_scenes.push_back( scene );
            return scene;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void PhysxManager::removeScene( SmartPtr<IPhysicsScene3> scene )
    {
        if( scene )
        {
            scene->unload( nullptr );
            scene = nullptr;
        }
    }

    auto PhysxManager::getEnableDebugDraw() const -> bool
    {
        return m_enableDebugDraw.load();
    }

    void PhysxManager::setEnableDebugDraw( bool enableDebugDraw )
    {
        m_enableDebugDraw = enableDebugDraw;
        PhysicsManager::setEnableDebugDraw( enableDebugDraw );
    }

    void PhysxManager::debugDraw()
    {
        if( !getEnableDebugDraw() )
        {
            return;
        }

        if( auto debug = getDebugRenderer() )
        {
            const auto rigidBodies = m_rigidBodies.snapshot();
            for( const auto &body : rigidBodies )
            {
                if( body && body->isEnabled() )
                {
                    drawDebugBody( *debug, *body );
                }
            }

            const auto staticBodies = m_staticBodies.snapshot();
            for( const auto &body : staticBodies )
            {
                if( body && body->isEnabled() )
                {
                    drawDebugBody( *debug, *body );
                }
            }
        }

        PhysicsManager::debugDraw();
    }

    auto PhysxManager::addCollisionShapeByType( hash64 type, SmartPtr<ISharedObject> data )
        -> SmartPtr<IPhysicsShape3>
    {
        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        const auto sphereTypeInfo = ISphereShape3::typeInfo();
        const auto boxTypeInfo = IBoxShape3::typeInfo();
        const auto meshTypeInfo = IMeshShape::typeInfo();
        const auto planeTypeInfo = IPlaneShape3::typeInfo();
        const auto terrainTypeInfo = ITerrainShape::typeInfo();

        const auto sphereType = typeManager->getHash( sphereTypeInfo );
        const auto boxType = typeManager->getHash( boxTypeInfo );
        const auto meshType = typeManager->getHash( meshTypeInfo );
        const auto planeType = typeManager->getHash( planeTypeInfo );
        const auto terrainType = typeManager->getHash( terrainTypeInfo );

        if( type == sphereType )
        {
            return createSphere();
        }
        if( type == boxType )
        {
            return createBox();
        }
        if( type == planeType )
        {
            return createPlane();
        }
        if( type == meshType )
        {
            return createMesh();
        }
        if( type == terrainType )
        {
            return createTerrain();
        }

        return nullptr;
    }

    auto PhysxManager::createRigidBody() -> SmartPtr<IRigidBody3>
    {
        return createRigidBody( nullptr, nullptr );
    }

    auto PhysxManager::createRigidBody( SmartPtr<IPhysicsShape3> collisionShape )
        -> SmartPtr<IRigidBody3>
    {
        return createRigidBody( collisionShape, nullptr );
    }

    auto PhysxManager::createRigidBody( SmartPtr<IPhysicsShape3> collisionShape,
                                        SmartPtr<Properties>     properties ) -> SmartPtr<IRigidBody3>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto transform = PxTransform::createIdentity();

        if( auto physics = getPhysics() )
        {
            PxRigidDynamic *actor = physics->createRigidDynamic( transform );

            if( collisionShape )
            {
                /*
                hash32 type = collisionShape->getType();
                if (type == StringUtil::getHash("sphere"))
                {
                }
                else if (type == StringUtil::getHash("box"))
                {
                    auto boxShape = fb::dynamic_pointer_cast<IBoxShape3>(collisionShape);
                    if (boxShape)
                    {
                        auto extents = boxShape->getExtents() / physics_Num(2.0);

                        PxVec3 dimensions(extents.X(), extents.Y(), extents.Z());
                        PxBoxGeometry geometry(dimensions);
                        PxTransform localPose = PxTransform::createIdentity();

                        auto shape = actor->createShape(geometry, *m_defaultMaterial, localPose);

                        //shape->setSimulationFilterData(simFilterData);
                        //shape->setQueryFilterData(qryFilterData);
                    }
                }
                */
            }

            auto rigidBody = factoryManager->make_ptr<PhysxRigidDynamic>();
            rigidBody->setActorDynamic( actor );
            PxRigidBodyExt::updateMassAndInertia( *actor, 1.0f );
            m_rigidBodies.push_back( rigidBody );
            loadObject( rigidBody );
            return rigidBody;
        }

        return nullptr;
    }

    auto PhysxManager::createRigidBody( const Transform3<physics_Num> &transform )
        -> SmartPtr<IRigidBody3>
    {
        auto rigidBody = createRigidBody( nullptr, nullptr );
        rigidBody->setTransform( transform );
        return rigidBody;
    }

    auto PhysxManager::addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape )
        -> SmartPtr<IRigidStatic3>
    {
        return addRigidStatic( collisionShape, nullptr );
    }

    auto PhysxManager::addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                       SmartPtr<Properties>     properties ) -> SmartPtr<IRigidStatic3>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto rigidStatic = factoryManager->make_ptr<PhysxRigidStatic>();

        if( collisionShape )
        {
            rigidStatic->addShape( collisionShape );
        }

        m_staticBodies.push_back( rigidStatic );
        loadObject( rigidStatic );
        return rigidStatic;
    }

    auto PhysxManager::addRigidStatic( const Transform3<physics_Num> &transform )
        -> SmartPtr<IRigidStatic3>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto rigidStatic = factoryManager->make_ptr<PhysxRigidStatic>();

        rigidStatic->setTransform( transform );
        m_staticBodies.push_back( rigidStatic );
        loadObject( rigidStatic );
        return rigidStatic;
    }

    auto PhysxManager::addRigidDynamic( const Transform3<physics_Num> &transform )
        -> SmartPtr<IRigidDynamic3>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto rigidDynamic = factoryManager->make_ptr<PhysxRigidDynamic>();
        rigidDynamic->setTransform( transform );
        m_rigidBodies.push_back( rigidDynamic );
        loadObject( rigidDynamic );
        return rigidDynamic;
    }

    PxU32 gNumVehicleAdded = 0;

    ////////////////////////////////////////////////////////////////
    // VEHICLE SETUP DATA
    ////////////////////////////////////////////////////////////////

    PxF32 gChassisMass = 1500.0f;
    PxF32 gSuspensionShimHeight = 0.125f;

    ////////////////////////////////////////////////////////////////
    // RENDER USER DATA TO ASSOCIATE EACH RENDER MESH WITH A
    // VEHICLE PHYSICS COMPONENT
    ////////////////////////////////////////////////////////////////

    enum
    {
        CAR_PART_FRONT_LEFT_WHEEL = 0,
        CAR_PART_FRONT_RIGHT_WHEEL,
        CAR_PART_REAR_LEFT_WHEEL,
        CAR_PART_REAR_RIGHT_WHEEL,
        CAR_PART_CHASSIS,
        CAR_PART_WINDOWS,
        NUM_CAR4W_RENDER_COMPONENTS
    };

    static char gCarPartNames[NUM_CAR4W_RENDER_COMPONENTS][64] = {
        "frontwheelleftshape", "frontwheelrightshape", "backwheelleftshape",
        "backwheelrightshape", "car_02_visshape",      "car_02_windowsshape"
    };

    struct CarRenderUserData
    {
        PxU8 carId;
        PxU8 carPart;
        PxU8 carPartDependency;
        PxU8 pad;
    };

    static CarRenderUserData gCar4WRenderUserData[NUM_CAR4W_RENDER_COMPONENTS] = {
        // wheel fl		wheel fr		wheel rl		wheel rl		chassis			windows
        { 0, 0, 255 }, { 0, 1, 255 }, { 0, 2, 255 }, { 0, 3, 255 }, { 0, 4, 255 }, { 0, 4, 4 }
    };

    CarRenderUserData gVehicleRenderUserData[NUM_PLAYER_CARS + NUM_NONPLAYER_4W_VEHICLES]
                                            [NUM_CAR4W_RENDER_COMPONENTS];

    static PxVec3 g4WCarPartDependencyOffsets[NUM_CAR4W_RENDER_COMPONENTS] = {
        PxVec3( 0, 0, 0 ), PxVec3( 0, 0, 0 ), PxVec3( 0, 0, 0 ),
        PxVec3( 0, 0, 0 ), PxVec3( 0, 0, 0 ), PxVec3( 0, 0, 0 )
    };

    //	RenderMeshActor*
    // gRenderMeshActors[NUM_CAR4W_RENDER_COMPONENTS]={NULL,NULL,NULL,NULL,NULL,NULL};

    ////////////////////////////////////////////////////////////////
    // TRANSFORM APPLIED TO CHASSIS RENDER MESH VERTS
    // THAT IS REQUIRED TO PLACE AABB OF CHASSIS RENDER MESH AT ORIGIN
    // AT CENTRE-POINT OF WHEELS.
    ////////////////////////////////////////////////////////////////

    static PxVec3 gChassisMeshTransform( 0, 0, 0 );

    ////////////////////////////////////////////////////////////////
    // WHEEL CENTRE OFFSETS FROM CENTRE OF CHASSIS RENDER MESH AABB
    // OF 4-WHEELED VEHICLE
    ////////////////////////////////////////////////////////////////

    static PxVec3 gWheelCentreOffsets4[4];

    ////////////////////////////////////////////////////////////////
    // CONVEX HULL OF RENDER MESH FOR CHASSIS AND WHEELS OF
    // 4-WHEELED VEHICLE
    ////////////////////////////////////////////////////////////////

    static PxConvexMesh *gChassisConvexMesh = nullptr;
    static PxConvexMesh *gWheelConvexMeshes4[4] = { nullptr, nullptr, nullptr, nullptr };

    auto PhysxManager::addVehicle( SmartPtr<IRigidBody3>       chassis,
                                   const SmartPtr<Properties> &properties ) -> SmartPtr<IPhysicsVehicle3>
    {
        // SmartPtr<PhysxVehicle3> vehicle(new PhysxVehicle3);

        // Ogre::MeshPtr chassisMesh = Ogre::MeshManager::getSingletonPtr()->load("4x4chassis.mesh",
        // Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME); chassisMesh->load(nullptr);
        // if(chassisMesh->isLoaded())
        //	gChassisConvexMesh = m_cooker->createPxConvexMesh(chassisMesh.get());

        // Ogre::MeshPtr wheelMeshFL =
        // Ogre::MeshManager::getSingletonPtr()->getByName("4x4WheelLeftFront.mesh");
        // wheelMeshFL->load(nullptr);
        // gWheelConvexMeshes4[0] = m_cooker->createPxConvexMesh(wheelMeshFL.get());

        // Ogre::MeshPtr wheelMeshFR =
        // Ogre::MeshManager::getSingletonPtr()->getByName("4x4WheelLeftFront.mesh");
        // wheelMeshFR->load(nullptr);
        // gWheelConvexMeshes4[1] = m_cooker->createPxConvexMesh(wheelMeshFR.get());

        // Ogre::MeshPtr wheelMeshRL =
        // Ogre::MeshManager::getSingletonPtr()->getByName("4x4WheelLeftFront.mesh");
        // wheelMeshRL->load(nullptr);
        // gWheelConvexMeshes4[2] = m_cooker->createPxConvexMesh(wheelMeshRL.get());

        // Ogre::MeshPtr wheelMeshRR =
        // Ogre::MeshManager::getSingletonPtr()->getByName("4x4WheelLeftFront.mesh");
        // wheelMeshRR->load(nullptr);
        // gWheelConvexMeshes4[3] = m_cooker->createPxConvexMesh(wheelMeshRR.get());

        // vehicle->setVehicle(pVehicle);

        // m_vehicles.push_back(vehicle);

        // return vehicle;
        return nullptr;
    }

    auto PhysxManager::addVehicle( SmartPtr<IBuildDirector> pTemplate ) -> SmartPtr<IPhysicsVehicle3>
    {
        // SmartPtr<VehicleTemplate> vehicleTemplate; // = pTemplate;

        // SmartPtr<PhysxVehicle3> vehicle(new PhysxVehicle3);

        // PhysxCooker::Params vehicleParams;
        // vehicleParams.scale(Vector3F::unit() * 0.2f);

        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IGraphicsSystem> graphicsSystem = engine->getGraphicsSystem();
        // SmartPtr<IMeshManager> meshMgr = graphicsSystem->getMeshManager();

        // SmartPtr<IMesh> chassisMesh = meshMgr->loadMesh("4x4chassis.mesh");
        // gChassisConvexMesh = m_cooker->createPxConvexMesh(chassisMesh, vehicleParams);

        // SmartPtr<IMesh> wheelMeshFL = meshMgr->loadMesh("4x4WheelLeftFront.mesh");
        // gWheelConvexMeshes4[0] = m_cooker->createPxConvexMesh(wheelMeshFL, vehicleParams);

        // SmartPtr<IMesh> wheelMeshFR = meshMgr->loadMesh("4x4WheelLeftFront.mesh");
        // gWheelConvexMeshes4[1] = m_cooker->createPxConvexMesh(wheelMeshFR, vehicleParams);

        // SmartPtr<IMesh> wheelMeshRL = meshMgr->loadMesh("4x4WheelLeftFront.mesh");
        // gWheelConvexMeshes4[2] = m_cooker->createPxConvexMesh(wheelMeshRL, vehicleParams);

        // SmartPtr<IMesh> wheelMeshRR = meshMgr->loadMesh("4x4WheelLeftFront.mesh");
        // gWheelConvexMeshes4[3] = m_cooker->createPxConvexMesh(wheelMeshRR, vehicleParams);

        // Array<SmartPtr<VehicleWheelTemplate>> wheels = vehicleTemplate->getWheels();
        // for (u32 i = 0; i < wheels.size(); ++i)
        //	gWheelCentreOffsets4[i] = PxVec3(wheels[i]->getOffset().X(), wheels[i]->getOffset().Y(),
        //	                                 wheels[i]->getOffset().Z());

        // PxTransform transform = PxTransform::createIdentity();
        // PxVehicleDrive4W* pVehicle = m_vehicleManager->create4WVehicle(*getScene(), *getPhysics(),
        // *getCooking(),
        //                                                                *mStandardMaterials[SURFACE_TYPE_TARMAC],
        //                                                                gChassisMass,
        //                                                                gWheelCentreOffsets4,
        //                                                                gChassisConvexMesh,
        //                                                                gWheelConvexMeshes4,
        //                                                                transform, true);

        // vehicle->setVehicle(pVehicle);

        // m_vehicles.push_back(vehicle);

        // return vehicle;

        return nullptr;
    }

    auto PhysxManager::removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape ) -> bool
    {
        ScopedLock lock( this );

        if( collisionShape )
        {
            collisionShape->unload( nullptr );

            if( collisionShape->isDerived<IBoxShape3>() )
            {
                auto it = std::find( m_boxShapes.begin(), m_boxShapes.end(), collisionShape );
                if( it != m_boxShapes.end() )
                {
                    m_boxShapes.erase( it );
                }
            }
            else if( collisionShape->isDerived<IMeshShape>() )
            {
                auto it = std::find( m_meshShapes.begin(), m_meshShapes.end(), collisionShape );
                if( it != m_meshShapes.end() )
                {
                    m_meshShapes.erase( it );
                }
            }
            else if( collisionShape->isDerived<IPlaneShape3>() )
            {
                auto it = std::find( m_planeShapes.begin(), m_planeShapes.end(), collisionShape );
                if( it != m_planeShapes.end() )
                {
                    m_planeShapes.erase( it );
                }
            }
            else if( collisionShape->isDerived<ISphereShape3>() )
            {
                auto it = std::find( m_sphereShapes.begin(), m_sphereShapes.end(), collisionShape );
                if( it != m_sphereShapes.end() )
                {
                    m_sphereShapes.erase( it );
                }
            }
            else if( collisionShape->isDerived<ITerrainShape>() )
            {
                auto it = std::find( m_terrainShapes.begin(), m_terrainShapes.end(), collisionShape );
                if( it != m_terrainShapes.end() )
                {
                    m_terrainShapes.erase( it );
                }
            }

            return true;
        }

        return false;
    }

    auto PhysxManager::removePhysicsBody( SmartPtr<IRigidBody3> body ) -> bool
    {
        ScopedLock lock( this );

        if( body )
        {
            body->unload( nullptr );

            if( body->isDerived<PhysxRigidDynamic>() )
            {
                auto it = std::find( m_rigidBodies.begin(), m_rigidBodies.end(), body );
                if( it != m_rigidBodies.end() )
                {
                    m_rigidBodies.erase( it );
                }
            }
            else if( body->isDerived<PhysxRigidStatic>() )
            {
                auto it = std::find( m_staticBodies.begin(), m_staticBodies.end(), body );
                if( it != m_staticBodies.end() )
                {
                    m_staticBodies.erase( it );
                }
            }

            return true;
        }

        return false;
    }

    auto PhysxManager::removeVehicle( SmartPtr<IPhysicsVehicle3> vehicle ) -> bool
    {
        if( vehicle )
        {
            vehicle->unload( nullptr );

            m_vehicles.erase( std::remove( m_vehicles.begin(), m_vehicles.end(), vehicle ),
                              m_vehicles.end() );

            return true;
        }

        return false;
    }

    auto PhysxManager::addCharacter() -> SmartPtr<ICharacterController3>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto character = factoryManager->make_ptr<PhysxCharacterController>();
        character->load( nullptr );
        m_characters.push_back( character );
        return character;
    }

    auto PhysxManager::createTerrain( SmartPtr<IBuildDirector> objectTemplate )
        -> SmartPtr<ITerrainShape>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto terrain = factoryManager->make_ptr<PhysxTerrain>();
        terrain->load( objectTemplate );
        return terrain;
    }

    auto PhysxManager::rayTest( const Vector3<physics_Num> &start, const Vector3<physics_Num> &direction,
                                Vector3<physics_Num> &hitPos, Vector3<physics_Num> &hitNormal,
                                u32 collisionType, u32 collisionMask ) -> bool
    {
        if( m_scenes.empty() )
        {
            return false;
        }

        // Convert start position and direction to PhysX types
        auto origin = PhysxUtil::toPx( start );
        auto dir = PhysxUtil::toPx( direction );

        // Get the magnitude for max distance and normalize the direction
        auto maxDistance = dir.magnitude();
        if( maxDistance < MathF::epsilon() )
        {
            return false;
        }

        dir.normalize();

        // Setup raycast hit buffer
        PxRaycastBuffer hitBuffer;

        // Setup filter data if collision type/mask are provided
        PxQueryFilterData        filterData( PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC );
        CollisionMaskQueryFilter queryFilter;
        auto                     useCollisionFilter = collisionType != 0 || collisionMask != 0;
        if( useCollisionFilter )
        {
            filterData.flags |= PxQueryFlag::ePREFILTER;
            filterData.data.word0 = collisionType;
            filterData.data.word1 = collisionMask;
        }

        // Perform raycast on the first scene
        auto scene = m_scenes[0];
        if( auto physxScene = workphone::static_pointer_cast<PhysxScene>( scene ) )
        {
            if( auto pxScene = physxScene->getScene() )
            {
                auto status =
                    pxScene->raycast( origin, dir, maxDistance, hitBuffer, PxHitFlag::eDEFAULT,
                                      filterData, useCollisionFilter ? &queryFilter : nullptr );

                if( status && hitBuffer.hasBlock )
                {
                    // Extract hit information
                    const auto &hit = hitBuffer.block;

                    hitPos = PhysxUtil::toFB( hit.position );
                    hitNormal = PhysxUtil::toFB( hit.normal );

                    return true;
                }
            }
        }

        return false;
    }

    auto PhysxManager::intersects( const Vector3<physics_Num> &start, const Vector3<physics_Num> &end,
                                   Vector3<physics_Num> &hitPos, Vector3<physics_Num> &hitNormal,
                                   SmartPtr<ISharedObject> &object, u32 collisionType /*= 0*/,
                                   u32 collisionMask /*= 0 */ ) -> bool
    {
        if( m_scenes.empty() )
        {
            return false;
        }

        // Calculate direction from start to end
        auto direction = end - start;

        // Convert start position and direction to PhysX types
        auto origin = PhysxUtil::toPx( start );
        auto dir = PhysxUtil::toPx( direction );

        // Get the magnitude for max distance and normalize the direction
        auto maxDistance = dir.magnitude();
        if( maxDistance < MathF::epsilon() )
        {
            return false;
        }

        dir.normalize();

        // Setup raycast hit buffer
        PxRaycastBuffer hitBuffer;

        // Setup filter data if collision type/mask are provided
        PxQueryFilterData        filterData( PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC );
        CollisionMaskQueryFilter queryFilter;
        auto                     useCollisionFilter = collisionType != 0 || collisionMask != 0;
        if( useCollisionFilter )
        {
            filterData.flags |= PxQueryFlag::ePREFILTER;
            filterData.data.word0 = collisionType;
            filterData.data.word1 = collisionMask;
        }

        // Perform raycast on the first scene
        auto scene = m_scenes[0];
        if( auto physxScene = workphone::static_pointer_cast<PhysxScene>( scene ) )
        {
            if( auto pxScene = physxScene->getScene() )
            {
                auto status =
                    pxScene->raycast( origin, dir, maxDistance, hitBuffer, PxHitFlag::eDEFAULT,
                                      filterData, useCollisionFilter ? &queryFilter : nullptr );

                if( status && hitBuffer.hasBlock )
                {
                    // Extract hit information
                    const auto &hit = hitBuffer.block;

                    hitPos = PhysxUtil::toFB( hit.position );
                    hitNormal = PhysxUtil::toFB( hit.normal );

                    // Get the hit actor and find associated object
                    if( hit.actor )
                    {
                        auto userData = hit.actor->userData;
                        if( userData )
                        {
                            if( hit.actor->isRigidDynamic() )
                            {
                                auto rigidDynamic = static_cast<PhysxRigidDynamic *>( userData );
                                object = rigidDynamic->getSharedFromThis<ISharedObject>();
                            }
                            else if( hit.actor->isRigidStatic() )
                            {
                                auto rigidStatic = static_cast<PhysxRigidStatic *>( userData );
                                object = rigidStatic->getSharedFromThis<ISharedObject>();
                            }
                        }
                    }

                    return true;
                }
            }
        }

        return false;
    }

    auto PhysxManager::getPhysics() const -> PxPhysics *
    {
        auto p = m_physics.load();
        return p;
    }

    void PhysxManager::setPhysics( PxPhysics *physics )
    {
        m_physics = physics;
    }

    auto PhysxManager::getCooking() const -> PxCooking *
    {
        return m_cooking;
    }

    void PhysxManager::setCooking( PxCooking *cooking )
    {
        m_cooking = cooking;
    }

    auto PhysxManager::getDefaultMaterial() const -> PxMaterial *
    {
        return m_defaultMaterial;
    }

    auto PhysxManager::getStandardMaterials() const -> PxMaterial *
    {
        return m_standardMaterials[0];
    }

    auto PhysxManager::addConstraintD6( SmartPtr<IPhysicsBody3>        actor0,
                                        const Transform3<physics_Num> &localFrame0,
                                        SmartPtr<IPhysicsBody3>        actor1,
                                        const Transform3<physics_Num> &localFrame1 )
        -> SmartPtr<IConstraintD6>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto constraint = factoryManager->make_ptr<PhysxConstraintD6>();

        constraint->setBodyA( actor0 );
        constraint->setBodyB( actor1 );

        constraint->setLocalPose( JointActorIndexEnum::eACTOR0, localFrame0 );
        constraint->setLocalPose( JointActorIndexEnum::eACTOR1, localFrame1 );

        loadObject( constraint );
        m_constraints.emplace_back( constraint );
        return constraint;
    }

    auto PhysxManager::addFixedConstraint( SmartPtr<IPhysicsBody3>        actor0,
                                           const Transform3<physics_Num> &localFrame0,
                                           SmartPtr<IPhysicsBody3>        actor1,
                                           const Transform3<physics_Num> &localFrame1 )
        -> SmartPtr<IConstraintFixed3>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto physics = getPhysics();
        auto j = factoryManager->make_ptr<PhysxConstraintFixed3>();

        RawPtr<PxRigidDynamic> pxActor0;
        RawPtr<PxRigidDynamic> pxActor1;

        if( actor0 )
        {
            auto pActor0 = workphone::static_pointer_cast<PhysxRigidDynamic>( actor0 );
            pxActor0 = pActor0->getActor();
        }

        if( actor1 )
        {
            auto pActor1 = workphone::static_pointer_cast<PhysxRigidDynamic>( actor1 );
            pxActor1 = pActor1->getActor();
        }

        auto pxLocalFrame0 = PhysxUtil::toPx( localFrame0 );
        auto pxLocalFrame1 = PhysxUtil::toPx( localFrame1 );

        auto fixedJoint =
            PxFixedJointCreate( *physics, pxActor0, pxLocalFrame0, pxActor1, pxLocalFrame1 );
        j->setJoint( fixedJoint );

        loadObject( j );
        m_constraints.emplace_back( j );
        return j;
    }

    auto PhysxManager::addConstraintDrive() -> SmartPtr<IConstraintDrive>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto constraintDrive = factoryManager->make_ptr<PhysxConstraintDrive>();
        return constraintDrive;
    }

    auto PhysxManager::addConstraintLinearLimit( physics_Num extent, physics_Num contactDist )
        -> SmartPtr<IConstraintLinearLimit>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto constraintLinearLimit = factoryManager->make_ptr<PhysxConstraintLimit>();
        return constraintLinearLimit;
    }

    auto PhysxManager::addRaycastHitData() -> SmartPtr<IRaycastHit>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto raycastHit = factoryManager->make_ptr<RaycastHit>();
        return raycastHit;
    }

    auto PhysxManager::getPhysicsTask() const -> TaskId
    {
        auto applicationManager = core::IApplicationManager::instance();

        // if (isUpdating())
        //{
        //	return Thread::getCurrentTask();
        // }

        auto taskManager = applicationManager->getTaskManager();
        if( taskManager )
        {
            auto task = taskManager->getTask( TaskId::Physics );
            if( task )
            {
                if( task->isExecuting() )
                {
                    return TaskId::Physics;
                }
                if( task->isPrimary() )
                {
                    return TaskId::Primary;
                }
            }
        }

        return applicationManager->hasTasks() ? TaskId::Physics : TaskId::Primary;
    }

    void PhysxManager::loadObject( SmartPtr<ISharedObject> object, bool forceQueue )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto threadPool = applicationManager->getThreadPool();

        if( !threadPool )
        {
            ScopedLock lock( this );
            object->load( nullptr );
        }
        else if( threadPool && threadPool->getNumThreads() == 0 )
        {
            ScopedLock lock( this );
            object->load( nullptr );
        }
        else
        {
            auto taskManager = applicationManager->getTaskManager();
            auto task = taskManager->getTask( TaskId::Physics );

            auto currentTaskId = Thread::getCurrentTask();
            auto physicsTaskId = getPhysicsTask();

            if( forceQueue )
            {
                m_loadQueue.push( object );
            }
            else if( task->isExecuting() )
            {
                m_loadQueue.push( object );
            }
            else if( currentTaskId != physicsTaskId )
            {
                m_loadQueue.push( object );
            }
            else
            {
                ScopedLock lock( this );
                object->load( nullptr );
            }
        }
    }

    void PhysxManager::unloadObject( SmartPtr<ISharedObject> object, bool forceQueue )
    {
        ScopedLock lock( this );
        object->unload( nullptr );
    }

    auto PhysxManager::getStateTask() const -> TaskId
    {
        return TaskId::Physics;
    }

} // namespace workphone::physics
