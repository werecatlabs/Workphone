#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CFont.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTechniqueOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialPassOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreFontManager.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CFont, ResourceGraphics<IFont> );

        const hash_type CFont::SET_TEXTURE_HASH = StringUtil::getHash( "setTexture" );
        const hash_type CFont::FRAGMENT_FLOAT_HASH = StringUtil::getHash( "fragmentFloat" );
        const hash_type CFont::FRAGMENT_VECTOR2F_HASH = StringUtil::getHash( "fragmentVector2f" );
        const hash_type CFont::FRAGMENT_Vector3f_HASH = StringUtil::getHash( "fragmentVector3f" );
        const hash_type CFont::FRAGMENT_VECTOR4F_HASH = StringUtil::getHash( "fragmentVector4f" );
        const hash_type CFont::FRAGMENT_COLOUR_HASH = StringUtil::getHash( "fragmentColour" );

        u32 CFont::m_nameExt = 0;

        CFont::CFont()
        {
            static const auto MaterialStr = String( "Material" );
            auto name = MaterialStr + StringUtil::toString( m_nameExt++ );

            setName( name );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            m_stateContext = stateManager->addStateContext();

            auto stateListener = factoryManager->make_ptr<MaterialStateListener>();
            stateListener->setOwner( this );
            m_stateListener = stateListener;
            m_stateContext->addStateListener( m_stateListener );
        }

        CFont::CFont( u32 poolTypeId )
        {
        }

        CFont::~CFont()
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                unload( nullptr );
            }
        }

        void CFont::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto ogreFontManager = Ogre::FontManager::getSingletonPtr();

                auto uuid = "default";  //StringUtil::getUUID();
                auto grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
                auto fontResource = ogreFontManager->createOrRetrieve( uuid, grp );

                auto font = Ogre::static_pointer_cast<Ogre::Font>( fontResource.first );
                font->setSource( m_fontSource.c_str() );
                font->setType( Ogre::FT_TRUETYPE );
                font->setTrueTypeResolution( m_fontResolution );
                font->setTrueTypeSize( (u32)m_fontSize );

                font->load();

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CFont::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &state = getLoadingState();
                if( state != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    //auto ogreFontManager = Ogre::FontManager::getSingletonPtr();
                    //if( ogreFontManager )
                    //{
                    //    if( m_font )
                    //    {
                    //        ogreFontManager->remove( m_font );
                    //        m_font = nullptr;
                    //    }
                    //}

                    ResourceGraphics<IFont>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        hash32 CFont::getRendererType() const
        {
            return m_rendererType;
        }

        void CFont::setRendererType( hash32 rendererType )
        {
            m_rendererType = rendererType;
        }

        void CFont::createMaterialByType()
        {
            try
            {
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CFont::setDirty( bool dirty )
        {
            // if (m_isDirty != dirty)
            //{
            //	m_isDirty = dirty;

            //	auto applicationManager = core::IApplicationManager::instance();
            //	WP_ASSERT(applicationManager);

            //	auto factoryManager = applicationManager->getFactoryManager();
            //	auto message = factoryManager->make_ptr<StateMessageDirty>();
            //	message->setDirty(dirty);

            //	auto graphicsSystem = applicationManager->getGraphicsSystem();
            //	auto stateTask = graphicsSystem->getStateTask();
            //	m_stateContext->addMessage(stateTask, message);
            //}
        }

        bool CFont::isDirty() const
        {
            return false;
        }

        SmartPtr<Properties> CFont::getProperties() const
        {
            return nullptr;
        }

        void CFont::setProperties( SmartPtr<Properties> properties )
        {
            properties->getPropertyValue( "font_type", font_type );
            properties->getPropertyValue( "font_source", m_fontSource );
            properties->getPropertyValue( "font_size", m_fontSize );
            properties->getPropertyValue( "font_resolution", m_fontResolution );
        }

        Array<SmartPtr<ISharedObject>> CFont::getChildObjects() const
        {
            auto objects = Array<SmartPtr<ISharedObject>>();

            return objects;
        }

        bool CFont::MaterialStateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            if( message->isExactly<StateMessageLoad>() )
            {
                auto loadMessage = workphone::static_pointer_cast<StateMessageLoad>( message );
                if( loadMessage->getType() == StateMessageLoad::LOAD_HASH )
                {
                    m_owner->load( nullptr );
                }
                else
                {
                    m_owner->reload( nullptr );
                }
            }
            else if( message->isExactly<StateMessageFragmentParam>() )
            {
            }
            else if( message->isExactly<StateMessageSetTexture>() )
            {
            }

            return false;
        }

        bool CFont::MaterialStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        CFont *CFont::MaterialStateListener::getOwner() const
        {
            return m_owner;
        }

        void CFont::MaterialStateListener::setOwner( CFont *owner )
        {
            m_owner = owner;
        }

        CFont::MaterialStateListener::~MaterialStateListener()
        {
        }

        CFont::MaterialStateListener::MaterialStateListener()
        {
        }

        CFont::MaterialStateListener::MaterialStateListener( CFont *material ) : m_owner( material )
        {
        }

        void CFont::MaterialEvents::preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt )
        {
        }

        void CFont::MaterialEvents::postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt )
        {
        }

        void CFont::MaterialEvents::preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CFont::MaterialEvents::postViewportUpdate( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CFont::MaterialEvents::viewportAdded( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CFont::MaterialEvents::viewportRemoved( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        CFont::MaterialEvents::~MaterialEvents()
        {
        }

        CFont::MaterialEvents::MaterialEvents( CFont *material ) : m_material( material )
        {
        }
    }  // end namespace render
}  // namespace workphone
