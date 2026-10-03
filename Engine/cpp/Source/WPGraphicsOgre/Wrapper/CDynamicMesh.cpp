#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDynamicMesh.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CDynamicMesh, CGraphicsObjectOgre<IDynamicMesh> );

        CDynamicMesh::CDynamicMesh( SmartPtr<IGraphicsScene> creator )
        {
        }

        CDynamicMesh::~CDynamicMesh()
        {
        }

        void CDynamicMesh::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CDynamicMesh::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IGraphicsObject> CDynamicMesh::clone( const String &name ) const
        {
            auto dynamicMesh = workphone::make_ptr<CDynamicMesh>();

            auto subMesh = m_dynamicMesh->getSubMesh();
            dynamicMesh->setSubMesh( subMesh );
            dynamicMesh->setDirty( true );
            return dynamicMesh;
        }

        void CDynamicMesh::_getObject( void **ppObject ) const
        {
            *ppObject = m_dynamicMesh;
        }

        void CDynamicMesh::setMesh( SmartPtr<IMesh> mesh )
        {
            if( mesh )
            {
                auto subMeshes = mesh->getSubMeshes();
                m_subMeshes;
            }
        }

        SmartPtr<IMesh> CDynamicMesh::getMesh() const
        {
            return nullptr;
        }

        void CDynamicMesh::setSubMesh( SmartPtr<ISubMesh> subMesh )
        {
            m_dynamicMesh->setMesh( subMesh );
        }

        SmartPtr<ISubMesh> CDynamicMesh::getSubMesh() const
        {
            return m_dynamicMesh->getSubMesh();
        }

        void CDynamicMesh::setDirty( bool dirty )
        {
            m_dynamicMesh->setDirty();
        }

        void CDynamicMesh::setOwner( SmartPtr<IGraphicsSceneNode> owner )
        {
            CGraphicsObjectOgre<IDynamicMesh>::setOwner( owner );

            if( owner )
            {
                Ogre::SceneNode *sceneNode = nullptr;
                owner->_getObject( (void **)&sceneNode );

                if( sceneNode )
                {
                    m_dynamicMesh->setOwner( sceneNode );
                }
            }
        }
    }  // end namespace render
}  // namespace workphone
