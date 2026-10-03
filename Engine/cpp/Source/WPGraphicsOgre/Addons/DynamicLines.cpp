#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/DynamicLines.hpp"
#include <Ogre.h>
#include <cassert>
#include <cmath>

namespace workphone
{
    namespace render
    {

        using namespace Ogre;

        enum
        {
            POSITION_BINDING,
            TEXCOORD_BINDING
        };

        DynamicLinesOgre::DynamicLinesOgre( Ogre::RenderOperation::OperationType opType )
        {
            initialise( opType, false );
            // setMaterial("BaseWhiteNoLighting");
            mDirty = true;

            Ogre::MaterialPtr mat = getMaterial();
            Ogre::Technique *tech = mat->getTechnique( 0 );
            Ogre::Pass *pass = tech->getPass( 0 );
            pass->setDepthCheckEnabled( true );
            pass->setDepthWriteEnabled( true );
        }

        DynamicLinesOgre::~DynamicLinesOgre()
        {
        }

        void DynamicLinesOgre::setOperationType( Ogre::RenderOperation::OperationType opType )
        {
            mRenderOp.operationType = opType;
        }

        Ogre::RenderOperation::OperationType DynamicLinesOgre::getOperationType() const
        {
            return mRenderOp.operationType;
        }

        void DynamicLinesOgre::addPoint( const Vector3 &p )
        {
            mPoints.push_back( p );
            mDirty = true;
        }
        void DynamicLinesOgre::addPoint( Real x, Real y, Real z )
        {
            mPoints.push_back( Vector3( x, y, z ) );
            mDirty = true;
        }
        const Ogre::Vector3 &DynamicLinesOgre::getPoint( unsigned short index ) const
        {
            assert( index < mPoints.size() && "Point index is out of bounds!!" );
            return mPoints[index];
        }
        unsigned short DynamicLinesOgre::getNumPoints( void ) const
        {
            return (unsigned short)mPoints.size();
        }

        void DynamicLinesOgre::setPoint( unsigned short index, const Vector3 &value )
        {
            assert( index < mPoints.size() && "Point index is out of bounds!!" );

            mPoints[index] = value;
            mDirty = true;
        }

        void DynamicLinesOgre::clear()
        {
            mPoints.clear();
            mDirty = true;
        }

        void DynamicLinesOgre::update()
        {
            if( mDirty )
            {
                fillHardwareBuffers();
                this->getParentNode()->needUpdate( true );
            }
        }

        void DynamicLinesOgre::createVertexDeclaration()
        {
            Ogre::VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
            decl->addElement( POSITION_BINDING, 0,
                              (Ogre::VertexElementType)VertexElementType::VET_FLOAT3,
                              (Ogre::VertexElementSemantic)VertexElementSemantic::VES_POSITION );
        }

        void DynamicLinesOgre::fillHardwareBuffers()
        {
            int size = mPoints.size();

            prepareHardwareBuffers( size, 0 );

            if( !size )
            {
                mBox.setExtents( Vector3::ZERO, Vector3::ZERO );
                mDirty = false;
                return;
            }

            Vector3 vaabMin = mPoints[0];
            Vector3 vaabMax = mPoints[0];

            HardwareVertexBufferSharedPtr vbuf =
                mRenderOp.vertexData->vertexBufferBinding->getBuffer( 0 );

            Real *prPos = static_cast<Real *>( vbuf->lock( HardwareBuffer::HBL_DISCARD ) );

            for( int i = 0; i < size; i++ )
            {
                Vector3 point = mPoints[i];

                *prPos++ = point.x;
                *prPos++ = point.y;
                *prPos++ = point.z;

                if( point.x < vaabMin.x )
                    vaabMin.x = point.x;
                if( point.y < vaabMin.y )
                    vaabMin.y = point.y;
                if( point.z < vaabMin.z )
                    vaabMin.z = point.z;

                if( point.x > vaabMax.x )
                    vaabMax.x = point.x;
                if( point.y > vaabMax.y )
                    vaabMax.y = point.y;
                if( point.z > vaabMax.z )
                    vaabMax.z = point.z;
            }

            vbuf->unlock();

            mBox.setExtents( vaabMin, vaabMax );

            mDirty = false;
        }

    }  // namespace render
}  // namespace workphone
