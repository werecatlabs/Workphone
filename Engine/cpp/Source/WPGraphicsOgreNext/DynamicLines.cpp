#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/DynamicLines.hpp>
#include <Ogre.h>
#include <cassert>
#include <cmath>

namespace workphone::render
{

    DynamicLinesOgreNext::DynamicLinesOgreNext( Ogre::IdType id,
                                                Ogre::ObjectMemoryManager *objectMemoryManager,
                                                Ogre::SceneManager *manager,
                                                Ogre::OperationType opType ) :
        DynamicRenderable( id, objectMemoryManager, manager )
    {
        initialise( opType, false );
        // setMaterial("BaseWhiteNoLighting");

        Ogre::MaterialPtr mat = getMaterial();
        Ogre::Technique *tech = mat->getTechnique( 0 );
        Ogre::Pass *pass = tech->getPass( 0 );
        // pass->setDepthCheckEnabled(true);
        // pass->setDepthWriteEnabled(true);
    }

    DynamicLinesOgreNext::~DynamicLinesOgreNext() = default;

    void DynamicLinesOgreNext::setOperationType( Ogre::OperationType opType )
    {
        mRenderOp.operationType = opType;
    }

    auto DynamicLinesOgreNext::getOperationType() const -> Ogre::OperationType
    {
        return mRenderOp.operationType;
    }

    void DynamicLinesOgreNext::addPoint( const Ogre::Vector3 &p )
    {
        mPoints.push_back( p );
        mDirty = true;
    }
    void DynamicLinesOgreNext::addPoint( Ogre::Real x, Ogre::Real y, Ogre::Real z )
    {
        mPoints.emplace_back( x, y, z );
        mDirty = true;
    }
    auto DynamicLinesOgreNext::getPoint( unsigned short index ) const -> const Ogre::Vector3 &
    {
        assert( index < mPoints.size() && "Point index is out of bounds!!" );
        return mPoints[index];
    }
    auto DynamicLinesOgreNext::getNumPoints() const -> unsigned short
    {
        return static_cast<unsigned short>( mPoints.size() );
    }

    void DynamicLinesOgreNext::setPoint( unsigned short index, const Ogre::Vector3 &value )
    {
        assert( index < mPoints.size() && "Point index is out of bounds!!" );

        mPoints[index] = value;
        mDirty = true;
    }

    void DynamicLinesOgreNext::clear()
    {
        mPoints.clear();
        mDirty = true;
    }

    void DynamicLinesOgreNext::update()
    {
        if( mDirty )
        {
            fillHardwareBuffers();
            // this->getParentNode()->u(true);
        }
    }

    void DynamicLinesOgreNext::createVertexDeclaration()
    {
        auto decl = mRenderOp.vertexData->vertexDeclaration;
        decl->addElement( POSITION_BINDING, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION );
    }

    void DynamicLinesOgreNext::fillHardwareBuffers()
    {
        auto size = mPoints.size();

        prepareHardwareBuffers( size, 0 );

        if( !size )
        {
            mBox.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
            mDirty = false;
            return;
        }

        Ogre::Vector3 vaabMin = mPoints[0];
        Ogre::Vector3 vaabMax = mPoints[0];

        auto vbuf = mRenderOp.vertexData->vertexBufferBinding->getBuffer( 0 );

        auto *prPos = static_cast<Ogre::Real *>( vbuf->lock( Ogre::v1::HardwareBuffer::HBL_DISCARD ) );

        for( int i = 0; i < size; i++ )
        {
            Ogre::Vector3 point = mPoints[i];

            *prPos++ = point.x;
            *prPos++ = point.y;
            *prPos++ = point.z;

            if( point.x < vaabMin.x )
            {
                vaabMin.x = point.x;
            }
            if( point.y < vaabMin.y )
            {
                vaabMin.y = point.y;
            }
            if( point.z < vaabMin.z )
            {
                vaabMin.z = point.z;
            }

            if( point.x > vaabMax.x )
            {
                vaabMax.x = point.x;
            }
            if( point.y > vaabMax.y )
            {
                vaabMax.y = point.y;
            }
            if( point.z > vaabMax.z )
            {
                vaabMax.z = point.z;
            }
        }

        vbuf->unlock();

        mBox.setExtents( vaabMin, vaabMax );

        mDirty = false;
    }

}  // namespace workphone::render
