#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/DynamicRenderable.hpp"
#include <OgreCamera.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreVertexIndexData.h>

using namespace Ogre;

DynamicRenderable::DynamicRenderable() : Ogre::v1::SimpleRenderable( 0, nullptr, nullptr )
{
}

DynamicRenderable::DynamicRenderable( Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager,
                                      Ogre::SceneManager *manager ) :
    Ogre::v1::SimpleRenderable( id, objectMemoryManager, manager )
{
}

DynamicRenderable::~DynamicRenderable()
{
    if( mRenderOp.vertexData )
    {
        delete mRenderOp.vertexData;
        mRenderOp.vertexData = nullptr;
    }

    if( mRenderOp.indexData )
    {
        delete mRenderOp.indexData;
        mRenderOp.indexData = nullptr;
    }
}

void DynamicRenderable::initialise( Ogre::OperationType operationType, bool useIndices )
{
    // Initialize render operation
    mRenderOp.operationType = operationType;
    mRenderOp.useIndexes = useIndices;
    mRenderOp.vertexData = new Ogre::v1::VertexData( nullptr );
    if( mRenderOp.useIndexes )
    {
        mRenderOp.indexData = new Ogre::v1::IndexData;
    }

    // Reset buffer capacities
    mVertexBufferCapacity = 0;
    mIndexBufferCapacity = 0;

    // Create vertex declaration
    createVertexDeclaration();
}

void DynamicRenderable::prepareHardwareBuffers( size_t vertexCount, size_t indexCount )
{
    // Prepare vertex buffer
    size_t newVertCapacity = mVertexBufferCapacity;
    if( ( vertexCount > mVertexBufferCapacity ) || ( !mVertexBufferCapacity ) )
    {
        // vertexCount exceeds current capacity!
        // It is necessary to reserve the buffer.

        // Check if this is the first call
        if( !newVertCapacity )
        {
            newVertCapacity = 1;
        }

        // Make capacity the next power of two
        while( newVertCapacity < vertexCount )
        {
            newVertCapacity <<= 1;
        }
    }
    else if( vertexCount < mVertexBufferCapacity >> 1 )
    {
        // Make capacity the previous power of two
        while( vertexCount < newVertCapacity >> 1 )
        {
            newVertCapacity >>= 1;
        }
    }

    if( newVertCapacity != mVertexBufferCapacity )
    {
        mVertexBufferCapacity = newVertCapacity;
        // Create new vertex buffer
        Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
            Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
                mRenderOp.vertexData->vertexDeclaration->getVertexSize( 0 ), mVertexBufferCapacity,
                Ogre::v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY );  // TODO(fluffylion): Custom HBU_?

        // Bind buffer
        mRenderOp.vertexData->vertexBufferBinding->setBinding( 0, vbuf );
    }
    // Update vertex count in the render operation
    mRenderOp.vertexData->vertexCount = vertexCount;

    if( mRenderOp.useIndexes )
    {
        size_t newIndexCapacity = mIndexBufferCapacity;
        // Prepare index buffer
        if( ( indexCount > newIndexCapacity ) || ( !newIndexCapacity ) )
        {
            // indexCount exceeds current capacity!
            // It is necessary to reserve the buffer.

            // Check if this is the first call
            if( !newIndexCapacity )
            {
                newIndexCapacity = 1;
            }

            // Make capacity the next power of two
            while( newIndexCapacity < indexCount )
            {
                newIndexCapacity <<= 1;
            }
        }
        else if( indexCount < newIndexCapacity >> 1 )
        {
            // Make capacity the previous power of two
            while( indexCount < newIndexCapacity >> 1 )
            {
                newIndexCapacity >>= 1;
            }
        }

        if( newIndexCapacity != mIndexBufferCapacity )
        {
            mIndexBufferCapacity = newIndexCapacity;
            if( vertexCount <= static_cast<size_t>( std::numeric_limits<unsigned short>::max() ) + 1u )
            {
                // Create new index buffer
                mRenderOp.indexData->indexBuffer =
                    Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
                        Ogre::v1::HardwareIndexBuffer::IT_16BIT, mIndexBufferCapacity,
                        Ogre::v1::HardwareBuffer::
                            HBU_DYNAMIC_WRITE_ONLY );  // TODO(fluffylion): Custom HBU_?
            }
            else
            {
                // Create new index buffer
                mRenderOp.indexData->indexBuffer =
                    Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
                        Ogre::v1::HardwareIndexBuffer::IT_32BIT, mIndexBufferCapacity,
                        Ogre::v1::HardwareBuffer::
                            HBU_DYNAMIC_WRITE_ONLY );  // TODO(fluffylion): Custom HBU_?
            }
        }

        // Update index count in the render operation
        mRenderOp.indexData->indexCount = indexCount;
    }
}

auto DynamicRenderable::getBoundingRadius() const -> Real
{
    return Math::Sqrt(
        std::max( mBox.getMaximum().squaredLength(), mBox.getMinimum().squaredLength() ) );
}

auto DynamicRenderable::getSquaredViewDepth( const Camera *cam ) const -> Real
{
    Vector3 vMin, vMax, vMid, vDist;
    vMin = mBox.getMinimum();
    vMax = mBox.getMaximum();
    vMid = ( ( vMax - vMin ) * 0.5 ) + vMin;
    vDist = cam->getDerivedPosition() - vMid;

    return vDist.squaredLength();
}

void DynamicRenderable::removeQueryFlags( unsigned int flags )
{
}
