#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CCubemap.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreViewport.h>
#include <OgreTexture.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreMaterialManager.h>
#include <OgreRenderTarget.h>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone, CCubemap, IGraphicsCubemap );
        u32 CCubemap::m_nameExt = 0;

        CCubemap::CCubemap() : m_currentIndex( 0 ), m_enable( false )
        {
        }

        CCubemap::~CCubemap()
        {
            unload( nullptr );
        }

        void CCubemap::load( SmartPtr<ISharedObject> data )
        {
        }

        void CCubemap::unload( SmartPtr<ISharedObject> data )
        {
        }

        void CCubemap::render()
        {
            for( u32 i = 0; i < m_renderTargets.size(); ++i )
            {
                // m_renderTargets[i]->update();
            }
        }

        void CCubemap::setVisibilityMask( u32 visibilityMask )
        {
            for( u32 i = 0; i < m_viewports.size(); ++i )
                m_viewports[i]->setVisibilityMask( visibilityMask );

            m_visibilityMask = visibilityMask;
        }

        u32 CCubemap::getExclusionMask() const
        {
            return m_exclusionMask;
        }

        void CCubemap::setExclusionMask( u32 exclusionMask )
        {
            m_exclusionMask = exclusionMask;
        }

        SmartPtr<IGraphicsScene> CCubemap::getSceneManager() const
        {
            return nullptr;
        }

        void CCubemap::setSceneManager( SmartPtr<IGraphicsScene> smgr )
        {
            smgr->_getObject( (void **)&m_sceneMgr );

            m_camera = m_sceneMgr->createCamera(
                ( String( "CubemapCamera" ) + StringUtil::toString( m_nameExt++ ) ).c_str() );
            m_camera->setFOVy( Ogre::Radian( Ogre::Degree( 90.0f ) ) );
            m_camera->setAspectRatio( 1.0f );
            // m_camera->setFixedYawAxis(false);
            m_camera->setNearClipDistance( 0.001f );
            m_camera->setFarClipDistance( 10000.0f );

            Ogre::TextureManager *textureManager = Ogre::TextureManager::getSingletonPtr();
            m_cubemapTexture = textureManager->createManual(
                m_textureName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_CUBE_MAP, 256, 256, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET );

            m_renderTargets.reserve( 6 );
            m_viewports.reserve( 6 );

            for( u32 i = 0; i < 6; ++i )
            {
                auto renderTarget = m_cubemapTexture->getBuffer( i )->getRenderTarget();
                m_renderTargets.push_back( renderTarget );

                Ogre::Viewport *vp = renderTarget->addViewport( m_camera );
                vp->setOverlaysEnabled( false );
                vp->setAutoUpdated( false );
                vp->setBackgroundColour( Ogre::ColourValue::Red );
                vp->setClearEveryFrame( false );
                m_viewports.push_back( vp );

                renderTarget->addListener( new CubemapRTListener( this ) );
            }

            m_frameListener = new CubemapFrameListener( this );
        }

        void CCubemap::setUpdateInterval( u32 milliseconds )
        {
        }

        void CCubemap::addExcludedObject( SmartPtr<IGraphicsObject> object )
        {
            m_objects.push_back( object );
        }

        Array<SmartPtr<IGraphicsObject>> CCubemap::getExcludedObjects() const
        {
            return m_objects;
        }

        void CCubemap::generateMaterial( const String &materialName )
        {
            Ogre::MaterialPtr material = Ogre::MaterialManager::getSingletonPtr()->create(
                materialName.c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            Ogre::Technique *technique = material->getTechnique( 0 );
            if( technique )
            {
                Ogre::Pass *pass = technique->getPass( 0 );
                if( pass )
                {
                    pass->setAmbient( Ogre::ColourValue( 0.5, 0.5, 0.5, 1.0 ) );
                    pass->setDiffuse( Ogre::ColourValue( 0.5, 0.5, 0.5, 1.0 ) );

                    Ogre::TextureUnitState *texState =
                        pass->createTextureUnitState( m_textureName.c_str() );
                    texState->setCubicTextureName( m_cubemapTexture->getName(), false );
                    // texState->set
                    // texState->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);
                    texState->setEnvironmentMap( true, Ogre::TextureUnitState::ENV_REFLECTION );
                }
            }
        }

        u32 CCubemap::getVisibilityMask() const
        {
            return m_visibilityMask;
        }

        Vector3F CCubemap::getPosition() const
        {
            return m_position;
        }

        void CCubemap::setPosition( const Vector3F &position )
        {
            m_position = position;
        }

        bool CCubemap::getEnable() const
        {
            return m_enable;
        }

        void CCubemap::setEnable( bool enable )
        {
            if( m_enable != enable )
            {
                m_enable = enable;

                if( m_enable )
                    Ogre::Root::getSingletonPtr()->addFrameListener( m_frameListener );
                else
                    Ogre::Root::getSingletonPtr()->removeFrameListener( m_frameListener );

                // for(u32 i=0; i<m_viewports.size(); ++i)
                //	m_viewports[i]->setAutoUpdated(m_enable);
            }
        }

        u32 CCubemap::getUpdateInterval() const
        {
            return 0;
        }

        String CCubemap::getTextureName() const
        {
            return m_textureName;
        }

        void CCubemap::setTextureName( const String &textureName )
        {
            m_textureName = textureName;
        }

        CCubemap::CubemapFrameListener::CubemapFrameListener( CCubemap *cubemap ) : m_cubemap( cubemap )
        {
        }

        CCubemap::CubemapFrameListener::~CubemapFrameListener()
        {
        }

        bool CCubemap::CubemapFrameListener::frameEnded( const Ogre::FrameEvent &evt )
        {
            return true;
        }

        bool CCubemap::CubemapFrameListener::frameStarted( const Ogre::FrameEvent &evt )
        {
            if( m_cubemap->m_enable )
            {
                // for(u32 i=0; i<m_cubemap->m_renderTargets.size(); ++i)
                {
                    m_cubemap->m_viewports[m_cubemap->m_currentIndex]->clear();
                    // m_cubemap->m_renderTargets[i]->update();
                    m_cubemap->m_viewports[m_cubemap->m_currentIndex]->update();
                    ++m_cubemap->m_currentIndex;

                    if( m_cubemap->m_currentIndex > 5 )
                        m_cubemap->m_currentIndex = 0;
                }

                // m_cubemap->m_renderTargets[0]->writeContentsToFile("face1.png");
            }

            return true;
        }

        bool CCubemap::CubemapFrameListener::frameRenderingQueued( const Ogre::FrameEvent &evt )
        {
            return true;
        }

        CCubemap::CubemapRTListener::CubemapRTListener( CCubemap *cubemap ) : m_cubemap( cubemap )
        {
        }

        CCubemap::CubemapRTListener::~CubemapRTListener()
        {
        }

        void CCubemap::CubemapRTListener::preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CCubemap::CubemapRTListener::viewportAdded( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CCubemap::CubemapRTListener::postViewportUpdate(
            const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CCubemap::CubemapRTListener::postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt )
        {
            // for(u32 i=0; i<m_cubemap->m_objects.size() && i<m_cubemap->m_oldMasks.size(); ++i)
            for( u32 i = 0; i < m_cubemap->m_objects.size(); ++i )
            {
                // m_cubemap->m_objects[i]->setVisibilityFlags(m_cubemap->m_oldMasks[i]);
                m_cubemap->m_objects[i]->setVisible( true );
            }
        }

        void CCubemap::CubemapRTListener::viewportRemoved( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CCubemap::CubemapRTListener::preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt )
        {
            if( !m_cubemap->m_enable )
                return;

            m_cubemap->m_oldMasks.clear();

            for( u32 i = 0; i < m_cubemap->m_objects.size(); ++i )
            {
                // m_cubemap->m_oldMasks.push_back(m_cubemap->m_objects[i]->getVisibilityFlags());
                // m_cubemap->m_objects[i]->setVisibilityFlags(m_cubemap->getExclusionMask());
                m_cubemap->m_objects[i]->setVisible( false );
            }

            // m_cubemap->m_camera->setOrientation(Ogre::Quaternion::IDENTITY);

            Vector3F position = m_cubemap->getPosition();
            // m_cubemap->m_camera->setPosition(Ogre::Vector3(position[0], position[1], position[2]));

            // if (evt.source == m_cubemap->m_renderTargets[0])
            //	m_cubemap->m_camera->yaw(Ogre::Radian(Ogre::Degree(-90.0f)));
            // else if (evt.source == m_cubemap->m_renderTargets[1])
            //	m_cubemap->m_camera->yaw(Ogre::Radian(Ogre::Degree(90.0f)));
            // else if (evt.source == m_cubemap->m_renderTargets[2])
            //	m_cubemap->m_camera->pitch(Ogre::Radian(Ogre::Degree(90.0f)));
            // else if (evt.source == m_cubemap->m_renderTargets[3])
            //	m_cubemap->m_camera->pitch(Ogre::Radian(Ogre::Degree(-90.0f)));
            ////else if(evt.source == m_cubemap->m_renderTargets[4])
            ////	m_cubemap->m_camera->yaw(Ogre::Degree(90.0f).valueRadians());
            // else if (evt.source == m_cubemap->m_renderTargets[5])
            //	m_cubemap->m_camera->yaw(Ogre::Radian(Ogre::Degree(180.0f)));
        }

    }  // end namespace render
}  // namespace workphone
