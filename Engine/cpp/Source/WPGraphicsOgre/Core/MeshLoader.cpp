#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Core/MeshLoader.hpp>
#include <Workphone/Workphone.hpp>
#include "Ogre.h"
#include <OgreMeshSerializer.h>
#include <OgreSkeletonSerializer.h>
#include <OgreSubMesh.h>
#include <OgreMaterialManager.h>
#include <OgreMaterialSerializer.h>
#include <OgreLogManager.h>
#include <OgreSkeleton.h>
#include <OgreStringConverter.h>
#include <OgreDefaultHardwareBufferManager.h>

namespace workphone
{
    namespace render
    {

        MeshLoader::MeshLoader()
        {
        }

        MeshLoader::~MeshLoader()
        {
        }

        void MeshLoader::loadFBMesh( Ogre::MeshPtr meshPtr, SmartPtr<IMesh> mesh )
        {
            using namespace Ogre;

            mesh->load( nullptr );

            auto loadingState = meshPtr->getLoadingState();

            if( loadingState != Ogre::Resource::LoadingState::LOADSTATE_UNLOADED &&
                loadingState != Ogre::Resource::LoadingState::LOADSTATE_LOADING )
            {
                OGRE_EXCEPT( Ogre::Exception::ERR_INVALID_STATE,
                             "To import a v1 mesh, the v2 mesh must be in unloaded state!",
                             "Mesh::importV1" );
            }

            if( !mesh )
            {
                return;
            }

            static int meshIdx = 0;

            auto meshManager = Ogre::MeshManager::getSingletonPtr();
            WP_ASSERT( meshManager );

            auto logManager = Ogre::LogManager::getSingletonPtr();
            WP_ASSERT( logManager );

            auto hardwareBufferManager = Ogre::HardwareBufferManager::getSingletonPtr();
            WP_ASSERT( hardwareBufferManager );

            auto defaultHardwareBufferManager = Ogre::DefaultHardwareBufferManager::getSingletonPtr();
            WP_ASSERT( defaultHardwareBufferManager );

            // Ogre::String meshName = "OutMesh" + Ogre::StringConverter::toString(meshIdx++);
            auto newMeshName = mesh->getName();
            Ogre::String meshName = newMeshName.c_str();

            auto resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
            //auto createOrRetriveMesh =
            //    meshManager->createOrRetrieve( meshName, resourceGroupName, true );
            //if( !createOrRetriveMesh.second )
            //{
            //    logManager->logMessage( "maxExporter::doMesh resource already exists" );
            //    logManager->logMessage( "maxExporter::doMesh end" );
            //    return ;
            //}

            Ogre::MeshPtr pMesh = meshPtr;
            Ogre::VertexData *pData = NULL;

            Ogre::HardwareVertexBufferSharedPtr pBuf;
            Ogre::Real *pReal = NULL;
            Ogre::Vector3 currPos;
            Ogre::Real maxSquaredRadius;
            bool firstVert = true;

            Ogre::Vector3 aabbMin( 1e10, 1e10, 1e10 );
            Ogre::Vector3 aabbMax( -1e10, -1e10, -1e10 );

            auto subMeshList = mesh->getSubMeshes();
            u32 numSubMeshes = subMeshList.size();

            for( u32 subMeshIdx = 0; subMeshIdx < numSubMeshes; ++subMeshIdx )
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

                Ogre::String matName = fbSubMesh->getMaterialName().c_str();

                // create subMesh
                Ogre::SubMesh *ogreSubMesh = pMesh->createSubMesh( matName );
                ogreSubMesh->setMaterialName( matName );
                ogreSubMesh->useSharedVertices = false;
                ogreSubMesh->vertexData = new Ogre::VertexData();
                ogreSubMesh->indexData = new Ogre::IndexData();

                size_t ogreNumVertices = ogreSubMesh->vertexData->vertexCount = fbVertexCount;
                Ogre::VertexDeclaration *decl = ogreSubMesh->vertexData->vertexDeclaration;

                WP_ASSERT_TRUE( ogreSubMesh->vertexData->vertexCount == 0 );

                auto fbVertexDeclaration = fbVertexBuffer->getVertexDeclaration();
                const auto posElem =
                    fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                const auto normalElem =
                    fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                const auto texCoordElem0 = fbVertexDeclaration->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
                const auto texCoordElem1 = fbVertexDeclaration->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 1 );

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
                    decl->addElement( iBinding, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES,
                                      0 );
                    offset += sizeof( f32 ) * 2;
                }

