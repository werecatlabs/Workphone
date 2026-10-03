#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTextureOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTechniqueOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialPassOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreTextureManager.h>
#include <OgreTexture.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CMaterialTextureOgre, MaterialTexture );
        WP_CLASS_REGISTER_DERIVED( workphone, CMaterialTextureOgre::MaterialTextureOgreStateListener,
                                   MaterialTexture::MaterialTextureStateListener );
        WP_CLASS_REGISTER_DERIVED( workphone, CMaterialTextureOgre::TextureListener, IEventListener );

        CMaterialTextureOgre::CMaterialTextureOgre()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            auto stateContext = stateManager->addStateContext();
            setStateContext( stateContext );

            auto stateListener = factoryManager->make_ptr<MaterialTextureOgreStateListener>();
            stateListener->setOwner( this );
            setStateListener( stateListener );
            stateContext->addStateListener( stateListener );

            auto state = factoryManager->make_ptr<State>();
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<MaterialPassStateData>();
            state->setData( stateData );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );

            auto listener = workphone::make_ptr<TextureListener>();
            listener->setOwner( this );
            m_textureListener = listener;
        }

        CMaterialTextureOgre::~CMaterialTextureOgre()
        {
            unload( nullptr );
        }

        void CMaterialTextureOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                if( isEnabled() )
                {
                    if( !m_textureUnitState )
                    {
                        createTextureUnitState();
                    }
                }

                if( auto texture = getTexture() )
                {
                    if( !texture->isLoaded() )
                    {
                        texture->load( nullptr );
                    }
                }

                if( auto texture = getTexture() )
                {
                    auto pTexture = workphone::static_pointer_cast<CTextureOgre>( texture );

                    if( auto t = pTexture->getTexture() )
                    {
                        if( auto textureUnitState = getTextureUnitState() )
                        {
                            textureUnitState->setTexture( t );
                        }
                    }
                }

                if( auto stateContext = getStateContext() )
                {
                    stateContext->setDirty( true );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialTextureOgre::createTextureUnitState()
        {
            WP_ASSERT( getParent() );

            if( auto parent = getParent() )
            {
                WP_ASSERT( parent->getMaterial() == getMaterial() );

                auto parentPass = workphone::static_pointer_cast<CMaterialPassOgre>( parent );
                if( parentPass )
                {
                    WP_ASSERT( parentPass->getPass() );

                    if( auto pass = parentPass->getPass() )
                    {
                        m_textureUnitState = pass->createTextureUnitState();
                    }
                }
            }
        }

        void CMaterialTextureOgre::reload( SmartPtr<ISharedObject> data )
        {
        }

        void CMaterialTextureOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    if( m_texture )
                    {
                        m_texture->removeObjectListener( m_textureListener.get() );
                    }

                    m_textureUnitState = nullptr;

                    m_animator = nullptr;
                    m_texture = nullptr;
                    MaterialNode<IMaterialTexture>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialTextureOgre::initialise( Ogre::TextureUnitState *textureUnitState )
        {
            m_textureUnitState = textureUnitState;
        }

        void CMaterialTextureOgre::setTexture( SmartPtr<ITexture> texture )
        {
            //WP_ASSERT( getMaterial() );
            //WP_ASSERT( getMaterial() && getMaterial()->isValid() );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            if( m_texture != texture )
            {
                if( m_texture )
                {
                    m_texture->removeObjectListener( m_textureListener );
                }

                m_texture = texture;

                if( m_texture )
                {
                    m_texture->addObjectListener( m_textureListener );
                }

                if( !texture->isLoaded() )
                {
                    graphicsSystem->loadObject( texture );
                }

                WP_ASSERT( isValid() );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }
        }

        void CMaterialTextureOgre::_getObject( void **ppObject )
        {
            *ppObject = nullptr;
        }

        Array<SmartPtr<ISharedObject>> CMaterialTextureOgre::getChildObjects() const
        {
            try
            {
                auto objects = MaterialNode<IMaterialTexture>::getChildObjects();
                objects.push_back( m_animator );
                objects.push_back( m_texture.load() );
                return objects;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return Array<SmartPtr<ISharedObject>>();
        }

        u32 CMaterialTextureOgre::getTextureType() const
        {
            return m_textureType;
        }

        void CMaterialTextureOgre::setTextureType( u32 textureType )
        {
            m_textureType = textureType;
        }

        Ogre::TextureUnitState *CMaterialTextureOgre::getTextureUnitState() const
        {
            return m_textureUnitState;
        }

        void CMaterialTextureOgre::setTextureUnitState( Ogre::TextureUnitState *textureUnitState )
        {
            m_textureUnitState = textureUnitState;
        }

        CMaterialTextureOgre::TextureListener::TextureListener() = default;

        CMaterialTextureOgre::TextureListener::~TextureListener() = default;

        Parameter CMaterialTextureOgre::TextureListener::handleEvent(
            EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
            SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
        {
            if( eventValue == IEvent::loadingStateChanged )
            {
                auto newState = (LoadingState)arguments[0].getS32();
                if( newState == LoadingState::Loaded )
                {
                    if( auto owner = getOwner() )
                    {
                        if( auto stateContext = owner->getStateContext() )
                        {
                            stateContext->setDirty( true );
                        }
                    }
                }
            }

            return Parameter();
        }

        CMaterialTextureOgre *CMaterialTextureOgre::TextureListener::getOwner() const
        {
            return m_owner;
        }

        void CMaterialTextureOgre::TextureListener::setOwner( CMaterialTextureOgre *owner )
        {
            m_owner = owner;
        }

        CMaterialTextureOgre::MaterialTextureOgreStateListener::MaterialTextureOgreStateListener() =
            default;

        CMaterialTextureOgre::MaterialTextureOgreStateListener::~MaterialTextureOgreStateListener() =
            default;

        bool CMaterialTextureOgre::MaterialTextureOgreStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool CMaterialTextureOgre::MaterialTextureOgreStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            if( auto owner = workphone::static_pointer_cast<CMaterialTextureOgre>( getOwner() ) )
            {
                if( !owner->isLoaded() )
                {
                    return false;
                }

                WP_ASSERT( owner->getMaterial() );

                if( auto material = owner->getMaterial() )
                {
                    //WP_ASSERT( material->isValid() );

                    if( material->isLoaded() )
                    {
                        if( auto texture = owner->getTexture() )
                        {
                            auto pTexture = workphone::static_pointer_cast<CTextureOgre>( texture );

                            if( auto t = pTexture->getTexture() )
                            {
                                if( auto textureUnitState = owner->getTextureUnitState() )
                                {
                                    textureUnitState->setTexture( t );

                                    state->setDirty( false );
                                }
                            }
                        }

                        if( auto stateContext = material->getStateContext() )
                        {
                            stateContext->setDirty( true );
                        }

                        state->setDirty( false );
                    }
                }
            }

            return false;
        }

    }  // end namespace render
}  // namespace workphone
