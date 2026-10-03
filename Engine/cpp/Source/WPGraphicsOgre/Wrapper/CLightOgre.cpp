#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CLightOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreLight.h>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, CLightOgre, CGraphicsObjectOgre<GraphicsLight> );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CLightOgre::CLightStateListener,
                                   GraphicsObjectOgreStateListener );

        CLightOgre::CLightOgre()
        {
            setupStateObject();
        }

        CLightOgre::~CLightOgre()
        {
            unload( nullptr );
        }

        void CLightOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                WP_ASSERT( !getOwner() );

                auto name = getName();

                Ogre::SceneManager *smgr = nullptr;

                if( auto creator = getCreator() )
                {
                    creator->_getObject( (void **)&smgr );
                }

                if( smgr )
                {
                    auto ogreLight = smgr->createLight( name.c_str() );
                    m_light = ogreLight;

                    m_dummyNode = smgr->createSceneNode();
                    m_dummyNode->attachObject( ogreLight );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( Ogre::Exception &e )
            {
                auto message = e.getFullDescription();
                WP_LOG_ERROR( message.c_str() );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CLightOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto owner = workphone::static_pointer_cast<CSceneNodeOgre>( getOwner() );
                    if( owner )
                    {
                        owner->detachObjectPtr( this );
                        setOwner( nullptr );
                    }

                    if( auto smgr = getCreator() )
                    {
                        Ogre::SceneManager *ogreSmgr = nullptr;
                        smgr->_getObject( (void **)&ogreSmgr );

                        if( m_dummyNode )
                        {
                            m_dummyNode->detachAllObjects();
                            ogreSmgr->destroySceneNode( m_dummyNode );
                            m_dummyNode = nullptr;
                        }

                        if( auto light = getLight() )
                        {
                            ogreSmgr->destroyLight( light );
                            setLight( nullptr );
                        }
                    }

                    CGraphicsObjectOgre<GraphicsLight>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( Ogre::Exception &e )
            {
                auto message = e.getFullDescription();
                WP_LOG_ERROR( message.c_str() );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IGraphicsObject> CLightOgre::clone( const String &name ) const
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto light = factoryManager->make_ptr<CLightOgre>();
            return light;
        }

        void CLightOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_light;
        }

        Ogre::Light *CLightOgre::getLight() const
        {
            return m_light;
        }

        void CLightOgre::setLight( Ogre::Light *light )
        {
            m_light = light;
        }

        void CLightOgre::setupStateObject()
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                CGraphicsObjectOgre<GraphicsLight>::setupStateObject();

                auto pThis = getSharedFromThis<CLightOgre>();

                auto stateContext = getStateContext();

                auto lightState = factoryManager->make_ptr<State>();
                lightState->setOwner( pThis );

                auto stateTask = graphicsSystem->getStateTask();
                stateContext->setTaskId( stateTask );
                stateContext->addState( lightState );

                auto stateListener = factoryManager->make_ptr<CLightStateListener>();
                stateListener->setOwner( pThis );

                m_stateListener = stateListener;
                stateContext->addStateListener( stateListener );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CLightOgre::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            if( parent )
            {
                if( m_dummyNode )
                {
                    if( !m_light->isAttached() )
                    {
                        m_dummyNode->attachObject( m_light );
                    }
                }
            }
        }

        void CLightOgre::detachFromParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            if( parent )
            {
                if( m_dummyNode )
                {
                    if( m_light->isAttached() && m_light->getParentSceneNode() == m_dummyNode )
                    {
                        m_dummyNode->detachObject( m_light );
                    }
                }
            }
        }

        CLightOgre::CLightStateListener::CLightStateListener() = default;

        CLightOgre::CLightStateListener::~CLightStateListener() = default;

        bool CLightOgre::CLightStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            auto renderTask = graphicsSystem->getRenderTask();
            auto currentTaskId = Thread::getCurrentTask();
            WP_ASSERT( currentTaskId == renderTask );

            if( currentTaskId == renderTask )
            {
                if( auto owner = workphone::static_pointer_cast<CLightOgre>( getOwner() ) )
                {
                    if( message->isExactly<StateMessageVisible>() )
                    {
                        auto visibleMessage =
                            workphone::static_pointer_cast<StateMessageVisible>( message );
                        WP_ASSERT( visibleMessage );

                        auto visible = visibleMessage->isVisible();
                        owner->setVisible( visible );
                    }
                    else if( message->isExactly<StateMessageUIntValue>() )
                    {
                        auto valueMessage =
                            workphone::static_pointer_cast<StateMessageUIntValue>( message );
                        auto type = valueMessage->getType();
                        auto value = valueMessage->getValue();

                        if( type == VISIBILITY_MASK_HASH )
                        {
                            owner->setVisibilityFlags( value );
                        }
                    }
                    else if( message->isExactly<StateMessageIntValue>() )
                    {
                        auto valueMessage =
                            workphone::static_pointer_cast<StateMessageIntValue>( message );
                        auto type = valueMessage->getType();
                        auto value = valueMessage->getValue();

                        if( type == LIGHT_TYPE_HASH )
                        {
                            owner->setType( static_cast<LightTypes>( value ) );
                        }
                    }
                    else if( message->isExactly<StateMessageVector3>() )
                    {
                        auto valueMessage =
                            workphone::static_pointer_cast<StateMessageVector3>( message );
                        auto type = valueMessage->getType();
                        auto value = valueMessage->getValue();

                        if( type == STATE_MESSAGE_DIRECTION )
                        {
                            owner->setDirection( value );
                        }
                    }
                    else if( message->isExactly<StateMessageVector4>() )
                    {
                        auto valueMessage =
                            workphone::static_pointer_cast<StateMessageVector4>( message );
                        auto type = valueMessage->getType();
                        auto value = valueMessage->getValue();

                        if( type == DIFFUSE_COLOUR_HASH )
                        {
                            owner->setDiffuseColour(
                                ColourF( value.X(), value.Y(), value.Z(), value.W() ) );
                        }
                    }
                }
            }

            return false;
        }

        bool CLightOgre::CLightStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            try
            {
                if( auto owner = workphone::static_pointer_cast<CLightOgre>( getOwner() ) )
                {
                    if( state->isDerived<LightStateData>() )
                    {
                        auto lightState = workphone::static_pointer_cast<LightStateData>( state );
                        if( lightState )
                        {
                            auto visible =
                                BitUtil::getFlagValue( lightState->flags, IGraphicsObject::visibleFlag );

                            if( auto light = owner->getLight() )
                            {
                                light->setDirection( OgreUtil::convertToOgre( lightState->direction ) );

                                light->setVisible( visible );
                                light->setRenderQueueGroup( lightState->renderQueueGroup );

                                light->setDiffuseColour(
                                    OgreUtil::convertToOgre( lightState->diffuseColour ) );
                                light->setSpecularColour(
                                    OgreUtil::convertToOgre( lightState->specularColour ) );

                                if( (u32)lightState->lightType < 3 )
                                {
                                    light->setType( (Ogre::Light::LightTypes)lightState->lightType );
                                }
                            }
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

    }  // end namespace render
}  // namespace workphone
