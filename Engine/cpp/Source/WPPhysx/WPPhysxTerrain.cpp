#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxTerrain.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <WPPhysx/WPPhysxCooker.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/WPPhysxStream.hpp>
#include <WPPhysx/PhysxMemoryOutputStream.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <geometry/PxHeightFieldDesc.h>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxTerrain, PhysxShape<TerrainShape> );

    PhysxTerrain::PhysxTerrain()
    {
        m_heightScale = Vector3F::unit();
        m_terrainSize = Vector3F( 1024, 0.0, 1024 );
        m_size = Vector2I( 513, 513 );
        m_heightData.resize( 513 * 513 );

        for( auto &value : m_heightData )
        {
            value = 0.0f;
        }
    }

    PhysxTerrain::~PhysxTerrain()
    {
        unload( nullptr );
    }

    void PhysxTerrain::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager =
                workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
            WP_ASSERT( physicsManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto physics = physicsManager->getPhysics();
            WP_ASSERT( physics );

            ScopedLock lock( physicsManager );

            // auto shape = createDefaultTerrain( 513, 513, 0.0f );
            // setShape( shape );

            // smart_ptr<physics::PxScene> physicsScene =
            // applicationManager->getPhysicsManager()->getPhysicsScene();

            // smart_ptr<physics::PxPhysics> physics =
            // applicationManager->getPhysicsManager()->getPhysics();
            // PhysicsManagerPtr physicsManager =
            // applicationManager->getPhysicsManager();
            // physx::PxCooking* cooking =
            // physicsManager->getCooking();

            auto heightData = getHeightData();

            // auto transform = getTransform();
            // auto pos = transform.getPosition();
            // auto rot = transform.getOrientation();
            // auto tranformScale = transform.getScale();

            auto transformF = Transform3F();

            auto hfGeom = createTerrainGeometry( heightData, getSize(), getHeightScale(),
                                                 getTerrainSize(), transformF );

            auto pose = physx::PxTransform::createIdentity();

            // Transform3<physics_Num> t = getTransform();
            // Vector3<physics_Num> position = t.getPosition();
            // Vector3<physics_Num> scale = t.getScale();
            Quaternion<physics_Num> orientation; // = t.getOrientation();

            auto size = getSize();
            auto terrainSize = getTerrainSize();
            auto terrainScale = terrainSize / Vector3F( static_cast<f32>( size.x - 1 ), 1.0f,
                                                        static_cast<f32>( size.y - 1 ) );
            auto halfTerrainSize = terrainSize * static_cast<physics_Num>( 0.5 );

            auto center = Vector3<physics_Num>( -halfTerrainSize.x, 0.0f, -halfTerrainSize.z );
            pose.p = physx::PxVec3( center.x, center.y, center.z );
            pose.q = physx::PxQuat( orientation.x, orientation.y, orientation.z, orientation.w );

            auto terrainMat = physics->createMaterial( 1, 1, 1 );

            auto shape = physics->createShape( hfGeom, *terrainMat );
            shape->setLocalPose( pose );
            setupCollisionMask( shape );
            setShape( shape );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxTerrain::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto physicsManager = workphone::static_pointer_cast<PhysxManager>(
                    applicationManager->getPhysicsManager() );
                WP_ASSERT( physicsManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto physics = physicsManager->getPhysics();
                WP_ASSERT( physics );

                if( auto shape = getShape() )
                {
                    if( auto actor = getPxActor() )
                    {
                        actor->detachShape( *shape, false );
                    }

                    shape->release();
                    setShape( nullptr );
                }

                PhysxShape<TerrainShape>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    inline auto getIndex( u32 tileSize, u32 x, u32 z ) -> u32
    {
        return x + z * tileSize;
    }

    auto PhysxTerrain::createTerrainGeometry( const Array<f32> &heightData, const Vector2I &size,
                                              const Vector3F &heightScale, const Vector3F &terrainSize,
                                              const Transform3<f32> &t ) -> PxHeightFieldGeometry
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager =
            workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
        auto physics = physicsManager->getPhysics();

        auto physicsScene = physicsManager->getPhysicsScene();

        PxCooking *cooking = physicsManager->getCooking();

        PxHeightFieldDesc heightFieldDesc;
        heightFieldDesc.format = PxHeightFieldFormat::eS16_TM;
        heightFieldDesc.nbColumns = size.X();
        heightFieldDesc.nbRows = size.Y();
        heightFieldDesc.samples.stride = sizeof( PxHeightFieldSample );

        PxReal fHeightScale = heightScale.Y() / std::numeric_limits<PxI16>::max();

        int  numSamples = size.X() * size.Y();
        auto samples = new PxHeightFieldSample[numSamples];

        auto currentByte = reinterpret_cast<PxU8 *>( samples );

        // for ( int y = 0; y < m_size.y; y++ )
        //{

        for( int x = 0; x < size.X(); x++ )
        //{
        // for ( int x = m_size.x - 1; x >= 0; x-- )
        {
            for( int y = size.Y() - 1; y >= 0; y-- )
            {
                auto currentSample = reinterpret_cast<PxHeightFieldSample *>( currentByte );

                int   heightIdx = x + y * size.X();
                float heightValue = heightData[heightIdx];
                float height = heightValue * std::numeric_limits<PxI16>::max();

                currentSample->height = static_cast<PxI16>( height );
                currentSample->materialIndex0 = 0;
                currentSample->materialIndex1 = 0;

                currentByte += heightFieldDesc.samples.stride;
            }
        }

        heightFieldDesc.samples.data = samples;

        PxHeightField *heightField = physics->createHeightField( heightFieldDesc );

        PxTransform pose = PxTransform::createIdentity();

        auto position = t.getPosition();
        auto scale = t.getScale();
        auto orientation = t.getOrientation();

        auto terrainScale = terrainSize / Vector3F( static_cast<f32>( size.X() - 1 ), 1.0f,
                                                    static_cast<f32>( size.Y() - 1 ) );

        auto center = position + Vector3F( 0.0f, 0.0f, -terrainSize.Z() );
        pose.p = PhysxUtil::toPx( center );
        pose.q = PhysxUtil::toPx( orientation );

        return { heightField, PxMeshGeometryFlags(), fHeightScale, terrainScale.X(), terrainScale.Z() };
    }

    auto PhysxTerrain::createDefaultTerrain( s32 sizeX, s32 sizeY, f32 maxHeight ) -> physx::PxShape *
    {
        using namespace physx;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager =
            workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
        auto gPhysics = physicsManager->getPhysics();

        // Create a heightfield
        PxHeightFieldDesc heightFieldDesc;
        heightFieldDesc.format = PxHeightFieldFormat::eS16_TM;
        heightFieldDesc.nbColumns = sizeX;
        heightFieldDesc.nbRows = sizeY;

        auto pData = new PxU16[sizeX * sizeY];
        heightFieldDesc.samples.data = pData;

        heightFieldDesc.samples.stride = sizeof( PxU16 );

        // Fill in the heightfield samples
        for( int i = 0; i < sizeX * sizeY; ++i )
        {
            // Generate some random height values
            // float height = maxHeight * (float)rand() / RAND_MAX;
            // pData[i] = PxU16( height * 0xffff );
            pData[i] = 0;
        }

        // Create a heightfield object
        PxHeightField *heightField = gPhysics->createHeightField( heightFieldDesc );
        BOOST_ASSERT( heightField != nullptr );

        PxMaterial *material = gPhysics->createMaterial( 1, 1, 1 );
        BOOST_ASSERT( material != nullptr );

        // Create a heightfield shape
        auto                         tolerancesScale = gPhysics->getTolerancesScale();
        auto                         heightScale = maxHeight / 0xffff;
        physx::PxHeightFieldGeometry hfGeom( heightField, physx::PxMeshGeometryFlags(), heightScale,
                                             static_cast<physx::PxReal>( 1 ),
                                             static_cast<physx::PxReal>( 1 ) );
        PxShape                     *heightFieldShape = gPhysics->createShape( hfGeom, *material );
        BOOST_ASSERT( heightFieldShape != nullptr );

        // Create a heightfield actor
        // PxRigidActor *heightFieldActor =
        ///   gPhysics->createRigidStatic( PxTransform( PxIdentity ) );
        // heightFieldActor->attachShape( *heightFieldShape );

        // Add the heightfield actor to the scene
        // gScene->addActor( *heightFieldActor );

        return heightFieldShape;
    }

    auto PhysxTerrain::getUserData() const -> void *
    {
        return m_userData;
    }

    void PhysxTerrain::setUserData( void *userData )
    {
        m_userData = userData;
    }

    auto PhysxTerrain::getHeightData() const -> Array<f32>
    {
        return m_heightData;
    }

    void PhysxTerrain::setHeightData( const Array<f32> &heightData )
    {
        m_heightData = heightData;
    }

    auto PhysxTerrain::getSize() const -> Vector2I
    {
        return m_size;
    }

    void PhysxTerrain::setSize( const Vector2I &size )
    {
        m_size = size;
    }

    auto PhysxTerrain::getTerrainSize() const -> Vector3<physics_Num>
    {
        return m_terrainSize;
    }

    void PhysxTerrain::setTerrainSize( const Vector3<physics_Num> &terrainSize )
    {
        m_terrainSize = terrainSize;
    }

    auto PhysxTerrain::getHeightScale() const -> Vector3<physics_Num>
    {
        return m_heightScale;
    }

    void PhysxTerrain::setHeightScale( const Vector3<physics_Num> &heightScale )
    {
        m_heightScale = heightScale;
    }

} // namespace workphone::physics
