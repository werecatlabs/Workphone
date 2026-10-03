#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <tinyxml.h>
#include <stdio.h>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        Ogre::String OgreUtil::toString( const String &str )
        {
            return str.c_str();
        }

        Ogre::String toString( const StringW &str )
        {
            auto msg = StringUtil::toUTF16to8( str );
            return msg.c_str();
        }

        bool OgreUtil::equals( const Ogre::Vector3 &a, const Ogre::Vector3 &b )
        {
            return Math<real_Num>::equals( a.x, b.x ) && Math<real_Num>::equals( a.y, b.y ) &&
                   Math<real_Num>::equals( a.z, b.z );
        }

        bool OgreUtil::equals( const Ogre::ColourValue &a, const Ogre::ColourValue &b )
        {
            return Math<real_Num>::equals( a.r, b.r ) && Math<real_Num>::equals( a.g, b.g ) &&
                   Math<real_Num>::equals( a.b, b.b ) && Math<real_Num>::equals( a.a, b.a );
        }

        bool OgreUtil::equals( const Ogre::Quaternion &a, const Ogre::Quaternion &b )
        {
            return Math<real_Num>::equals( a.x, b.x ) && Math<real_Num>::equals( a.y, b.y ) &&
                   Math<real_Num>::equals( a.z, b.z ) && Math<real_Num>::equals( a.w, b.w );
        }

        Ogre::ColourValue OgreUtil::convertToOgre( const ColourF &c )
        {
            return Ogre::ColourValue( c.r, c.g, c.b, c.a );
        }

        AABB3F OgreUtil::convert( const Ogre::AxisAlignedBox &box )
        {
            const Ogre::Vector3 &minPoint = box.getMinimum();
            const Ogre::Vector3 &maxPoint = box.getMaximum();

            return AABB3F( minPoint.x, minPoint.y, minPoint.z, maxPoint.x, maxPoint.y, maxPoint.z );
        }

        QuaternionF OgreUtil::convert( const Ogre::Quaternion &q )
        {
            return QuaternionF( q.w, q.x, q.y, q.z );
        }

        Vector3F OgreUtil::convert( const Ogre::Vector3 &vector )
        {
            return Vector3F( vector.x, vector.y, vector.z );
        }

        ColourF OgreUtil::convert( const Ogre::ColourValue &c )
        {
            return ColourF( c.r, c.g, c.b, c.a );
        }

        Ogre::Quaternion OgreUtil::convertToOgre( const QuaternionF &q )
        {
            return Ogre::Quaternion( (Ogre::Real)q.W(), (Ogre::Real)q.X(), (Ogre::Real)q.Y(),
                                     (Ogre::Real)q.Z() );
        }

        Ogre::Vector3 OgreUtil::getDerivedPosition( Ogre::SceneNode *sceneNode )
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

        Ogre::AxisAlignedBox OgreUtil::getDerivedBoundingBox( Ogre::SceneNode *node )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AxisAlignedBox box;
            box.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );

            for( u32 i = 0; i < node->numAttachedObjects(); ++i )
            {
                Ogre::MovableObject *object = node->getAttachedObject( i );
                if( object->getTypeFlags() & Ogre::SceneManager::ENTITY_TYPE_MASK )
                {
                    Ogre::Entity *entity = (Ogre::Entity *)object;

                    Ogre::Vector3 minPoint( 1e10, 1e10, 1e10 );
                    Ogre::Vector3 maxPoint( -1e10, -1e10, -1e10 );
                    getVertexMinMax( entity, minPoint, maxPoint );

                    Ogre::Vector3 derivedPos = getDerivedPosition( node );
                    Ogre::AxisAlignedBox entityBox;  // = entity->getBoundingBox();
                    entityBox.scale( node->getScale() );

                    entityBox.setMinimum( minPoint + derivedPos );
                    entityBox.setMaximum( maxPoint + derivedPos );

                    box.merge( entityBox );
                }
            }

            Ogre::Node::ChildNodeIterator node_it = node->getChildIterator();
            while( node_it.hasMoreElements() )
            {
                Ogre::SceneNode *child = static_cast<Ogre::SceneNode *>( node_it.getNext() );
                box.merge( getDerivedBoundingBox( child ) );
            }

            return box;
        }

        void OgreUtil::getVertexMinMax( Ogre::Entity *entity, Ogre::Vector3 &minPoint,
                                        Ogre::Vector3 &maxPoint )
        {
            Ogre::MeshPtr mesh = entity->getMesh();
            for( u32 i = 0; i < entity->getNumSubEntities(); ++i )
            {
                Ogre::SubEntity *subEntity = entity->getSubEntity( i );
                Ogre::SubMesh *subMesh = subEntity->getSubMesh();

                Ogre::VertexData *data = nullptr;

                if( subMesh->useSharedVertices )
                    data = mesh->sharedVertexData;
                else
                    data = subMesh->vertexData;

                const Ogre::VertexElement *posElem =
                    data->vertexDeclaration->findElementBySemantic( Ogre::VES_POSITION );
                Ogre::HardwareVertexBufferSharedPtr vbuf =
                    data->vertexBufferBinding->getBuffer( posElem->getSource() );
                const unsigned int vSize = (unsigned int)vbuf->getVertexSize();

                unsigned char *vertex =
                    static_cast<unsigned char *>( vbuf->lock( Ogre::HardwareBuffer::HBL_READ_ONLY ) );
                float *pReal;
                const unsigned int vertexCount = (unsigned int)data->vertexCount;
                for( unsigned int j = 0; j < vertexCount; ++j )
                {
                    posElem->baseVertexPointerToElement( vertex, &pReal );
                    vertex += vSize;

                    Ogre::Vector3 pos;
                    pos.x = ( *pReal++ );
                    pos.y = ( *pReal++ );
                    pos.z = ( *pReal++ );

                    if( minPoint.x > pos.x )
                        minPoint.x = pos.x;

                    if( minPoint.y > pos.y )
                        minPoint.y = pos.y;

                    if( minPoint.z > pos.z )
                        minPoint.z = pos.z;

                    if( maxPoint.x < pos.x )
                        maxPoint.x = pos.x;

                    if( maxPoint.y < pos.y )
                        maxPoint.y = pos.y;

                    if( maxPoint.z < pos.z )
                        maxPoint.z = pos.z;
                }

                vbuf->unlock();
            }
        }

        s32 OgreUtil::calculateNearest2Pow( s32 input )
        {
            if( input <= 32 )
                return 32;
            if( input <= 64 )
                return 64;
            if( input <= 128 )
                return 128;
            if( input <= 256 )
                return 256;
            if( input <= 512 )
                return 512;
            if( input <= 1024 )
                return 1024;
            if( input <= 2048 )
                return 2048;
            return input;
        }

    }  // namespace render
}  // namespace workphone
