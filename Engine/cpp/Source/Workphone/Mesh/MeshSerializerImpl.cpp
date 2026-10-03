#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshSerializerImpl.hpp>
#include <Workphone/Mesh/MeshFileFormat.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshPose.hpp>
#include <Workphone/Mesh/MeshSerializerListener.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Animation/IAnimationInterface.hpp>
#include <Workphone/Interface/Animation/IAnimationVertexTrack.hpp>
#include <Workphone/Interface/Animation/IAnimationPoseKeyFrame.hpp>
#include <Workphone/Interface/Animation/IAnimationMorphKeyFrame.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Mesh/IMeshPose.hpp>

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
// Disable conversion warnings, we do a lot of them, intentionally
#    pragma warning( disable : 4267 )
#endif

namespace workphone
{
    /// stream overhead = ID + size
    const long MSTREAM_OVERHEAD_SIZE = sizeof( u16 ) + sizeof( u32 );

    MeshSerializerImpl::MeshSerializerImpl()
    {
        // Version number
        m_version = "[MeshSerializer_v1.8]";
    }

    MeshSerializerImpl::~MeshSerializerImpl() = default;

    void MeshSerializerImpl::exportMesh( const Mesh *pMesh, SmartPtr<IStream> stream, Endian endianMode )
    {
        WP_LOG( String( "MeshSerializer writing mesh data to stream " ) + stream->getFileName() +
                String( "..." ) );

        // Decide on endian mode
        determineEndianness( endianMode );

        // Check that the mesh has it's bounds set
        auto meshAABB = pMesh->getAABB();
        if( meshAABB.isNull() )
        {
            WP_LOG_ERROR(
                "The Mesh you have supplied does not have its"
                " bounds completely defined. Define them first before exporting." );
        }
        m_stream = stream;

        if( !stream->isWriteable() )
        {
            WP_LOG_ERROR( "Unable to use stream " + stream->getFileName() + " for writing" );
        }

        writeFileHeader();
        WP_LOG( "File header written." );

        WP_LOG( "Writing mesh data..." );
        writeMesh( pMesh );
        WP_LOG( "Mesh data exported." );

        WP_LOG( "MeshSerializer export successful." );
    }

    void MeshSerializerImpl::importMesh( SmartPtr<IStream> &stream, Mesh *pMesh,
                                         MeshSerializerListener *listener )
    {
        // Determine endianness (must be the first thing we do!)
        determineEndianness( stream );
        stream->seek( 0 );

        // Check header
        readFileHeader( stream );

        auto retries = 0;
        auto readStream = true;
        unsigned short streamID;
        while( readStream && !stream->eof() )
        {
            streamID = readChunk( stream );
            switch( streamID )
            {
            case M_MESH:
            {
                readMesh( stream, pMesh, listener );
            }
            break;
            default:
            {
                if( retries > 1000 )
                {
                    readStream = false;
                }
            }
            }

            retries++;
        }
    }

    void MeshSerializerImpl::writeMesh( const Mesh *pMesh )
    {
        // Header
        writeChunkHeader( M_MESH, calcMeshSize( pMesh ) );

        // bool skeletallyAnimated
        bool skelAnim = pMesh->hasSkeleton();
        writeBools( &skelAnim, 1 );

        // Write shared geometry
        if( pMesh->getHasSharedVertexData() )
        {
            auto sharedVertexData = pMesh->getSharedVertexBuffer();
            if( sharedVertexData )
            {
                writeGeometry( static_cast<const VertexBuffer *>( sharedVertexData.get() ) );
            }
        }

        // Write Submeshes
        auto subMeshes = pMesh->getSubMeshes();
        for( const auto &subMeshe : subMeshes )
        {
            WP_LOG( "Writing submesh..." );

            auto pSubMesh = subMeshe.get();
            writeSubMesh( static_cast<SubMesh *>( pSubMesh ) );

            WP_LOG( "Submesh exported." );
        }

        // Write skeleton info if required
        if( pMesh->hasSkeleton() )
        {
            WP_LOG( "Exporting skeleton link..." );
            // Write skeleton link
            writeSkeletonLink( pMesh->getSkeletonName() );
            WP_LOG( "Skeleton link exported." );

            // Write bone assignments - UNCOMMENTED AND UPDATED
            auto meshBoneAssignments = pMesh->getBoneAssignments();
            if( !meshBoneAssignments.empty() )
            {
                WP_LOG( "Exporting shared geometry bone assignments..." );

                for( const auto &assignment : meshBoneAssignments )
                {
                    VertexBoneAssignment assignmentData;
                    assignmentData.m_vertexIndex = assignment->getVertexIndex();
                    assignmentData.m_boneIndex = assignment->getBoneIndex();
                    assignmentData.m_weight = assignment->getWeight();

                    writeMeshBoneAssignment( assignmentData );
                }

                WP_LOG( "Shared geometry bone assignments exported." );
            }
        }

        // Write LOD data if any
        if( pMesh->getNumLodLevels() > 1 )
        {
            WP_LOG( "Exporting LOD information...." );
            writeLodInfo( pMesh );
            WP_LOG( "LOD information exported." );
        }

        // Write bounds information
        WP_LOG( "Exporting bounds information...." );
        writeBoundsInfo( pMesh );
        WP_LOG( "Bounds information exported." );

        // Write submesh name table
        WP_LOG( "Exporting submesh name table..." );
        writeSubMeshNameTable( pMesh );
        WP_LOG( "Submesh name table exported." );

        // Write edge lists
        if( pMesh->isEdgeListBuilt() )
        {
            WP_LOG( "Exporting edge lists..." );
            writeEdgeList( pMesh );
            WP_LOG( "Edge lists exported" );
        }

        // Write morph animation
        writePoses( pMesh );

        if( pMesh->hasVertexAnimation() )
        {
            writeAnimations( pMesh );
        }

        // Write submesh extremes
        writeExtremes( pMesh );
    }

    // Added by DrEvil
    void MeshSerializerImpl::writeSubMeshNameTable( const Mesh *pMesh )
    {
        //// Header
        // writeChunkHeader(M_SUBMESH_NAME_TABLE, calcSubMeshNameTableSize(pMesh));

        // typedef HashMap<String, ushort> SubMeshNameMap ;
        //// Loop through and save out the index and names.
        // SubMeshNameMap::const_iterator it = pMesh->mSubMeshNameMap.begin();

        // while(it != pMesh->mSubMeshNameMap.end())
        //{
        //	// Header
        //	writeChunkHeader(M_SUBMESH_NAME_TABLE_ELEMENT, MSTREAM_OVERHEAD_SIZE +
        //		sizeof(unsigned short) + (unsigned long)it->first.length() + 1);

        //	// write the index
        //	writeShorts(&it->second, 1);
        //	// name
        //       writeString(it->first);

        //	++it;
        //}
    }

    void MeshSerializerImpl::writeSubMesh( const SubMesh *s )
    {
        // Header
        writeChunkHeader( M_SUBMESH, calcSubMeshSize( s ) );

        // char* materialName
        writeString( s->getMaterialName() );

        // bool useSharedVertices
        bool useSharedVertices = s->getUseSharedVertices();
        writeBools( &useSharedVertices, 1 );

        auto indexBuffer = s->getIndexBuffer();

        unsigned int indexCount = indexBuffer->getNumIndices();
        writeInts( &indexCount, 1 );

        // bool indexes32Bit
        bool idx32bit = ( indexBuffer && indexBuffer->getIndexType() == IIndexBuffer::Type::IT_32BIT );
        writeBools( &idx32bit, 1 );

        if( indexCount > 0 )
        {
            // unsigned short* faceVertexIndices ((indexCount)
            void *pIdx = indexBuffer->getIndexData();
            auto numIndices = indexBuffer->getNumIndices();

            if( idx32bit )
            {
                auto pIdx32 = static_cast<unsigned int *>( pIdx );
                writeInts( pIdx32, numIndices );
            }
            else
            {
                auto pIdx16 = static_cast<unsigned short *>( pIdx );
                writeShorts( pIdx16, numIndices );
            }
        }

        // M_GEOMETRY stream (Optional: present only if useSharedVertices = false)
        if( !useSharedVertices )
        {
            auto vertexData = s->getVertexBuffer();
            auto pVertexBuffer = static_cast<VertexBuffer *>( vertexData.get() );
            writeGeometry( pVertexBuffer );
        }

        // end of sub mesh chunk

        // write out texture alias chunks
        writeSubMeshTextureAliases( s );

        // Operation type
        writeSubMeshOperation( s );

        // Bone assignments - NOW UNCOMMENTED AND UPDATED
        auto boneAssignments = s->getBoneAssignments();
        if( !boneAssignments.empty() )
        {
            WP_LOG( "Exporting dedicated geometry bone assignments..." );

            for( auto assignment : boneAssignments )
            {
                auto p = (VertexBoneAssignment *)assignment.get();
                writeSubMeshBoneAssignment( *p );
            }

            WP_LOG( "Dedicated geometry bone assignments exported." );
        }
    }

    void MeshSerializerImpl::writeExtremes( const Mesh *pMesh )
    {
        bool has_extremes = false;

        auto subMeshes = pMesh->getSubMeshes();
        for( auto sm : subMeshes )
        {
            // if (sm->extremityPoints.empty())
            //	continue;

            // if (!has_extremes)
            //{
            //	has_extremes = true;
            //	WP_LOG("Writing submesh extremes...");
            // }
            // writeSubMeshExtremes(i, sm);
        }

        if( has_extremes )
        {
            WP_LOG( "Extremes exported." );
        }
    }

    void MeshSerializerImpl::writeSubMeshExtremes( unsigned short idx, const SubMesh *s )
    {
        //      u32 chunkSize = MSTREAM_OVERHEAD_SIZE + sizeof (unsigned short) +
        //          s->extremityPoints.size () * sizeof (float) * 3;
        //      writeChunkHeader(M_TABLE_EXTREMES, chunkSize);

        //      writeShorts(&idx, 1);

        //      float *vertices = OGRE_ALLOC_T(float, s->extremityPoints.size() * 3,
        //      MEMCATEGORY_GEOMETRY);
        // float *pVert = vertices;

        //      for (vector<Vector3>::type::const_iterator i = s->extremityPoints.begin();
        //           i != s->extremityPoints.end(); ++i)
        //      {
        //	*pVert++ = i->x;
        //	*pVert++ = i->y;
        //	*pVert++ = i->z;
        //      }

        //      writeFloats(vertices, s->extremityPoints.size () * 3);
        //      OGRE_FREE(vertices, MEMCATEGORY_GEOMETRY);
    }

    void MeshSerializerImpl::writeSubMeshTextureAliases( const SubMesh *s )
    {
        //      u32 chunkSize;
        //      AliasTextureNamePairList::const_iterator i;

        // WP_LOG("Exporting submesh texture aliases...");

        //      // iterate through texture aliases and write them out as a chunk
        //      for (i = s->mTextureAliases.begin(); i != s->mTextureAliases.end(); ++i)
        //      {
        //          // calculate chunk size based on string length + 1.  Add 1 for the line feed.
        //          chunkSize = MSTREAM_OVERHEAD_SIZE + i->first.length() + i->second.length() + 2;
        //	writeChunkHeader(M_SUBMESH_TEXTURE_ALIAS, chunkSize);
        //          // write out alias name
        //          writeString(i->first);
        //          // write out texture name
        //          writeString(i->second);
        //      }

        // WP_LOG("Submesh texture aliases exported.");
    }

    void MeshSerializerImpl::writeSubMeshOperation( const SubMesh *sm )
    {
        //// Header
        writeChunkHeader( M_SUBMESH_OPERATION, calcSubMeshOperationSize( sm ) );

        //// unsigned short operationType
        auto opType = static_cast<u16>( 0 );
        writeShorts( &opType, 1 );
    }

    auto MeshSerializerImpl::calcGeometrySize( const VertexBuffer *vertexData ) -> u32
    {
        auto vertexDeclaration = vertexData->getVertexDeclaration();
        // calc size
        const auto &elemList = vertexDeclaration->getElements();
        auto dataArray = vertexData->getDataArray();

        /*
        size_t size = MSTREAM_OVERHEAD_SIZE;

        // Vertex count
        size += sizeof(u32);

        // Vertex declaration
        size += MSTREAM_OVERHEAD_SIZE + elemList.size() * (MSTREAM_OVERHEAD_SIZE + sizeof(u16) * 5);

        // Buffers and bindings
        size += dataArray.size() * ((MSTREAM_OVERHEAD_SIZE * 2) + (sizeof(u16) * 2));
        */

        auto size =
            MSTREAM_OVERHEAD_SIZE + sizeof( unsigned int ) +  // base
            ( MSTREAM_OVERHEAD_SIZE +
              elemList.size() * ( MSTREAM_OVERHEAD_SIZE + sizeof( unsigned short ) * 5 ) );  // elements

        auto numVerticies = vertexData->getNumVertices();

        // Buffer data
        for( u32 i = 0; i < dataArray.size(); ++i )
        {
            auto vertexSize = vertexDeclaration->getSize( i );
            auto vertexBufferSize = vertexSize * numVerticies;
            size += vertexBufferSize;  // vbuf->getSizeInBytes() is too large for meshes prepared for
            // shadow volumes
        }

        return static_cast<u32>( size );
    }

