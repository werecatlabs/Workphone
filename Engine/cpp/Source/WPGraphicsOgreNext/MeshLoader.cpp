#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/MeshLoader.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreMesh2.h>
#include <OgreMeshManager2.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreSubMesh2.h>
#include <OgreSubMesh.h>
#include <OgreMeshManager.h>
#include <OgreMeshSerializer.h>
#include <OgreMesh.h>
#include <OgreMesh2.h>
#include <OgreException.h>
#include <OgreHardwareBufferManager.h>
#include <OgreLogManager.h>
#include <OgreBitwise.h>
#include <Vao/OgreVaoManager.h>
#include <Vao/OgreAsyncTicket.h>
#include <OgreVertexShadowMapHelper.h>
#include <OgreStringConverter.h>
#include <OgreDefaultHardwareBufferManager.h>
#include <OgreVertexIndexData.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreDistanceLodStrategy.h>
#include <OgreLodCollapseCostQuadric.h>
#include <OgreLodConfig.h>
#include <OgreMeshLodGenerator.h>
#include <OgrePixelCountLodStrategy.h>
#include "OgreMesh2Serializer.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace workphone::render
{
    namespace
    {
        size_t getTriangleCount( const SmartPtr<IMesh> &mesh )
        {
            size_t triangleCount = 0;
            if( mesh )
            {
                for( const auto &subMesh : mesh->getSubMeshes() )
                {
                    if( subMesh )
                    {
                        if( const auto indexBuffer = subMesh->getIndexBuffer() )
                        {
                            triangleCount += indexBuffer->getNumIndices() / 3u;
                        }
                    }
                }
            }
            return triangleCount;
        }

        Ogre::LodStrategy *getLodStrategy( MeshLodTransitionMode transitionMode )
        {
            if( transitionMode == MeshLodTransitionMode::Distance )
            {
                return Ogre::DistanceLodStrategy::getSingletonPtr();
            }
            return Ogre::ScreenRatioPixelCountLodStrategy::getSingletonPtr();
        }

        Ogre::Real getLodTransition( const ProgressiveMeshLodLevel &level,
                                     MeshLodTransitionMode transitionMode )
        {
            switch( transitionMode )
            {
            case MeshLodTransitionMode::ScreenHeight:
                return static_cast<Ogre::Real>( Ogre::Math::PI * level.screenHeight *
                                                level.screenHeight * 0.25f );
            case MeshLodTransitionMode::ScreenArea:
                return static_cast<Ogre::Real>( level.screenArea );
            case MeshLodTransitionMode::Distance:
            default:
                return static_cast<Ogre::Real>( level.distance );
            }
        }

        bool createProgressiveV2Mesh( Ogre::Mesh *mesh, const SmartPtr<IMesh> &fbmesh,
                                      const ProgressiveMeshOptions &options )
        {
            if( !mesh || !fbmesh || !options.isEnabled() )
            {
                return false;
            }

            String validationError;
            if( !options.validate( &validationError ) )
            {
                WP_LOG_ERROR( "Progressive mesh options are invalid for '" +
                              String( mesh->getName().c_str() ) + "': " + validationError );
                return false;
            }

            if( options.generationMode == MeshLodGenerationMode::Manual )
            {
                WP_LOG_ERROR(
                    "Manual progressive mesh levels are not supported by the Ogre-Next v2 "
                    "import path; loading the source mesh without generated LODs." );
                return false;
            }

            const auto sourceTriangleCount = getTriangleCount( fbmesh );
            if( sourceTriangleCount < options.minimumSourceTriangleCount )
            {
                return false;
            }

            const auto sourceMeshName =
                String( mesh->getName().c_str() ) + "_progressive_source";
            auto meshManagerV1 = Ogre::v1::MeshManager::getSingletonPtr();
            WP_ASSERT( meshManagerV1 );
            const auto resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            if( const auto existingSource =
                    meshManagerV1->getByName( sourceMeshName.c_str(), resourceGroup ) )
            {
                existingSource->unload();
                meshManagerV1->remove( existingSource );
            }

            auto sourceMesh = MeshLoader::convertFBMeshToOgreMesh( sourceMeshName, fbmesh );
            if( !sourceMesh )
            {
                WP_LOG_ERROR( "Unable to create the Ogre v1 source mesh needed for progressive "
                              "mesh generation: " +
                              sourceMeshName );
                return false;
            }

            sourceMesh->load();
            sourceMesh->removeLodLevels();

            auto lodStrategy = getLodStrategy( options.transitionMode );
            if( !lodStrategy )
            {
                WP_LOG_ERROR( "No Ogre LOD strategy is available for progressive mesh generation." );
                return false;
            }

            Ogre::LodConfig lodConfig( sourceMesh, lodStrategy );
            lodConfig.advanced.useBackgroundQueue = false;
            lodConfig.advanced.useCompression = options.compressIndexBuffers;
            lodConfig.advanced.useVertexNormals = options.useVertexNormals;
            lodConfig.advanced.outsideWeight =
                options.optimiseHiddenInterior ? options.outsideImportance : 0.0f;
            lodConfig.advanced.outsideWalkAngle =
                static_cast<Ogre::Real>( std::cos( options.outsideWalkAngleDegrees *
                                                   Ogre::Math::PI / 180.0f ) );

            const auto maximumProportionalReduction =
                sourceTriangleCount > options.minimumLodTriangleCount
                    ? 1.0f - static_cast<f32>( options.minimumLodTriangleCount ) /
                                 static_cast<f32>( sourceTriangleCount )
                    : 0.0f;

            for( const auto &level : options.getResolvedLevels() )
            {
                const auto transition = getLodTransition( level, options.transitionMode );
                switch( level.reductionMode )
                {
                case MeshLodReductionMode::Percentage:
                {
                    const auto reduction =
                        std::clamp( 1.0f - level.remainingGeometry, 0.0f,
                                    maximumProportionalReduction );
                    lodConfig.createGeneratedLodLevel( transition, reduction,
                                                       Ogre::LodLevel::VRM_PROPORTIONAL );
                }
                break;
                case MeshLodReductionMode::FixedVertexReduction:
                    lodConfig.createGeneratedLodLevel(
                        transition, static_cast<Ogre::Real>( level.verticesToRemove ),
                        Ogre::LodLevel::VRM_CONSTANT );
                    break;
                case MeshLodReductionMode::ErrorThreshold:
                    lodConfig.createGeneratedLodLevel(
                        transition, static_cast<Ogre::Real>( level.maximumError ),
                        Ogre::LodLevel::VRM_COLLAPSE_COST );
                    break;
                }
            }

            std::unique_ptr<Ogre::MeshLodGenerator> ownedGenerator;
            if( !Ogre::MeshLodGenerator::getSingletonPtr() )
            {
                ownedGenerator = std::make_unique<Ogre::MeshLodGenerator>();
            }

            Ogre::LodCollapseCostPtr collapseCost;
            if( options.simplificationMetric == MeshSimplificationMetric::QuadricError )
            {
                collapseCost =
                    Ogre::LodCollapseCostPtr( new Ogre::LodCollapseCostQuadric() );
            }

            Ogre::MeshLodGenerator::getSingleton().generateLodLevels( lodConfig, collapseCost );
            mesh->importV1( sourceMesh.get(), options.halfPosition, options.halfTexCoords,
                            options.qTangents, options.halfPoseData );
            mesh->setLodStrategyName( lodStrategy->getName() );
            mesh->prepareForShadowMapping( !options.generateShadowLods );

            WP_LOG_INFO( "Created progressive mesh '" + String( mesh->getName().c_str() ) +
                         "' with " + StringUtil::toString( mesh->getNumLodLevels() ) +
                         " LOD levels." );
            return true;
        }
    }  // namespace

    MeshLoader::MeshLoader()
    {
        setUseSingleMesh( true );
    }

    MeshLoader::~MeshLoader() = default;

    void MeshLoader::createV2Mesh( Ogre::Mesh *mesh, SmartPtr<IMesh> fbmesh )
    {
        createV2Mesh( mesh, fbmesh, ProgressiveMeshOptions() );
    }

    void MeshLoader::createV2Mesh( Ogre::Mesh *mesh, SmartPtr<IMesh> fbmesh,
                                   const ProgressiveMeshOptions &options )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        ScopedLock lock( graphicsSystem );

        if( createProgressiveV2Mesh( mesh, fbmesh, options ) )
        {
            return;
        }

        auto root = Ogre::Root::getSingletonPtr();
        auto renderSystem = root->getRenderSystem();
        auto vaoManager = renderSystem->getVaoManager();

        auto subMeshList = fbmesh->getSubMeshes();
        auto numSubMeshes = subMeshList.size();

        WP_ASSERT( mesh->getSubMeshes().empty() );  //already had submeshes

        for( size_t subMeshIdx = 0; subMeshIdx < numSubMeshes; ++subMeshIdx )
        {
            auto fbSubMesh = subMeshList[subMeshIdx];

            auto fbVertexBuffer = fbSubMesh->getVertexBuffer();
            auto fbIndexBuffer = fbSubMesh->getIndexBuffer();

            auto fbVertexCount = fbVertexBuffer->getNumVertices();
            auto fbIndexCount = fbIndexBuffer->getNumIndices();

            if( fbVertexCount <= 1 )
            {
                continue;
            }

            if( fbIndexCount <= 1 )
            {
                continue;
            }

            auto matName = fbSubMesh->getMaterialName();

            auto fbVertexDeclaration = fbVertexBuffer->getVertexDeclaration();

            //auto vertexElements = fbVertexDeclaration->getVertexElements();

            const auto pPosElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
            auto posElem = pPosElem.get();

            const auto pNormalElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
            auto normalElem = pNormalElem.get();

            Array<SmartPtr<IVertexElement>> texCoordElems;
            texCoordElems.reserve( 8 );

            for( u32 i = 0; i < 8; ++i )
            {
                auto texCoordElem = fbVertexDeclaration->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, i );
                if( texCoordElem )
                {
                    texCoordElems.push_back( texCoordElem );
                }
            }

            const auto pDiffuseElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_DIFFUSE );
            auto diffuseElem = pDiffuseElem.get();

            const auto pTangentElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_TANGENT );
            auto tangentElem = pTangentElem.get();

            auto subMesh = mesh->createSubMesh();

            Ogre::VertexElement2Vec vertexElements;
            vertexElements.reserve( 8 );

            vertexElements.emplace_back( Ogre::VET_FLOAT3, Ogre::VES_POSITION );
            vertexElements.emplace_back( Ogre::VET_FLOAT3, Ogre::VES_NORMAL );

            if( tangentElem )
            {
                vertexElements.emplace_back( Ogre::VET_FLOAT3, Ogre::VES_TANGENT );
            }

            //uvs
            for( auto texCoordElem : texCoordElems )
            {
                if( texCoordElem )
                {
                    auto texCoordElemType = texCoordElem->getType();

                    if( texCoordElemType == VertexElementType::VET_FLOAT3 )
                    {
                        vertexElements.emplace_back( Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES );
                    }
                    else if( texCoordElemType == VertexElementType::VET_FLOAT2 )
                    {
                        vertexElements.emplace_back( Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES );
                    }
                }
            }

            auto vertexSize = vaoManager->calculateVertexSize( vertexElements );

            auto numVertexDataBytes = vertexSize * fbVertexCount;
            auto vertexData = static_cast<Ogre::Real *>(
                OGRE_MALLOC_SIMD( numVertexDataBytes, Ogre::MEMCATEGORY_GEOMETRY ) );
            auto pVertex = vertexData;

            auto fbVertexSize = fbVertexDeclaration->getSize();
            auto fbVertexDataPtr = static_cast<u8 *>( fbVertexBuffer->getVertexData() );
            f32 *fbElementData = nullptr;

            for( u32 i = 0; i < fbVertexCount; ++i )
            {
                if( fbVertexDataPtr )
                {
                    posElem->getElementData( fbVertexDataPtr, &fbElementData );

                    *pVertex++ = fbElementData[0];
                    *pVertex++ = fbElementData[1];
                    *pVertex++ = fbElementData[2];

                    if( normalElem )
                    {
                        normalElem->getElementData( fbVertexDataPtr, &fbElementData );

                        *pVertex++ = fbElementData[0];
                        *pVertex++ = fbElementData[1];
                        *pVertex++ = fbElementData[2];
                    }

                    if( tangentElem )
                    {
                        tangentElem->getElementData( fbVertexDataPtr, &fbElementData );

                        *pVertex++ = fbElementData[0];
                        *pVertex++ = fbElementData[1];
                        *pVertex++ = fbElementData[2];
                    }

                    for( auto texCoordElem : texCoordElems )
                    {
                        if( texCoordElem )
                        {
                            auto texCoordElemType = texCoordElem->getType();

                            if( texCoordElemType == VertexElementType::VET_FLOAT3 )
                            {
                                texCoordElem->getElementData( fbVertexDataPtr, &fbElementData );

                                *pVertex++ = fbElementData[0];
                                *pVertex++ = fbElementData[1];
                                *pVertex++ = fbElementData[2];
                            }
                            else if( texCoordElemType == VertexElementType::VET_FLOAT2 )
                            {
                                texCoordElem->getElementData( fbVertexDataPtr, &fbElementData );
                                *pVertex++ = fbElementData[0];
                                *pVertex++ = fbElementData[1];
                            }
                        }
                    }

                    fbVertexDataPtr += fbVertexSize;
                }
            }

            Ogre::VertexBufferPackedVec vertexBuffers;

            auto pVertexBuffer =
                vaoManager->createVertexBuffer( vertexElements, static_cast<size_t>( fbVertexCount ),
                                                Ogre::BT_DEFAULT, vertexData, true );
            vertexBuffers.push_back( pVertexBuffer );

            auto indexBufferNumIndices = fbIndexBuffer->getNumIndices();
            auto buffType = fbIndexBuffer->getIndexType() == IIndexBuffer::Type::IT_16BIT
                                ? Ogre::IndexBufferPacked::IT_16BIT
                                : Ogre::IndexBufferPacked::IT_32BIT;

            Ogre::IndexBufferPacked *indexBuffer = nullptr;

            if( fbIndexBuffer->getIndexType() == IIndexBuffer::Type::IT_16BIT )
            {
                auto indexData = static_cast<u16 *>( OGRE_MALLOC_SIMD(
                    sizeof( u16 ) * indexBufferNumIndices, Ogre::MEMCATEGORY_GEOMETRY ) );
                auto fbIndexData = (u16 *)fbIndexBuffer->getIndexData();

                for( size_t i = 0; i < indexBufferNumIndices; ++i )
                {
                    indexData[i] = fbIndexData[i];
                }

                indexBuffer = vaoManager->createIndexBuffer(
                    buffType, static_cast<size_t>( indexBufferNumIndices ), Ogre::BT_DEFAULT, indexData,
                    true );
            }
            else
            {
                auto indexData = static_cast<u32 *>( OGRE_MALLOC_SIMD(
                    sizeof( u32 ) * indexBufferNumIndices, Ogre::MEMCATEGORY_GEOMETRY ) );
                auto fbIndexData = (u32 *)fbIndexBuffer->getIndexData();
                for( size_t i = 0; i < indexBufferNumIndices; ++i )
                {
                    indexData[i] = fbIndexData[i];
                }

                indexBuffer = vaoManager->createIndexBuffer(
                    buffType, static_cast<size_t>( indexBufferNumIndices ), Ogre::BT_DEFAULT, indexData,
                    true );
            }

            auto vao = vaoManager->createVertexArrayObject( vertexBuffers, indexBuffer,
                                                            Ogre::OT_TRIANGLE_LIST );

            subMesh->mVao[0].push_back( vao );
            subMesh->mVao[1].push_back( vao );
        }

        auto aabb = fbmesh->getAABB();
        auto vMin = aabb.getMinimum();
        auto vMax = aabb.getMaximum();

        Ogre::Aabb bounds;
        bounds.setExtents( Ogre::Vector3( vMin.x, vMin.y, vMin.z ),
                           Ogre::Vector3( vMax.x, vMax.y, vMax.z ) );
        mesh->_setBounds( bounds, false );
        mesh->_setBoundingSphereRadius( bounds.getRadius() );
    }

    void MeshLoader::loadFBMesh( Ogre::MeshPtr mesh, const String &meshPath )
    {
        auto pOgreMesh = mesh.get();
        loadFBMesh( pOgreMesh, meshPath );
    }

    void MeshLoader::loadFBMesh( Ogre::Mesh *mesh, const String &meshPath )
    {
        loadFBMesh( mesh, meshPath, ProgressiveMeshOptions() );
    }

    void MeshLoader::loadFBMesh( Ogre::Mesh *mesh, const String &meshPath,
                                 const ProgressiveMeshOptions &options )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto fileSystem = applicationManager->getFileSystemPtr();
        auto meshManager = applicationManager->getMeshManager();
        if( meshManager )
        {
            auto resource = meshManager->loadFromFile( meshPath );
            if( resource )
            {
                auto meshResource = workphone::static_pointer_cast<MeshResource>( resource );
                if( !meshResource->isLoaded() )
                {
                    meshResource->load( nullptr );
                }

                if( auto fbMesh = meshResource->getMesh() )
                {
                    createV2Mesh( mesh, fbMesh, options );

                    if( mesh->getNumSubMeshes() > 0 )
                    {
                        return;
                    }
                }
            }

            auto fbMeshStream = fileSystem->open( meshPath, true, true, false, false );
            if( !fbMeshStream )
            {
                fbMeshStream = fileSystem->open( meshPath, true, true, false, true );
            }

            if( fbMeshStream )
            {
                MeshSerializer meshSerializer;
                auto fbMesh = meshSerializer.loadMesh( fbMeshStream );

                createV2Mesh( mesh, fbMesh, options );

                if( mesh->getNumSubMeshes() > 0 )
                {
                    return;
                }
            }
            else
            {
                WP_LOG_ERROR( "Failed to open mesh file: " + meshPath );
            }
        }
    }

    void MeshLoader::loadFBMesh( Ogre::Mesh *mesh, SmartPtr<IResource> resource )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager->getMeshManager();
        if( meshManager )
        {
            if( resource )
            {
                auto meshResource = workphone::static_pointer_cast<MeshResource>( resource );
                if( !meshResource->isLoaded() )
                {
                    meshResource->load( nullptr );
                }

                if( auto fbMesh = meshResource->getMesh() )
                {
                    createV2Mesh( mesh, fbMesh );
                }
            }
        }
    }

    auto MeshLoader::loadFBMesh( SmartPtr<IStream> stream ) -> Ogre::Mesh *
    {
        return nullptr;
    }

    void MeshLoader::loadFBMesh( Ogre::MeshPtr meshPtr, SmartPtr<IMesh> mesh )
    {
        using namespace Ogre;

        WP_LOG( "Mesh2::importV1" );

        mesh->load( nullptr );

        auto loadingState = meshPtr->getLoadingState();

        if( loadingState != Ogre::Resource::LoadingState::LOADSTATE_UNLOADED &&
            loadingState != Ogre::Resource::LoadingState::LOADSTATE_LOADING )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INVALID_STATE,
                         "To import a v1 mesh, the v2 mesh must be in unloaded state!",
                         "Mesh::importV1" );
        }

        if( mesh->getHasSharedVertexData() )
        {
            WP_LOG( "WARNING: Mesh '" + mesh->getName() +
                    "' has shared vertices. They're being "
                    "'unshared' for importing to v2" );
            MeshUtil::unshareVertices( mesh );
        }

        auto aabb = mesh->getAABB();
        auto minimum = aabb.getMinimum();
        auto maximum = aabb.getMaximum();

        auto ogreAabb = Ogre::AxisAlignedBox( Ogre::Vector3( minimum.X(), minimum.Y(), minimum.Z() ),
                                              Ogre::Vector3( maximum.X(), maximum.Y(), maximum.Z() ) );
        // auto ogreBoundingRadius = ogreAabb.getHalfSize().length();

        /*
        try
        {
            if( qTangents )
            {
                unsigned short sourceCoordSet;
                unsigned short index;
                bool alreadyHasTangents = mesh->suggestTangentVectorBuildParams( VES_TANGENT,
                        sourceCoordSet,
                        index );
                if( !alreadyHasTangents )
                    mesh->buildTangentVectors( VES_TANGENT, sourceCoordSet, index, false, false, true
        );
            }
        }
        catch( Exception & )
        {
        }*/

        bool halfPos = false;
        bool halfTexCoords = false;
        bool qTangents = false;
        bool halfPose = false;

        auto subMeshes = mesh->getSubMeshes();
        for( auto pSubMesh : subMeshes )
        {
            auto subMesh = meshPtr->createSubMesh();
            importFromV1( meshPtr, subMesh, mesh, pSubMesh, halfPos, halfTexCoords, qTangents,
                          halfPose );
        }
        /*
                    mSubMeshNameMap = mesh->getSubMeshNameMap();

                    mSkeletonName = mesh->getSkeletonName();
                    v1::SkeletonPtr v1Skeleton = mesh->getOldSkeleton();
                    if( !v1Skeleton.isNull() )
                        mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( v1Skeleton.get()
           );

                    //So far we only import manual LOD levels. If the mesh had manual LOD levels,
                    //mLodValues will have more entries than Vaos, causing an out of bounds
           exception.
                    //Don't use LOD if the imported mesh had manual levels.
                    //Note: Mesh2 supports LOD levels that have their own vertex and index buffers,
                    //so it should be possible to import them as well.
                    if( !mesh->hasManualLodLevel() )
                        mLodValues = *mesh->_getLodValueArray();
                    else
                        mLodValues = MovableObject::c_DefaultLodMesh;
        */

        meshPtr->setManuallyLoaded( true );
        meshPtr->setToLoaded();
    }

    auto MeshLoader::convertFBMeshToOgreMesh( const String &newMeshName, SmartPtr<IMesh> mesh )
        -> Ogre::v1::MeshPtr
    {
        if( !mesh )
        {
            return {};
        }

        auto bLogUVs = StringUtil::contains( newMeshName, "cube" );

        static int meshIdx = 0;

        auto meshManager = Ogre::v1::MeshManager::getSingletonPtr();
        WP_ASSERT( meshManager );

        auto logManager = Ogre::LogManager::getSingletonPtr();
        WP_ASSERT( logManager );

        auto hardwareBufferManager = Ogre::v1::HardwareBufferManager::getSingletonPtr();
        WP_ASSERT( hardwareBufferManager );

        auto defaultHardwareBufferManager = Ogre::v1::DefaultHardwareBufferManager::getSingletonPtr();
        WP_ASSERT( defaultHardwareBufferManager );

        auto group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
        // Ogre::String meshName = "OutMesh" + Ogre::StringConverter::toString(meshIdx++);
        Ogre::String meshName = newMeshName.c_str();
        auto createOrRetriveMesh =
            meshManager->createOrRetrieve( meshName, Ogre::String( "General" ), true );
        if( !createOrRetriveMesh.second )
        {
            logManager->logMessage( "maxExporter::doMesh resource already exists" );
            logManager->logMessage( "maxExporter::doMesh end" );
            return {};
        }

        Ogre::v1::MeshPtr pMesh = createOrRetriveMesh.first.staticCast<Ogre::v1::Mesh>();

        // Ogre::v1::Mesh* pMesh = new Ogre::v1::Mesh(nullptr, newMeshName, -1, group);
        // Ogre::v1::VertexData* pData = nullptr;

        Ogre::v1::HardwareVertexBufferSharedPtr pBuf;
        // Ogre::Real* pReal = nullptr;
        Ogre::Vector3 currPos;
        Ogre::Real maxSquaredRadius = 0.0f;
        bool firstVert = true;

        Ogre::Vector3 aabbMin( 1e10, 1e10, 1e10 );
        Ogre::Vector3 aabbMax( -1e10, -1e10, -1e10 );

        auto subMeshList = mesh->getSubMeshes();
        auto numSubMeshes = subMeshList.size();

        for( size_t subMeshIdx = 0; subMeshIdx < numSubMeshes; ++subMeshIdx )
        {
            auto fbSubMesh = subMeshList[subMeshIdx];

            auto fbVertexBuffer = fbSubMesh->getVertexBuffer();
            auto fbIndexBuffer = fbSubMesh->getIndexBuffer();

            u32 fbVertexCount = fbVertexBuffer->getNumVertices();
            u32 fbIndexCount = fbIndexBuffer->getNumIndices();

            if( fbVertexCount <= 1 )
            {
                continue;
            }

            if( fbIndexCount <= 1 )
            {
                continue;
            }

            auto matName = fbSubMesh->getMaterialName();
            //auto matName = String( "" );

            // create subMesh
            auto ogreSubMesh = pMesh->createSubMesh( matName.c_str() );
            ogreSubMesh->setMaterialName( matName.c_str() );

            ogreSubMesh->useSharedVertices = false;
            ogreSubMesh->vertexData[Ogre::VpNormal] = new Ogre::v1::VertexData( nullptr );
            ogreSubMesh->indexData[Ogre::VpNormal] = new Ogre::v1::IndexData();

            ogreSubMesh->vertexData[Ogre::VpShadow] = ogreSubMesh->vertexData[Ogre::VpNormal];
            ogreSubMesh->indexData[Ogre::VpShadow] = ogreSubMesh->indexData[Ogre::VpNormal];

            size_t ogreNumVertices = ogreSubMesh->vertexData[0]->vertexCount = fbVertexCount;
            auto decl = ogreSubMesh->vertexData[0]->vertexDeclaration;

            WP_ASSERT_TRUE( ogreSubMesh->vertexData[0]->vertexCount == 0 );

            auto fbVertexDeclaration = fbVertexBuffer->getVertexDeclaration();
            const auto posElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
            const auto normalElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
            const auto texCoordElem0 = fbVertexDeclaration->findElementBySemantic(
                VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
            const auto texCoordElem1 = fbVertexDeclaration->findElementBySemantic(
                VertexElementSemantic::VES_TEXTURE_COORDINATES, 1 );
            const auto diffuseElem =
                fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_DIFFUSE );

            unsigned short iBinding = 0;
            u32 offset = 0;
            decl->addElement( iBinding, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION );
            offset += sizeof( f32 ) * 3;

            if( normalElem )
            {
                decl->addElement( iBinding, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL );
                offset += sizeof( f32 ) * 3;
            }

            if( texCoordElem0 )
            {
                decl->addElement( iBinding, offset, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES, 0 );
                offset += sizeof( f32 ) * 3;
            }

            if( texCoordElem1 )
            {
                decl->addElement( iBinding, offset, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES, 1 );
                offset += sizeof( f32 ) * 3;
            }

            if( diffuseElem )
            {
                decl->addElement( iBinding, offset, Ogre::VET_FLOAT4, Ogre::VES_DIFFUSE, 0 );
                offset += sizeof( f32 ) * 4;
            }

            auto ogreVertexSize = decl->getVertexSize( 0 );

            // Create new vertex buffer
            auto ogreVertexBuffer = defaultHardwareBufferManager->createVertexBuffer(
                ogreVertexSize, ogreNumVertices, Ogre::v1::HardwareBuffer::HBU_STATIC );

            // Bind buffer
            auto pBind = ogreSubMesh->vertexData[0]->vertexBufferBinding;
            pBind->setBinding( 0, ogreVertexBuffer );

            auto *ogreVertexDataPtr = static_cast<Ogre::Real *>(
                ogreVertexBuffer->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );

            u32 fbVertexSize = fbVertexDeclaration->getSize();
            u8 *fbVertexDataPtr = static_cast<u8 *>( fbVertexBuffer->getVertexData() );
            f32 *fbElementData = nullptr;

            for( u32 vertIdx = 0; vertIdx < fbVertexCount; ++vertIdx )
            {
                posElem->getElementData( fbVertexDataPtr, &fbElementData );

                Ogre::Vector3 ogrePosition( fbElementData[0], fbElementData[1], fbElementData[2] );

                // Deal with bounds
                currPos = ogrePosition;
                if( firstVert )
                {
                    aabbMin = aabbMax = currPos;
                    maxSquaredRadius = currPos.squaredLength();
                    firstVert = false;
                }
                else
                {
                    aabbMin.makeFloor( currPos );
                    aabbMax.makeCeil( currPos );
                    maxSquaredRadius = MathF::max( maxSquaredRadius, currPos.squaredLength() );
                }

                if( ogrePosition.x < aabbMin.x )
                {
                    aabbMin.x = ogrePosition.x;
                }
                if( ogrePosition.y < aabbMin.y )
                {
                    aabbMin.y = ogrePosition.y;
                }
                if( ogrePosition.z < aabbMin.z )
                {
                    aabbMin.z = ogrePosition.z;
                }

                if( ogrePosition.x > aabbMax.x )
                {
                    aabbMax.x = ogrePosition.x;
                }
                if( ogrePosition.y > aabbMax.y )
                {
                    aabbMax.y = ogrePosition.y;
                }
                if( ogrePosition.z > aabbMax.z )
                {
                    aabbMax.z = ogrePosition.z;
                }

                *ogreVertexDataPtr++ = ogrePosition.x;
                *ogreVertexDataPtr++ = ogrePosition.y;
                *ogreVertexDataPtr++ = ogrePosition.z;

                if( normalElem )
                {
                    normalElem->getElementData( fbVertexDataPtr, &fbElementData );
                    Ogre::Vector3 ogreNormal( fbElementData[0], fbElementData[1], fbElementData[2] );

                    *ogreVertexDataPtr++ = ogreNormal.x;
                    *ogreVertexDataPtr++ = ogreNormal.y;
                    *ogreVertexDataPtr++ = ogreNormal.z;
                }

                if( texCoordElem0 )
                {
                    if( texCoordElem0->getType() == VertexElementType::VET_FLOAT3 )
                    {
                        texCoordElem0->getElementData( fbVertexDataPtr, &fbElementData );
                        *ogreVertexDataPtr++ = fbElementData[0];
                        *ogreVertexDataPtr++ = fbElementData[1];
                        *ogreVertexDataPtr++ = fbElementData[2];

                        if( bLogUVs )
                        {
                            WP_LOG(
                                StringUtil::toString( Vector2F( fbElementData[0], fbElementData[1] ) ) );
                        }
                    }
                    else if( texCoordElem0->getType() == VertexElementType::VET_FLOAT2 )
                    {
                        texCoordElem0->getElementData( fbVertexDataPtr, &fbElementData );
                        *ogreVertexDataPtr++ = fbElementData[0];
                        *ogreVertexDataPtr++ = fbElementData[1];
                        *ogreVertexDataPtr++ = 0.0f;

                        if( bLogUVs )
                        {
                            WP_LOG(
                                StringUtil::toString( Vector2F( fbElementData[0], fbElementData[1] ) ) );
                        }
                    }
                }

                if( texCoordElem1 )
                {
                    if( texCoordElem1->getType() == VertexElementType::VET_FLOAT3 )
                    {
                        texCoordElem1->getElementData( fbVertexDataPtr, &fbElementData );
                        *ogreVertexDataPtr++ = fbElementData[0];
                        *ogreVertexDataPtr++ = fbElementData[1];
                        *ogreVertexDataPtr++ = fbElementData[2];
                    }
                    else if( texCoordElem1->getType() == VertexElementType::VET_FLOAT2 )
                    {
                        texCoordElem1->getElementData( fbVertexDataPtr, &fbElementData );
                        *ogreVertexDataPtr++ = fbElementData[0];
                        *ogreVertexDataPtr++ = fbElementData[1];
                        *ogreVertexDataPtr++ = 0.0f;
                    }
                }

                if( diffuseElem )
                {
                    diffuseElem->getElementData( fbVertexDataPtr, &fbElementData );
                    *ogreVertexDataPtr++ = fbElementData[0];
                    *ogreVertexDataPtr++ = fbElementData[1];
                    *ogreVertexDataPtr++ = fbElementData[2];
                    *ogreVertexDataPtr++ = fbElementData[3];
                }

                fbVertexDataPtr += fbVertexSize;
            }

            ogreVertexBuffer->unlock();

            auto indexData = ogreSubMesh->indexData[0];
            indexData->indexCount = fbIndexCount;
            WP_ASSERT_TRUE( indexData->indexCount == 0 );

            bool use32bit = fbVertexCount > std::numeric_limits<u16>::max();

            auto ogreIndexBuffer = defaultHardwareBufferManager->createIndexBuffer(
                use32bit ? Ogre::v1::HardwareIndexBuffer::IT_32BIT
                         : Ogre::v1::HardwareIndexBuffer::IT_16BIT,
                indexData->indexCount, Ogre::v1::HardwareBuffer::HBU_STATIC );
            indexData->indexBuffer = ogreIndexBuffer;

            u16 *pWords = use32bit ? nullptr
                                   : static_cast<u16 *>( ogreIndexBuffer->lock(
                                         Ogre::v1::HardwareBuffer::HBL_DISCARD ) );
            u32 *pDWords = use32bit ? static_cast<u32 *>( ogreIndexBuffer->lock(
                                          Ogre::v1::HardwareBuffer::HBL_DISCARD ) )
                                    : nullptr;

            if( fbIndexBuffer->getIndexType() == IIndexBuffer::Type::IT_32BIT )
            {
                const u32 *fbIndexData = reinterpret_cast<u32 *>( fbIndexBuffer->getIndexData() );

                // for (s32 i = (s32)fbIndexCount - 1; i >= 0; --i)
                for( u32 i = 0; i < fbIndexCount; ++i )
                {
                    auto index = fbIndexData[i];
                    if( use32bit )
                    {
                        *pDWords++ = static_cast<u32>( index );
                    }
                    else
                    {
                        *pWords++ = static_cast<u16>( index );
                    }
                }
            }
            else
            {
                const u16 *fbIndexData = reinterpret_cast<u16 *>( fbIndexBuffer->getIndexData() );

                // for (s32 i = (s32)fbIndexCount - 1; i >= 0; --i)
                for( u32 i = 0; i < fbIndexCount; ++i )
                {
                    auto index = fbIndexData[i];
                    if( use32bit )
                    {
                        *pDWords++ = static_cast<u32>( index );
                    }
                    else
                    {
                        *pWords++ = index;
                    }
                }
            }

            ogreIndexBuffer->unlock();
        }

        // Set bounds
        if( !subMeshList.empty() )
        {
            pMesh->_setBoundingSphereRadius( Ogre::Math::Sqrt( maxSquaredRadius ) );
            pMesh->_setBounds( Ogre::AxisAlignedBox( aabbMin, aabbMax ) );
        }
        else
        {
            pMesh->_setBoundingSphereRadius( 0.1f );
            pMesh->_setBounds( Ogre::AxisAlignedBox( Ogre::Vector3::UNIT_SCALE * -1.0f,
                                                     Ogre::Vector3::UNIT_SCALE * 1.0f ) );
        }

        /*Ogre::Mesh::LodValueList lodList;
        lodList.push_back(500.0f);
        lodList.push_back(750.0f);
        lodList.push_back(900.0f);
        lodList.push_back(1100.0f);
        pMesh->generateLodLevels(lodList,
        Ogre::ProgressiveMesh::VertexReductionQuota::VRQ_PROPORTIONAL, 0.2f);*/

        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager->isEditor() )
        {
            auto cacheFolder = applicationManager->getCachePath();

            if( !StringUtil::isNullOrEmpty( cacheFolder ) )
            {
                auto meshFileName = newMeshName + ".mesh";
                auto meshFilePath = Path::lexically_normal( cacheFolder, meshFileName );

                Ogre::v1::MeshSerializer serializer;
                serializer.exportMesh( pMesh.get(), meshFilePath.c_str(),
                                       Ogre::v1::MeshVersion::MESH_VERSION_1_10 );
            }
        }

        // meshManager->unload(meshName);

        return pMesh;
    }

    void MeshLoader::importBuffersFromV1( Ogre::MeshPtr newMesh, Ogre::SubMesh *pSubMesh,
                                          SmartPtr<IMesh> mesh, SmartPtr<ISubMesh> subMesh, bool halfPos,
                                          bool halfTexCoords, bool qTangents, bool halfPose,
                                          size_t vaoPassIdx )
    {
        using namespace Ogre;

        /*
        VertexElement2Vec vertexElements;
        char *data = _arrangeEfficient( subMesh, halfPos, halfTexCoords, qTangents, &vertexElements,
                vaoPassIdx );

        //Wrap the ptrs around these, because the VaoManager's call
        //can throw thus causing a leak if we don't free them.
        FreeOnDestructor dataPtrContainer( data );

        VaoManager* vaoManager = newMesh->_getVaoManager();
        VertexBufferPackedVec vertexBuffers;

        //Create the vertex buffer
        bool keepAsShadow = newMesh->mVertexBufferShadowBuffer;
        VertexBufferPacked *vertexBuffer = vaoManager->createVertexBuffer( vertexElements,
                subMesh->vertexData[vaoPassIdx]->vertexCount,
                mParent->mVertexBufferDefaultType,
                data, keepAsShadow );
        vertexBuffers.push_back( vertexBuffer );

        if( keepAsShadow ) //Don't free the pointer ourselves
            dataPtrContainer.ptr = 0;

        IndexBufferPacked *indexBuffer = importFromV1( subMesh->indexData[vaoPassIdx] );

        {
            VertexArrayObject *vao = vaoManager->createVertexArrayObject( vertexBuffers, indexBuffer,
                    subMesh->operationType );
            mVao[vaoPassIdx].push_back( vao );
        }

        //Now deal with the automatic LODs
        v1::SubMesh::LODFaceList::const_iterator itor = subMesh->mLodFaceList[vaoPassIdx].begin();
        v1::SubMesh::LODFaceList::const_iterator end  = subMesh->mLodFaceList[vaoPassIdx].end();

        while( itor != end )
        {
            IndexBufferPacked *lodIndexBuffer = importFromV1( *itor );

            VertexArrayObject *vao = vaoManager->createVertexArrayObject( vertexBuffers,
        lodIndexBuffer, subMesh->operationType );

            mVao[vaoPassIdx].push_back( vao );
            ++itor;
        }

        importPosesFromV1( subMesh, vertexBuffer, halfPose );
         */
    }

    auto MeshLoader::importFromV1( SmartPtr<IIndexBuffer> indexData ) -> Ogre::IndexBufferPacked *
    {
        /*
        if( !indexData || indexData->indexBuffer.isNull() )
            return 0;

        //Create & copy the index buffer
        const size_t indexSize = indexData->indexBuffer->getIndexSize();
        bool keepAsShadow = mParent->mIndexBufferShadowBuffer;
        VaoManager *vaoManager = mParent->mVaoManager;
        void *indexDataPtr = OGRE_MALLOC_SIMD( indexData->indexCount * indexSize,
                MEMCATEGORY_GEOMETRY );
        FreeOnDestructor indexDataPtrContainer( indexDataPtr );
        IndexBufferPacked::IndexType indexType = static_cast<IndexBufferPacked::IndexType>(
                indexData->indexBuffer->getType() );

        const uint8 *srcIndexDataPtr = reinterpret_cast<uint8*>(
                indexData->indexBuffer->lock( v1::HardwareBuffer::HBL_READ_ONLY ) );

        memcpy( indexDataPtr, srcIndexDataPtr + indexData->indexStart * indexSize,
                indexSize * indexData->indexCount );
        indexData->indexBuffer->unlock();

        IndexBufferPacked *indexBuffer = vaoManager->createIndexBuffer( indexType,
        indexData->indexCount, mParent->mIndexBufferDefaultType, indexDataPtr, keepAsShadow );

        if( keepAsShadow ) //Don't free the pointer ourselves
            indexDataPtrContainer.ptr = 0;

        return indexBuffer;
         */

        return nullptr;
    }

    void MeshLoader::importPosesFromV1( SmartPtr<ISubMesh> subMesh,
                                        Ogre::VertexBufferPacked *vertexBuffer, bool halfPrecision )
    {
        /*
        // Find the index of this subMesh and only process poses which have this
        // subMesh as their target.
        v1::Mesh::SubMeshList::const_iterator subMeshBegin
        =subMesh->parent->getSubMeshIterator().begin(); v1::Mesh::SubMeshList::const_iterator
        subMeshEnd = subMesh->parent->getSubMeshIterator().end();
        v1::Mesh::SubMeshList::const_iterator subMeshIt = std::find( subMeshBegin, subMeshEnd,
        subMesh );

        assert( subMeshIt != subMeshEnd && "Parent mesh does not contain this subMesh.");

        const size_t subMeshIndex = static_cast<size_t>( subMeshIt - subMeshBegin );

        const v1::PoseList &poseListOrig = subMesh->parent->getPoseList();
        v1::PoseList poseList;
        poseList.reserve( poseListOrig.size() );
        {
            v1::PoseList::const_iterator itor = poseListOrig.begin();
            v1::PoseList::const_iterator end  = poseListOrig.end();

            while( itor != end )
            {
                if( (*itor)->getTarget() == subMeshIndex )
                    poseList.push_back( *itor );
                ++itor;
            }
        }

        mNumPoses = static_cast<uint16>( poseList.size() );
        mPoseHalfPrecision = halfPrecision;

        if( mNumPoses > 0 )
        {
            mPoseNormals = poseList[0]->getIncludesNormals();
            size_t numVertices = vertexBuffer->getNumElements();
            size_t elementSize = halfPrecision ? sizeof( uint16 ) : sizeof( float );
            size_t elementsPerVertex = mPoseNormals ? 8 : 4;
            size_t singlePoseBufferSize = numVertices * elementSize * elementsPerVertex;
            size_t bufferSize = mNumPoses * singlePoseBufferSize;
            char *buffer = static_cast<char*>( OGRE_MALLOC_SIMD( bufferSize,
                    MEMCATEGORY_GEOMETRY ) );
            FreeOnDestructor bufferPtrContainer( buffer );
            memset( buffer, 0, bufferSize );

            v1::Mesh::PoseIterator poseIt = subMesh->parent->getPoseIterator();

            size_t index = 0u;

            while( poseIt.hasMoreElements() )
            {
                v1::Pose* pose = poseIt.getNext();
                v1::Pose::VertexOffsetMap::const_iterator v = pose->getVertexOffsets().begin();
                v1::Pose::NormalsIterator::const_iterator n = pose->getNormalsIterator().begin();

                if( halfPrecision )
                {
                    uint16* pHalf = reinterpret_cast<uint16*>( buffer +  index * singlePoseBufferSize
        ); while( v != pose->getVertexOffsets().end() )
                    {
                        size_t idx = v->first * elementsPerVertex;
                        pHalf[idx+0] = Bitwise::floatToHalf( v->second.x );
                        pHalf[idx+1] = Bitwise::floatToHalf( v->second.y );
                        pHalf[idx+2] = Bitwise::floatToHalf( v->second.z );
                        pHalf[idx+3] = Bitwise::floatToHalf( 0.f );
                        ++v;

                        if( mPoseNormals )
                        {
                            pHalf[idx+4] = Bitwise::floatToHalf( n->second.x );
                            pHalf[idx+5] = Bitwise::floatToHalf( n->second.y );
                            pHalf[idx+6] = Bitwise::floatToHalf( n->second.z );
                            pHalf[idx+7] = Bitwise::floatToHalf( 0.f );
                            ++n;
                        }
                    }
                }
                else
                {
                    float* pFloat = reinterpret_cast<float*>( buffer + index * singlePoseBufferSize
        ); while( v != pose->getVertexOffsets().end() )
                    {
                        size_t idx = v->first * elementsPerVertex;
                        pFloat[idx+0] = v->second.x;
                        pFloat[idx+1] = v->second.y;
                        pFloat[idx+2] = v->second.z;
                        pFloat[idx+3] = 0.f;
                        ++v;

                        if( mPoseNormals )
                        {
                            pFloat[idx+4] = n->second.x;
                            pFloat[idx+5] = n->second.y;
                            pFloat[idx+6] = n->second.z;
                            pFloat[idx+7] = 0.f;
                            ++n;
                        }
                    }
                }

                mPoseIndexMap[pose->getName()] = index++;
            }

            PixelFormatGpu pixelFormat = halfPrecision ? PFG_RGBA16_FLOAT : PFG_RGBA32_FLOAT;
            mPoseTexBuffer = mParent->mVaoManager->createTexBuffer( pixelFormat, bufferSize,
                    BT_IMMUTABLE, buffer, false );
        }
         */
    }

    void MeshLoader::importFromV1( Ogre::MeshPtr newMesh, Ogre::SubMesh *newSubMesh,
                                   SmartPtr<IMesh> mesh, SmartPtr<ISubMesh> subMesh, bool halfPos,
                                   bool halfTexCoords, bool qTangents, bool halfPose )
    {
        auto mMaterialName = subMesh->getMaterialName();

        if( mesh->hasSkeleton() )
        {
            // subMesh->_compileBoneAssignments();
        }

        /*
        const v1::SubMesh::VertexBoneAssignmentList& v1BoneAssignments =
        subMesh->getBoneAssignments(); mBoneAssignments.reserve(v1BoneAssignments.size());

        {
            v1::SubMesh::VertexBoneAssignmentList::const_iterator itor = v1BoneAssignments.begin();
            v1::SubMesh::VertexBoneAssignmentList::const_iterator end = v1BoneAssignments.end();

            while (itor != end)
            {
                mBoneAssignments.push_back(VertexBoneAssignment(itor->second));
                ++itor;
            }
        }
        */

        /*
        std::sort(mBoneAssignments.begin(), mBoneAssignments.end());
        mBlendIndexToBoneIndexMap = subMesh->blendIndexToBoneIndexMap;
        mBoneAssignmentsOutOfDate = false;
        */

        importBuffersFromV1( newMesh, newSubMesh, mesh, subMesh, halfPos, halfTexCoords, qTangents,
                             halfPose, 0 );

        // assert(subMesh->parent->hasValidShadowMappingBuffers());

        /*
        //Deal with shadow mapping optimized buffers
        if (subMesh->vertexData[VpNormal] != subMesh->vertexData[VpShadow] ||
            subMesh->indexData[VpNormal] != subMesh->indexData[VpShadow])
        {
            //Use the special version already built for v1
            importBuffersFromV1(subMesh, halfPos, halfTexCoords, qTangents, halfPose, 1);
        }
        else
        {
            //No special version in the v1 format, let the autogeneration routine decide.
            this->_prepareForShadowMapping(false);
        }
        */
    }

    void MeshLoader::importV1( IMesh *mesh, bool halfPos, bool halfTexCoords, bool qTangents,
                               bool halfPose )
    {
        using namespace Ogre;

        auto meshPtr = Ogre::MeshManager::getSingleton().createManual(
            "Barrel Imported", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        WP_LOG( "Mesh2::importV1" );

        mesh->load( nullptr );

        auto loadingState = meshPtr->getLoadingState();

        if( loadingState != Ogre::Resource::LoadingState::LOADSTATE_UNLOADED &&
            loadingState != Ogre::Resource::LoadingState::LOADSTATE_LOADING )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INVALID_STATE,
                         "To import a v1 mesh, the v2 mesh must be in unloaded state!",
                         "Mesh::importV1" );
        }

        if( mesh->getHasSharedVertexData() )
        {
            WP_LOG( "WARNING: Mesh '" + mesh->getName() +
                    "' has shared vertices. They're being "
                    "'unshared' for importing to v2" );
            MeshUtil::unshareVertices( mesh );
        }

        auto aabb = mesh->getAABB();
        auto minimum = aabb.getMinimum();
        auto maximum = aabb.getMaximum();

        auto ogreAabb = Ogre::AxisAlignedBox( Ogre::Vector3( minimum.X(), minimum.Y(), minimum.Z() ),
                                              Ogre::Vector3( maximum.X(), maximum.Y(), maximum.Z() ) );
        // auto ogreBoundingRadius = ogreAabb.getHalfSize().length();

        /*
        try
        {
            if( qTangents )
            {
                unsigned short sourceCoordSet;
                unsigned short index;
                bool alreadyHasTangents = mesh->suggestTangentVectorBuildParams( VES_TANGENT,
                        sourceCoordSet,
                        index );
                if( !alreadyHasTangents )
                    mesh->buildTangentVectors( VES_TANGENT, sourceCoordSet, index, false, false, true
        );
            }
        }
        catch( Exception & )
        {
        }*/

        auto subMeshes = mesh->getSubMeshes();
        for( auto pSubMesh : subMeshes )
        {
            auto subMesh = meshPtr->createSubMesh();
            importFromV1( meshPtr, subMesh, mesh, pSubMesh, halfPos, halfTexCoords, qTangents,
                          halfPose );
        }
        /*
                    mSubMeshNameMap = mesh->getSubMeshNameMap();

                    mSkeletonName = mesh->getSkeletonName();
                    v1::SkeletonPtr v1Skeleton = mesh->getOldSkeleton();
                    if( !v1Skeleton.isNull() )
                        mSkeleton = SkeletonManager::getSingleton().getSkeletonDef( v1Skeleton.get()
           );

                    //So far we only import manual LOD levels. If the mesh had manual LOD levels,
                    //mLodValues will have more entries than Vaos, causing an out of bounds
           exception.
                    //Don't use LOD if the imported mesh had manual levels.
                    //Note: Mesh2 supports LOD levels that have their own vertex and index buffers,
                    //so it should be possible to import them as well.
                    if( !mesh->hasManualLodLevel() )
                        mLodValues = *mesh->_getLodValueArray();
                    else
                        mLodValues = MovableObject::c_DefaultLodMesh;
        */

        meshPtr->setManuallyLoaded( true );
        meshPtr->setToLoaded();
    }

    void MeshLoader::loadMesh( const String &meshName )
    {
        /*
        String fileExt = Path::getFileExtension(meshName);
        if(fileExt==(".mesh"))
        {
            auto engine = core::IApplicationManager::instance();
            SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();

            //String fileName = Path::getFileName(meshName);
            //SmartPtr<IStream> stream = fileSystem->open(fileName);
            //if(stream)
            //{
            //	MeshGeometryPtr meshResource = m_meshMgr->create(fileName,
        IResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

            //	SmartPtr<IMesh> mesh(new CMesh);
            //	mesh->setName(meshName);
            //	meshResource->setMesh(mesh);

            //	MeshSerializer meshSerializer;
            //	meshSerializer.importMesh(stream, (CMesh*)mesh.get());

            //	mesh->updateAABB(true);
            //	meshResource->setAABB(mesh->getAABB());
            //}
        }
        else
        {
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile( meshName.c_str(),
        aiProcessPreset_TargetRealtime_Quality | aiProcess_TransformUVCoords | aiProcess_FlipUVs);

            String path = Path::getFilePath(meshName);
            loadDataFromNode(scene, scene->mRootNode, meshName.c_str());
        }
        */
    }

    auto MeshLoader::loadEngineMesh( const String &meshName ) -> SmartPtr<IMesh>
    {
        /*
        String fileExt = Path::getFileExtension(meshName);
        if(fileExt==(".mesh"))
        {
            auto engine = core::IApplicationManager::instance();
            SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();

            String fileName = Path::getFileName(meshName);
            SmartPtr<IStream> stream = fileSystem->open(fileName);
            if(stream)
            {

                SmartPtr<IMesh> mesh(new CMesh);

                MeshSerializer meshSerializer;
                meshSerializer.importMesh(stream, (CMesh*)mesh.get());

                return mesh;
            }
        }
        */

        return nullptr;
    }

    auto MeshLoader::load( const String &meshName, SmartPtr<IGraphicsSceneNode> fbParent,
                           SmartPtr<render::IGraphicsScene> smgr ) -> SmartPtr<IMesh>
    {
        /*
        String fileExt = Path::getFileExtension(meshName);
        if(fileExt==(".mesh"))
        {
            auto engine = core::IApplicationManager::instance();
            SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();

            String fileName = Path::getFileName(meshName);
            String path = Path::getFilePath(meshName);

            //SmartPtr<IStream> stream = fileSystem->open(fileName, path);
            //if(stream)
            //{
            //	MeshGeometryPtr meshResource = m_meshMgr->create(fileName,
        IResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

            //	SmartPtr<IMesh> mesh(new CMesh);
            //	meshResource->setMesh(mesh);

            //	MeshSerializer meshSerializer;
            //	meshSerializer.importMesh(stream, (CMesh*)mesh.get());
            //}
        }
        else
        {
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile( meshName.c_str(),
        aiProcessPreset_TargetRealtime_Quality | aiProcess_TransformUVCoords | aiProcess_FlipUVs);

            fb::String path = Path::getFilePath(meshName);
            loadDataFromNode(scene, scene->mRootNode, meshName.c_str());

            if(scene->mRootNode)
            {
                fb::SmartPtr<IGraphicsSceneNode> fbNode = fbParent->addChildSceneNode();
                loadNode(scene->mRootNode, fbNode, smgr);
            }
        }
        */

        return nullptr;
    }

    auto MeshLoader::getUseSingleMesh() const -> bool
    {
        return m_useSingleMesh;
    }

    void MeshLoader::setUseSingleMesh( bool useSingleMesh )
    {
        m_useSingleMesh = useSingleMesh;
    }

    auto MeshLoader::getQuietMode() const -> bool
    {
        return m_quietMode;
    }

    void MeshLoader::setQuietMode( bool quietMode )
    {
        m_quietMode = quietMode;
    }

    auto MeshLoader::getMeshMgr() const -> SmartPtr<IResourceManager>
    {
        return m_meshManager;
    }

    void MeshLoader::setMeshMgr( SmartPtr<IResourceManager> meshManager )
    {
        m_meshManager = meshManager;
    }

}  // namespace workphone::render