                if( texCoordElem1 )
                {
                    decl->addElement( iBinding, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES,
                                      1 );
                    offset += sizeof( f32 ) * 2;
                }

                u32 ogreVertexSize = decl->getVertexSize( 0 );

                // Create new vertex buffer
                auto ogreVertexBuffer = hardwareBufferManager->createVertexBuffer(
                    ogreVertexSize, ogreNumVertices, Ogre::HardwareBuffer::HBU_STATIC );

                // Bind buffer
                Ogre::VertexBufferBinding *pBind = pBind = ogreSubMesh->vertexData->vertexBufferBinding;
                pBind->setBinding( 0, ogreVertexBuffer );

                Ogre::Real *ogreVertexDataPtr = static_cast<Ogre::Real *>(
                    ogreVertexBuffer->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );

                u32 fbVertexSize = fbVertexBuffer->getVertexDeclaration()->getSize();
                u8 *fbVertexDataPtr = (u8 *)fbVertexBuffer->getVertexData();
                f32 *fbElementData = 0;

                for( u32 vertIdx = 0; vertIdx < fbVertexCount;
                     ++vertIdx, fbVertexDataPtr += fbVertexSize )
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
                        aabbMin.x = ogrePosition.x;
                    if( ogrePosition.y < aabbMin.y )
                        aabbMin.y = ogrePosition.y;
                    if( ogrePosition.z < aabbMin.z )
                        aabbMin.z = ogrePosition.z;

                    if( ogrePosition.x > aabbMax.x )
                        aabbMax.x = ogrePosition.x;
                    if( ogrePosition.y > aabbMax.y )
                        aabbMax.y = ogrePosition.y;
                    if( ogrePosition.z > aabbMax.z )
                        aabbMax.z = ogrePosition.z;

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
                        texCoordElem0->getElementData( fbVertexDataPtr, &fbElementData );
                        *ogreVertexDataPtr++ = fbElementData[0];
                        *ogreVertexDataPtr++ = fbElementData[1];
                    }

                    if( texCoordElem1 )
                    {
                        texCoordElem1->getElementData( fbVertexDataPtr, &fbElementData );
                        *ogreVertexDataPtr++ = fbElementData[0];
                        *ogreVertexDataPtr++ = fbElementData[1];
                    }
                }

                ogreVertexBuffer->unlock();

                Ogre::IndexData *indexData = ogreSubMesh->indexData;
                indexData->indexCount = fbIndexCount;
                WP_ASSERT_TRUE( ogreSubMesh->indexData->indexCount == 0 );

                bool useWords = ( fbVertexCount >= std::numeric_limits<u16>::max() ) ? false : true;

                auto ogreIndexBuffer = defaultHardwareBufferManager->createIndexBuffer(
                    useWords ? Ogre::HardwareIndexBuffer::IT_16BIT : Ogre::HardwareIndexBuffer::IT_32BIT,
                    ogreSubMesh->indexData->indexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY );
                ogreSubMesh->indexData->indexBuffer = ogreIndexBuffer;

                u16 *pWords = useWords ? static_cast<u16 *>(
                                             ogreIndexBuffer->lock( Ogre::HardwareBuffer::HBL_DISCARD ) )
                                       : NULL;
                u32 *pDWords = useWords ? NULL
                                        : static_cast<u32 *>( ogreIndexBuffer->lock(
                                              Ogre::HardwareBuffer::HBL_DISCARD ) );

                if( fbIndexBuffer->getIndexType() == IIndexBuffer::Type::IT_32BIT )
                {
                    const u32 *fbIndexData = reinterpret_cast<u32 *>( fbIndexBuffer->getIndexData() );

                    // for (s32 i = (s32)fbIndexCount - 1; i >= 0; --i)
                    for( u32 i = 0; i < fbIndexCount; ++i )
                    {
                        auto index = fbIndexData[i];
                        if( useWords )
                            *pWords++ = index;
                        else
                            *pDWords++ = index;
                    }
                }
                else
                {
                    const u16 *fbIndexData = reinterpret_cast<u16 *>( fbIndexBuffer->getIndexData() );
                    // for (s32 i = (s32)fbIndexCount - 1; i >= 0; --i)
                    for( u32 i = 0; i < fbIndexCount; ++i )
                    {
                        auto index = fbIndexData[i];
                        if( useWords )
                            *pWords++ = index;
                        else
                            *pDWords++ = index;
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

            //Ogre::MeshSerializer serializer;
            //auto fileName = newMeshName + ".mesh";
            //serializer.exportMesh( pMesh.get(), fileName );

            //meshManager->unload( meshName );
        }

    }  // namespace render
}  // namespace workphone