    void MeshSerializerImpl::writeGeometry( const VertexBuffer *vertexData )
    {
        auto vertexDeclaration = vertexData->getVertexDeclaration();
        // calc size
        const auto &elemList = vertexDeclaration->getElements();

        auto geomSize = calcGeometrySize( vertexData );
        writeChunkHeader( M_GEOMETRY, geomSize );

        auto vertexCount = vertexData->getNumVertices();
        writeInts( &vertexCount, 1 );

        // Vertex declaration
        auto size = static_cast<u32>( 0 );
        size = MSTREAM_OVERHEAD_SIZE +
               elemList.size() * ( MSTREAM_OVERHEAD_SIZE + sizeof( unsigned short ) * 5 );
        writeChunkHeader( M_GEOMETRY_VERTEX_DECLARATION, size );

        unsigned short tmp = 0;
        size = MSTREAM_OVERHEAD_SIZE + sizeof( unsigned short ) * 5;

        for( auto pElem : elemList )
        {
            const auto &elem = *static_cast<VertexElement *>( pElem.get() );

            writeChunkHeader( M_GEOMETRY_VERTEX_ELEMENT, size );

            // unsigned short source;  	// buffer bind source
            tmp = elem.getSource();
            writeShorts( &tmp, 1 );

            // unsigned short type;    	// VertexElementType
            tmp = static_cast<unsigned short>( elem.getType() );
            writeShorts( &tmp, 1 );

            // unsigned short semantic; // VertexElementSemantic
            tmp = static_cast<unsigned short>( elem.getSemantic() );
            writeShorts( &tmp, 1 );

            // unsigned short offset;	// start offset in buffer in bytes
            tmp = static_cast<unsigned short>( elem.getOffset() );
            writeShorts( &tmp, 1 );

            // unsigned short index;	// index of the semantic (for colours and texture coords)
            tmp = elem.getIndex();
            writeShorts( &tmp, 1 );
        }

        auto bindIndex = static_cast<u16>( 0 );
        auto vertexDataArray = vertexData->getDataArray();
        for( auto data : vertexDataArray )
        {
            auto vbufSizeInBytes = vertexDeclaration->getSize() * vertexData->getNumVertices();
            size = ( MSTREAM_OVERHEAD_SIZE * 2 ) + ( sizeof( u16 ) * 2 ) + vbufSizeInBytes;

            auto chunkId = static_cast<u16>( M_GEOMETRY_VERTEX_BUFFER );
            writeChunkHeader( chunkId, size );

            writeShorts( &bindIndex, 1 );

            tmp = static_cast<u16>( vertexDeclaration->getSize() );
            writeShorts( &tmp, 1 );

            // Data
            size = MSTREAM_OVERHEAD_SIZE + vbufSizeInBytes;

            chunkId = static_cast<u16>( M_GEOMETRY_VERTEX_BUFFER_DATA );
            writeChunkHeader( chunkId, size );

            if( m_flipEndian )
            {
                // endian conversion
                // Copy data
                auto tempData = new u8[vbufSizeInBytes];
                memcpy( tempData, data, vbufSizeInBytes );

                flipToLittleEndian( tempData, vertexData->getNumVertices(), vertexDeclaration->getSize(),
                                    vertexDeclaration->findElementsBySource( bindIndex ) );

                writeData( tempData, vertexDeclaration->getSize(), vertexData->getNumVertices() );
                delete[] tempData;
            }
            else
            {
                writeData( data, vertexDeclaration->getSize(), vertexData->getNumVertices() );
            }

            bindIndex++;
        }
    }

    auto MeshSerializerImpl::calcSubMeshNameTableSize( const Mesh *pMesh ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // Figure out the size of the Name table.
        // Iterate through the subMeshList & add up the size of the indexes and names.
        // Mesh::SubMeshNameMap::const_iterator it = pMesh->mSubMeshNameMap.begin();
        // while(it != pMesh->mSubMeshNameMap.end())
        //{
        //	// size of the index + header size for each element chunk
        //	size += MSTREAM_OVERHEAD_SIZE + sizeof(u16);
        //	// name
        //	size += it->first.length() + 1;

        //	++it;
        //}

        // size of the sub-mesh name table.
        return size;
    }

    auto MeshSerializerImpl::calcMeshSize( const Mesh *pMesh ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // Num shared vertices
        //      size += sizeof(u32);

        //      // Geometry
        //      if (pMesh->sharedVertexData && pMesh->sharedVertexData->vertexCount > 0)
        //      {
        //          size += calcGeometrySize(pMesh->sharedVertexData);
        //      }

        //      // Submeshes
        //      for (unsigned short i = 0; i < pMesh->getNumSubMeshes(); ++i)
        //      {
        //          size += calcSubMeshSize(pMesh->getSubMesh(i));
        //      }

        //      // Skeleton link
        //      if (pMesh->hasSkeleton())
        //      {
        //          size += calcSkeletonLinkSize(pMesh->getSkeletonName());
        //      }

        //// Submesh name table
        // size += calcSubMeshNameTableSize(pMesh);

        //// Edge list
        // if (pMesh->isEdgeListBuilt())
        //{
        //	size += calcEdgeListSize(pMesh);
        // }

        //// Animations
        // for (unsigned short a = 0; a < pMesh->getNumAnimations(); ++a)
        //{
        //	Animation* anim = pMesh->getAnimation(a);
        //	size += calcAnimationSize(anim);
        // }

        return size;
    }

    auto MeshSerializerImpl::calcSubMeshSize( const SubMesh *pSub ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // bool idx32bit = (!pSub->indexData->indexBuffer.isNull() &&
        //				 pSub->indexData->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT);

        //      // Material name
        //      size += pSub->getMaterialName().length() + 1;

        //      // bool useSharedVertices
        //      size += sizeof(bool);
        //      // unsigned int indexCount
        //      size += sizeof(unsigned int);
        //      // bool indexes32bit
        //      size += sizeof(bool);
        //      // unsigned int* / unsigned short* faceVertexIndices
        // if (idx32bit)
        //	size += sizeof(unsigned int) * pSub->indexData->indexCount;
        // else
        //	size += sizeof(unsigned short) * pSub->indexData->indexCount;
        //      // Geometry
        //      if (!pSub->useSharedVertices)
        //      {
        //          size += calcGeometrySize(pSub->vertexData);
        //      }

        //      size += calcSubMeshTextureAliasesSize(pSub);
        //      size += calcSubMeshOperationSize(pSub);

        //      // Bone assignments
        //      if (!pSub->mBoneAssignments.empty())
        //      {
        //          SubMesh::VertexBoneAssignmentList::const_iterator vi;
        //          for (vi = pSub->mBoneAssignments.begin();
        //               vi != pSub->mBoneAssignments.end(); ++vi)
        //          {
        //              size += calcBoneAssignmentSize();
        //          }
        //      }

        return size;
    }

    auto MeshSerializerImpl::calcSubMeshOperationSize( const SubMesh *pSub ) -> u32
    {
        return MSTREAM_OVERHEAD_SIZE + sizeof( u16 );
    }

    auto MeshSerializerImpl::calcSubMeshTextureAliasesSize( const SubMesh *pSub ) -> u32
    {
        u32 chunkSize = 0;
        // AliasTextureNamePairList::const_iterator i;

        //// iterate through texture alias map and calc size of strings
        // for (i = pSub->mTextureAliases.begin(); i != pSub->mTextureAliases.end(); ++i)
        //{
        //     // calculate chunk size based on string length + 1.  Add 1 for the line feed.
        //     chunkSize += MSTREAM_OVERHEAD_SIZE + i->first.length() + i->second.length() + 2;
        // }

        return chunkSize;
    }

    void MeshSerializerImpl::readGeometry( SmartPtr<IStream> &stream, Mesh *pMesh, VertexBuffer *dest )
    {
        // dest->vertexStart = 0;

        unsigned int vertexCount = 0;
        readInts( stream, &vertexCount, 1 );
        dest->setNumVertices( vertexCount );

        // Find optional geometry streams
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() && ( streamID == M_GEOMETRY_VERTEX_DECLARATION ||
                                       streamID == M_GEOMETRY_VERTEX_BUFFER ) )
            {
                switch( streamID )
                {
                case M_GEOMETRY_VERTEX_DECLARATION:
                    readGeometryVertexDeclaration( stream, pMesh, dest );
                    break;
                case M_GEOMETRY_VERTEX_BUFFER:
                    readGeometryVertexBuffer( stream, pMesh, dest );
                    break;
                }

                // Get next stream
                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }

            if( !stream->eof() )
            {
                // Backpedal back to start of non-submesh stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }

        //// Perform any necessary colour conversion for an active rendersystem
        // if (Root::getSingletonPtr() && Root::getSingleton().getRenderSystem())
        //{
        //	// We don't know the source type if it's VertexDeclaration::VET_COLOUR, but assume ARGB
        //	// since that's the most common. Won't get used unless the mesh is
        //	// ambiguous anyway, which will have been warned about in the log
        //	dest->convertPackedColour(VET_COLOUR_ARGB,
        //		VertexElement::getBestColourVertexElementType());
        // }
    }

