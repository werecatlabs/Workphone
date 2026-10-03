#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxCooker.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <Workphone/Workphone.hpp>
#include "PxPhysics.h"
#include "PxPhysicsAPI.h"
#include <unordered_map>
#include <list>
#include <fstream>

namespace workphone::physics
{
    PhysxCooker::PhysxCooker()
    {
    }

    PhysxCooker::~PhysxCooker() = default;

    auto PhysxCooker::hasPxMesh( String PxsFile ) -> bool
    {
        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();
        // return fileSystem->isExistingFile(PxsFile);

        return false;
    }

    auto PhysxCooker::loadPxTriangleMeshFromFile( String pxsFile ) -> PxTriangleMesh *
    {
        if( !hasPxMesh( pxsFile ) )
        {
            // throw exception
            return nullptr;
        }

        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();
        // SmartPtr<IStream> ds = fileSystem->open(pxsFile);
        //
        // SmartPtr<IPhysicsManager3> pPhysicsManager = engine->getPhysicsManager3();
        // SmartPtr<PhysxManager> physicsManager =
        // fb::static_pointer_cast<PhysxManager>(pPhysicsManager);
        return nullptr; // physxMgr->getPhysics()->createTriangleMesh(OgrePxStream(ds));
    }

    void PhysxCooker::getMeshInfo( SmartPtr<IMesh> mesh, Params params, MeshInfo &outInfo )
    {
        // First, we compute the total number of vertices and indices and init the buffers.
        unsigned int numVertices = 0;
        unsigned int numIndices = 0;

        String name = mesh->getName();

        // VertexData* vertexData = mesh->getSharedVertexData();
        // if (vertexData)
        //	numVertices += vertexData->vertexCount;

        bool indices32 = true;

        Array<SmartPtr<ISubMesh>> subMeshes = mesh->getSubMeshes();

        for( auto subMesh : subMeshes )
        {
            SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();

            if( vertexBuffer )
            {
                numVertices += vertexBuffer->getNumVertices();
            }

            SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();

            if( params.mAddBackfaces )
            {
                numIndices += indexBuffer->getNumIndices() * 2;
            }
            else
            {
                numIndices += indexBuffer->getNumIndices();
            }

            // We assume that every submesh uses the same index format
            indices32 = ( indexBuffer->getIndexType() == IndexBuffer::Type::IT_32BIT );
        }

        outInfo.vertices.resize( numVertices );
        outInfo.indices.resize( numIndices );
        outInfo.materials.resize( numIndices / 3 );

        unsigned int addedVertices = 0;
        unsigned int addedIndices = 0;
        unsigned int addedMaterialIndices = 0;

        /*
        Read shared vertices
        */
        // unsigned int shared_index_offset = 0;
        // Ogre::VertexData *shared_vertex_data = mesh->sharedVertexData;
        // if (shared_vertex_data)
        //{
        //	const Ogre::VertexElement* posElem =
        //		shared_vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
        //	Ogre::HardwareVertexBufferSharedPtr vbuf =
        //		shared_vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

        //	shared_index_offset = shared_vertex_data->vertexCount;

        //	unsigned char* pVertices = static_cast<unsigned
        // char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY)); 	Ogre::Real* pReal; 	for (size_t
        // i = addedVertices; i < shared_vertex_data->vertexCount; i++)
        //	{
        //		posElem->baseVertexPointerToElement(pVertices, &pReal);
        //		Vector3F vec;
        //		vec.x = (*pReal++) * params.mScale.x;
        //		vec.y = (*pReal++) * params.mScale.y;
        //		vec.z = (*pReal++) * params.mScale.z;
        //		outInfo.vertices[i] = vec;
        //		pVertices += vbuf->getVertexSize();
        //	}
        //	vbuf->unlock();
        //	addedVertices += shared_vertex_data->vertexCount;

        //}

        unsigned int index_offset = 0;

        /*
        Read submeshes
        */
        for( auto subMesh : subMeshes )
        {
            SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();

            // Read vertex data
            if( vertexBuffer )
            {
                const SmartPtr<IVertexElement> posElem =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_POSITION );

                auto pVertices = static_cast<unsigned char *>( vertexBuffer->getVertexData() );
                f32 *pReal;
                for( size_t i = addedVertices; i < addedVertices + vertexBuffer->getNumVertices(); i++ )
                {
                    posElem->getElementData( pVertices, &pReal );

                    Vector3F vec;
                    vec[0] = ( *pReal++ ) * params.mScale[0];
                    vec[1] = ( *pReal++ ) * params.mScale[1];
                    vec[2] = ( *pReal++ ) * params.mScale[2];
                    outInfo.vertices[i] = vec;

                    pVertices += vertexBuffer->getVertexDeclaration()->getSize();
                }

                addedVertices += vertexBuffer->getNumVertices();
            }

            // Read index data
            SmartPtr<IIndexBuffer> index_data = subMesh->getIndexBuffer();
            if( index_data )
            {
                PxU32 *pIndices = nullptr;
                if( indices32 )
                {
                    pIndices = static_cast<PxU32 *>( index_data->getIndexData() );
                }
                else
                {
                    auto pShortIndices = static_cast<PxU16 *>( index_data->getIndexData() );
                    pIndices = new PxU32[index_data->getNumIndices()];

                    for( size_t k = 0; k < index_data->getNumIndices(); k++ )
                    {
                        pIndices[k] = static_cast<PxU32>( pShortIndices[k] );
                    }
                }

                int shared_index_offset = 0;

                unsigned int bufferIndex = 0;
                if( params.mAddBackfaces )
                {
                    size_t numTris = index_data->getNumIndices() / 3;
                    size_t i = addedIndices;

                    for( unsigned int x = 0; x < numTris; x++ )
                    {
                        if( subMesh->getUseSharedVertices() )
                        {
                            if( pIndices[bufferIndex] > static_cast<u32>( shared_index_offset ) )
                            {
                                outInfo.indices[i] = pIndices[bufferIndex] + index_offset;
                            }
                            else
                            {
                                outInfo.indices[i] = pIndices[bufferIndex];
                            }

                            bufferIndex++;

                            if( pIndices[bufferIndex] > static_cast<u32>( shared_index_offset ) )
                            {
                                outInfo.indices[i + 1] = pIndices[bufferIndex] + index_offset;
                            }
                            else
                            {
                                outInfo.indices[i + 1] = pIndices[bufferIndex];
                            }

                            bufferIndex++;

                            if( pIndices[bufferIndex] > static_cast<u32>( shared_index_offset ) )
                            {
                                outInfo.indices[i + 2] = pIndices[bufferIndex] + index_offset;
                            }
                            else
                            {
                                outInfo.indices[i + 2] = pIndices[bufferIndex];
                            }

                            bufferIndex++;
                        }
                        else
                        {
                            outInfo.indices[i] = pIndices[bufferIndex] + index_offset;
                            bufferIndex++;
                            outInfo.indices[i + 1] = pIndices[bufferIndex] + index_offset;
                            bufferIndex++;
                            outInfo.indices[i + 2] = pIndices[bufferIndex] + index_offset;
                            bufferIndex++;
                        }

                        outInfo.indices[i + 3] = outInfo.indices[i + 2];
                        outInfo.indices[i + 4] = outInfo.indices[i + 1];
                        outInfo.indices[i + 5] = outInfo.indices[i];
                        i += 6;
                    }

                    addedIndices += index_data->getNumIndices() * 2;
                }
                else
                {
                    for( size_t i = addedIndices; i < addedIndices + index_data->getNumIndices(); i++ )
                    {
                        if( subMesh->getUseSharedVertices() )
                        {
                            if( pIndices[bufferIndex] > static_cast<u32>( shared_index_offset ) )
                            {
                                outInfo.indices[i] = pIndices[bufferIndex] + index_offset;
                            }
                            else
                            {
                                outInfo.indices[i] = pIndices[bufferIndex];
                            }
                        }
                        else
                        {
                            outInfo.indices[i] = pIndices[bufferIndex] + index_offset;
                        }

                        bufferIndex++;
                    }
                    addedIndices += index_data->getNumIndices();
                }

                if( !indices32 )
                {
                    delete[] pIndices;
                }

                // All triangles of a submesh have the same material
                unsigned int numTris = index_data->getNumIndices() / 3;

                if( params.mAddBackfaces )
                {
                    numTris *= 2;
                }

                for( size_t i = addedMaterialIndices; i < addedMaterialIndices + numTris; ++i )
                {
                    outInfo.materials[i] = subMesh->getMaterialName();
                }

                addedMaterialIndices += numTris;
            }

            if( vertexBuffer )
            {
                index_offset += vertexBuffer->getNumVertices();
            }
        }
    }

    struct OctreeNode
    {
        OctreeNode()
        {
            vPos.X() = 0.0f;
            vPos.Y() = 0.0f;
            vPos.Z() = 0.0f;
            aSubNodes[0] = nullptr;
            aSubNodes[1] = nullptr;
            aSubNodes[2] = nullptr;
            aSubNodes[3] = nullptr;
            aSubNodes[4] = nullptr;
            aSubNodes[5] = nullptr;
            aSubNodes[6] = nullptr;
            aSubNodes[7] = nullptr;
        }

        Vector3F       vPos;
        OctreeNode    *aSubNodes[8];
        std::list<int> liIndices;
    };

    struct STri
    {
        STri()
        {
            i1 = -1;
            i2 = -1;
            i3 = -1;
        }

        STri( int iIndex1, int iIndex2, int iIndex3, String material, bool bSort = true )
        {
            if( !bSort )
            {
                i1 = iIndex1;
                i2 = iIndex2;
                i3 = iIndex3;
                return;
            }
            // rotate indices
            if( iIndex2 < iIndex1 )
            {
                if( iIndex3 < iIndex2 )
                {
                    // index 3 is the smallest
                    i1 = iIndex3;
                    i2 = iIndex1;
                    i3 = iIndex2;
                }
                else
                {
                    i1 = iIndex2;
                    i2 = iIndex3;
                    i3 = iIndex1;
                }
            }
            else
            {
                i1 = iIndex1;
                i2 = iIndex2;
                i3 = iIndex3;
            }
            mat = material;
        }

        auto operator!=( STri &op ) -> bool
        {
            if( op.i1 != i1 || op.i2 != i2 || op.i3 != i3 )
            {
                return true;
            }
            return false;
        }

        auto operator<( STri &op ) -> bool
        {
            if( op.i1 != i1 )
            {
                return i1 < op.i1;
            }
            if( op.i2 != i2 )
            {
                return i2 < op.i2;
            }
            return i3 < op.i3;
        }

        int    i1, i2, i3;
        String mat;
    };

    // returns current vertex count
    auto ExtractOctree( OctreeNode *pNode, int iVertexCount, int *aiIndexTable, Vector3F *aNewVertices )
        -> int
    {
        for( int liIndice : pNode->liIndices )
        {
            aiIndexTable[liIndice] = iVertexCount;
        }
        aNewVertices[iVertexCount++] = pNode->vPos;
        for( auto &aSubNode : pNode->aSubNodes )
        {
            if( aSubNode )
            {
                iVertexCount = ExtractOctree( aSubNode, iVertexCount, aiIndexTable, aNewVertices );
                delete aSubNode;
                aSubNode = nullptr;
            }
        }
        return iVertexCount;
    }

