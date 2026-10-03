#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxMeshShape.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/PhysxMemoryInputStream.hpp>
#include <WPPhysx/PhysxMemoryOutputStream.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxMeshShape, PhysxShape<MeshShape> );

    PhysxMeshShape::PhysxMeshShape()
    {
        createStateObject();
    }

    PhysxMeshShape::~PhysxMeshShape()
    {
        unload( nullptr );
    }

    void PhysxMeshShape::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            WP_ASSERT( !isLoaded() );

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            WP_ASSERT( physicsManager );

            ScopedLock lock( physicsManager );

            if( auto meshResource = getMeshResource() )
            {
                if( !meshResource->isLoaded() )
                {
                    meshResource->load( nullptr );
                }
            }

            if( data )
            {
                if( data->isDerived<IMeshResource>() )
                {
                    auto meshResource = workphone::static_pointer_cast<IMeshResource>( data );
                    setMeshResource( meshResource );
                }
            }

            if( auto body = getActor() )
            {
                if( body->isExactly<PhysxRigidDynamic>() )
                {
                    auto rigidDynamic = workphone::static_pointer_cast<PhysxRigidDynamic>( body );

                    auto pRigidDynamic = rigidDynamic->getActor();
                    setPxActor( pRigidDynamic );
                }

                if( body->isExactly<PhysxRigidStatic>() )
                {
                    auto rigidStatic = workphone::static_pointer_cast<PhysxRigidStatic>( body );

                    auto pRigidStatic = rigidStatic->getRigidStatic();
                    setPxActor( pRigidStatic );
                }
            }

            createShape();

            // WP_ASSERT( getShape() );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxMeshShape::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            ScopedLock lock( this );

            auto cleanMesh = getCleanMesh();
            if( cleanMesh )
            {
                cleanMesh->unload( nullptr );
                cleanMesh = nullptr;
                setCleanMesh( nullptr );
            }

            setMeshResource( nullptr );

            if( auto shape = getShape() )
            {
                if( auto pxActor = getPxActor() )
                {
                    pxActor->detachShape( *shape, false );
                }

                setShape( nullptr );
            }

            PhysxShape<MeshShape>::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxMeshShape::setAABB( const AABB3<physics_Num> &box )
    {
        m_extents = box.getExtent();
    }

    auto PhysxMeshShape::getAABB() const -> AABB3<physics_Num>
    {
        AABB3<physics_Num> box;
        box.setExtents( m_extents * -0.5f, m_extents * 0.5f );
        return box;
    }

    auto PhysxMeshShape::getWorldTransform( const Transform3<physics_Num> &t ) -> Transform3<physics_Num>
    {
        auto subMeshPosition = t.getPosition();
        auto subMeshScale = t.getScale();
        auto subMeshOrientation = t.getOrientation();

        auto collisionTransform = Transform3<physics_Num>();
        auto worldScale = subMeshScale * collisionTransform.getScale();
        auto worldOrientation = collisionTransform.getOrientation() * subMeshOrientation;
        auto worldPosition =
            collisionTransform.getOrientation() * ( collisionTransform.getScale() * subMeshPosition );
        worldPosition += collisionTransform.getPosition();

        worldPosition += m_colliderTransform.getPosition();
        worldOrientation = m_colliderTransform.getOrientation() * worldOrientation;

        Transform3<physics_Num> transform;
        transform.setPosition( worldPosition );
        transform.setOrientation( worldOrientation );
        transform.setScale( worldScale * m_colliderTransform.getScale() );
        return transform;
    }

    auto PhysxMeshShape::getMeshTransform( const Transform3<physics_Num> &t ) -> physx::PxTransform
    {
        auto subMeshPosition = t.getPosition();
        auto subMeshScale = t.getScale();
        auto subMeshOrientation = t.getOrientation();

        auto collisionTransform = Transform3<physics_Num>();
        auto worldScale = subMeshScale * collisionTransform.getScale();
        auto worldOrientation = collisionTransform.getOrientation() * subMeshOrientation;
        auto worldPosition =
            collisionTransform.getOrientation() * ( collisionTransform.getScale() * subMeshPosition );
        worldPosition += collisionTransform.getPosition();

        worldPosition += m_colliderTransform.getPosition();
        worldOrientation = m_colliderTransform.getOrientation() * worldOrientation;

        auto transform = physx::PxTransform::createIdentity();
        transform.p = physx::PxVec3( worldPosition.X(), worldPosition.Y(), worldPosition.Z() );
        transform.q = physx::PxQuat( worldOrientation.X(), worldOrientation.Y(), worldOrientation.Z(),
                                     worldOrientation.W() );
        return transform;
    }

    auto PhysxMeshShape::getMeshCachePath( u32 subMeshIndex ) const -> String
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto fileSystem = applicationManager->getFileSystemPtr();

        auto cachePath = applicationManager->getCachePath();

        auto meshName = String();
        auto meshResource = getMeshResource();
        if( meshResource )
        {
            auto fileSystemId = meshResource->getFileSystemId();
            if( !fileSystemId.is_nil() )
            {
                FileInfo fileInfo;
                if( !fileSystem->findFileInfo( fileSystemId, fileInfo, false ) )
                {
                    fileSystem->findFileInfo( fileSystemId, fileInfo, true );
                }

                meshName = fileInfo.filePath;
            }

            if( StringUtil::isNullOrEmpty( meshName ) )
            {
                meshName = meshResource->getFilePath();
            }

            if( StringUtil::isNullOrEmpty( meshName ) )
            {
                WP_LOG_ERROR( "Mesh has no name. Unable to cook physics mesh." );
            }

            if( !StringUtil::isNullOrEmpty( meshName ) )
            {
                if( !StringUtil::isNullOrEmpty( cachePath ) )
                {
                    auto meshPath = cachePath + "/" + meshName;

                    auto fileName = Path::getFileNameWithoutExtension( meshPath );
                    auto filePath = cachePath + "/" + fileName + ".pxtrianglemesh";
                    filePath = StringUtil::cleanupPath( filePath );

                    return filePath;
                }
                else
                {
                    auto fileName = Path::getFileNameWithoutExtension( meshName );
                    auto filePath = fileName + ".pxtrianglemesh";
                    filePath = StringUtil::cleanupPath( filePath );

                    return filePath;
                }
            }
        }

        return {};
    }

    void PhysxMeshShape::createMeshGeometry()
    {
        auto meshResource = getMeshResource();
        if( meshResource )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto physicsManager = (PhysxManager *)applicationManager->getPhysicsManagerPtr();
            WP_ASSERT( physicsManager );

            auto physics = physicsManager->getPhysics();
            WP_ASSERT( physics );

            auto cooking = physicsManager->getCooking();
            WP_ASSERT( cooking );

            auto localPose = getLocalPose();
            auto worldScale = localPose.getScale();

            physx::PxShape *shape = nullptr;

            if( isConvex() )
            {
                auto meshPath = getMeshCachePath( 0 );
                if( !StringUtil::isNullOrEmpty( meshPath ) )
                {
                    if( auto dataStream = fileSystem->open( meshPath ) )
                    {
                        const auto size = dataStream->size();
                        auto       buffer = new u8[size];
                        dataStream->read( buffer, size );
                        MemoryInputStream inputStream( buffer, static_cast<u32>( size ) );

                        auto convexMesh = physics->createConvexMesh( inputStream );

                        if( buffer )
                        {
                            delete[] buffer;
                            buffer = nullptr;
                        }

                        if( convexMesh )
                        {
                            auto bounds = convexMesh->getLocalBounds();

                            auto minExtents = Vector3<physics_Num>( bounds.minimum.x, bounds.minimum.y,
                                                                    bounds.minimum.z );
                            auto maxExtents = Vector3<physics_Num>( bounds.maximum.x, bounds.maximum.y,
                                                                    bounds.maximum.z );
                            auto aabb = AABB3<physics_Num>( minExtents, maxExtents );
                            setAABB( aabb );

                            physx::PxMeshScale meshScale( PhysxUtil::toPx( worldScale ),
                                                          physx::PxQuat::createIdentity() );

                            auto material = getMaterial();
                            auto pMat = workphone::static_pointer_cast<PhysxMaterial>( material );
                            auto pxMat = pMat->getMaterial();

                            shape = physics->createShape(
                                physx::PxConvexMeshGeometry( convexMesh, meshScale ), *pxMat );

                            setShape( shape );
                        }

                        setupCollisionMask( shape );
                    }
                }
            }
            else
            {
                auto meshPath = getMeshCachePath( 0 );
                if( !StringUtil::isNullOrEmpty( meshPath ) )
                {
                    MemoryInputStream *inputStream = nullptr;

                    if( !m_outputStream )
                    {
                        auto dataStream = fileSystem->open( meshPath, true, true, false, false, false );
                        if( !dataStream )
                        {
                            dataStream = fileSystem->open( meshPath, true, true, false, true, true );
                        }

                        if( dataStream )
                        {
                            const auto size = dataStream->size();
                            if( size == 0 )
                            {
                                return;
                            }

                            auto buffer = new u8[size];
                            dataStream->read( buffer, size );
                            inputStream =
                                new MemoryInputStream( buffer, static_cast<u32>( size ), true );
                        }
                    }
                    else
                    {
                        auto outputStream = (MemoryOutputStream *)m_outputStream;
                        inputStream = new MemoryInputStream( outputStream->getData(),
                                                             outputStream->getSize(), false );
                    }

                    if( inputStream )
                    {
                        auto triangleMesh = physics->createTriangleMesh( *inputStream );

                        if( triangleMesh )
                        {
                            auto bounds = triangleMesh->getLocalBounds();

                            auto minExtents = Vector3<physics_Num>( bounds.minimum.x, bounds.minimum.y,
                                                                    bounds.minimum.z );
                            auto maxExtents = Vector3<physics_Num>( bounds.maximum.x, bounds.maximum.y,
                                                                    bounds.maximum.z );
                            auto aabb = AABB3<physics_Num>( minExtents, maxExtents );
                            setAABB( aabb );

                            physx::PxMeshScale meshScale( PhysxUtil::toPx( worldScale ),
                                                          physx::PxQuat::createIdentity() );

                            auto material = getMaterial();
                            auto pMat = workphone::static_pointer_cast<PhysxMaterial>( material );
                            auto pxMat = pMat->getMaterial();

                            auto meshGeometry = physx::PxTriangleMeshGeometry( triangleMesh, meshScale );
                            if( meshGeometry.isValid() )
                            {
                                shape = physics->createShape( meshGeometry, *pxMat );
                            }

                            setShape( shape );
                        }

                        setupCollisionMask( shape );
                    }
                }
            }

            if( shape )
            {
                auto transform = PhysxUtil::toPx( localPose );
                shape->setLocalPose( transform );
            }
        }
    }

    void PhysxMeshShape::setIndices( s32 subMesh, const Array<u32> &indices )
    {
        WP_ASSERT( subMesh < m_indices.size() );

        auto &rIndices = m_indices[subMesh];
        rIndices.reserve( indices.size() );

        for( unsigned int index : indices )
        // for( s32 i = indices.size() - 1; i >= 0; --i )
        {
            rIndices.push_back( index );
        }
    }

    void PhysxMeshShape::readMeshData()
    {
        try
        {
            if( auto mesh = getMesh() )
            {
                if( mesh->getHasSharedVertexData() )
                {
                    auto vertices = MeshUtil::getPoints( mesh );
                    WP_ASSERT( !vertices.empty() );
                    m_sharedVertices = vertices;
                }
                else
                {
                    m_vertices.clear();
                    auto subMeshes = mesh->getSubMeshes();

                    auto numSubMeshes = subMeshes.size();
                    m_vertices.reserve( numSubMeshes );

                    for( size_t i = 0; i < numSubMeshes; ++i )
                    {
                        auto subMesh = subMeshes[i];

                        auto vertices = MeshUtil::getPoints( subMesh );
                        if( !vertices.empty() )
                        {
                            WP_LOG_ERROR( "Mesh has no shared vertex data. Submesh " +
                                          StringUtil::toString( i ) + " has " +
                                          StringUtil::toString( vertices.size() ) + " vertices." );
                        }

                        m_vertices.push_back( vertices );
                    }
                }

                auto subMeshes = mesh->getSubMeshes();
                auto numSubMeshes = subMeshes.size();
                m_indices.resize( numSubMeshes );

                for( size_t i = 0; i < numSubMeshes; ++i )
                {
                    auto subMesh = subMeshes[i];

                    auto indices = MeshUtil::getIndices( subMesh );
                    setIndices( static_cast<s32>( i ), indices );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxMeshShape::buildSubMesh( s32 subMeshIdx, const Array<u16> &indexBuffer,
                                       const Transform3<physics_Num> &t )
    {
        AABB3<physics_Num> bounds;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto pPhysicsManager = applicationManager->getPhysicsManager();
        auto physicsManager = workphone::static_pointer_cast<PhysxManager>( pPhysicsManager );
        auto physics = physicsManager->getPhysics();

        auto cooking = physicsManager->getCooking();

        auto transform = getMeshTransform( t );
        auto worldTransform = getWorldTransform( t );
        auto worldScale = worldTransform.getScale();

        physx::PxShape *shape = nullptr;

        if( isConvex() )
        {
            physx::PxConvexMeshDesc convex;

            convex.points.count = static_cast<u32>( m_sharedVertices.size() );
            convex.triangles.count = static_cast<u32>( indexBuffer.size() / 3 );
            convex.points.stride = sizeof( Vector3<physics_Num> );
            convex.triangles.stride = sizeof( u16 ) * 3;
            convex.points.data = &m_sharedVertices[0];
            convex.triangles.data = &indexBuffer[0];
            // convex.flags = physx::PxConvexFlag::e16_BIT_INDICES |
            // physx::PxConvexFlag::eCOMPUTE_CONVEX | physx::PxConvexFlag::eINFLATE_CONVEX;
            convex.flags = physx::PxConvexFlag::e16_BIT_INDICES;

            MemoryOutputStream outputStream;
            bool               success = cooking->cookConvexMesh( convex, outputStream );
            if( !success )
            {
                WP_LOG_ERROR( "Error cooking mesh." );
            }

            MemoryInputStream inputStream( outputStream.getData(), outputStream.getSize() );

            physx::PxConvexMesh *convexMesh = physics->createConvexMesh( inputStream );
            if( convexMesh )
            {
                physx::PxBounds3 bounds = convexMesh->getLocalBounds();

                AABB3<physics_Num> aabb;
                aabb.setMinimum(
                    Vector3<physics_Num>( bounds.minimum.x, bounds.minimum.y, bounds.minimum.z ) );
                aabb.setMaximum(
                    Vector3<physics_Num>( bounds.maximum.x, bounds.maximum.y, bounds.maximum.z ) );
                setAABB( aabb );

                physx::PxMeshScale meshScale( PhysxUtil::toPx( worldScale ),
                                              physx::PxQuat::createIdentity() );
                // shape = m_actor->createShape(physx::PxConvexMeshGeometry(convexMesh, meshScale),
                // *m_material);
                if( shape )
                {
                    shape->setLocalPose( transform );
                }
            }
        }
        else
        {
            physx::PxTriangleMeshDesc meshDesc;

            meshDesc.points.count = static_cast<u32>( m_sharedVertices.size() );
            meshDesc.triangles.count = static_cast<u32>( indexBuffer.size() / 3 );
            meshDesc.points.stride = sizeof( Vector3<physics_Num> );
            meshDesc.triangles.stride = sizeof( u16 ) * 3;
            meshDesc.points.data = &m_sharedVertices[0];
            meshDesc.triangles.data = &indexBuffer[0];
            meshDesc.flags = physx::PxMeshFlag::e16_BIT_INDICES | physx::PxMeshFlag::eFLIPNORMALS;

            MemoryOutputStream outputStream;
            bool               success = cooking->cookTriangleMesh( meshDesc, outputStream );
            if( !success )
            {
                WP_LOG_ERROR( "Error cooking mesh." );
            }

            MemoryInputStream inputStream( outputStream.getData(), outputStream.getSize() );

            physx::PxTriangleMesh *triangleMesh = physics->createTriangleMesh( inputStream );
            if( triangleMesh )
            {
                physx::PxBounds3 bounds = triangleMesh->getLocalBounds();

                AABB3<physics_Num> aabb;
                aabb.setMinimum(
                    Vector3<physics_Num>( bounds.minimum.x, bounds.minimum.y, bounds.minimum.z ) );
                aabb.setMaximum(
                    Vector3<physics_Num>( bounds.maximum.x, bounds.maximum.y, bounds.maximum.z ) );
                setAABB( aabb );

                physx::PxMeshScale meshScale( PhysxUtil::toPx( worldScale ),
                                              physx::PxQuat::createIdentity() );
                // shape = m_actor->createShape(physx::PxTriangleMeshGeometry(triangleMesh,
                // meshScale), *m_material);
                if( shape )
                {
                    shape->setLocalPose( transform );
                }
            }
        }

        setAABB( bounds );
    }

    void PhysxMeshShape::createStateObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto stateManager = applicationManager->getStateManager();
        auto factoryManager = applicationManager->getFactoryManager();
        auto physicsManager = applicationManager->getPhysicsManager();

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        setStateContext( stateContext );

        auto stateListener = factoryManager->make_ptr<ShapeStateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );
        stateContext->addStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        state->setOwner( this );
        stateContext->addState( state );

        auto shapeStateData = factoryManager->make_ptr<ShapeStateData>();
        state->setData( shapeStateData );

        auto meshShapeState = factoryManager->make_ptr<State>();
        meshShapeState->setOwner( this );
        stateContext->addState( meshShapeState );

        auto meshShapeStateData = factoryManager->make_ptr<MeshShapeStateData>();
        meshShapeState->setData( meshShapeStateData );

        auto physicsTask = physicsManager->getPhysicsTask();
        stateContext->setTaskId( physicsTask );
    }

    void PhysxMeshShape::createShape()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto physicsManager = applicationManager->getPhysicsManagerPtr();

            ScopedLock lock( physicsManager );

            auto material = getMaterial();
            if( !material )
            {
                auto material = physicsManager->addMaterial();
                setMaterial( material );
            }

            readMeshData();
            cookMesh();
            createMeshGeometry();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto PhysxMeshShape::getMeshResource() const -> SmartPtr<IMeshResource>
    {
        return m_meshResource;
    }

    void PhysxMeshShape::setMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        if( m_meshResource != meshResource )
        {
            m_meshResource = meshResource;
            createShape();
        }
    }

    auto PhysxMeshShape::getMesh() const -> SmartPtr<IMesh>
    {
        if( auto meshResource = getMeshResource() )
        {
            return meshResource->getMesh();
        }

        return nullptr;
    }

    auto PhysxMeshShape::getCleanMesh() const -> SmartPtr<IMesh>
    {
        return m_cleanMesh;
    }

    void PhysxMeshShape::setCleanMesh( SmartPtr<IMesh> cleanMesh )
    {
        m_cleanMesh = cleanMesh;
    }

    auto PhysxMeshShape::getMeshPath() const -> String
    {
        if( auto meshResource = getMeshResource() )
        {
            return meshResource->getFilePath();
        }

        return {};
    }

    auto PhysxMeshShape::isValid() const -> bool
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto state = scene->getState();
        if( state == scene::IGameScene::State::Play )
        {
            auto shape = getShape();

            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                if( !shape )
                {
                    return false;
                }
            }
        }

        return true;
    }

    auto PhysxMeshShape::getOutputStream() const -> physx::PxOutputStream *
    {
        return m_outputStream;
    }

    void PhysxMeshShape::setOutputStream( physx::PxOutputStream *outputStream )
    {
        m_outputStream = outputStream;
    }

    bool PhysxMeshShape::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        WP_ASSERT( message );

        PhysxShape<MeshShape>::handleStateChanged( message );

        if( message->isExactly<StateMessageObject>() )
        {
            auto stateMessageObject = workphone::static_pointer_cast<StateMessageObject>( message );
            auto type = stateMessageObject->getType();

            if( type == StateMessage::SET_MESH )
            {
                if( auto meshObject = stateMessageObject->getObject() )
                {
                    setMeshResource( meshObject );
                }
            }

            return true;
        }

        return false;
    }

    bool PhysxMeshShape::handleStateChanged( SmartPtr<IState> &state )
    {
        return PhysxShape<MeshShape>::handleStateChanged( state );
    }

    void PhysxMeshShape::cookMesh( const String &path )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto physicsManager = (PhysxManager *)applicationManager->getPhysicsManagerPtr();
            auto physics = physicsManager->getPhysics();
            WP_ASSERT( physics );

            auto cooking = physicsManager->getCooking();
            WP_ASSERT( cooking );

            auto meshCachePath = getMeshCachePath( 0 );
            readMeshData();

            auto cachePath = applicationManager->getCachePath();

            if( auto mesh = getMesh() )
            {
                if( mesh->getHasSharedVertexData() )
                {
                    auto numSubMeshes = m_indices.size();
                    for( size_t i = 0; i < numSubMeshes; ++i )
                    {
                        auto &indices = m_indices[i];

                        physx::PxTriangleMeshDesc meshDesc;

                        meshDesc.points.count = static_cast<u32>( m_sharedVertices.size() );
                        meshDesc.triangles.count = static_cast<u32>( indices.size() / 3 );
                        meshDesc.points.stride = sizeof( Vector3<physics_Num> );
                        meshDesc.triangles.stride = sizeof( u32 ) * 3;
                        meshDesc.points.data = &m_sharedVertices[0];
                        meshDesc.triangles.data = &indices[0];
                        // meshDesc.flags = physx::PxMeshFlag::eFLIPNORMALS;

                        auto fileName = Path::getFileNameWithoutExtension( path );
                        auto filePath = cachePath + "/" + fileName + ".pxtrianglemesh";
                        filePath = StringUtil::cleanupPath( filePath );

                        if( !fileSystem->isExistingFile( filePath ) )
                        {
                            // if (cooking->validateTriangleMesh(meshDesc))
                            {
                                physx::PxDefaultFileOutputStream outputStream( filePath.c_str() );
                                bool success = cooking->cookTriangleMesh( meshDesc, outputStream );
                                if( !success )
                                {
                                    fileSystem->deleteFile( filePath );
                                }
                            }
                        }

                        auto refreshPath = Path::getFilePath( filePath );
                        fileSystem->refreshPath( refreshPath, true );
                    }
                }
                else
                {
                    auto numSubMeshes = m_indices.size();
                    for( size_t i = 0; i < numSubMeshes; ++i )
                    {
                        auto &vertices = m_vertices[i];
                        auto &indices = m_indices[i];

                        physx::PxTriangleMeshDesc meshDesc;

                        meshDesc.points.count = static_cast<u32>( vertices.size() );
                        meshDesc.triangles.count = static_cast<u32>( indices.size() / 3 );
                        meshDesc.points.stride = sizeof( Vector3<physics_Num> );
                        meshDesc.triangles.stride = sizeof( u32 ) * 3;
                        meshDesc.points.data = &vertices[0];
                        meshDesc.triangles.data = &indices[0];
                        // meshDesc.flags = physx::PxMeshFlag::eFLIPNORMALS;

                        auto fileName = Path::getFileNameWithoutExtension( path );

                        auto filePath = String();
                        if( !StringUtil::isNullOrEmpty( cachePath ) )
                        {
                            filePath = cachePath + "/" + fileName + ".pxtrianglemesh";
                        }
                        else
                        {
                            filePath = fileName + ".pxtrianglemesh";
                        }

                        filePath = StringUtil::cleanupPath( filePath );

                        auto folderPath = Path::getFilePath( filePath );
                        if( fileSystem->isExistingFolder( folderPath ) )
                        {
                            if( !fileSystem->isExistingFile( filePath ) )
                            {
                                // auto validateResult = cooking->validateTriangleMesh( meshDesc );
                                // WP_ASSERT( validateResult )

                                // if( validateResult )
                                {
                                    physx::PxDefaultFileOutputStream outputStream( filePath.c_str() );
                                    bool success = cooking->cookTriangleMesh( meshDesc, outputStream );
                                    if( !success )
                                    {
                                        fileSystem->deleteFile( filePath );
                                    }
                                }
                            }

                            auto refreshPath = Path::getFilePath( filePath );
                            fileSystem->refreshPath( refreshPath, true );
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

    void PhysxMeshShape::cookMesh()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto fileSystem = applicationManager->getFileSystemPtr();

            auto physicsManager = (PhysxManager *)applicationManager->getPhysicsManagerPtr();
            WP_ASSERT( physicsManager );

            auto physics = physicsManager->getPhysics();
            WP_ASSERT( physics );

            auto cooking = physicsManager->getCooking();
            WP_ASSERT( cooking );

            auto path = getMeshPath();
            auto cachePath = applicationManager->getCachePath();

            if( auto mesh = getMesh() )
            {
                if( mesh->getHasSharedVertexData() )
                {
                    auto numSubMeshes = m_indices.size();
                    for( size_t i = 0; i < numSubMeshes; ++i )
                    {
                        auto &indices = m_indices[i];

                        physx::PxTriangleMeshDesc meshDesc;

                        meshDesc.points.count = static_cast<u32>( m_sharedVertices.size() );
                        meshDesc.triangles.count = static_cast<u32>( indices.size() / 3 );
                        meshDesc.points.stride = sizeof( Vector3<physics_Num> );
                        meshDesc.triangles.stride = sizeof( u32 ) * 3;
                        meshDesc.points.data = &m_sharedVertices[0];
                        meshDesc.triangles.data = &indices[0];
                        // meshDesc.flags = physx::PxMeshFlag::eFLIPNORMALS;

                        auto fileName = Path::getFileNameWithoutExtension( path );
                        auto filePath = cachePath + "/" + fileName + ".pxtrianglemesh";
                        filePath = StringUtil::cleanupPath( filePath );

                        if( !fileSystem->isExistingFile( filePath ) )
                        {
                            // if (cooking->validateTriangleMesh(meshDesc))
                            {
                                physx::PxDefaultFileOutputStream outputStream( filePath.c_str() );
                                bool success = cooking->cookTriangleMesh( meshDesc, outputStream );
                                if( !success )
                                {
                                    fileSystem->deleteFile( filePath );
                                }
                            }
                        }
                    }
                }
                else
                {
                    auto numSubMeshes = m_indices.size();
                    for( size_t i = 0; i < numSubMeshes; ++i )
                    {
                        auto &vertices = m_vertices[i];
                        auto &indices = m_indices[i];

                        if( vertices.empty() )
                        {
                            continue;
                        }

                        if( indices.empty() )
                        {
                            continue;
                        }

                        physx::PxTriangleMeshDesc meshDesc;

                        meshDesc.points.count = static_cast<u32>( vertices.size() );
                        meshDesc.triangles.count = static_cast<u32>( indices.size() / 3 );
                        meshDesc.points.stride = sizeof( Vector3<physics_Num> );
                        meshDesc.triangles.stride = sizeof( u32 ) * 3;
                        meshDesc.points.data = &vertices[0];
                        meshDesc.triangles.data = &indices[0];
                        // meshDesc.flags = physx::PxMeshFlag::eFLIPNORMALS;

                        auto fileName = Path::getFileNameWithoutExtension( path );

                        auto filePath = String();
                        if( !StringUtil::isNullOrEmpty( cachePath ) )
                        {
                            filePath = cachePath + "/" + fileName + ".pxtrianglemesh";
                        }
                        else
                        {
                            filePath = fileName + ".pxtrianglemesh";
                        }

                        filePath = StringUtil::cleanupPath( filePath );

                        if( StringUtil::contains( filePath, "grass" ) )
                        {
                            int a = 0;
                            a = 0;
                        }

                        auto folderPath = Path::getFilePath( filePath );
                        if( fileSystem->isExistingFolder( folderPath ) )
                        {
                            if( !fileSystem->isExistingFile( filePath ) )
                            {
                                // auto validateResult = cooking->validateTriangleMesh( meshDesc );
                                // WP_ASSERT( validateResult )

                                // if( validateResult )
                                {
                                    auto *outputStream = new MemoryOutputStream;
                                    bool  success = cooking->cookTriangleMesh( meshDesc, *outputStream );
                                    if( !success )
                                    {
                                        fileSystem->deleteFile( filePath );
                                    }

                                    fileSystem->writeAllBytes( filePath, outputStream->getData(),
                                                               outputStream->getSize() );

                                    setOutputStream( outputStream );
                                }
                            }
                        }
                        else
                        {
                            auto *outputStream = new MemoryOutputStream;
                            bool  success = cooking->cookTriangleMesh( meshDesc, *outputStream );
                            if( !success )
                            {
                                fileSystem->deleteFile( filePath );
                            }

                            fileSystem->writeAllBytes( filePath, outputStream->getData(),
                                                       outputStream->getSize() );

                            setOutputStream( outputStream );
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

    void PhysxMeshShape::setVertices( const Array<Vector3<physics_Num>> &vertices )
    {
        m_sharedVertices = vertices;
    }

    auto PhysxMeshShape::getVertices() const -> const Array<Vector3<physics_Num>> &
    {
        return m_sharedVertices;
    }

    auto PhysxMeshShape::getVertices() -> Array<Vector3<physics_Num>> &
    {
        return m_sharedVertices;
    }

    auto PhysxMeshShape::getNumSubMeshes() const -> s32
    {
        return static_cast<s32>( m_indices.size() );
    }

    auto PhysxMeshShape::getIndices( s32 subMeshIdx ) -> Array<u32> &
    {
        return m_indices[subMeshIdx];
    }

    auto PhysxMeshShape::getSubMeshTransform( s32 subMeshIdx ) -> Transform3<physics_Num>
    {
        return m_transforms[subMeshIdx];
    }

    auto PhysxMeshShape::getIndicesAs2dArray() const -> Array<Array<u16>>
    {
        Array<Array<u16>> indices;

        // std::map<s32, Array<u16>>::const_iterator it = m_indices.begin();
        // for (; it != m_indices.end(); ++it)
        //{
        //	const Array<u16>& data = it->second;
        //	indices.push_back(data);
        // }

        return indices;
    }

    auto PhysxMeshShape::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;

        // objects.push_back(m_rigidDynamic);
        // objects.push_back(m_rigidStatic);
        // objects.push_back(m_clonedActor);

        if( auto stateContext = getStateContext() )
        {
            auto states = stateContext->getStates();
            for( auto state : states )
            {
                objects.emplace_back( state );
            }
        }

        return objects;
    }

    auto PhysxMeshShape::getProperties() const -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto properties = factoryManager->make_ptr<Properties>();

        auto convex = isConvex();
        properties->setProperty( "convex", convex );

        return properties;
    }

    void PhysxMeshShape::setProperties( SmartPtr<Properties> properties )
    {
        auto convex = isConvex();
        properties->getPropertyValue( "convex", convex );
        setConvex( convex );
    }
} // namespace workphone::physics