    void MeshSerializerImpl::readGeometryVertexDeclaration( SmartPtr<IStream> &stream, Mesh *pMesh,
                                                            VertexBuffer *dest )
    {
        auto vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        dest->setVertexDeclaration( vertexDeclaration );

        // Find optional geometry streams
        if( !stream->eof() )
        {
            auto streamID = readChunk( stream );
            while( !stream->eof() && ( streamID == M_GEOMETRY_VERTEX_ELEMENT ) )
            {
                switch( streamID )
                {
                case M_GEOMETRY_VERTEX_ELEMENT:
                {
                    readGeometryVertexElement( stream, pMesh, dest );
                }
                break;
                case M_GEOMETRY_VERTEX_BUFFER:
                {
                    int stop = 0;
                    stop = 0;
                }
                break;
                }

                // Get next stream
                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }

            if( !stream->eof() )
            {
                // Backpedal back to start of non-submesh stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readGeometryVertexElement( SmartPtr<IStream> &stream, Mesh *pMesh,
                                                        VertexBuffer *dest )
    {
        unsigned short source, offset, index, tmp;
        int vType;
        int vSemantic;
        // unsigned short source;  	// buffer bind source
        readShorts( stream, &source, 1 );
        // unsigned short type;    	// VertexElementType
        readShorts( stream, &tmp, 1 );
        vType = static_cast<int>( tmp );
        // unsigned short semantic; // VertexElementSemantic
        readShorts( stream, &tmp, 1 );
        vSemantic = static_cast<int>( tmp );
        // unsigned short offset;	// start offset in buffer in bytes
        readShorts( stream, &offset, 1 );
        // unsigned short index;	// index of the semantic
        readShorts( stream, &index, 1 );

        dest->getVertexDeclaration()->addElement( source, offset,
                                                  static_cast<VertexElementSemantic>( vSemantic ),
                                                  static_cast<VertexElementType>( vType ), index );

        // if (vType == VertexDeclaration::VET_COLOUR)
        //{
        //	LogManager::getSingleton().stream()
        //		<< "Warning: VertexDeclaration::VET_COLOUR element type is deprecated, you should use "
        //		<< "one of the more specific types to indicate the byte order. "
        //		<< "Use OgreMeshUpgrade on " << pMesh->getName() << " as soon as possible. ";
        // }
    }

    void MeshSerializerImpl::readGeometryVertexBuffer( SmartPtr<IStream> &stream, Mesh *pMesh,
                                                       VertexBuffer *dest )
    {
        unsigned short bindIndex, vertexSize;
        // unsigned short bindIndex;	// Index to bind this buffer to
        readShorts( stream, &bindIndex, 1 );
        // unsigned short vertexSize;	// Per-vertex size, must agree with declaration at this index
        readShorts( stream, &vertexSize, 1 );

        // Check for vertex data header
        unsigned short headerID;
        headerID = readChunk( stream );
        if( headerID != M_GEOMETRY_VERTEX_BUFFER_DATA )
        {
            WP_EXCEPTION( "Can't find vertex buffer data area" );
        }
        // Check that vertex size agrees
        if( dest->getVertexDeclaration()->getSize( bindIndex ) != vertexSize )
        {
            WP_EXCEPTION( "Buffer vertex size does not agree with vertex declaration" );
        }

        // Create / populate vertex buffer
        void *pBuf = dest->createVertexData( bindIndex );
        stream->read( pBuf, dest->getNumVertices() * vertexSize );

        auto vertexCount = dest->getNumVertices();
        auto declaration = dest->getVertexDeclaration();
        auto element = declaration->findElementsBySource( bindIndex );

        // endian conversion for OSX
        flipFromLittleEndian( pBuf, vertexCount, vertexSize, element );

        // Set binding
        // dest->vertexBufferBinding->setBinding(bindIndex, vbuf);
    }

    void MeshSerializerImpl::readSubMeshNameTable( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // The map for
        // map<unsigned short, String>::type subMeshNames;
        // unsigned short streamID, subMeshIndex;

        //// Need something to store the index, and the objects name
        //// This table is a method that imported meshes can retain their naming
        //// so that the names established in the modelling software can be used
        //// to get the sub-meshes by name. The exporter must support exporting
        //// the optional stream M_SUBMESH_NAME_TABLE.

        //      // Read in all the sub-streams. Each sub-stream should contain an index and Ogre::String
        //      for the name.
        // if (!stream->eof())
        //{
        //	streamID = readChunk(stream);
        //	while(!stream->eof() && (streamID == M_SUBMESH_NAME_TABLE_ELEMENT ))
        //	{
        //		// Read in the index of the submesh.
        //		readShorts(stream, &subMeshIndex, 1);
        //		// Read in the String and map it to its index.
        //		subMeshNames[subMeshIndex] = readString(stream);

        //		// If we're not end of file get the next stream ID
        //		if (!stream->eof())
        //			streamID = readChunk(stream);
        //	}
        //	if (!stream->eof())
        //	{
        //		// Backpedal back to start of stream
        //		stream->skip(-MSTREAM_OVERHEAD_SIZE);
        //	}
        //}

        //// Set all the submeshes names
        //// ?

        //// Loop through and save out the index and names.
        // map<unsigned short, String>::type::const_iterator it = subMeshNames.begin();

        // while(it != subMeshNames.end())
        //{
        //	// Name this submesh to the stored name.
        //	pMesh->nameSubMesh(it->second, it->first);
        //	++it;
        // }
    }

    void MeshSerializerImpl::readMesh( SmartPtr<IStream> &stream, Mesh *pMesh,
                                       MeshSerializerListener *listener )
    {
        // Never automatically build edge lists for this version
        // expect them in the file or not at all
        // pMesh->mAutoBuildEdgeLists = false;

        // bool skeletallyAnimated
        bool skeletallyAnimated;
        readBools( stream, &skeletallyAnimated, 1 );

        // Find all substreams
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() &&
                   ( streamID == M_GEOMETRY || streamID == M_SUBMESH ||
                     streamID == M_MESH_SKELETON_LINK || streamID == M_MESH_BONE_ASSIGNMENT ||
                     streamID == M_MESH_LOD || streamID == M_MESH_BOUNDS ||
                     streamID == M_SUBMESH_NAME_TABLE || streamID == M_EDGE_LISTS ||
                     streamID == M_POSES || streamID == M_ANIMATIONS || streamID == M_TABLE_EXTREMES ) )
            {
                switch( streamID )
                {
                case M_GEOMETRY:
                {
                    /*
                     * A top-level geometry chunk is the vertex buffer shared by
                     * submeshes whose useSharedVertices flag is set. The old
                     * reader discarded this chunk, leaving those submeshes with
                     * indices but no positions (and therefore an empty physics
                     * collision mesh).
                     */
                    auto sharedVertexBuffer = workphone::make_ptr<VertexBuffer>();
                    readGeometry( stream, pMesh, sharedVertexBuffer.get() );
                    pMesh->setSharedVertexBuffer( sharedVertexBuffer );
                    pMesh->setHasSharedVertexData( sharedVertexBuffer->getNumVertices() > 0 );
                }
                break;
                case M_SUBMESH:
                    readSubMesh( stream, pMesh, listener );
                    break;
                case M_MESH_SKELETON_LINK:
                    readSkeletonLink( stream, pMesh, listener );
                    break;
                case M_MESH_BONE_ASSIGNMENT:
                    readMeshBoneAssignment( stream, pMesh );
                    break;
                case M_MESH_LOD:
                    readMeshLodInfo( stream, pMesh );
                    break;
                case M_MESH_BOUNDS:
                    readBoundsInfo( stream, pMesh );
                    break;
                case M_SUBMESH_NAME_TABLE:
                    readSubMeshNameTable( stream, pMesh );
                    break;
                case M_EDGE_LISTS:
                    readEdgeList( stream, pMesh );
                    break;
                case M_POSES:
                    readPoses( stream, pMesh );
                    break;
                case M_ANIMATIONS:
                    readAnimations( stream, pMesh );
                    break;
                case M_TABLE_EXTREMES:
                    readExtremes( stream, pMesh );
                    break;
                }

                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }

            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readSubMesh( SmartPtr<IStream> &stream, Mesh *pMesh,
                                          MeshSerializerListener *listener )
    {
        unsigned short streamID;

        auto sm = workphone::make_ptr<SubMesh>();
        pMesh->addSubMesh( sm );

        // char* materialName
        auto materialName = readString( stream );
        if( listener )
        {
            listener->processMaterialName( pMesh, &materialName );
        }

        sm->setMaterialName( materialName );

        bool useSharedVertices = false;
        readBools( stream, &useSharedVertices, 1 );
        sm->setUseSharedVertices( useSharedVertices );

        auto indexBuffer = workphone::make_ptr<IndexBuffer>();
        sm->setIndexBuffer( indexBuffer );

        // sm->indexData->indexStart = 0;
        unsigned int indexCount = 0;
        readInts( stream, &indexCount, 1 );
        indexBuffer->setNumIndices( indexCount );

        // bool indexes32Bit
        bool idx32bit;
        readBools( stream, &idx32bit, 1 );
        if( indexCount > 0 )
        {
            if( idx32bit )
            {
                indexBuffer->setIndexType( IndexBuffer::Type::IT_32BIT );
                // unsigned int* faceVertexIndices
                auto pIdx = static_cast<unsigned int *>( indexBuffer->createIndexData() );
                readInts( stream, pIdx, indexBuffer->getNumIndices() );
            }
            else  // 16-bit
            {
                indexBuffer->setIndexType( IndexBuffer::Type::IT_16BIT );
                // unsigned int* faceVertexIndices
                auto pIdx = static_cast<unsigned short *>( indexBuffer->createIndexData() );
                readShorts( stream, pIdx, indexBuffer->getNumIndices() );
            }
        }

        // M_GEOMETRY stream (Optional: present only if useSharedVertices = false)
        if( !sm->getUseSharedVertices() )
        {
            streamID = readChunk( stream );
            if( streamID != M_GEOMETRY )
            {
                WP_EXCEPTION( "Missing geometry data in mesh file" );
            }

            auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
            sm->setVertexBuffer( vertexBuffer );

            readGeometry( stream, pMesh, static_cast<VertexBuffer *>( vertexBuffer.get() ) );
        }

        // Find all bone assignments, submesh operation, and texture aliases (if present)
        if( !stream->eof() )
        {
            streamID = readChunk( stream );
            while( !stream->eof() &&
                   ( streamID == M_SUBMESH_BONE_ASSIGNMENT || streamID == M_SUBMESH_OPERATION ||
                     streamID == M_SUBMESH_TEXTURE_ALIAS ) )
            {
                switch( streamID )
                {
                case M_SUBMESH_OPERATION:
                    readSubMeshOperation( stream, pMesh, static_cast<SubMesh *>( sm.get() ) );
                    break;
                case M_SUBMESH_BONE_ASSIGNMENT:
                    readSubMeshBoneAssignment( stream, pMesh, static_cast<SubMesh *>( sm.get() ) );
                    break;
                case M_SUBMESH_TEXTURE_ALIAS:
                    readSubMeshTextureAlias( stream, pMesh, static_cast<SubMesh *>( sm.get() ) );
                    break;
                }

                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readSubMeshOperation( SmartPtr<IStream> &stream, Mesh *pMesh, SubMesh *sm )
    {
        // unsigned short operationType;
        unsigned short opType;
        readShorts( stream, &opType, 1 );
        // sm->operationType = static_cast<RenderOperation::OperationType>(opType);
    }

    void MeshSerializerImpl::readSubMeshTextureAlias( SmartPtr<IStream> &stream, Mesh *pMesh,
                                                      SubMesh *sub )
    {
        String aliasName = readString( stream );
        String textureName = readString( stream );
        // sub->addTextureAlias(aliasName, textureName);
    }

    void MeshSerializerImpl::writeSkeletonLink( const String &skelName )
    {
        writeChunkHeader( M_MESH_SKELETON_LINK, calcSkeletonLinkSize( skelName ) );

        writeString( skelName );
    }

    void MeshSerializerImpl::readSkeletonLink( SmartPtr<IStream> &stream, Mesh *pMesh,
                                               MeshSerializerListener *listener )
    {
        String skelName = readString( stream );

        if( listener )
        {
            listener->processSkeletonName( pMesh, &skelName );
        }

        //  pMesh->setSkeletonName(skelName);
    }

    void MeshSerializerImpl::readTextureLayer( SmartPtr<IStream> &stream, Mesh *pMesh,
                                               SmartPtr<render::IMaterial> pMat )
    {
        // Material definition section phased out of 1.1
    }

    auto MeshSerializerImpl::calcSkeletonLinkSize( const String &skelName ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        size += skelName.length() + 1;

        return size;
    }

    void MeshSerializerImpl::writeMeshBoneAssignment( const VertexBoneAssignment &assign )
    {
        writeChunkHeader( M_MESH_BONE_ASSIGNMENT, calcBoneAssignmentSize() );

        // unsigned int vertexIndex;
        writeInts( &( assign.m_vertexIndex ), 1 );
        // unsigned short boneIndex;
        writeShorts( &( assign.m_boneIndex ), 1 );
        // float weight;
        writeFloats( &( assign.m_weight ), 1 );
    }

    void MeshSerializerImpl::writeSubMeshBoneAssignment( const VertexBoneAssignment &assign )
    {
        writeChunkHeader( M_SUBMESH_BONE_ASSIGNMENT, calcBoneAssignmentSize() );

        // unsigned int vertexIndex;
        writeInts( &( assign.m_vertexIndex ), 1 );
        // unsigned short boneIndex;
        writeShorts( &( assign.m_boneIndex ), 1 );
        // float weight;
        writeFloats( &( assign.m_weight ), 1 );
    }

    void MeshSerializerImpl::readMeshBoneAssignment( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        auto assign = workphone::make_ptr<VertexBoneAssignment>();

        // unsigned int vertexIndex;
        readInts( stream, &( assign->m_vertexIndex ), 1 );
        // unsigned short boneIndex;
        readShorts( stream, &( assign->m_boneIndex ), 1 );
        // float weight;
        readFloats( stream, &( assign->m_weight ), 1 );

        pMesh->addBoneAssignment( assign );
    }

    void MeshSerializerImpl::readSubMeshBoneAssignment( SmartPtr<IStream> &stream, Mesh *pMesh,
                                                        SubMesh *sub )
    {
        auto assign = workphone::make_ptr<VertexBoneAssignment>();

        // unsigned int vertexIndex;
        readInts( stream, &( assign->m_vertexIndex ), 1 );
        // unsigned short boneIndex;
        readShorts( stream, &( assign->m_boneIndex ), 1 );
        // float weight;
        readFloats( stream, &( assign->m_weight ), 1 );

        sub->addBoneAssignment( assign );
    }

    auto MeshSerializerImpl::calcBoneAssignmentSize() -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // Vert index
        size += sizeof( unsigned int );
        // Bone index
        size += sizeof( unsigned short );
        // weight
        size += sizeof( float );

        return size;
    }

    void MeshSerializerImpl::writeLodInfo( const Mesh *pMesh )
    {
        //      const LodStrategy *strategy = pMesh->getLodStrategy();
        //      unsigned short numLods = pMesh->getNumLodLevels();
        //      bool manual = pMesh->isLodManual();
        //      writeLodSummary(numLods, manual, strategy);

        //// Loop from LOD 1 (not 0, this is full detail)
        //      for (unsigned short i = 1; i < numLods; ++i)
        //      {
        //	const MeshLodUsage& usage = pMesh->getLodLevel(i);
        //	if (manual)
        //	{
        //		writeLodUsageManual(usage);
        //	}
        //	else
        //	{
        //		writeLodUsageGenerated(pMesh, usage, i);
        //	}

        //      }
    }

    void MeshSerializerImpl::writeLodSummary( unsigned short numLevels, bool manual,
                                              const LodStrategy *strategy )
    {
        // Header
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // unsigned short numLevels;
        size += sizeof( unsigned short );
        // bool manual;  (true for manual alternate meshes, false for generated)
        size += sizeof( bool );
        writeChunkHeader( M_MESH_LOD, size );

        // Details
        // string strategyName;
        // writeString(strategy->getName());
        // unsigned short numLevels;
        writeShorts( &numLevels, 1 );
        // bool manual;  (true for manual alternate meshes, false for generated)
        writeBools( &manual, 1 );
    }

    // void MeshSerializerImpl::writeLodUsageManual(const MeshLodUsage& usage)
    //{
    //    // Header
    //    u32 size = MSTREAM_OVERHEAD_SIZE;
    //    u32 manualSize = MSTREAM_OVERHEAD_SIZE;
    //    // float lodValue;
    //    size += sizeof(float);
    //    // Manual part size

    //    // String manualMeshName;
    //    manualSize += usage.manualName.length() + 1;

    //    size += manualSize;

    //    writeChunkHeader(M_MESH_LOD_USAGE, size);
    //    writeFloats(&(usage.userValue), 1);

    //    writeChunkHeader(M_MESH_LOD_MANUAL, manualSize);
    //    writeString(usage.manualName);

    //}

    //  void MeshSerializerImpl::writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage& usage,
    // unsigned short lodNum)
    //  {
    //// Usage Header
    //      u32 size = MSTREAM_OVERHEAD_SIZE;
    // unsigned short subidx;

    //      // float fromDepthSquared;
    //      size += sizeof(float);

    //      // Calc generated SubMesh sections size
    // for(subidx = 0; subidx < pMesh->getNumSubMeshes(); ++subidx)
    //{
    //	// header
    //	size += MSTREAM_OVERHEAD_SIZE;
    //	// unsigned int numFaces;
    //	size += sizeof(unsigned int);
    //	SubMesh* sm = pMesh->getSubMesh(subidx);
    //          const IndexData* indexData = sm->mLodFaceList[lodNum - 1];

    //          // bool indexes32Bit
    //	size += sizeof(bool);
    //	// unsigned short*/int* faceIndexes;
    //          if (!indexData->indexBuffer.isNull() &&
    //		indexData->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT)
    //          {
    //	    size += static_cast<unsigned long>(
    //                  sizeof(unsigned int) * indexData->indexCount);
    //          }
    //          else
    //          {
    //	    size += static_cast<unsigned long>(
    //                  sizeof(unsigned short) * indexData->indexCount);
    //          }

    //}

    //      writeChunkHeader(M_MESH_LOD_USAGE, size);
    //      writeFloats(&(usage.userValue), 1);

    //// Now write sections
    //      // Calc generated SubMesh sections size
    // for(subidx = 0; subidx < pMesh->getNumSubMeshes(); ++subidx)
    //{
    //	size = MSTREAM_OVERHEAD_SIZE;
    //	// unsigned int numFaces;
    //	size += sizeof(unsigned int);
    //	SubMesh* sm = pMesh->getSubMesh(subidx);
    //          const IndexData* indexData = sm->mLodFaceList[lodNum - 1];
    //          // bool indexes32Bit
    //	size += sizeof(bool);
    //	// Lock index buffer to write
    //	HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
    //	// bool indexes32bit
    //	bool idx32 = (!ibuf.isNull() && ibuf->getType() == HardwareIndexBuffer::IT_32BIT);
    //	// unsigned short*/int* faceIndexes;
    //          if (idx32)
    //          {
    //	    size += static_cast<unsigned long>(
    //                  sizeof(unsigned int) * indexData->indexCount);
    //          }
    //          else
    //          {
    //	    size += static_cast<unsigned long>(
    //                  sizeof(unsigned short) * indexData->indexCount);
    //          }

    //	writeChunkHeader(M_MESH_LOD_GENERATED, size);
    //	unsigned int idxCount = static_cast<unsigned int>(indexData->indexCount);
    //	writeInts(&idxCount, 1);
    //	writeBools(&idx32, 1);

    //	if (idxCount > 0)
    //	{
    //		if (idx32)
    //		{
    //			unsigned int* pIdx = static_cast<unsigned int*>(
    //				ibuf->lock(HardwareBuffer::HBL_READ_ONLY));
    //			writeInts(pIdx, indexData->indexCount);
    //			ibuf->unlock();
    //		}
    //		else
    //		{
    //			unsigned short* pIdx = static_cast<unsigned short*>(
    //				ibuf->lock(HardwareBuffer::HBL_READ_ONLY));
    //			writeShorts(pIdx, indexData->indexCount);
    //			ibuf->unlock();
    //		}
    //	}
    //}

    //  }

    void MeshSerializerImpl::writeBoundsInfo( const Mesh *pMesh )
    {
        // Usage Header
        unsigned long size = MSTREAM_OVERHEAD_SIZE;

        size += sizeof( float ) * 7;
        writeChunkHeader( M_MESH_BOUNDS, size );

        // float minx, miny, minz
        auto aabb = pMesh->getAABB();
        const auto &min = aabb.getMinimum();
        const auto &max = aabb.getMaximum();

        writeFloats( min.ptr(), 3 );

        // float maxx, maxy, maxz
        writeFloats( max.ptr(), 3 );

        auto radius = aabb.getRadius();

        // float radius
        writeFloats( &radius, 1 );
    }

    void MeshSerializerImpl::readBoundsInfo( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        auto min = Vector3<f32>();
        auto max = Vector3<f32>();

        // float minx, miny, minz
        readFloats( stream, min.ptr(), 3 );

        // float maxx, maxy, maxz
        readFloats( stream, max.ptr(), 3 );

        auto box = AABB3<real_Num>( Vector3<real_Num>( min.x, min.y, min.z ),
                                    Vector3<real_Num>( max.x, max.y, max.z ) );
        pMesh->setAABB( box );

        // float radius
        float radius;
        readFloats( stream, &radius, 1 );
        pMesh->setBoundingSphereRadius( radius );
    }

    void MeshSerializerImpl::readMeshLodInfo( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // unsigned short streamID, i;

        //      // Read the strategy to be used for this mesh
        //      String strategyName = readString(stream);
        //      LodStrategy *strategy = LodStrategyManager::getSingleton().getStrategy(strategyName);
        //      pMesh->setLodStrategy(strategy);

        //      // unsigned short numLevels;
        // readShorts(stream, &(pMesh->mNumLods), 1);
        //      // bool manual;  (true for manual alternate meshes, false for generated)
        // readBools(stream, &(pMesh->mIsLodManual), 1);

        //// Preallocate submesh lod face data if not manual
        // if (!pMesh->mIsLodManual)
        //{
        //	unsigned short numsubs = pMesh->getNumSubMeshes();
        //	for (i = 0; i < numsubs; ++i)
        //	{
        //		SubMesh* sm = pMesh->getSubMesh(i);
        //		sm->mLodFaceList.resize(pMesh->mNumLods-1);
        //	}
        // }

        //// Loop from 1 rather than 0 (full detail index is not in file)
        // for (i = 1; i < pMesh->mNumLods; ++i)
        //{
        //	streamID = readChunk(stream);
        //	if (streamID != M_MESH_LOD_USAGE)
        //	{
        //		OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
        //			"Missing M_MESH_LOD_USAGE stream in " + pMesh->getName(),
        //			"MeshSerializerImpl::readMeshLodInfo");
        //	}
        //	// Read depth
        //	MeshLodUsage usage;
        //	readFloats(stream, &(usage.userValue), 1);

        //	if (pMesh->isLodManual())
        //	{
        //		readMeshLodUsageManual(stream, pMesh, i, usage);
        //	}
        //	else //(!pMesh->isLodManual)
        //	{
        //		readMeshLodUsageGenerated(stream, pMesh, i, usage);
        //	}
        //          usage.edgeData = NULL;

        //	// Save usage
        //	pMesh->mMeshLodUsageList.push_back(usage);
        //}
    }

    ////void MeshSerializerImpl::readMeshLodUsageManual(SmartPtr<IStream>& stream,
    ////       Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage)
    ////{
    ////	unsigned long streamID;
    ////	// Read detail stream
    ////	streamID = readChunk(stream);
    ////	if (streamID != M_MESH_LOD_MANUAL)
    ////	{
    ////		OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
    ////			"Missing M_MESH_LOD_MANUAL stream in " + pMesh->getName(),
    ////			"MeshSerializerImpl::readMeshLodUsageManual");
    ////	}

    ////	usage.manualName = readString(stream);
    ////	usage.manualMesh.setNull(); // will trigger load later
    ////}
    ////
    ////void MeshSerializerImpl::readMeshLodUsageGenerated(SmartPtr<IStream>& stream,
    ////       Mesh* pMesh, unsigned short lodNum, MeshLodUsage& usage)
    ////{
    ////	usage.manualName = "";
    ////	usage.manualMesh.setNull();

    ////	// Get one set of detail per SubMesh
    ////	unsigned short numSubs, i;
    ////	unsigned long streamID;
    ////	numSubs = pMesh->getNumSubMeshes();
    ////	for (i = 0; i < numSubs; ++i)
    ////	{
    ////		streamID = readChunk(stream);
    ////		if (streamID != M_MESH_LOD_GENERATED)
    ////		{
    ////			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
    ////				"Missing M_MESH_LOD_GENERATED stream in " + pMesh->getName(),
    ////				"MeshSerializerImpl::readMeshLodUsageGenerated");
    ////		}

    ////		SubMesh* sm = pMesh->getSubMesh(i);
    ////		// lodNum - 1 because SubMesh doesn't store full detail LOD
    ////           sm->mLodFaceList[lodNum - 1] = OGRE_NEW IndexData();
    ////		IndexData* indexData = sm->mLodFaceList[lodNum - 1];
    ////           // unsigned int numIndexes
    ////           unsigned int numIndexes;
    ////		readInts(stream, &numIndexes, 1);
    ////           indexData->indexCount = static_cast<u32>(numIndexes);
    ////           // bool indexes32Bit
    ////           bool idx32Bit;
    ////           readBools(stream, &idx32Bit, 1);
    ////           // unsigned short*/int* faceIndexes;  ((v1, v2, v3) * numFaces)
    ////           if (idx32Bit)
    ////           {
    ////               indexData->indexBuffer = HardwareBufferManager::getSingleton().
    ////                   createIndexBuffer(HardwareIndexBuffer::IT_32BIT, indexData->indexCount,
    ////                   pMesh->mIndexBufferUsage, pMesh->mIndexBufferShadowBuffer);
    ////               unsigned int* pIdx = static_cast<unsigned int*>(
    ////                   indexData->indexBuffer->lock(
    ////                       0,
    ////                       indexData->indexBuffer->getSizeInBytes(),
    ////                       HardwareBuffer::HBL_DISCARD) );

    ////		    readInts(stream, pIdx, indexData->indexCount);
    ////               indexData->indexBuffer->unlock();

    ////           }
    ////           else
    ////           {
    ////               indexData->indexBuffer = HardwareBufferManager::getSingleton().
    ////                   createIndexBuffer(HardwareIndexBuffer::IT_16BIT, indexData->indexCount,
    ////                   pMesh->mIndexBufferUsage, pMesh->mIndexBufferShadowBuffer);
    ////               unsigned short* pIdx = static_cast<unsigned short*>(
    ////                   indexData->indexBuffer->lock(
    ////                       0,
    ////                       indexData->indexBuffer->getSizeInBytes(),
    ////                       HardwareBuffer::HBL_DISCARD) );
    ////		    readShorts(stream, pIdx, indexData->indexCount);
    ////               indexData->indexBuffer->unlock();

    ////           }

    ////	}
    ////}

    void MeshSerializerImpl::flipFromLittleEndian( void *pData, u32 vertexCount, u32 vertexSize,
                                                   const Array<SmartPtr<IVertexElement>> &elems )
    {
        if( m_flipEndian )
        {
            flipEndian( pData, vertexCount, vertexSize, elems );
        }
    }

    void MeshSerializerImpl::flipToLittleEndian( void *pData, u32 vertexCount, u32 vertexSize,
                                                 const Array<SmartPtr<IVertexElement>> &elems )
    {
        if( m_flipEndian )
        {
            flipEndian( pData, vertexCount, vertexSize, elems );
        }
    }

    void MeshSerializerImpl::flipEndian( void *pData, u32 vertexCount, u32 vertexSize,
                                         const Array<SmartPtr<IVertexElement>> &elems )
    {
        void *pBase = pData;
        for( u32 v = 0; v < vertexCount; ++v )
        {
            for( auto &elem : elems )
            {
                void *pElem;

                // re-base pointer to the element
                elem->baseVertexPointerToElement( pBase, &pElem );

                // Flip the endian based on the type
                u32 typeSize = 0;

                auto elementType = elem->getType();
                auto baseElementType = MeshUtil::getBaseType( elementType );

                switch( baseElementType )
                {
                case VertexElementType::VET_FLOAT1:
                    typeSize = sizeof( float );
                    break;
                case VertexElementType::VET_SHORT1:
                    typeSize = sizeof( short );
                    break;
                case VertexElementType::VET_COLOUR:
                    // case VET_COLOUR_ABGR:
                    // case VET_COLOUR_ARGB:
                    typeSize = sizeof( u32 );
                    break;
                case VertexElementType::VET_UBYTE4:
                    typeSize = 0;  // NO FLIPPING
                    break;
                default:
                    assert( false );  // Should never happen
                }

                auto typeCount = MeshUtil::getTypeCount( elementType );
                Serializer::flipEndian( pElem, typeSize, typeCount );
            }

            pBase = static_cast<void *>( static_cast<unsigned char *>( pBase ) + vertexSize );
        }
    }

    auto MeshSerializerImpl::calcEdgeListSize( const Mesh *pMesh ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // for (ushort i = 0; i < pMesh->getNumLodLevels(); ++i)
        //{

        //    const EdgeData* edgeData = pMesh->getEdgeList(i);
        //    bool isManual = pMesh->isLodManual() && (i > 0);

        //    size += calcEdgeListLodSize(edgeData, isManual);

        //}

        return size;
    }

    //
    // u32 MeshSerializerImpl::calcEdgeListLodSize(const EdgeData* edgeData, bool isManual)
    //{
    //     u32 size = MSTREAM_OVERHEAD_SIZE;

    //    // unsigned short lodIndex
    //    size += sizeof(u16);

    //    // bool isManual			// If manual, no edge data here, loaded from manual mesh
    //    size += sizeof(bool);
    //    if (!isManual)
    //    {
    //        // bool isClosed
    //        size += sizeof(bool);
    //        // unsigned long numTriangles
    //        size += sizeof(u32);
    //        // unsigned long numEdgeGroups
    //        size += sizeof(u32);
    //        // Triangle* triangleList
    //        u32 triSize = 0;
    //        // unsigned long indexSet
    //        // unsigned long vertexSet
    //        // unsigned long vertIndex[3]
    //        // unsigned long sharedVertIndex[3]
    //        // float normal[4]
    //        triSize += sizeof(u32) * 8
    //                + sizeof(float) * 4;

    //        size += triSize * edgeData->triangles.size();
    //        // Write the groups
    //        for (EdgeData::EdgeGroupList::const_iterator gi = edgeData->edgeGroups.begin();
    //            gi != edgeData->edgeGroups.end(); ++gi)
    //        {
    //            const EdgeData::EdgeGroup& edgeGroup = *gi;
    //            size += calcEdgeGroupSize(edgeGroup);
    //        }

    //    }

    //    return size;
    //}
    //
    // u32 MeshSerializerImpl::calcEdgeGroupSize(const EdgeData::EdgeGroup& group)
    //{
    //     u32 size = MSTREAM_OVERHEAD_SIZE;

    //    // unsigned long vertexSet
    //    size += sizeof(u32);
    //    // unsigned long triStart
    //    size += sizeof(u32);
    //    // unsigned long triCount
    //    size += sizeof(u32);
    //    // unsigned long numEdges
    //    size += sizeof(u32);
    //    // Edge* edgeList
    //    u32 edgeSize = 0;
    //    // unsigned long  triIndex[2]
    //    // unsigned long  vertIndex[2]
    //    // unsigned long  sharedVertIndex[2]
    //    // bool degenerate
    //    edgeSize += sizeof(u32) * 6 + sizeof(bool);
    //    size += edgeSize * group.edges.size();

    //    return size;
    //}

    void MeshSerializerImpl::writeEdgeList( const Mesh *pMesh )
    {
        // writeChunkHeader(M_EDGE_LISTS, calcEdgeListSize(pMesh));

        // for (unsigned short i = 0; i < pMesh->getNumLodLevels(); ++i)
        //{
        //     const EdgeData* edgeData = pMesh->getEdgeList(i);
        //     bool isManual = pMesh->isLodManual() && (i > 0);
        //     writeChunkHeader(M_EDGE_LIST_LOD, calcEdgeListLodSize(edgeData, isManual));

        //    // unsigned short lodIndex
        //    writeShorts(&i, 1);

        //    // bool isManual			// If manual, no edge data here, loaded from manual mesh
        //    writeBools(&isManual, 1);
        //    if (!isManual)
        //    {
        //        // bool isClosed
        //        writeBools(&edgeData->isClosed, 1);
        //        // unsigned long  numTriangles
        //        u32 count = static_cast<u32>(edgeData->triangles.size());
        //        writeInts(&count, 1);
        //        // unsigned long numEdgeGroups
        //        count = static_cast<u32>(edgeData->edgeGroups.size());
        //        writeInts(&count, 1);
        //        // Triangle* triangleList
        //        // Iterate rather than writing en-masse to allow endian conversion
        //        EdgeData::TriangleList::const_iterator t = edgeData->triangles.begin();
        //        EdgeData::TriangleFaceNormalList::const_iterator fni =
        //        edgeData->triangleFaceNormals.begin(); for ( ; t != edgeData->triangles.end(); ++t,
        //        ++fni)
        //        {
        //            const EdgeData::Triangle& tri = *t;
        //            // unsigned long indexSet;
        //            u32 tmp[3];
        //            tmp[0] = tri.indexSet;
        //            writeInts(tmp, 1);
        //            // unsigned long vertexSet;
        //            tmp[0] = tri.vertexSet;
        //            writeInts(tmp, 1);
        //            // unsigned long vertIndex[3];
        //            tmp[0] = tri.vertIndex[0];
        //            tmp[1] = tri.vertIndex[1];
        //            tmp[2] = tri.vertIndex[2];
        //            writeInts(tmp, 3);
        //            // unsigned long sharedVertIndex[3];
        //            tmp[0] = tri.sharedVertIndex[0];
        //            tmp[1] = tri.sharedVertIndex[1];
        //            tmp[2] = tri.sharedVertIndex[2];
        //            writeInts(tmp, 3);
        //            // float normal[4];
        //            writeFloats(&(fni->x), 4);

        //        }
        //        // Write the groups
        //        for (EdgeData::EdgeGroupList::const_iterator gi = edgeData->edgeGroups.begin();
        //            gi != edgeData->edgeGroups.end(); ++gi)
        //        {
        //            const EdgeData::EdgeGroup& edgeGroup = *gi;
        //            writeChunkHeader(M_EDGE_GROUP, calcEdgeGroupSize(edgeGroup));
        //            // unsigned long vertexSet
        //            u32 vertexSet = static_cast<u32>(edgeGroup.vertexSet);
        //            writeInts(&vertexSet, 1);
        //            // unsigned long triStart
        //            u32 triStart = static_cast<u32>(edgeGroup.triStart);
        //            writeInts(&triStart, 1);
        //            // unsigned long triCount
        //            u32 triCount = static_cast<u32>(edgeGroup.triCount);
        //            writeInts(&triCount, 1);
        //            // unsigned long numEdges
        //            count = static_cast<u32>(edgeGroup.edges.size());
        //            writeInts(&count, 1);
        //            // Edge* edgeList
        //            // Iterate rather than writing en-masse to allow endian conversion
        //            for (EdgeData::EdgeList::const_iterator ei = edgeGroup.edges.begin();
        //                ei != edgeGroup.edges.end(); ++ei)
        //            {
        //                const EdgeData::Edge& edge = *ei;
        //                u32 tmp[2];
        //                // unsigned long  triIndex[2]
        //                tmp[0] = edge.triIndex[0];
        //                tmp[1] = edge.triIndex[1];
        //                writeInts(tmp, 2);
        //                // unsigned long  vertIndex[2]
        //                tmp[0] = edge.vertIndex[0];
        //                tmp[1] = edge.vertIndex[1];
        //                writeInts(tmp, 2);
        //                // unsigned long  sharedVertIndex[2]
        //                tmp[0] = edge.sharedVertIndex[0];
        //                tmp[1] = edge.sharedVertIndex[1];
        //                writeInts(tmp, 2);
        //                // bool degenerate
        //                writeBools(&(edge.degenerate), 1);
        //            }

        //        }

        //    }

        //}
    }

    void MeshSerializerImpl::readEdgeList( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // if (!stream->eof())
        //{
        //     unsigned short streamID = readChunk(stream);
        //     while(!stream->eof() &&
        //         streamID == M_EDGE_LIST_LOD)
        //     {
        //         // Process single LOD

        //        // unsigned short lodIndex
        //        unsigned short lodIndex;
        //        readShorts(stream, &lodIndex, 1);

        //        // bool isManual			// If manual, no edge data here, loaded from manual mesh
        //        bool isManual;
        //        readBools(stream, &isManual, 1);
        //        // Only load in non-manual levels; others will be connected up by Mesh on demand
        //        if (!isManual)
        //        {
        //            MeshLodUsage& usage = const_cast<MeshLodUsage&>(pMesh->getLodLevel(lodIndex));

        //            usage.edgeData = OGRE_NEW EdgeData();

        //            // Read detail information of the edge list
        //            readEdgeListLodInfo(stream, usage.edgeData);

        //            // Postprocessing edge groups
        //            EdgeData::EdgeGroupList::iterator egi, egend;
        //            egend = usage.edgeData->edgeGroups.end();
        //            for (egi = usage.edgeData->edgeGroups.begin(); egi != egend; ++egi)
        //            {
        //                EdgeData::EdgeGroup& edgeGroup = *egi;
        //                // Populate edgeGroup.vertexData pointers
        //                // If there is shared vertex data, vertexSet 0 is that,
        //                // otherwise 0 is first dedicated
        //                if (pMesh->sharedVertexData)
        //                {
        //                    if (edgeGroup.vertexSet == 0)
        //                    {
        //                        edgeGroup.vertexData = pMesh->sharedVertexData;
        //                    }
        //                    else
        //                    {
        //                        edgeGroup.vertexData = pMesh->getSubMesh(
        //                            (unsigned short)edgeGroup.vertexSet-1)->vertexData;
        //                    }
        //                }
        //                else
        //                {
        //                    edgeGroup.vertexData = pMesh->getSubMesh(
        //                        (unsigned short)edgeGroup.vertexSet)->vertexData;
        //                }
        //            }
        //        }

        //        if (!stream->eof())
        //        {
        //            streamID = readChunk(stream);
        //        }

        //    }
        //    if (!stream->eof())
        //    {
        //        // Backpedal back to start of stream
        //        stream->skip(-MSTREAM_OVERHEAD_SIZE);
        //    }
        //}

        // pMesh->mEdgeListsBuilt = true;
    }

    // void MeshSerializerImpl::readEdgeListLodInfo(SmartPtr<IStream>& stream,
    //    EdgeData* edgeData)
    //{
    //    // bool isClosed
    //    readBools(stream, &edgeData->isClosed, 1);
    //    // unsigned long numTriangles
    //    u32 numTriangles;
    //    readInts(stream, &numTriangles, 1);
    //    // Allocate correct amount of memory
    //    edgeData->triangles.resize(numTriangles);
    //    edgeData->triangleFaceNormals.resize(numTriangles);
    //    edgeData->triangleLightFacings.resize(numTriangles);
    //    // unsigned long numEdgeGroups
    //    u32 numEdgeGroups;
    //    readInts(stream, &numEdgeGroups, 1);
    //    // Allocate correct amount of memory
    //    edgeData->edgeGroups.resize(numEdgeGroups);
    //    // Triangle* triangleList
    //    u32 tmp[3];
    //    for (u32 t = 0; t < numTriangles; ++t)
    //    {
    //        EdgeData::Triangle& tri = edgeData->triangles[t];
    //        // unsigned long indexSet
    //        readInts(stream, tmp, 1);
    //        tri.indexSet = tmp[0];
    //        // unsigned long vertexSet
    //        readInts(stream, tmp, 1);
    //        tri.vertexSet = tmp[0];
    //        // unsigned long vertIndex[3]
    //        readInts(stream, tmp, 3);
    //        tri.vertIndex[0] = tmp[0];
    //        tri.vertIndex[1] = tmp[1];
    //        tri.vertIndex[2] = tmp[2];
    //        // unsigned long sharedVertIndex[3]
    //        readInts(stream, tmp, 3);
    //        tri.sharedVertIndex[0] = tmp[0];
    //        tri.sharedVertIndex[1] = tmp[1];
    //        tri.sharedVertIndex[2] = tmp[2];
    //        // float normal[4]
    //        readFloats(stream, &(edgeData->triangleFaceNormals[t].x), 4);

    //    }

    //    for (u32 eg = 0; eg < numEdgeGroups; ++eg)
    //    {
    //        unsigned short streamID = readChunk(stream);
    //        if (streamID != M_EDGE_GROUP)
    //        {
    //            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
    //                "Missing M_EDGE_GROUP stream",
    //                "MeshSerializerImpl::readEdgeListLodInfo");
    //        }
    //        EdgeData::EdgeGroup& edgeGroup = edgeData->edgeGroups[eg];

    //        // unsigned long vertexSet
    //        readInts(stream, tmp, 1);
    //        edgeGroup.vertexSet = tmp[0];
    //        // unsigned long triStart
    //        readInts(stream, tmp, 1);
    //        edgeGroup.triStart = tmp[0];
    //        // unsigned long triCount
    //        readInts(stream, tmp, 1);
    //        edgeGroup.triCount = tmp[0];
    //        // unsigned long numEdges
    //        u32 numEdges;
    //        readInts(stream, &numEdges, 1);
    //        edgeGroup.edges.resize(numEdges);
    //        // Edge* edgeList
    //        for (u32 e = 0; e < numEdges; ++e)
    //        {
    //            EdgeData::Edge& edge = edgeGroup.edges[e];
    //            // unsigned long  triIndex[2]
    //            readInts(stream, tmp, 2);
    //            edge.triIndex[0] = tmp[0];
    //            edge.triIndex[1] = tmp[1];
    //            // unsigned long  vertIndex[2]
    //            readInts(stream, tmp, 2);
    //            edge.vertIndex[0] = tmp[0];
    //            edge.vertIndex[1] = tmp[1];
    //            // unsigned long  sharedVertIndex[2]
    //            readInts(stream, tmp, 2);
    //            edge.sharedVertIndex[0] = tmp[0];
    //            edge.sharedVertIndex[1] = tmp[1];
    //            // bool degenerate
    //            readBools(stream, &(edge.degenerate), 1);
    //        }
    //    }
    //}

    auto MeshSerializerImpl::calcAnimationsSize( const Mesh *pMesh ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        for( unsigned short a = 0; a < pMesh->getNumAnimations(); ++a )
        {
            auto anim = pMesh->getAnimation( a );
            size += calcAnimationSize( anim.get() );
        }

        return size;
    }

    u32 MeshSerializerImpl::calcAnimationSize( const IAnimation *anim )
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // char* name
        size += anim->getName().length() + 1;

        // float length
        size += sizeof( float );

        auto vertexTracks = anim->getVertexTracks();
        for( auto vertexTrack : vertexTracks )
        {
            size += calcAnimationTrackSize( vertexTrack.get() );
        }

        return size;
    }

    u32 MeshSerializerImpl::calcAnimationTrackSize( const IAnimationVertexTrack *track )
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // u16 type
        size += sizeof( u16 );
        // unsigned short target		// 0 for shared geometry,
        size += sizeof( unsigned short );

        if( track->getAnimationType() == VertexAnimationType::VAT_MORPH )
        {
            for( unsigned short i = 0; i < track->getNumKeyFrames(); ++i )
            {
                auto kf = track->getVertexMorphKeyFrame( i );
                size += calcMorphKeyframeSize( kf, track->getAssociatedVertexData()->getNumVertices() );
            }
        }
        else
        {
            for( unsigned short i = 0; i < track->getNumKeyFrames(); ++i )
            {
                auto kf = track->getVertexPoseKeyFrame( i );
                size += calcPoseKeyframeSize( kf );
            }
        }

        return size;
    }

    u32 MeshSerializerImpl::calcMorphKeyframeSize( const IAnimationMorphKeyFrame *kf, u32 vertexCount )
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // float time
        size += sizeof( float );
        // float x,y,z[,nx,ny,nz]
        //bool includesNormals = kf->getVertexBuffer()->getVertexSize() > (sizeof(float) * 3);
        //size += sizeof(float) * (includesNormals ? 6 : 3) * vertexCount;

        return size;
    }

    u32 MeshSerializerImpl::calcPoseKeyframeSize( const IAnimationPoseKeyFrame *kf )
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // float time
        size += sizeof( float );

        //size += calcPoseKeyframePoseRefSize() * kf->getPoseReferences().size();

        return size;
    }