#define IS_IN_BOX( v1, v2, d )                                                            \
    ( ( v1.X() <= v2.X() + d ) && ( v1.X() >= v2.X() - d ) && ( v1.Y() <= v2.Y() + d ) && \
      ( v1.Y() >= v2.Y() - d ) && ( v1.Z() <= v2.Z() + d ) && ( v1.Z() >= v2.Z() - d ) )

#define EIGHTH_SPACE_INDEX( v1, v2 )                                      \
    ( ( ( v1.X() > v2.X() ) ? 4 : 0 ) + ( ( v1.Y() > v2.Y() ) ? 2 : 0 ) + \
      ( ( v1.Z() > v2.Z() ) ? 1 : 0 ) )

    void PhysxCooker::mergeVertices( MeshInfo &meshInfo, float fMergeDist )
    {
        // const float fMergeDist=1e-3f;

        OctreeNode root;
        root.vPos = meshInfo.vertices[0];
        int iVertex = 0;
        int numAdded = 0;

        for( ; iVertex < static_cast<int>( meshInfo.vertices.size() ); iVertex++ )
        {
            OctreeNode *pCurrNode = &root;
            while( true )
            {
                if( IS_IN_BOX( meshInfo.vertices[iVertex], pCurrNode->vPos, fMergeDist ) )
                {
                    pCurrNode->liIndices.push_back( iVertex );
                    break;
                }
                // vertex is not in merge distance to this node
                int iSubNode = EIGHTH_SPACE_INDEX( pCurrNode->vPos, meshInfo.vertices[iVertex] );
                if( pCurrNode->aSubNodes[iSubNode] )
                {
                    // proceed deeper into the tree
                    pCurrNode = pCurrNode->aSubNodes[iSubNode];
                }
                else
                {
                    // there is no branch so make one
                    pCurrNode->aSubNodes[iSubNode] = new OctreeNode;
                    pCurrNode = pCurrNode->aSubNodes[iSubNode];
                    pCurrNode->vPos = meshInfo.vertices[iVertex];
                    numAdded++;
                }
                // pCurrNode is now one level lower in the tree
            }
        }
        auto aiIndexTable = new int[meshInfo.vertices.size()]; // maps old indices to new
        auto aNewVertices = new Vector3F[meshInfo.vertices.size()];
        // extract indextable and vertex list
        int nNewVertices = ExtractOctree( &root, 0, aiIndexTable, aNewVertices );
        for( int &indice : meshInfo.indices )
        {
            assert( meshInfo.indices[indice] < static_cast<int>( meshInfo.indices.size() ) );
            assert( meshInfo.indices[indice] >= 0 );
            indice = aiIndexTable[indice];
        }

        meshInfo.vertices.resize( nNewVertices );
        for( iVertex = 0; iVertex < nNewVertices; iVertex++ )
        {
            meshInfo.vertices[iVertex] = aNewVertices[iVertex];
        }

        delete[] aiIndexTable;
        delete[] aNewVertices;

        // search for duplicated and degenerate tris
        Array<STri> vTris;
        vTris.resize( meshInfo.indices.size() / 3 );
        int nTrisCopied = 0;
        int iTri = 0;
        for( ; iTri < static_cast<int>( meshInfo.indices.size() ) / 3; iTri++ )
        {
            // check if this tri is degenerate
            int index1 = meshInfo.indices[iTri * 3 + 0], index2 = meshInfo.indices[iTri * 3 + 1],
                index3 = meshInfo.indices[iTri * 3 + 2];
            if( index1 == index2 || index3 == index2 || index1 == index3 )
            {
                // degenerate tri: two or more vertices are the same
                continue;
            }
            vTris[nTrisCopied++] = STri( index1, index2, index3, meshInfo.materials[iTri] );
        }
        vTris.resize( nTrisCopied );
        // std::sort(vTris.begin(), vTris.end());//sort tris to find duplicates easily
        nTrisCopied = 0;
        STri lastTri;
        for( iTri = 0; iTri < static_cast<int>( vTris.size() ); iTri++ )
        {
            if( lastTri != vTris[iTri] )
            {
                meshInfo.indices[nTrisCopied * 3 + 0] = vTris[iTri].i1;
                meshInfo.indices[nTrisCopied * 3 + 1] = vTris[iTri].i2;
                meshInfo.indices[nTrisCopied * 3 + 2] = vTris[iTri].i3;
                meshInfo.materials[nTrisCopied] = vTris[iTri].mat;
                lastTri = vTris[iTri];
                nTrisCopied++;
            }
        }
        meshInfo.materials.resize( nTrisCopied );
        meshInfo.indices.resize( nTrisCopied * 3 );
    }

    // function to generate ccd mesh
    void PhysxCooker::insetMesh( MeshInfo &meshInfo, float fAmount )
    {
        // STri* tris= new STri[meshInfo.numTriangles];
        Array<STri> vTris;
        vTris.resize( meshInfo.indices.size() / 3 ); // create rotated tris
        int iTri = 0;
        for( ; iTri < static_cast<int>( meshInfo.indices.size() ) / 3; iTri++ )
        {
            vTris[iTri * 3] =
                STri( meshInfo.indices[iTri * 3], meshInfo.indices[iTri * 3 + 1],
                      meshInfo.indices[iTri * 3 + 2], nullptr, false ); // no mat indices this time!
            vTris[iTri * 3 + 1] = STri( meshInfo.indices[iTri * 3 + 2], meshInfo.indices[iTri * 3],
                                        meshInfo.indices[iTri * 3 + 1], nullptr, false );
            vTris[iTri * 3 + 2] = STri( meshInfo.indices[iTri * 3 + 1], meshInfo.indices[iTri * 3 + 2],
                                        meshInfo.indices[iTri * 3], nullptr, false );
        }
        // std::sort(vTris.begin(), vTris.end());
        int             iLastIndex = -1;
        int             nVertices = 0;
        Vector3F        vAccNomals;
        Vector3F        vPos;
        Array<Vector3F> vNewVertices;
        vNewVertices.resize( meshInfo.vertices.size() );
        for( iTri = 0; iTri < static_cast<int>( vTris.size() ); iTri++ )
        {
            if( vTris[iTri].i1 != iLastIndex )
            {
                if( iLastIndex != -1 )
                {
                    vAccNomals.normalise();
                    vNewVertices[iLastIndex] = meshInfo.vertices[iLastIndex] - fAmount * vAccNomals;
                }
                nVertices = 0;
                vAccNomals = Vector3F( 0, 0, 0 );
                iLastIndex = vTris[iTri].i1;
                vPos = meshInfo.vertices[iLastIndex];
            }
            Vector3F v = ( meshInfo.vertices[vTris[iTri].i2] - vPos )
                             .crossProduct( meshInfo.vertices[vTris[iTri].i3] - vPos );
            v.normalise();
            vAccNomals += v;
            nVertices++;
        }
        vAccNomals.normalise();
        vNewVertices[iLastIndex] = meshInfo.vertices[iLastIndex] + fAmount * vAccNomals;
        for( unsigned int iVertex = 0; iVertex < meshInfo.vertices.size(); iVertex++ )
        {
            meshInfo.vertices[iVertex] = vNewVertices[iVertex];
        }
    }

    void PhysxCooker::cookPxTriangleMesh( SmartPtr<IMesh> mesh, PxOutputStream &outputStream,
                                          Params params, AddedMaterials *out_addedMaterials )
    {
        MeshInfo meshInfo;
        getMeshInfo( mesh, params, meshInfo );
        mergeVertices( meshInfo );

        PxTriangleMeshDesc desc;
        desc.setToDefault();

        desc.points.count = static_cast<u32>( meshInfo.vertices.size() );
        desc.points.stride = 12;
        auto fVertices = new float[meshInfo.vertices.size() * 3];
        for( u32 i = 0; i < meshInfo.vertices.size(); ++i )
        {
            fVertices[i * 3] = meshInfo.vertices[i].X();
            fVertices[i * 3 + 1] = meshInfo.vertices[i].Y();
            fVertices[i * 3 + 2] = meshInfo.vertices[i].Z();
        }

        desc.points.data = fVertices;

        desc.triangles.count = static_cast<u32>( meshInfo.indices.size() / 3 );
        desc.triangles.stride = 12;
        auto iIndices = new PxU32[meshInfo.indices.size()];
        for( u32 i = 0; i < meshInfo.indices.size(); ++i )
        {
            iIndices[i] = meshInfo.indices[i];
        }

        desc.triangles.data = iIndices;

        std::unordered_map<PxMaterial *, PxMaterialTableIndex> materialIndicesMap;
        Array<PxMaterial *>                                    orderedMaterials;

        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IPhysicsManager3> pPhysicsManager = engine->getPhysicsManager3();
        // SmartPtr<PhysxManager> physicsManager =
        // fb::static_pointer_cast<PhysxManager>(pPhysicsManager);
        // materialIndicesMap.insert(std::make_pair<PxMaterial*,
        // PxMaterialTableIndex>(physicsManager->getDefaultMaterial(), 0));	//add default material at
        // index 0 orderedMaterials.push_back(physicsManager->getDefaultMaterial());

        PxMaterialTableIndex *materialIndices = nullptr;
        if( !params.mMaterialBindings.empty() )
        {
            desc.materialIndices.stride = sizeof( PxMaterialTableIndex );
            materialIndices = new PxMaterialTableIndex[meshInfo.indices.size() / 3];
            for( unsigned int i = 0; i < meshInfo.indices.size() / 3; ++i )
            {
                auto mat = params.mMaterialBindings.find( meshInfo.materials[i] );
                if( mat == params.mMaterialBindings.end() )
                {
                    materialIndices[i] = 0; // default material
                }
                else
                {
                    auto                 matIndex = materialIndicesMap.find( mat->second );
                    PxMaterialTableIndex index;
                    if( matIndex == materialIndicesMap.end() )
                    {
                        // create material index if not existing
                        index = static_cast<u32>( materialIndicesMap.size() );
                        // materialIndicesMap.insert(std::make_pair<PxMaterial*,
                        // PxMaterialTableIndex>(mat->second, index));
                        orderedMaterials.push_back( mat->second );
                    }
                    else
                    {
                        index = matIndex->second;
                    }
                    materialIndices[i] = index;
                }
            }
        }
        desc.materialIndices.data = materialIndices;

        if( out_addedMaterials )
        {
            out_addedMaterials->materialCount = static_cast<u32>( materialIndicesMap.size() );
            out_addedMaterials->materials = new PxMaterial *[out_addedMaterials->materialCount];
            for( unsigned int i = 0; i < orderedMaterials.size(); ++i )
            {
                out_addedMaterials->materials[i] = orderedMaterials[i];
            }
        }

        // physx::PxCooking* cooking = physicsManager->getCooking();
        // cooking->cookTriangleMesh(desc, outputStream);

        // delete[] fVertices;
        // delete[] iIndices;
        // if (materialIndices) delete[] materialIndices;
    }

    void PhysxCooker::cookPxConvexMesh( SmartPtr<IMesh> mesh, PxOutputStream &outputStream,
                                        Params params )
    {
        MeshInfo meshInfo;
        getMeshInfo( mesh, params, meshInfo );
        mergeVertices( meshInfo, 0.1f );

        PxConvexMeshDesc desc;
        desc.points.count = static_cast<u32>( meshInfo.vertices.size() );
        desc.points.stride = 12;
        desc.flags |= PxConvexFlag::eCOMPUTE_CONVEX;

        auto fVertices = new float[meshInfo.vertices.size() * 3];
        for( unsigned int i = 0; i < meshInfo.vertices.size(); ++i )
        {
            Vector3F vertex = meshInfo.vertices[i];

            fVertices[i * 3] = vertex.X();
            fVertices[i * 3 + 1] = vertex.Y();
            fVertices[i * 3 + 2] = vertex.Z();
        }
        desc.points.data = fVertices;

        desc.triangles.count = static_cast<u32>( meshInfo.indices.size() / 3 );
        desc.triangles.stride = 12;
        auto iIndices = new int[meshInfo.indices.size()];
        for( unsigned int i = 0; i < meshInfo.indices.size(); ++i )
        {
            iIndices[i] = meshInfo.indices[i];
        }
        desc.triangles.data = iIndices;

        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IPhysicsManager3> pPhysicsManager = engine->getPhysicsManager3();
        // SmartPtr<PhysxManager> physicsManager =
        // fb::static_pointer_cast<PhysxManager>(pPhysicsManager); physx::PxCooking* cooking =
        // physicsManager->getCooking(); cooking->cookConvexMesh(desc, outputStream);

        delete[] fVertices;
        delete[] iIndices;
    }

    void PhysxCooker::cookPxTriangleMeshToFile( SmartPtr<IMesh> mesh, String pxsOutputFile,
                                                Params params, AddedMaterials *out_addedMaterials )
    {
        std::fstream f;
        f.open( pxsOutputFile.c_str() );

        // MemoryOutputStream outputStream;
        // cookPxTriangleMesh(mesh, OgrePxStream(Ogre::DataStreamPtr(&Ogre::FileStreamDataStream(&f,
        // false))), params);
        f.close();
    }

    auto PhysxCooker::createPxTriangleMesh( SmartPtr<IMesh> mesh, Params params,
                                            AddedMaterials *out_addedMaterial ) -> PxTriangleMesh *
    {
        // MemoryOutputStream outputStream;
        // cookPxTriangleMesh(mesh, outputStream, params, out_addedMaterial);

        // if (outputStream.getData() == nullptr)
        //	return nullptr;

        // MemoryInputData stream(outputStream.getData(), outputStream.getSize());

        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IPhysicsManager3> pPhysicsManager = engine->getPhysicsManager3();
        // SmartPtr<PhysxManager> physicsManager =
        // fb::static_pointer_cast<PhysxManager>(pPhysicsManager); return
        // physicsManager->getPhysics()->createTriangleMesh(stream);

        return nullptr;
    }

    auto PhysxCooker::createPxConvexMesh( SmartPtr<IMesh> mesh, Params params ) -> PxConvexMesh *
    {
        // MemoryOutputStream outputStream;
        // cookPxConvexMesh(mesh, outputStream, params);
        // if (outputStream.getData() == nullptr) return nullptr;

        // MemoryInputData stream(outputStream.getData(), outputStream.getSize());

        // auto engine = core::ApplicationManager::instance();
        // SmartPtr<IPhysicsManager3> pPhysicsManager = engine->getPhysicsManager3();
        // SmartPtr<PhysxManager> physicsManager =
        // fb::static_pointer_cast<PhysxManager>(pPhysicsManager); return
        // physicsManager->getPhysics()->createConvexMesh(stream);

        return nullptr;
    }

    auto PhysxCooker::getSingleton() -> PhysxCooker &
    {
        static PhysxCooker instance;
        return instance;
    }

    PhysxCooker::AddedMaterials::AddedMaterials() : materials( nullptr )
    {
    }

    PhysxCooker::AddedMaterials::~AddedMaterials()
    {
        if( materials )
        {
            delete[] materials;
        }
    }

    PhysxCooker::Params::Params() : mScale( Vector3F( 1, 1, 1 ) ), mAddBackfaces( false )
    {
    }

    PhysxCooker::Params::~Params() = default;

    auto PhysxCooker::Params::scale( const Vector3F &scale ) -> PhysxCooker::Params &
    {
        mScale = scale;
        return *this;
    }

    auto PhysxCooker::Params::materials( std::map<String, PxMaterial *> &bindings )
        -> PhysxCooker::Params &
    {
        mMaterialBindings = bindings;
        return *this;
    }

    auto PhysxCooker::Params::backfaces( bool addBackfaces ) -> PhysxCooker::Params &
    {
        mAddBackfaces = addBackfaces;
        return *this;
    }
} // namespace workphone::physics
