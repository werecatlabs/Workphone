#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/CompositorPassProvider.hpp"
#include "WPGraphicsOgreNext/CompositorPassUI.hpp"
#include "WPGraphicsOgreNext/CompositorPassUiDef.hpp"
#include "WPGraphicsOgreNext/CompositorPassUiDef.hpp"
#include "WPGraphicsOgreNext/CompositorPassUI.hpp"
#include <Workphone/WorkphoneInterface.hpp>

namespace workphone
{
    namespace render
    {

        class DefaultCompositorPass : public Ogre::CompositorPass
        {
        public:
            DefaultCompositorPass( const Ogre::CompositorPassDef *definition,
                                   Ogre::Camera *defaultCamera, Ogre::CompositorNode *parentNode,
                                   const Ogre::RenderTargetViewDef *rtv,
                                   Ogre::SceneManager *sceneManager ) :
                Ogre::CompositorPass( definition, parentNode )
            {
            }

            void execute( const Ogre::Camera *lodCameraconst ) override
            {
            }
        };

        CompositorPassProvider::CompositorPassProvider() = default;

        auto CompositorPassProvider::addPassDef( Ogre::CompositorPassType passType,
                                                 Ogre::IdString customId,
                                                 Ogre::CompositorTargetDef *parentTargetDef,
                                                 Ogre::CompositorNodeDef *parentNodeDef )
            -> Ogre::CompositorPassDef *
        {
            if( customId == "app_gui" )
            {
                return OGRE_NEW CompositorPassUiDef( parentTargetDef );
            }

            if( customId == "scene_gui" )
            {
                auto p = OGRE_NEW CompositorPassUiDef( parentTargetDef );
                p->mSkipLoadStoreSemantics = true;
                return p;
            }

            return nullptr;
        }

        auto CompositorPassProvider::addPass( const Ogre::CompositorPassDef *definition,
                                              Ogre::Camera *defaultCamera,
                                              Ogre::CompositorNode *parentNode,
                                              const Ogre::RenderTargetViewDef *rtvDef,
                                              Ogre::SceneManager *sceneManager )
            -> Ogre::CompositorPass *
        {
            if( dynamic_cast<const CompositorPassUiDef *>( definition ) )
            {
                auto uiDef = static_cast<const CompositorPassUiDef *>( definition );
                return OGRE_NEW CompositorPassUI( uiDef, sceneManager, rtvDef, parentNode );
            }

            if( dynamic_cast<const CompositorPassUiDef *>( definition ) )
            {
                auto applicationManager = workphone::core::IApplicationManager::instance();
                auto ui = applicationManager->getRenderUI();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto graphicsScene = graphicsSystem->getGraphicsScene();

                auto camera = graphicsScene->getActiveCamera();

                Ogre::SceneManager *smgr = nullptr;
                graphicsScene->_getObject( reinterpret_cast<void **>( &smgr ) );

                Ogre::Camera *ogreCamera = nullptr;
                if( camera )
                {
                    camera->_getObject( reinterpret_cast<void **>( &ogreCamera ) );
                }

                if( !ui )
                {
                    return nullptr;
                }

                if( !ui->isLoaded() )
                {
                    return nullptr;
                }

                auto uiDef = static_cast<const CompositorPassUiDef *>( definition );
                return OGRE_NEW CompositorPassUI( uiDef, sceneManager, rtvDef, parentNode );
            }

            return OGRE_NEW DefaultCompositorPass( definition, defaultCamera, parentNode, rtvDef,
                                                   sceneManager );
        }

    }  // namespace render
}  // namespace workphone
