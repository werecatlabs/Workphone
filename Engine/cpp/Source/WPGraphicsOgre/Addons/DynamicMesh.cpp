#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/DynamicMesh.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        DynamicMesh::DynamicMesh( Ogre::SceneNode *owner ) : m_owner( owner )
        {
            /*StateManagerPtr& stateManager = fb::IApplicationManager::instance()->getStateManager();
            m_stateListener = new fb::StateListenerAdapter<DynamicMesh>(this);
            m_stateListener->setTaskNotifyId(TI_GraphicsTaskId);
            stateManager->addStateListener(m_stateListener);
            stateManager->registerObserver(this, m_stateListener);

            initialise(Ogre::RenderOperation::OT_TRIANGLE_LIST, true);
            setMaterial("BaseWhiteNoLighting");*/
        }

        DynamicMesh::DynamicMesh( const String &name, Ogre::SceneNode *owner ) : m_owner( owner )
        {
            /*StateManagerPtr& stateManager = fb::IApplicationManager::instance()->getStateManager();
            m_stateListener = new fb::StateListenerAdapter<DynamicMesh>(this);
            m_stateListener->setTaskNotifyId(TI_GraphicsTaskId);
            stateManager->addStateListener(m_stateListener);
            stateManager->registerObserver(this, m_stateListener);

            initialise(Ogre::RenderOperation::OT_TRIANGLE_LIST, true);
            setMaterial("BaseWhiteNoLighting");*/
        }

        DynamicMesh::~DynamicMesh()
        {
            /*if(m_stateListener)
            {
                StateManagerPtr& stateManager = fb::IApplicationManager::instance()->getStateManager();
                m_stateListener->setListener(NULL);
                stateManager->removeStateListener(m_stateListener);
                stateManager->unregisterObserver(this, m_stateListener);
                WP_SAFE_REMOVE(m_stateListener);
            }*/
        }

        void DynamicMesh::setDirty()
        {
            /*fb::SmartPtr<IStateMessage> updateMsg = new fb::StateMessage(getSubjectId());
            updateMsg->setType(CSCT_UDPATE);
            fb::IApplicationManager::instance()->getStateManager()->notifyStateChanged(updateMsg);
            RemoveReference(updateMsg);*/
        }

        void DynamicMesh::setOwner( Ogre::SceneNode *owner )
        {
            m_owner = owner;
        }

        Ogre::SceneNode *DynamicMesh::getOwner() const
        {
            return m_owner;
        }

        void DynamicMesh::setMesh( SmartPtr<ISubMesh> subMesh )
        {
            m_fbMesh = subMesh;
        }

        SmartPtr<ISubMesh> DynamicMesh::getSubMesh() const
        {
            return m_fbMesh;
        }

        /*
        void DynamicMesh::OnNotifyStateChanged(s32 const SmartPtr<IStateMessage>& message)
        {
            if(!m_fbMesh.isNull())
            {
                fillHardwareBuffers();

                String materialName = m_fbMesh->getMaterialName();
                Ogre::MaterialPtr material =
        Ogre::MaterialManager::getSingletonPtr()->getByName(materialName.c_str()); if(!material.isNull())
                {
                    setMaterial(materialName.c_str());
                }

                if(m_owner)
                {
                    m_owner->_updateBounds();
                    m_owner->needUpdate(true);
                }
            }
            else
            {
                prepareHardwareBuffers(0,0);
            }
        }
        */

        void DynamicMesh::createVertexDeclaration()
        {
            Ogre::VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;

            unsigned short iBinding = 0;
            u32 offset = 0;
            decl->addElement( iBinding, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION );
            offset += sizeof( f32 ) * 3;
            decl->addElement( iBinding, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL );
            offset += sizeof( f32 ) * 3;
            decl->addElement( iBinding, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0 );
            offset += sizeof( f32 ) * 2;
            decl->addElement( iBinding, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 1 );
            offset += sizeof( f32 ) * 2;

            u32 vertexSize = decl->getVertexSize( 0 );
            vertexSize = 0;
        }

        void DynamicMesh::fillHardwareBuffers()
        {
            auto vertexCount = m_fbMesh->getVertexBuffer()->getNumVertices();
            auto indexCount = m_fbMesh->getIndexBuffer()->getNumIndices();
            prepareHardwareBuffers( vertexCount, indexCount );

            if( !vertexCount )
            {
                mBox.setExtents( Ogre::Vector3::ZERO, Ogre::Vector3::ZERO );
                return;
            }

            Ogre::Vector3 vaabMin( 1e10, 1e10, 1e10 );
            Ogre::Vector3 vaabMax( -1e10, -1e10, -1e10 );

            Ogre::HardwareVertexBufferSharedPtr vbuf =
                mRenderOp.vertexData->vertexBufferBinding->getBuffer( 0 );

            Ogre::Real *ogreVertexDataPtr =
                static_cast<Ogre::Real *>( vbuf->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );

            SmartPtr<ISubMesh> subMesh = m_fbMesh;
            SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();
            SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();

            const SmartPtr<IVertexElement> posElem =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_POSITION );
            const SmartPtr<IVertexElement> normalElem =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_NORMAL );
            const SmartPtr<IVertexElement> texCoordElem =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
            const SmartPtr<IVertexElement> texCoordElem1 =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 1 );

            u32 fbVertexSize = vertexBuffer->getVertexDeclaration()->getSize();
            u8 *fbVertexDataPtr = (u8 *)vertexBuffer->getVertexData();
            f32 *fbElementData = 0;

            u32 ogreVertexSize = mRenderOp.vertexData->vertexDeclaration->getVertexSize( 0 );

            for( u32 j = 0; j < vertexCount; ++j, fbVertexDataPtr += fbVertexSize )
            {
                posElem->getElementData( fbVertexDataPtr, &fbElementData );

                Ogre::Vector3 ogrePosition;
                ogrePosition.x = *fbElementData++;
                ogrePosition.y = *fbElementData++;
                ogrePosition.z = *fbElementData++;
                ogrePosition.z = -ogrePosition.z;

                if( ogrePosition.x < vaabMin.x )
                    vaabMin.x = ogrePosition.x;
                if( ogrePosition.y < vaabMin.y )
                    vaabMin.y = ogrePosition.y;
                if( ogrePosition.z < vaabMin.z )
                    vaabMin.z = ogrePosition.z;

                if( ogrePosition.x > vaabMax.x )
                    vaabMax.x = ogrePosition.x;
                if( ogrePosition.y > vaabMax.y )
                    vaabMax.y = ogrePosition.y;
                if( ogrePosition.z > vaabMax.z )
                    vaabMax.z = ogrePosition.z;

                *ogreVertexDataPtr++ = ogrePosition.x;
                *ogreVertexDataPtr++ = ogrePosition.y;
                *ogreVertexDataPtr++ = ogrePosition.z;

                normalElem->getElementData( fbVertexDataPtr, &fbElementData );

                Ogre::Vector3 ogreNormal;
                ogreNormal.x = *fbElementData++;
                ogreNormal.y = *fbElementData++;
                ogreNormal.z = *fbElementData++;
                ogreNormal.z = -ogreNormal.z;

                *ogreVertexDataPtr++ = ogreNormal.x;
                *ogreVertexDataPtr++ = ogreNormal.y;
                *ogreVertexDataPtr++ = ogreNormal.z;

                texCoordElem->getElementData( fbVertexDataPtr, &fbElementData );
                *ogreVertexDataPtr++ = *fbElementData++;
                *ogreVertexDataPtr++ = *fbElementData++;

                if( texCoordElem1 )
                {
                    texCoordElem1->getElementData( fbVertexDataPtr, &fbElementData );
                    *ogreVertexDataPtr++ = *fbElementData++;
                    *ogreVertexDataPtr++ = *fbElementData++;
                }
                else
                {
                    *ogreVertexDataPtr++ = 0.0f;
                    *ogreVertexDataPtr++ = 0.0f;
                }
            }

            vbuf->unlock();

            Ogre::IndexData *indexData = mRenderOp.indexData;

            Ogre::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;

            bool useWords = ( vertexCount >= std::numeric_limits<unsigned short>::max() ) ? false : true;
            unsigned short *pWords =
                useWords
                    ? static_cast<unsigned short *>( ibuf->lock( Ogre::HardwareBuffer::HBL_DISCARD ) )
                    : NULL;
            unsigned int *pDWords =
                useWords
                    ? NULL
                    : static_cast<unsigned int *>( ibuf->lock( Ogre::HardwareBuffer::HBL_DISCARD ) );

            const u32 *fbIndexData = reinterpret_cast<u32 *>( indexBuffer->getIndexData() );
            if( indexData->indexCount > 0 )
            {
                for( int i = (int)indexData->indexCount - 1; i >= 0; --i )
                {
                    u32 index = fbIndexData[i];
                    if( useWords )
                        *pWords++ = index;
                    else
                        *pDWords++ = index;
                }
            }

            ibuf->unlock();

            mBox.setExtents( vaabMin, vaabMax );
        }

    }  // namespace render
}  // namespace workphone
