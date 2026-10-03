#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <cstdio>
#include <Ogre.h>
#include <OgreFileSystem.h>
#include <OgreImage2.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreTextureBox.h>
#include <OgreLwString.h>
#include <fstream>

//#if OGRE_NO_FREEIMAGE == 0
//#    include "OgreFreeImageCodec2.h"
//#endif
//#if OGRE_NO_DDS_CODEC == 0
//#    include "OgreDDSCodec2.h"
//#endif
//#if OGRE_NO_STBI_CODEC == 0
//#    include "OgreSTBICodec.h"
//#endif
//#include "OgreOITDCodec.hpp"
//#if OGRE_NO_PVRTC_CODEC == 0
//#    include "OgrePVRTCCodec.h"
//#endif
//#if OGRE_NO_ETC_CODEC == 0
//#    include "OgreETCCodec.h"
//#endif
//#if OGRE_NO_ASTC_CODEC == 0
//#    include "OgreASTCCodec.h"
//#endif

namespace workphone::render
{

    auto OgreUtil::toString( const String &str ) -> Ogre::String
    {
        if( str.empty() )
        {
            return {};
        }

        return Ogre::String( str.c_str(), str.size() );
    }

    auto OgreUtil::toString( const StringW &str ) -> Ogre::String
    {
        try
        {
            auto s = StringUtil::toUTF16to8( str );
            return Ogre::String( s.c_str(), s.size() );
        }
        catch( const std::exception &e )
        {
            WP_LOG_ERROR( "OgreUtil::toString(StringW): Exception during string conversion." );
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto OgreUtil::equals( const Ogre::Vector3 &a, const Ogre::Vector3 &b ) -> bool
    {
        return Math<real_Num>::equals( a.x, b.x ) && Math<real_Num>::equals( a.y, b.y ) &&
               Math<real_Num>::equals( a.z, b.z );
    }

    auto OgreUtil::equals( const Ogre::ColourValue &a, const Ogre::ColourValue &b ) -> bool
    {
        return Math<real_Num>::equals( a.r, b.r ) && Math<real_Num>::equals( a.g, b.g ) &&
               Math<real_Num>::equals( a.b, b.b ) && Math<real_Num>::equals( a.a, b.a );
    }

    auto OgreUtil::equals( const Ogre::Quaternion &a, const Ogre::Quaternion &b ) -> bool
    {
        return Math<real_Num>::equals( a.x, b.x ) && Math<real_Num>::equals( a.y, b.y ) &&
               Math<real_Num>::equals( a.z, b.z ) && Math<real_Num>::equals( a.w, b.w );
    }

    auto OgreUtil::convertToOgre( const ColourF &c ) -> Ogre::ColourValue
    {
        return Ogre::ColourValue( c.r, c.g, c.b, c.a );
    }

    auto OgreUtil::convert( const Ogre::AxisAlignedBox &box ) -> AABB3F
    {
        const Ogre::Vector3 &minPoint = box.getMinimum();
        const Ogre::Vector3 &maxPoint = box.getMaximum();

        return { minPoint.x, minPoint.y, minPoint.z, maxPoint.x, maxPoint.y, maxPoint.z };
    }

    auto OgreUtil::convert( const Ogre::Quaternion &q ) -> QuaternionF
    {
        return { q.w, q.x, q.y, q.z };
    }

    auto OgreUtil::convert( const Ogre::Vector3 &vector ) -> Vector3F
    {
        return { vector.x, vector.y, vector.z };
    }

    auto OgreUtil::convert( const Ogre::ColourValue &c ) -> ColourF
    {
        return { c.r, c.g, c.b, c.a };
    }

    auto OgreUtil::convertToOgre( const QuaternionF &q ) -> Ogre::Quaternion
    {
        return { static_cast<Ogre::Real>( q.W() ), static_cast<Ogre::Real>( q.X() ),
                 static_cast<Ogre::Real>( q.Y() ), static_cast<Ogre::Real>( q.Z() ) };
    }

    auto OgreUtil::getDerivedPosition( Ogre::SceneNode *sceneNode ) -> Ogre::Vector3
    {
        Ogre::Vector3 position = sceneNode->getPosition();

        Ogre::SceneNode *prevParent = sceneNode;
        Ogre::SceneNode *parent = sceneNode->getParentSceneNode();
        while( parent )
        {
            position += prevParent->getOrientation() * parent->getPosition();

            prevParent = parent;
            parent = parent->getParentSceneNode();
        }

        return position;
    }

    auto OgreUtil::getDerivedBoundingBox( Ogre::SceneNode *node ) -> Ogre::AxisAlignedBox
    {
        Ogre::AxisAlignedBox box;
        // box.setExtents(Ogre::Vector3::ZERO, Ogre::Vector3::ZERO);

        // for (u32 i = 0; i < node->numAttachedObjects(); ++i)
        //{
        //	Ogre::MovableObject* object = node->getAttachedObject(i);
        //	if (object->getTypeFlags() & Ogre::SceneManager::ENTITY_TYPE_MASK)
        //	{
        //		Ogre::Entity* entity = (Ogre::Entity*)object;

        //		Ogre::Vector3 minPoint(1e10, 1e10, 1e10);
        //		Ogre::Vector3 maxPoint(-1e10, -1e10, -1e10);
        //		getVertexMinMax(entity, minPoint, maxPoint);

        //		Ogre::Vector3 derivedPos = getDerivedPosition(node);
        //		Ogre::AxisAlignedBox entityBox;// = entity->getBoundingBox();
        //		entityBox.scale(node->getScale());

        //		entityBox.setMinimum(minPoint + derivedPos);
        //		entityBox.setMaximum(maxPoint + derivedPos);

        //		box.merge(entityBox);
        //	}
        //}

        // Ogre::Node::ChildNodeIterator node_it = node->getChildIterator();
        // while (node_it.hasMoreElements())
        //{
        //	Ogre::SceneNode* child = static_cast<Ogre::SceneNode*>(node_it.getNext());
        //	box.merge(getDerivedBoundingBox(child));
        // }

        return box;
    }

    void OgreUtil::getVertexMinMax( Ogre::Entity *entity, Ogre::Vector3 &minPoint,
                                    Ogre::Vector3 &maxPoint )
    {
        //	Ogre::MeshPtr mesh = entity->getMesh();
        //	for (u32 i = 0; i < entity->getNumSubEntities(); ++i)
        //{
        //		Ogre::SubEntity* subEntity = entity->getSubEntity(i);
        //		Ogre::SubMesh* subMesh = subEntity->getSubMesh();

        //		Ogre::VertexData* data = nullptr;

        //		if (subMesh->useSharedVertices)
        //			data = mesh->sharedVertexData;
        //		else
        //			data = subMesh->vertexData;

        //	const Ogre::VertexElement* posElem =
        //data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
        //	Ogre::HardwareVertexBufferSharedPtr vbuf =
        //data->vertexBufferBinding->getBuffer(posElem->getSource()); 	const unsigned int vSize =
        //(unsigned int)vbuf->getVertexSize();

        //	unsigned char* vertex = static_cast<unsigned
        //char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY)); 	float* pReal; 	const unsigned int
        //vertexCount = (unsigned int)data->vertexCount; 	for(unsigned int j = 0; j < vertexCount;
        //++j)
        //	{
        //		posElem->baseVertexPointerToElement(vertex, &pReal);
        //		vertex += vSize;

        //		Ogre::Vector3 pos;
        //		pos.x = (*pReal++);
        //		pos.y = (*pReal++);
        //		pos.z = (*pReal++);

        //		if(minPoint.x > pos.x)
        //			minPoint.x = pos.x;

        //		if(minPoint.y > pos.y)
        //			minPoint.y = pos.y;

        //		if(minPoint.z > pos.z)
        //			minPoint.z = pos.z;

        //		if(maxPoint.x < pos.x)
        //			maxPoint.x = pos.x;

        //		if(maxPoint.y < pos.y)
        //			maxPoint.y = pos.y;

        //		if(maxPoint.z < pos.z)
        //			maxPoint.z = pos.z;
        //	}
        //	vbuf->unlock();
        //}
    }

    auto OgreUtil::calculateNearest2Pow( s32 input ) -> s32
    {
        if( input <= 32 )
        {
            return 32;
        }
        if( input <= 64 )
        {
            return 64;
        }
        if( input <= 128 )
        {
            return 128;
        }
        if( input <= 256 )
        {
            return 256;
        }
        if( input <= 512 )
        {
            return 512;
        }
        if( input <= 1024 )
        {
            return 1024;
        }
        if( input <= 2048 )
        {
            return 2048;
        }

        return input;
    }

    // Get the mesh information for the given mesh.
    // Code found in Wiki: www.ogre3d.org/wiki/index.php/RetrieveVertexData
    void OgreUtil::getMeshInformation( const Ogre::MeshPtr mesh, size_t &vertex_count,
                                       Ogre::Vector3 *&vertices, size_t &index_count,
                                       unsigned long *&indices, const Ogre::Vector3 &position,
                                       const Ogre::Quaternion &orient, const Ogre::Vector3 &scale )
    {
        bool added_shared = false;
        size_t current_offset = 0;
        size_t shared_offset = 0;
        size_t next_offset = 0;
        size_t index_offset = 0;

        vertex_count = index_count = 0;

        // Calculate how many vertices and indices we're going to need
        // for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
        //{
        //	Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        //	// We only need to add the shared vertices once
        //	if(submesh->useSharedVertices)
        //	{
        //		if( !added_shared )
        //		{
        //			vertex_count += mesh->sharedVertexData->vertexCount;
        //			added_shared = true;
        //		}
        //	}
        //	else
        //	{
        //		vertex_count += submesh->vertexData->vertexCount;
        //	}

        //	// Add the indices
        //	index_count += submesh->indexData->indexCount;
        //}

        // Allocate space for the vertices and indices
        vertices = new Ogre::Vector3[vertex_count];
        indices = new unsigned long[index_count];

        added_shared = false;

        // Run through the submeshes again, adding the data into the arrays
        // for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
        //{
        //	Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        //	Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData :
        //submesh->vertexData;

        //	if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        //	{
        //		if(submesh->useSharedVertices)
        //		{
        //			added_shared = true;
        //			shared_offset = current_offset;
        //		}

        //		const Ogre::VertexElement* posElem =
        //			vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

        //		Ogre::HardwareVertexBufferSharedPtr vbuf =
        //			vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

        //		unsigned char* vertex =
        //			static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

        //		// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
        //		//  as second argument. So make it float, to avoid trouble when Ogre::Real will
        //		//  be comiled/typedefed as double:
        //		//      Ogre::Real* pReal;
        //		float* pReal;

        //		for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex +=
        //vbuf->getVertexSize())
        //		{
        //			posElem->baseVertexPointerToElement(vertex, &pReal);

        //			Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

        //			vertices[current_offset + j] = (orient * (pt * scale)) + position;
        //		}

        //		vbuf->unlock();
        //		next_offset += vertex_data->vertexCount;
        //	}

        //	Ogre::IndexData* index_data = submesh->indexData;
        //	size_t numTris = index_data->indexCount / 3;
        //	Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
        //	if( ibuf.isNull() ) continue; // need to check if index buffer is valid (which will be
        //not if the mesh doesn't have triangles like a pointcloud)

        //	bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        //	unsigned long*  pLong = static_cast<unsigned
        //long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY)); 	unsigned short* pShort =
        //reinterpret_cast<unsigned short*>(pLong);

        //	size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;
        //	size_t index_start = index_data->indexStart;
        //	size_t last_index = numTris*3 + index_start;

        //	if (use32bitindexes)
        //		for (size_t k = index_start; k < last_index; ++k)
        //		{
        //			indices[index_offset++] = pLong[k] + static_cast<unsigned long>( offset );
        //		}

        //	else
        //		for (size_t k = index_start; k < last_index; ++k)
        //		{
        //			indices[ index_offset++ ] = static_cast<unsigned long>( pShort[k] ) +
        //				static_cast<unsigned long>( offset );
        //		}

        //		ibuf->unlock();
        //		current_offset = next_offset;
        //}
    }

    //--------------------------------------------
    void OgreUtil::getMeshInformation( const Ogre::v1::Entity *entity, size_t &vertex_count,
                                       Ogre::Vector3 *&vertices, size_t &index_count,
                                       unsigned long *&indices, const Ogre::Vector3 &position,
                                       const Ogre::Quaternion &orient, const Ogre::Vector3 &scale )
    {
        bool added_shared = false;
        size_t current_offset = 0;
        size_t shared_offset = 0;
        size_t next_offset = 0;
        size_t index_offset = 0;
        vertex_count = index_count = 0;

        auto mesh = entity->getMesh();

        bool useSoftwareBlendingVertices = entity->hasSkeleton();

        if( useSoftwareBlendingVertices )
        {
            // entity->_updateAnimation();
        }

        // Calculate how many vertices and indices we're going to need
        // for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
        //{
        //	Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        //	// We only need to add the shared vertices once
        //	if(submesh->useSharedVertices)
        //	{
        //		if( !added_shared )
        //		{
        //			vertex_count += mesh->sharedVertexData->vertexCount;
        //			added_shared = true;
        //		}
        //	}
        //	else
        //	{
        //		vertex_count += submesh->vertexData->vertexCount;
        //	}

        //	// Add the indices
        //	index_count += submesh->indexData->indexCount;
        //}

        // Allocate space for the vertices and indices
        vertices = new Ogre::Vector3[vertex_count];
        indices = new unsigned long[index_count];

        added_shared = false;

        // Run through the submeshes again, adding the data into the arrays
        // for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
        //{
        //	Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        //	//----------------------------------------------------------------
        //	// GET VERTEXDATA
        //	//----------------------------------------------------------------

        //	//Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData :
        //submesh->vertexData; 	Ogre::VertexData* vertex_data;

        //	//When there is animation:
        //	if(useSoftwareBlendingVertices)
        //		vertex_data = submesh->useSharedVertices ? entity->_getSkelAnimVertexData() :
        //entity->getSubEntity(i)->_getSkelAnimVertexData(); 	else 		vertex_data =
        //submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

        //	if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        //	{
        //		if(submesh->useSharedVertices)
        //		{
        //			added_shared = true;
        //			shared_offset = current_offset;
        //		}

        //		const Ogre::VertexElement* posElem =
        //			vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

        //		Ogre::HardwareVertexBufferSharedPtr vbuf =
        //			vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

        //		unsigned char* vertex =
        //			static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

        //		// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
        //		//  as second argument. So make it float, to avoid trouble when Ogre::Real will
        //		//  be comiled/typedefed as double:
        //		//      Ogre::Real* pReal;
        //		float* pReal;

        //		for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex +=
        //vbuf->getVertexSize())
        //		{
        //			posElem->baseVertexPointerToElement(vertex, &pReal);

        //			Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

        //			vertices[current_offset + j] = (orient * (pt * scale)) + position;
        //		}

        //		vbuf->unlock();
        //		next_offset += vertex_data->vertexCount;
        //	}

        //	Ogre::IndexData* index_data = submesh->indexData;
        //	size_t numTris = index_data->indexCount / 3;
        //	Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        //	bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        //	unsigned long*  pLong = static_cast<unsigned
        //long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY)); 	unsigned short* pShort =
        //reinterpret_cast<unsigned short*>(pLong);

        //	size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;
        //	size_t index_start = index_data->indexStart;
        //	size_t last_index = numTris*3 + index_start;

        //	if (use32bitindexes)
        //		for (size_t k = index_start; k < last_index; ++k)
        //		{
        //			indices[index_offset++] = pLong[k] + static_cast<unsigned long>( offset );
        //		}

        //	else
        //		for (size_t k = index_start; k < last_index; ++k)
        //		{
        //			indices[ index_offset++ ] = static_cast<unsigned long>( pShort[k] ) +
        //				static_cast<unsigned long>( offset );
        //		}

        //		ibuf->unlock();
        //		current_offset = next_offset;
        //}
    }

    //--------------------------------------------
    auto OgreUtil::getTerrainTextureType( TerrainTextureTypes terrainTextureType )
        -> Ogre::TerraTextureTypes
    {
        switch( terrainTextureType )
        {
        case TerrainTextureTypes::Base:
            return Ogre::TerraTextureTypes::TERRA_DIFFUSE;
        case TerrainTextureTypes::Splat:
            return Ogre::TerraTextureTypes::TERRA_DETAIL_WEIGHT;
        case TerrainTextureTypes::Diffuse1:
            return Ogre::TerraTextureTypes::TERRA_DETAIL0;
        case TerrainTextureTypes::Diffuse2:
            return Ogre::TerraTextureTypes::TERRA_DETAIL1;
        case TerrainTextureTypes::Diffuse3:
            return Ogre::TerraTextureTypes::TERRA_DETAIL2;
        case TerrainTextureTypes::Diffuse4:
            return Ogre::TerraTextureTypes::TERRA_DETAIL3;
        case TerrainTextureTypes::Normal1:
            return Ogre::TerraTextureTypes::TERRA_DETAIL0_NM;
        case TerrainTextureTypes::Normal2:
            return Ogre::TerraTextureTypes::TERRA_DETAIL1_NM;
        case TerrainTextureTypes::Normal3:
            return Ogre::TerraTextureTypes::TERRA_DETAIL2_NM;
        case TerrainTextureTypes::Normal4:
            return Ogre::TerraTextureTypes::TERRA_DETAIL3_NM;
        default:
        {
        }
        break;
        };

        return Ogre::TerraTextureTypes::TERRA_DIFFUSE;
    }

    auto OgreUtil::loadIntoImage( const String &filePath, Ogre::Image2 &outImage ) -> bool
    {
        try
        {
            if( !StringUtil::isNullOrEmpty( filePath ) )
            {
                using namespace Ogre;

                auto texExt = Path::getFileExtension( filePath );
                texExt = StringUtil::replaceAll( texExt, ".", "" );

                auto resourceGroupManager = ResourceGroupManager::getSingletonPtr();
                auto dataStream = resourceGroupManager->openResource(
                    filePath.c_str(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                if( dataStream )
                {
                    outImage.load( dataStream, texExt.c_str() );
                }

                return true;
            }
            else
            {
                WP_LOG_ERROR( "OgreUtil::loadIntoImage: filePath is null or empty." );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_ERROR( "OgreUtil::loadIntoImage: Exception during image loading." );
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

}  // namespace workphone::render