    auto MeshSerializerImpl::calcPoseKeyframePoseRefSize() -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // unsigned short poseIndex
        size += sizeof( u16 );
        // float influence
        size += sizeof( float );

        return size;
    }

    auto MeshSerializerImpl::calcPosesSize( const Mesh *pMesh ) -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // Mesh::ConstPoseIterator poseIt = pMesh->getPoseIterator();
        // while (poseIt.hasMoreElements())
        //{
        //	size += calcPoseSize(poseIt.getNext());
        // }
        return size;
    }

    u32 MeshSerializerImpl::calcPoseSize( const IMeshPose *pose )
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;

        // char* name (may be blank)
        size += pose->getName().length() + 1;
        // unsigned short target
        size += sizeof( u16 );
        // bool includesNormals
        size += sizeof( bool );

        // vertex offsets
        //size += pose->getVertexOffsets().size() * calcPoseVertexSize(pose);

        return size;
    }

    u32 MeshSerializerImpl::calcPoseVertexSize( const IMeshPose *pose )
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        //// unsigned long vertexIndex
        //size += sizeof(u32);
        //// float xoffset, yoffset, zoffset
        //size += sizeof(float) * 3;
        //// optional normals
        //if (!pose->getNormals().empty())
        //	size += sizeof(float) * 3;

        return size;
    }

    void MeshSerializerImpl::writePoses( const Mesh *pMesh )
    {
        // Mesh::ConstPoseIterator poseIterator = pMesh->getPoseIterator();
        // if (poseIterator.hasMoreElements())
        //{
        //	writeChunkHeader(M_POSES, calcPosesSize(pMesh));
        //	while (poseIterator.hasMoreElements())
        //	{
        //		writePose(poseIterator.getNext());
        //	}
        // }
    }

    void MeshSerializerImpl::writePose( const MeshPose *pose )
    {
        //	writeChunkHeader(M_POSE, calcPoseSize(pose));

        //	// char* name (may be blank)
        //	writeString(pose->getName());

        //	// unsigned short target
        //	ushort target = pose->getTarget();`r`n        //	writeShorts(&target, 1);
        //
        //	// bool includesNormals
        //	bool includesNormals = !pose->getNormals().empty();
        //	writeBools(&includesNormals, 1);

        //	u32 vertexSize = calcPoseVertexSize(pose);
        //	Pose::ConstVertexOffsetIterator vit = pose->getVertexOffsetIterator();
        //	Pose::ConstNormalsIterator nit = pose->getNormalsIterator();
        //	while (vit.hasMoreElements())
        //	{
        //		u32 vertexIndex = (u32)vit.peekNextKey();
        //		Vector3 offset = vit.getNext();
        //		writeChunkHeader(M_POSE_VERTEX, vertexSize);
        //		// unsigned long vertexIndex
        //		writeInts(&vertexIndex, 1);
        //		// float xoffset, yoffset, zoffset
        //		writeFloats(offset.ptr(), 3);
        //		if (includesNormals)
        //		{
        //			Vector3 normal = nit.getNext();
        //			// float xnormal, ynormal, znormal
        //			writeFloats(normal.ptr(), 3);
        //		}
        //	}
    }

    void MeshSerializerImpl::writeAnimations( const Mesh *pMesh )
    {
        writeChunkHeader( M_ANIMATIONS, calcAnimationsSize( pMesh ) );

        for( unsigned short a = 0; a < pMesh->getNumAnimations(); ++a )
        {
            auto anim = pMesh->getAnimation( a );
            WP_LOG( "Exporting animation " + anim->getName() );
            writeAnimation( anim.get() );
            WP_LOG( "Animation exported." );
        }
    }

    void MeshSerializerImpl::writeAnimation( const IAnimation *anim )
    {
        writeChunkHeader( M_ANIMATION, calcAnimationSize( anim ) );

        writeString( anim->getName() );

        // float length
        float len = anim->getLength();
        writeFloats( &len, 1 );

        if( anim->getUseBaseKeyFrame() )
        {
            u32 size = MSTREAM_OVERHEAD_SIZE;
            // char* baseAnimationName (including terminator)
            size += anim->getBaseKeyFrameAnimationName().length() + 1;
            // float baseKeyFrameTime
            size += sizeof( float );

            writeChunkHeader( M_ANIMATION_BASEINFO, size );

            // char* baseAnimationName (blank for self)
            writeString( anim->getBaseKeyFrameAnimationName() );

            // float baseKeyFrameTime
            float t = (float)anim->getBaseKeyFrameTime();
            writeFloats( &t, 1 );
        }

        //	// tracks
        //	Animation::VertexTrackIterator trackIt = anim->getVertexTrackIterator();
        //	while (trackIt.hasMoreElements())
        //	{
        //		VertexAnimationTrack* vt = trackIt.getNext();
        //		writeAnimationTrack(vt);
        //}
    }

    void MeshSerializerImpl::writeAnimationTrack( const AnimationVertexTrack *track )
    {
        writeChunkHeader( M_ANIMATION_TRACK, calcAnimationTrackSize( track ) );
        // unsigned short type			// 1 == morph, 2 == pose
        u16 animType = (u16)track->getAnimationType();
        writeShorts( &animType, 1 );

        // unsigned short target
        auto target = track->getAnimationHandle();
        writeShorts( &target, 1 );

        if( track->getAnimationType() == VertexAnimationType::VAT_MORPH )
        {
            for( unsigned short i = 0; i < track->getNumKeyFrames(); ++i )
            {
                auto kf = track->getVertexMorphKeyFrame( i );
                auto vertexCount = track->getAssociatedVertexData()->getNumVertices();
                //writeMorphKeyframe( kf, vertexCount );
            }
        }
        else  // VAT_POSE
        {
            for( unsigned short i = 0; i < track->getNumKeyFrames(); ++i )
            {
                auto kf = track->getVertexPoseKeyFrame( i );
                //writePoseKeyframe( kf );
            }
        }
    }

    void MeshSerializerImpl::writeMorphKeyframe( const AnimationMorphKeyFrame *kf, u32 vertexCount )
    {
        writeChunkHeader( M_ANIMATION_MORPH_KEYFRAME, calcMorphKeyframeSize( kf, vertexCount ) );
        // float time
        float timePos = kf->getTime();
        writeFloats( &timePos, 1 );
        // bool includeNormals
        auto vertexBuffer = kf->getVertexBuffer();
        if( vertexBuffer )
        {
            auto declaration = vertexBuffer->getVertexDeclaration();
            bool includeNormals = declaration->getSize() > ( sizeof( float ) * 3 );
            writeBools( &includeNormals, 1 );
            // float x,y,z			// repeat by number of vertices in original geometry
            auto dataArray = vertexBuffer->getDataArray();
            if( !dataArray.empty() )
            {
                auto pSrc = reinterpret_cast<float *>( dataArray[0] );
                writeFloats( pSrc, vertexCount * ( includeNormals ? 6 : 3 ) );
            }
        }
        else
        {
            // No vertex buffer - write minimal data
            bool includeNormals = false;
            writeBools( &includeNormals, 1 );
        }
    }

    void MeshSerializerImpl::writePoseKeyframe( const AnimationPoseKeyFrame *kf )
    {
        writeChunkHeader( M_ANIMATION_POSE_KEYFRAME, calcPoseKeyframeSize( kf ) );
        // float time
        float timePos = kf->getTime();
        writeFloats( &timePos, 1 );

        // pose references - this would need to be implemented in IAnimationPoseKeyFrame
        // For now, we'll write empty pose references
        // TODO: Implement pose reference iteration in IAnimationPoseKeyFrame
    }

    void MeshSerializerImpl::writePoseKeyframePoseRef( u16 poseIndex, float influence )
    {
        writeChunkHeader( M_ANIMATION_POSE_REF, calcPoseKeyframePoseRefSize() );
        // unsigned short poseIndex
        writeShorts( &poseIndex, 1 );
        // float influence
        writeFloats( &influence, 1 );
    }

    void MeshSerializerImpl::writePoseKeyframePoseRef( const AnimationPoseKeyFrame::PoseRef &poseRef )
    {
        //	writeChunkHeader(M_ANIMATION_POSE_REF, calcPoseKeyframePoseRefSize());
        //	// unsigned short poseIndex
        //	writeShorts(&(poseRef.poseIndex), 1);
        //	// float influence
        //	writeFloats(&(poseRef.influence), 1);
    }

    void MeshSerializerImpl::readPoses( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // Find all substreams
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() && ( streamID == M_POSE ) )
            {
                switch( streamID )
                {
                case M_POSE:
                    readPose( stream, pMesh );
                    break;
                }

                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readPose( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // char* name (may be blank)
        String name = readString( stream );
        // unsigned short target
        unsigned short target;
        readShorts( stream, &target, 1 );

        // bool includesNormals
        bool includesNormals;
        readBools( stream, &includesNormals, 1 );

        auto pose = workphone::make_ptr<MeshPose>( target, name );
        pose->setIncludesNormals( includesNormals );

        // Find all substreams
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() && ( streamID == M_POSE_VERTEX ) )
            {
                switch( streamID )
                {
                case M_POSE_VERTEX:
                {
                    // create vertex offset
                    u32 vertIndex;
                    Vector3F offset, normal;
                    // unsigned long vertexIndex
                    readInts( stream, &vertIndex, 1 );
                    // float xoffset, yoffset, zoffset
                    readFloats( stream, offset.ptr(), 3 );

                    if( includesNormals )
                    {
                        readFloats( stream, normal.ptr(), 3 );
                        pose->addVertex( vertIndex, offset, normal );
                    }
                    else
                    {
                        pose->addVertex( vertIndex, offset );
                    }
                }
                break;
                }

                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }

        // Add the pose to the mesh
        pMesh->addPose( pose );
    }

    void MeshSerializerImpl::readAnimations( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // Find all substreams
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() && ( streamID == M_ANIMATION ) )
            {
                switch( streamID )
                {
                case M_ANIMATION:
                    readAnimation( stream, pMesh );
                    break;
                }

                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readAnimation( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // char* name
        String name = readString( stream );
        // float length
        float len;
        readFloats( stream, &len, 1 );

        SmartPtr<IAnimation> anim = pMesh->getAnimationInterface()->createAnimation( name, len );

        // tracks
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );

            // Optional base info is possible
            if( streamID == M_ANIMATION_BASEINFO )
            {
                // char baseAnimationName
                String baseAnimName = readString( stream );
                // float baseKeyFrameTime
                float baseKeyTime;
                readFloats( stream, &baseKeyTime, 1 );

                // anim->setUseBaseKeyFrame(true, baseKeyTime, baseAnimName);

                if( !stream->eof() )
                {
                    // Get next stream
                    streamID = readChunk( stream );
                }
            }

            while( !stream->eof() && streamID == M_ANIMATION_TRACK )
            {
                switch( streamID )
                {
                case M_ANIMATION_TRACK:
                    readAnimationTrack( stream, anim, pMesh );
                    break;
                }
                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readAnimationTrack( SmartPtr<IStream> &stream, SmartPtr<IAnimation> anim,
                                                 Mesh *pMesh )
    {
        // ushort type
        u16 inAnimType;
        readShorts( stream, &inAnimType, 1 );
        auto animType = static_cast<VertexAnimationType>( inAnimType );

        // unsigned short target
        u16 target;
        readShorts( stream, &target, 1 );

        SmartPtr<IAnimationVertexTrack> track;  // = anim->createVertexTrack(target,
        // pMesh->getVertexDataByTrackHandle(target), animType);

        // keyframes
        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() &&
                   ( streamID == M_ANIMATION_MORPH_KEYFRAME || streamID == M_ANIMATION_POSE_KEYFRAME ) )
            {
                switch( streamID )
                {
                case M_ANIMATION_MORPH_KEYFRAME:
                    readMorphKeyFrame( stream, track );
                    break;
                case M_ANIMATION_POSE_KEYFRAME:
                    readPoseKeyFrame( stream, track );
                    break;
                }
                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readMorphKeyFrame( SmartPtr<IStream> &stream,
                                                SmartPtr<IAnimationVertexTrack> track )
    {
        // float time
        float timePos;
        readFloats( stream, &timePos, 1 );

        // bool includesNormals
        bool includesNormals;
        readBools( stream, &includesNormals, 1 );

        auto kf = track->createVertexMorphKeyFrame( timePos );

        // Create buffer, allow read and use shadow buffer
        u32 vertexCount = track->getAssociatedVertexData()->getNumVertices();
        u32 vertexSize = sizeof( float ) * ( includesNormals ? 6 : 3 );

        // Create vertex buffer for the morph keyframe
        auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
        vertexBuffer->setNumVertices( vertexCount );

        // Create vertex data and read from stream
        void *pDst = vertexBuffer->createVertexData( 0 );
        readFloats( stream, static_cast<float *>( pDst ), vertexCount * ( includesNormals ? 6 : 3 ) );

        // Set the vertex buffer to the keyframe
        kf->setVertexBuffer( vertexBuffer );
    }

    void MeshSerializerImpl::readPoseKeyFrame( SmartPtr<IStream> &stream,
                                               SmartPtr<IAnimationVertexTrack> track )
    {
        // float time
        float timePos;
        readFloats( stream, &timePos, 1 );

        // Create keyframe
        auto kf = track->createVertexPoseKeyFrame( timePos );

        if( !stream->eof() )
        {
            unsigned short streamID = readChunk( stream );
            while( !stream->eof() && streamID == M_ANIMATION_POSE_REF )
            {
                switch( streamID )
                {
                case M_ANIMATION_POSE_REF:
                    u16 poseIndex;
                    float influence;
                    // unsigned short poseIndex
                    readShorts( stream, &poseIndex, 1 );
                    // float influence
                    readFloats( stream, &influence, 1 );

                    //kf->addPoseReference(poseIndex, influence);

                    break;
                }
                if( !stream->eof() )
                {
                    streamID = readChunk( stream );
                }
            }
            if( !stream->eof() )
            {
                // Backpedal back to start of stream
                stream->skip( -MSTREAM_OVERHEAD_SIZE );
            }
        }
    }

    void MeshSerializerImpl::readExtremes( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        unsigned short idx;
        readShorts( stream, &idx, 1 );

        auto subMeshes = pMesh->getSubMeshes();
        if( idx >= subMeshes.size() )
        {
            auto message =
                std::string( "Invalid submesh index in extreme points data: " ) + std::to_string( idx );
            WP_LOG_ERROR( message.c_str() );
            return;
        }

        auto sm = subMeshes[idx];

        int n_floats =
            ( m_currentStreamLen - MSTREAM_OVERHEAD_SIZE - sizeof( unsigned short ) ) / sizeof( float );

        if( ( n_floats % 3 ) != 0 )
        {
            WP_LOG_ERROR( "Invalid extreme points data - not a multiple of 3 floats" );
            return;
        }

        auto vert = new float[n_floats];
        readFloats( stream, vert, n_floats );

        // For now, we'll store the extreme points but not use them until we extend the SubMesh interface
        // In a future update, we should add methods like addExtremePoint() to ISubMesh
        Array<Vector3F> extremePoints;
        extremePoints.reserve( n_floats / 3 );

        for( int i = 0; i < n_floats; i += 3 )
        {
            extremePoints.push_back( Vector3F( vert[i], vert[i + 1], vert[i + 2] ) );
        }

        // TODO: Store extreme points in submesh when interface is extended
        // For now, we just log that we've read them
        auto message = "Read " + std::to_string( extremePoints.size() ) +
                       " extreme points for submesh " + std::to_string( idx );
        WP_LOG( message.c_str() );

        delete[] vert;
    }

    MeshSerializerImpl_v1_41::MeshSerializerImpl_v1_41()
    {
        // Version number
        m_version = "[MeshSerializer_v1.41]";
    }

    MeshSerializerImpl_v1_41::~MeshSerializerImpl_v1_41() = default;

    // void MeshSerializerImpl_v1_41::writeMorphKeyframe(const VertexMorphKeyFrame* kf, u32 vertexCount)
    //{
    //	//writeChunkHeader(M_ANIMATION_MORPH_KEYFRAME, calcMorphKeyframeSize(kf, vertexCount));
    //	//// float time
    //	//float timePos = kf->getTime();
    //	//writeFloats(&timePos, 1);
    //	//// float x,y,z			// repeat by number of vertices in original geometry
    //	//float* pSrc = static_cast<float*>(
    //	//	kf->getVertexBuffer()->lock(HardwareBuffer::HBL_READ_ONLY));
    //	//writeFloats(pSrc, vertexCount * 3);
    //	//kf->getVertexBuffer()->unlock();
    //}

    // void MeshSerializerImpl_v1_41::readMorphKeyFrame(SmartPtr<IStream>& stream, VertexAnimationTrack*
    // track)
    //{
    //	// float time
    //	float timePos;
    //	readFloats(stream, &timePos, 1);

    //	VertexMorphKeyFrame* kf = track->createVertexMorphKeyFrame(timePos);

    //	// Create buffer, allow read and use shadow buffer
    //	u32 vertexCount = track->getAssociatedVertexData()->vertexCount;
    //	HardwareVertexBufferSharedPtr vbuf =
    //		HardwareBufferManager::getSingleton().createVertexBuffer(
    //			VertexElement::getTypeSize(VET_FLOAT3), vertexCount,
    //			HardwareBuffer::HBU_STATIC, true);
    //	// float x,y,z			// repeat by number of vertices in original geometry
    //	float* pDst = static_cast<float*>(
    //		vbuf->lock(HardwareBuffer::HBL_DISCARD));
    //	readFloats(stream, pDst, vertexCount * 3);
    //	vbuf->unlock();
    //	kf->setVertexBuffer(vbuf);
    //}
    //
    // void MeshSerializerImpl_v1_41::writePose(const Pose* pose)
    //{
    //	writeChunkHeader(M_POSE, calcPoseSize(pose));

    //	// char* name (may be blank)
    //	writeString(pose->getName());

    //	// unsigned short target
    //	ushort target = pose->getTarget();`r`n        //	writeShorts(&target, 1);

    //	u32 vertexSize = calcPoseVertexSize();
    //	Pose::ConstVertexOffsetIterator vit = pose->getVertexOffsetIterator();
    //	while (vit.hasMoreElements())
    //	{
    //		u32 vertexIndex = (u32)vit.peekNextKey();
    //		Vector3 offset = vit.getNext();
    //		writeChunkHeader(M_POSE_VERTEX, vertexSize);
    //		// unsigned long vertexIndex
    //		writeInts(&vertexIndex, 1);
    //		// float xoffset, yoffset, zoffset
    //		writeFloats(offset.ptr(), 3);
    //	}
    //}
    //
    // void MeshSerializerImpl_v1_41::readPose(SmartPtr<IStream>& stream, Mesh* pMesh)
    //{
    //	// char* name (may be blank)
    //	String name = readString(stream);
    //	// unsigned short target
    //	unsigned short target;
    //	readShorts(stream, &target, 1);

    //	Pose* pose = pMesh->createPose(target, name);

    //	// Find all substreams
    //	if (!stream->eof())
    //	{
    //		unsigned short streamID = readChunk(stream);
    //		while(!stream->eof() &&
    //			(streamID == M_POSE_VERTEX))
    //		{
    //			switch(streamID)
    //			{
    //			case M_POSE_VERTEX:
    //				// create vertex offset
    //				u32 vertIndex;
    //				Vector3 offset;
    //				// unsigned long vertexIndex
    //				readInts(stream, &vertIndex, 1);
    //				// float xoffset, yoffset, zoffset
    //				readFloats(stream, offset.ptr(), 3);

    //				pose->addVertex(vertIndex, offset);
    //				break;

    //			}

    //			if (!stream->eof())
    //			{
    //				streamID = readChunk(stream);
    //			}

    //		}
    //		if (!stream->eof())
    //		{
    //			// Backpedal back to start of stream
    //			stream->skip(-MSTREAM_OVERHEAD_SIZE);
    //		}
    //	}
    //}
    //
    // u32 MeshSerializerImpl_v1_41::calcPoseSize(const Pose* pose)
    //{
    //	u32 size = MSTREAM_OVERHEAD_SIZE;

    //	// char* name (may be blank)
    //	size += pose->getName().length() + 1;
    //	// unsigned short target
    //	size += sizeof(u16);

    //	// vertex offsets
    //	size += pose->getVertexOffsets().size() * calcPoseVertexSize();

    //	return size;

    //}

    auto MeshSerializerImpl_v1_41::calcPoseVertexSize() -> u32
    {
        u32 size = MSTREAM_OVERHEAD_SIZE;
        // unsigned long vertexIndex
        size += sizeof( u32 );
        // float xoffset, yoffset, zoffset
        size += sizeof( float ) * 3;

        return size;
    }

    //
    // u32 MeshSerializerImpl_v1_41::calcMorphKeyframeSize(const VertexMorphKeyFrame* kf,
    //	u32 vertexCount)
    //{
    //	u32 size = MSTREAM_OVERHEAD_SIZE;
    //	// float time
    //	size += sizeof(float);
    //	// float x,y,z
    //	size += sizeof(float) * 3 * vertexCount;

    //	return size;
    //}

    MeshSerializerImpl_v1_4::MeshSerializerImpl_v1_4()
    {
        // Version number
        m_version = "[MeshSerializer_v1.40]";
    }

    MeshSerializerImpl_v1_4::~MeshSerializerImpl_v1_4() = default;

    //
    //  void MeshSerializerImpl_v1_4::writeLodSummary(unsigned short numLevels, bool manual, const
    //  LodStrategy *strategy)
    //  {
    //      // Header
    //      u32 size = MSTREAM_OVERHEAD_SIZE;
    //      // unsigned short numLevels;
    //      size += sizeof(unsigned short);
    //      // bool manual;  (true for manual alternate meshes, false for generated)
    //      size += sizeof(bool);
    //      writeChunkHeader(M_MESH_LOD, size);

    //      // Details
    //      // unsigned short numLevels;
    //      writeShorts(&numLevels, 1);
    //      // bool manual;  (true for manual alternate meshes, false for generated)
    //      writeBools(&manual, 1);

    //  }
    //
    //  void MeshSerializerImpl_v1_4::writeLodUsageManual(const MeshLodUsage& usage)
    //  {
    //      // Header
    //      u32 size = MSTREAM_OVERHEAD_SIZE;
    //      u32 manualSize = MSTREAM_OVERHEAD_SIZE;
    //      // float fromDepthSquared;
    //      size += sizeof(float);
    //      // Manual part size
    //
    //      // String manualMeshName;
    //      manualSize += usage.manualName.length() + 1;
    //
    //      size += manualSize;
    //
    //      writeChunkHeader(M_MESH_LOD_USAGE, size);
    //// Main difference to later version here is that we use 'value' (squared depth)
    //// rather than 'userValue' which is just depth
    //      writeFloats(&(usage.value), 1);
    //
    //      writeChunkHeader(M_MESH_LOD_MANUAL, manualSize);
    //      writeString(usage.manualName);
    //
    //
    //  }
    //
    //  void MeshSerializerImpl_v1_4::writeLodUsageGenerated(const Mesh* pMesh, const MeshLodUsage&
    //  usage,
    //											unsigned short lodNum)
    //  {
    //// Usage Header
    //      u32 size = MSTREAM_OVERHEAD_SIZE;
    // unsigned short subidx;
    //
    //      // float fromDepthSquared;
    //      size += sizeof(float);
    //
    //      // Calc generated SubMesh sections size
    // for(subidx = 0; subidx < pMesh->getNumSubMeshes(); ++subidx)
    //{
    //	// header
    //	size += MSTREAM_OVERHEAD_SIZE;
    //	// unsigned int numFaces;
    //	size += sizeof(unsigned int);
    //	SubMesh* sm = pMesh->getSubMesh(subidx);
    //          const IndexData* indexData = sm->mLodFaceList[lodNum - 1];
    //
    //          // bool indexes32Bit
    //	size += sizeof(bool);
    //	// unsigned short*/int* faceIndexes;
    //          if (!indexData->indexBuffer.isNull() &&
    //		indexData->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT)
    //          {
    //	    size += static_cast<unsigned long>(
    //										   sizeof(unsigned int) * indexData->indexCount);
    //          }
    //          else
    //          {
    //	    size += static_cast<unsigned long>(
    //										   sizeof(unsigned short) * indexData->indexCount);
    //          }
    //
    //}
    //
    //      writeChunkHeader(M_MESH_LOD_USAGE, size);
    //// Main difference to later version here is that we use 'value' (squared depth)
    //// rather than 'userValue' which is just depth
    //      writeFloats(&(usage.value), 1);
    //
    //// Now write sections
    //      // Calc generated SubMesh sections size
    // for(subidx = 0; subidx < pMesh->getNumSubMeshes(); ++subidx)
    //{
    //	size = MSTREAM_OVERHEAD_SIZE;
    //	// unsigned int numFaces;
    //	size += sizeof(unsigned int);
    //	SubMesh* sm = pMesh->getSubMesh(subidx);
    //          const IndexData* indexData = sm->mLodFaceList[lodNum - 1];
    //          // bool indexes32Bit
    //	size += sizeof(bool);
    //	// Lock index buffer to write
    //	HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
    //	// bool indexes32bit
    //	bool idx32 = (!ibuf.isNull() && ibuf->getType() == HardwareIndexBuffer::IT_32BIT);
    //	// unsigned short*/int* faceIndexes;
    //          if (idx32)
    //          {
    //	    size += static_cast<unsigned long>(
    //										   sizeof(unsigned int) * indexData->indexCount);
    //          }
    //          else
    //          {
    //	    size += static_cast<unsigned long>(
    //										   sizeof(unsigned short) * indexData->indexCount);
    //          }
    //
    //	writeChunkHeader(M_MESH_LOD_GENERATED, size);
    //	unsigned int idxCount = static_cast<unsigned int>(indexData->indexCount);
    //	writeInts(&idxCount, 1);
    //	writeBools(&idx32, 1);
    //
    //	if (idxCount > 0)
    //	{
    //		if (idx32)
    //		{
    //			unsigned int* pIdx = static_cast<unsigned int*>(
    //															ibuf->lock(HardwareBuffer::HBL_READ_ONLY));
    //			writeInts(pIdx, indexData->indexCount);
    //			ibuf->unlock();
    //		}
    //		else
    //		{
    //			unsigned short* pIdx = static_cast<unsigned short*>(
    //																ibuf->lock(HardwareBuffer::HBL_READ_ONLY));
    //			writeShorts(pIdx, indexData->indexCount);
    //			ibuf->unlock();
    //		}
    //	}
    //}
    //
    //
    //  }

    void MeshSerializerImpl_v1_4::readMeshLodInfo( SmartPtr<IStream> &stream, Mesh *pMesh )
    {
        // unsigned short streamID, i;

        //// Use the old strategy for this mesh
        // LodStrategy *strategy = DistanceLodStrategy::getSingletonPtr();
        // pMesh->setLodStrategy(strategy);

        //// unsigned short numLevels;
        // readShorts(stream, &(pMesh->mNumLods), 1);
        //// bool manual;  (true for manual alternate meshes, false for generated)
        // readBools(stream, &(pMesh->mIsLodManual), 1);

        //// Preallocate submesh lod face data if not manual
        // if (!pMesh->mIsLodManual)
        //{
        //     unsigned short numsubs = pMesh->getNumSubMeshes();
        //     for (i = 0; i < numsubs; ++i)
        //     {
        //         SubMesh* sm = pMesh->getSubMesh(i);
        //         sm->mLodFaceList.resize(pMesh->mNumLods-1);
        //     }
        // }

        //// Loop from 1 rather than 0 (full detail index is not in file)
        // for (i = 1; i < pMesh->mNumLods; ++i)
        //{
        //     streamID = readChunk(stream);
        //     if (streamID != M_MESH_LOD_USAGE)
        //     {
        //         OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
        //             "Missing M_MESH_LOD_USAGE stream in " + pMesh->getName(),
        //             "MeshSerializerImpl::readMeshLodInfo");
        //     }
        //     // Read depth
        //     MeshLodUsage usage;
        //     readFloats(stream, &(usage.value), 1);
        //     usage.userValue = Math::Sqrt(usage.value);

        //    if (pMesh->isLodManual())
        //    {
        //        readMeshLodUsageManual(stream, pMesh, i, usage);
        //    }
        //    else //(!pMesh->isLodManual)
        //    {
        //        readMeshLodUsageGenerated(stream, pMesh, i, usage);
        //    }
        //    usage.edgeData = NULL;

        //    // Save usage
        //    pMesh->mMeshLodUsageList.push_back(usage);
        //}
    }

    MeshSerializerImpl_v1_3::MeshSerializerImpl_v1_3()
    {
        // Version number
        m_version = "[MeshSerializer_v1.30]";
    }

    MeshSerializerImpl_v1_3::~MeshSerializerImpl_v1_3() = default;

    //
    // void MeshSerializerImpl_v1_3::readEdgeListLodInfo(SmartPtr<IStream>& stream,
    //     EdgeData* edgeData)
    //{
    //     // unsigned long numTriangles
    //     u32 numTriangles;
    //     readInts(stream, &numTriangles, 1);
    //     // Allocate correct amount of memory
    //     edgeData->triangles.resize(numTriangles);
    //     edgeData->triangleFaceNormals.resize(numTriangles);
    //     edgeData->triangleLightFacings.resize(numTriangles);
    //     // unsigned long numEdgeGroups
    //     u32 numEdgeGroups;
    //     readInts(stream, &numEdgeGroups, 1);
    //     // Allocate correct amount of memory
    //     edgeData->edgeGroups.resize(numEdgeGroups);
    //     // Triangle* triangleList
    //     u32 tmp[3];
    //     for (u32 t = 0; t < numTriangles; ++t)
    //     {
    //         EdgeData::Triangle& tri = edgeData->triangles[t];
    //         // unsigned long indexSet
    //         readInts(stream, tmp, 1);
    //         tri.indexSet = tmp[0];
    //         // unsigned long vertexSet
    //         readInts(stream, tmp, 1);
    //         tri.vertexSet = tmp[0];
    //         // unsigned long vertIndex[3]
    //         readInts(stream, tmp, 3);
    //         tri.vertIndex[0] = tmp[0];
    //         tri.vertIndex[1] = tmp[1];
    //         tri.vertIndex[2] = tmp[2];
    //         // unsigned long sharedVertIndex[3]
    //         readInts(stream, tmp, 3);
    //         tri.sharedVertIndex[0] = tmp[0];
    //         tri.sharedVertIndex[1] = tmp[1];
    //         tri.sharedVertIndex[2] = tmp[2];
    //         // float normal[4]
    //         readFloats(stream, &(edgeData->triangleFaceNormals[t].x), 4);

    //    }

    //    // Assume the mesh is closed, it will update later
    //    edgeData->isClosed = true;

    //    for (u32 eg = 0; eg < numEdgeGroups; ++eg)
    //    {
    //        unsigned short streamID = readChunk(stream);
    //        if (streamID != M_EDGE_GROUP)
    //        {
    //            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
    //                "Missing M_EDGE_GROUP stream",
    //                "MeshSerializerImpl_v1_3::readEdgeListLodInfo");
    //        }
    //        EdgeData::EdgeGroup& edgeGroup = edgeData->edgeGroups[eg];

    //        // unsigned long vertexSet
    //        readInts(stream, tmp, 1);
    //        edgeGroup.vertexSet = tmp[0];
    //        // unsigned long numEdges
    //        u32 numEdges;
    //        readInts(stream, &numEdges, 1);
    //        edgeGroup.edges.resize(numEdges);
    //        // Edge* edgeList
    //        for (u32 e = 0; e < numEdges; ++e)
    //        {
    //            EdgeData::Edge& edge = edgeGroup.edges[e];
    //            // unsigned long  triIndex[2]
    //            readInts(stream, tmp, 2);
    //            edge.triIndex[0] = tmp[0];
    //            edge.triIndex[1] = tmp[1];
    //            // unsigned long  vertIndex[2]
    //            readInts(stream, tmp, 2);
    //            edge.vertIndex[0] = tmp[0];
    //            edge.vertIndex[1] = tmp[1];
    //            // unsigned long  sharedVertIndex[2]
    //            readInts(stream, tmp, 2);
    //            edge.sharedVertIndex[0] = tmp[0];
    //            edge.sharedVertIndex[1] = tmp[1];
    //            // bool degenerate
    //            readBools(stream, &(edge.degenerate), 1);

    //            // The mesh is closed only if no degenerate edge here
    //            if (edge.degenerate)
    //            {
    //                edgeData->isClosed = false;
    //            }
    //        }
    //    }

    //    reorganiseTriangles(edgeData);
    //}
    //
    // void MeshSerializerImpl_v1_3::reorganiseTriangles(EdgeData* edgeData)
    //{
    //     u32 numTriangles = edgeData->triangles.size();

    //    if (edgeData->edgeGroups.size() == 1)
    //    {
    //        // Special case for only one edge group in the edge list, which occurring
    //        // most time. In this case, all triangles belongs to that group.
    //        edgeData->edgeGroups.front().triStart = 0;
    //        edgeData->edgeGroups.front().triCount = numTriangles;
    //    }
    //    else
    //    {
    //        EdgeData::EdgeGroupList::iterator egi, egend;
    //        egend = edgeData->edgeGroups.end();

    //        // Calculate number of triangles for edge groups

    //        for (egi = edgeData->edgeGroups.begin(); egi != egend; ++egi)
    //        {
    //            egi->triStart = 0;
    //            egi->triCount = 0;
    //        }

    //        bool isGrouped = true;
    //        EdgeData::EdgeGroup* lastEdgeGroup = 0;
    //        for (u32 t = 0; t < numTriangles; ++t)
    //        {
    //            // Gets the edge group that the triangle belongs to
    //            const EdgeData::Triangle& tri = edgeData->triangles[t];
    //            EdgeData::EdgeGroup* edgeGroup = &edgeData->edgeGroups[tri.vertexSet];

    //            // Does edge group changes from last edge group?
    //            if (isGrouped && edgeGroup != lastEdgeGroup)
    //            {
    //                // Remember last edge group
    //                lastEdgeGroup = edgeGroup;

    //                // Is't first time encounter this edge group?
    //                if (!edgeGroup->triCount && !edgeGroup->triStart)
    //                {
    //                    // setup first triangle of this edge group
    //                    edgeGroup->triStart = t;
    //                }
    //                else
    //                {
    //                    // original triangles doesn't grouping by edge group
    //                    isGrouped = false;
    //                }
    //            }

    //            // Count number of triangles for this edge group
    //            if(edgeGroup)
    //                ++edgeGroup->triCount;
    //        }

    //        //
    //        // Note that triangles has been sorted by vertex set for a long time,
    //        // but never stored to old version mesh file.
    //        //
    //        // Adopt this fact to avoid remap triangles here.
    //        //

    //        // Does triangles grouped by vertex set?
    //        if (!isGrouped)
    //        {
    //            // Ok, the triangles of this edge list isn't grouped by vertex set
    //            // perfectly, seems ancient mesh file.
    //            //
    //            // We need work hardly to group triangles by vertex set.
    //            //

    //            // Calculate triStart and reset triCount to zero for each edge group first
    //            u32 triStart = 0;
    //            for (egi = edgeData->edgeGroups.begin(); egi != egend; ++egi)
    //            {
    //                egi->triStart = triStart;
    //                triStart += egi->triCount;
    //                egi->triCount = 0;
    //            }

    //            // The map used to mapping original triangle index to new index
    //            typedef vector<u32>::type TriangleIndexRemap;
    //            TriangleIndexRemap triangleIndexRemap(numTriangles);

    //            // New triangles information that should be group by vertex set.
    //            EdgeData::TriangleList newTriangles(numTriangles);
    //            EdgeData::TriangleFaceNormalList newTriangleFaceNormals(numTriangles);

    //            // Calculate triangle index map and organise triangles information
    //            for (u32 t = 0; t < numTriangles; ++t)
    //            {
    //                // Gets the edge group that the triangle belongs to
    //                const EdgeData::Triangle& tri = edgeData->triangles[t];
    //                EdgeData::EdgeGroup& edgeGroup = edgeData->edgeGroups[tri.vertexSet];

    //                // Calculate new index
    //                u32 newIndex = edgeGroup.triStart + edgeGroup.triCount;
    //                ++edgeGroup.triCount;

    //                // Setup triangle index mapping entry
    //                triangleIndexRemap[t] = newIndex;

    //                // Copy triangle info to new placement
    //                newTriangles[newIndex] = tri;
    //                newTriangleFaceNormals[newIndex] = edgeData->triangleFaceNormals[t];
    //            }

    //            // Replace with new triangles information
    //            edgeData->triangles.swap(newTriangles);
    //            edgeData->triangleFaceNormals.swap(newTriangleFaceNormals);

    //            // Now, update old triangle indices to new index
    //            for (egi = edgeData->edgeGroups.begin(); egi != egend; ++egi)
    //            {
    //                EdgeData::EdgeList::iterator ei, eend;
    //                eend = egi->edges.end();
    //                for (ei = egi->edges.begin(); ei != eend; ++ei)
    //                {
    //                    ei->triIndex[0] = triangleIndexRemap[ei->triIndex[0]];
    //                    if (!ei->degenerate)
    //                    {
    //                        ei->triIndex[1] = triangleIndexRemap[ei->triIndex[1]];
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}

    // void MeshSerializerImpl_v1_3::writeEdgeList(const Mesh* pMesh)
    //{
    //  writeChunkHeader(M_EDGE_LISTS, calcEdgeListSize(pMesh));

    //  for (ushort i = 0; i < pMesh->getNumLodLevels(); ++i)
    //  {
    //      const EdgeData* edgeData = pMesh->getEdgeList(i);
    //      bool isManual = pMesh->isLodManual() && (i > 0);
    //      writeChunkHeader(M_EDGE_LIST_LOD, calcEdgeListLodSize(edgeData, isManual));

    //      // unsigned short lodIndex
    //      writeShorts(&i, 1);

    //      // bool isManual			// If manual, no edge data here, loaded from manual mesh
    //      writeBools(&isManual, 1);
    //      if (!isManual)
    //      {
    //          // unsigned long  numTriangles
    //          u32 count = static_cast<u32>(edgeData->triangles.size());
    //          writeInts(&count, 1);
    //          // unsigned long numEdgeGroups
    //          count = static_cast<u32>(edgeData->edgeGroups.size());
    //          writeInts(&count, 1);
    //          // Triangle* triangleList
    //          // Iterate rather than writing en-masse to allow endian conversion
    //          EdgeData::TriangleList::const_iterator t = edgeData->triangles.begin();
    //          EdgeData::TriangleFaceNormalList::const_iterator fni =
    //          edgeData->triangleFaceNormals.begin(); for ( ; t != edgeData->triangles.end(); ++t,
    //          ++fni)
    //          {
    //              const EdgeData::Triangle& tri = *t;
    //              // unsigned long indexSet;
    //              u32 tmp[3];
    //              tmp[0] = tri.indexSet;
    //              writeInts(tmp, 1);
    //              // unsigned long vertexSet;
    //              tmp[0] = tri.vertexSet;
    //              writeInts(tmp, 1);
    //              // unsigned long vertIndex[3];
    //              tmp[0] = tri.vertIndex[0];
    //              tmp[1] = tri.vertIndex[1];
    //              tmp[2] = tri.vertIndex[2];
    //              writeInts(tmp, 3);
    //              // unsigned long sharedVertIndex[3];
    //              tmp[0] = tri.sharedVertIndex[0];
    //              tmp[1] = tri.sharedVertIndex[1];
    //              tmp[2] = tri.sharedVertIndex[2];
    //              writeInts(tmp, 3);
    //              // float normal[4];
    //              writeFloats(&(fni->x), 4);

    //          }
    //          // Write the groups
    //          for (EdgeData::EdgeGroupList::const_iterator gi = edgeData->edgeGroups.begin();
    // gi != edgeData->edgeGroups.end(); ++gi)
    //          {
    //              const EdgeData::EdgeGroup& edgeGroup = *gi;
    //              writeChunkHeader(M_EDGE_GROUP, calcEdgeGroupSize(edgeGroup));
    //              // unsigned long vertexSet
    //              u32 vertexSet = static_cast<u32>(edgeGroup.vertexSet);
    //              writeInts(&vertexSet, 1);
    //              // unsigned long numEdges
    //              count = static_cast<u32>(edgeGroup.edges.size());
    //              writeInts(&count, 1);
    //              // Edge* edgeList
    //              // Iterate rather than writing en-masse to allow endian conversion
    //              for (EdgeData::EdgeList::const_iterator ei = edgeGroup.edges.begin();
    // ei != edgeGroup.edges.end(); ++ei)
    //              {
    //                  const EdgeData::Edge& edge = *ei;
    //                  u32 tmp[2];
    //                  // unsigned long  triIndex[2]
    //                  tmp[0] = edge.triIndex[0];
    //                  tmp[1] = edge.triIndex[1];
    //                  writeInts(tmp, 2);
    //                  // unsigned long  vertIndex[2]
    //                  tmp[0] = edge.vertIndex[0];
    //                  tmp[1] = edge.vertIndex[1];
    //                  writeInts(tmp, 2);
    //                  // unsigned long  sharedVertIndex[2]
    //                  tmp[0] = edge.sharedVertIndex[0];
    //                  tmp[1] = edge.sharedVertIndex[1];
    //                  writeInts(tmp, 2);
    //                  // bool degenerate
    //                  writeBools(&(edge.degenerate), 1);
    //              }

    //          }

    //      }

    //  }
    //}

    MeshSerializerImpl_v1_2::MeshSerializerImpl_v1_2()
    {
        // Version number
        m_version = "[MeshSerializer_v1.20]";
    }

    MeshSerializerImpl_v1_2::~MeshSerializerImpl_v1_2() = default;

    void MeshSerializerImpl_v1_2::readMesh( SmartPtr<IStream> &stream, Mesh *pMesh,
                                            MeshSerializerListener *listener )
    {
        MeshSerializerImpl::readMesh( stream, pMesh, listener );
        // Always automatically build edge lists for this version
        // pMesh->mAutoBuildEdgeLists = true;
    }

    void MeshSerializerImpl_v1_2::readGeometry( SmartPtr<IStream> &stream, Mesh *pMesh,
                                                VertexBuffer *dest )
    {
        // unsigned short bindIdx = 0;

        // dest->vertexStart = 0;

        // unsigned int vertexCount = 0;
        // readInts(stream, &vertexCount, 1);
        // dest->vertexCount = vertexCount;

        //// Vertex buffers

        // readGeometryPositions(bindIdx, stream, pMesh, dest);
        //++bindIdx;

        //// Find optional geometry streams
        // if (!stream->eof())
        //{
        //     unsigned short streamID = readChunk(stream);
        //     unsigned short texCoordSet = 0;
        //
        //     while(!stream->eof() &&
        //         (streamID == M_GEOMETRY_NORMALS ||
        //          streamID == M_GEOMETRY_COLOURS ||
        //          streamID == M_GEOMETRY_TEXCOORDS ))
        //     {
        //         switch (streamID)
        //         {
        //         case M_GEOMETRY_NORMALS:
        //             readGeometryNormals(bindIdx++, stream, pMesh, dest);
        //             break;
        //         case M_GEOMETRY_COLOURS:
        //             readGeometryColours(bindIdx++, stream, pMesh, dest);
        //             break;
        //         case M_GEOMETRY_TEXCOORDS:
        //             readGeometryTexCoords(bindIdx++, stream, pMesh, dest, texCoordSet++);
        //             break;
        //         }
        //         // Get next stream
        //         if (!stream->eof())
        //         {
        //             streamID = readChunk(stream);
        //         }
        //     }
        //     if (!stream->eof())
        //     {
        //         // Backpedal back to start of non-submesh stream
        //         stream->skip(-MSTREAM_OVERHEAD_SIZE);
        //     }
        // }
    }

    void MeshSerializerImpl_v1_2::readGeometryPositions( unsigned short bindIdx,
                                                         SmartPtr<IStream> &stream, Mesh *pMesh,
                                                         VertexBuffer *dest )
    {
        //     float *pFloat = 0;
        //     HardwareVertexBufferSharedPtr vbuf;
        //     // float* pVertices (x, y, z order x numVertices)
        //     dest->vertexDeclaration->addElement(bindIdx, 0, VertexDeclaration::VET_FLOAT3,
        //     VertexDeclaration::VES_POSITION); vbuf =
        //     HardwareBufferManager::getSingleton().createVertexBuffer(
        //         dest->vertexDeclaration->getVertexSize(bindIdx),
        //         dest->vertexCount,
        //         pMesh->mVertexBufferUsage,
        // pMesh->mVertexBufferShadowBuffer);
        //     pFloat = static_cast<float*>(
        //         vbuf->lock(HardwareBuffer::HBL_DISCARD));
        //     readFloats(stream, pFloat, dest->vertexCount * 3);
        //     vbuf->unlock();
        //     dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }

    void MeshSerializerImpl_v1_2::readGeometryNormals( unsigned short bindIdx, SmartPtr<IStream> &stream,
                                                       Mesh *pMesh, VertexBuffer *dest )
    {
        //     float *pFloat = 0;
        //     HardwareVertexBufferSharedPtr vbuf;
        //     // float* pNormals (x, y, z order x numVertices)
        //     dest->vertexDeclaration->addElement(bindIdx, 0, VertexDeclaration::VET_FLOAT3,
        //     VertexDeclaration::VES_NORMAL); vbuf =
        //     HardwareBufferManager::getSingleton().createVertexBuffer(
        //         dest->vertexDeclaration->getVertexSize(bindIdx),
        //         dest->vertexCount,
        //         pMesh->mVertexBufferUsage,
        // pMesh->mVertexBufferShadowBuffer);
        //     pFloat = static_cast<float*>(
        //         vbuf->lock(HardwareBuffer::HBL_DISCARD));
        //     readFloats(stream, pFloat, dest->vertexCount * 3);
        //     vbuf->unlock();
        //     dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }

    void MeshSerializerImpl_v1_2::readGeometryColours( unsigned short bindIdx, SmartPtr<IStream> &stream,
                                                       Mesh *pMesh, VertexBuffer *dest )
    {
        //     RGBA* pRGBA = 0;
        //     HardwareVertexBufferSharedPtr vbuf;
        //     // unsigned long* pColours (RGBA 8888 format x numVertices)
        //     dest->vertexDeclaration->addElement(bindIdx, 0, VertexDeclaration::VET_COLOUR,
        //     VertexDeclaration::VES_DIFFUSE); vbuf =
        //     HardwareBufferManager::getSingleton().createVertexBuffer(
        //         dest->vertexDeclaration->getVertexSize(bindIdx),
        //         dest->vertexCount,
        //         pMesh->mVertexBufferUsage,
        // pMesh->mVertexBufferShadowBuffer);
        //     pRGBA = static_cast<RGBA*>(
        //         vbuf->lock(HardwareBuffer::HBL_DISCARD));
        //     readInts(stream, pRGBA, dest->vertexCount);
        //     vbuf->unlock();
        //     dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }

    void MeshSerializerImpl_v1_2::readGeometryTexCoords( unsigned short bindIdx,
                                                         SmartPtr<IStream> &stream, Mesh *pMesh,
                                                         VertexBuffer *dest, unsigned short texCoordSet )
    {
        //     float *pFloat = 0;
        //     HardwareVertexBufferSharedPtr vbuf;
        //     // unsigned short dimensions    (1 for 1D, 2 for 2D, 3 for 3D)
        //     unsigned short dim;
        //     readShorts(stream, &dim, 1);
        //     // float* pTexCoords  (u [v] [w] order, dimensions x numVertices)
        //     dest->vertexDeclaration->addElement(
        //         bindIdx,
        //         0,
        //         VertexElement::multiplyTypeCount(VET_FLOAT1, dim),
        //         VertexDeclaration::VES_TEXTURE_COORDINATES,
        //         texCoordSet);
        //     vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
        //         dest->vertexDeclaration->getVertexSize(bindIdx),
        //         dest->vertexCount,
        //         pMesh->mVertexBufferUsage,
        // pMesh->mVertexBufferShadowBuffer);
        //     pFloat = static_cast<float*>(
        //         vbuf->lock(HardwareBuffer::HBL_DISCARD));
        //     readFloats(stream, pFloat, dest->vertexCount * dim);
        //     vbuf->unlock();
        //     dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }

    MeshSerializerImpl_v1_1::MeshSerializerImpl_v1_1()
    {
        // Version number
        m_version = "[MeshSerializer_v1.10]";
    }

    MeshSerializerImpl_v1_1::~MeshSerializerImpl_v1_1() = default;

    void MeshSerializerImpl_v1_1::readGeometryTexCoords( unsigned short bindIdx,
                                                         SmartPtr<IStream> &stream, Mesh *pMesh,
                                                         VertexBuffer *dest, unsigned short texCoordSet )
    {
        //     float *pFloat = 0;
        //     HardwareVertexBufferSharedPtr vbuf;
        //     // unsigned short dimensions    (1 for 1D, 2 for 2D, 3 for 3D)
        //     unsigned short dim;
        //     readShorts(stream, &dim, 1);
        //     // float* pTexCoords  (u [v] [w] order, dimensions x numVertices)
        //     dest->vertexDeclaration->addElement(
        //         bindIdx,
        //         0,
        //         VertexElement::multiplyTypeCount(VET_FLOAT1, dim),
        //         VertexDeclaration::VES_TEXTURE_COORDINATES,
        //         texCoordSet);
        //     vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
        //         dest->vertexDeclaration->getVertexSize(bindIdx),
        //         dest->vertexCount,
        //         pMesh->getVertexBufferUsage(),
        // pMesh->isVertexBufferShadowed());
        //     pFloat = static_cast<float*>(
        //         vbuf->lock(HardwareBuffer::HBL_DISCARD));
        //     readFloats(stream, pFloat, dest->vertexCount * dim);

        //     // Adjust individual v values to (1 - v)
        //     if (dim == 2)
        //     {
        //         for (u32 i = 0; i < dest->vertexCount; ++i)
        //         {
        //             ++pFloat; // skip u
        //             *pFloat = 1.0f - *pFloat; // v = 1 - v
        //             ++pFloat;
        //         }

        //     }
        //     vbuf->unlock();
        // dest->vertexBufferBinding->setBinding(bindIdx, vbuf);
    }

}  // namespace workphone
