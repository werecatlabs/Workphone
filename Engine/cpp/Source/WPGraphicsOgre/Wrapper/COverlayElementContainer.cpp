#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementContainer.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

#include "OgreUtil.hpp"

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayElementContainer,
                                   OverlayElement<IOverlayElementContainer> );

        COverlayElementContainer::COverlayElementContainer() :
            COverlayElementOgre<IOverlayElementContainer>()
        {
            createStateContext();
        }

        COverlayElementContainer::~COverlayElementContainer()
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                unload( nullptr );
            }
        }

        void COverlayElementContainer::load( SmartPtr<ISharedObject> data )
        {
            using namespace Ogre;

            try
            {
                setLoadingState( LoadingState::Loading );

                auto overlayMgr = OverlayManager::getSingletonPtr();
                WP_ASSERT( overlayMgr );

                auto name = getName();
                auto panel = static_cast<Ogre::OverlayContainer *>(
                    overlayMgr->createOverlayElement( "Panel", name.c_str() ) );
                WP_ASSERT( panel );

                panel->initialise();
                //panel->updatePositionGeometry();
                //panel->updateTextureGeometry();

                auto materialManager = Ogre::MaterialManager::getSingletonPtr();
                auto defaultMaterial = materialManager->createOrRetrieve(
                    "DefaultUi", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                panel->setMaterial(
                    Ogre::dynamic_pointer_cast<Ogre::Material>( defaultMaterial.first ) );

                //panel->setMetricsMode( Ogre::GuiMetricsMode::GMM_RELATIVE );
                panel->setMetricsMode( Ogre::GuiMetricsMode::GMM_PIXELS );
                panel->_setLeft( 0.0f );
                panel->_setTop( 0.0f );
                panel->_setWidth( 1.0f );
                panel->_setHeight( 1.0f );
                panel->initialise();

                m_container = panel;

                setElement( m_container );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void COverlayElementContainer::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &state = getLoadingState();
                if( state != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto stateManager = applicationManager->getStateManager();
                    WP_ASSERT( stateManager );

                    if( m_materialStateListener )
                    {
                        if( m_material )
                        {
                            if( auto materialStateObject = m_material->getStateContext() )
                            {
                                materialStateObject->removeStateListener( m_materialStateListener );
                            }
                        }

                        m_materialStateListener->unload( nullptr );
                        m_materialStateListener = nullptr;
                    }

                    if( auto stateListener = getStateListener() )
                    {
                        stateListener->unload( nullptr );
                        setStateListener( nullptr );
                    }

                    //if( m_state )
                    //{
                    //    m_state->unload( nullptr );
                    //    m_state = nullptr;
                    //}

                    if( auto stateContext = getStateContext() )
                    {
                        if( auto stateListener = getStateListener() )
                        {
                            stateContext->removeStateListener( stateListener );
                        }

                        stateManager->removeStateContext( stateContext );

                        stateContext->unload( nullptr );
                        setStateContext( nullptr );
                    }

                    if( m_container )
                    {
                        //m_container->_setNullDatablock();

                        auto overlayMgr = Ogre::OverlayManager::getSingletonPtr();
                        WP_ASSERT( overlayMgr );

                        overlayMgr->destroyOverlayElement( m_container );
                        m_container = nullptr;
                    }

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void COverlayElementContainer::setupMaterial( SmartPtr<IMaterial> material )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            if( m_material )
            {
                auto materialStateObject = m_material->getStateContext();
                if( materialStateObject )
                {
                    materialStateObject->removeStateListener( m_materialStateListener );
                }
            }

            m_material = material;

            if( m_material )
            {
                auto materialStateObject = m_material->getStateContext();
                if( materialStateObject )
                {
                    materialStateObject->addStateListener( m_materialStateListener );
                }
            }

            if( m_material )
            {
                if( m_material->getLoadingState() == LoadingState::Loaded )
                {
                    auto pMaterial = workphone::static_pointer_cast<CMaterialOgre>( m_material );
                    auto pOgreMaterial = pMaterial->getMaterial();
                    if( pOgreMaterial )
                    {
                        auto materialName = pOgreMaterial->getName();
                        if( !StringUtil::isNullOrEmpty( materialName.c_str() ) )
                        {
                            m_container->setMaterialName( materialName );
                        }
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageMaterial>();
                        message->setMaterial( material );

                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->addMessage( stateTask, message );
                        }
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageMaterial>();
                    message->setMaterial( material );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->addMessage( stateTask, message );
                    }
                }
            }
        }

        bool COverlayElementContainer::isContainer() const
        {
            return true;
        }

        void COverlayElementContainer::_getObject( void **ppObject ) const
        {
            *ppObject = m_container;
        }

        void COverlayElementContainer::addChild( SmartPtr<IOverlayElement> element )
        {
            try
            {
                WP_ASSERT( element );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto renderTask = graphicsSystem->getRenderTask();
                auto stateTask = graphicsSystem->getStateTask();
                auto task = Thread::getCurrentTask();

                const auto &loadingState = getLoadingState();
                if( loadingState == LoadingState::Loaded && task == renderTask )
                {
                    const auto &childLoadingState = element->getLoadingState();

                    if( childLoadingState == LoadingState::Loaded )
                    {
                        element->setParent( this );

                        Ogre::OverlayElement *ogreElement;
                        element->_getObject( (void **)&ogreElement );

                        WP_ASSERT( m_container );
                        WP_ASSERT( ogreElement );

                        if( m_container )
                        {
                            if( ogreElement )
                            {
                                m_container->addChild( ogreElement );
                            }
                        }

                        OverlayElement<IOverlayElementContainer>::addChild( element );
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setType( STATE_MESSAGE_ADDCHILD );
                        message->setObject( element );

                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->addMessage( stateTask, message );
                        }
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( STATE_MESSAGE_ADDCHILD );
                    message->setObject( element );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->addMessage( stateTask, message );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void COverlayElementContainer::removeChild( SmartPtr<IOverlayElement> element )
        {
            try
            {
                WP_ASSERT( element );

                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto factoryManager = applicationManager->getFactoryManager();

                auto renderTask = graphicsSystem->getRenderTask();
                auto stateTask = graphicsSystem->getStateTask();
                auto task = Thread::getCurrentTask();

                const auto &loadingState = getLoadingState();
                if( loadingState == LoadingState::Loaded && task == renderTask )
                {
                    if( element )
                    {
                        element->setParent( nullptr );

                        Ogre::OverlayElement *ogreElement;
                        element->_getObject( (void **)&ogreElement );

                        if( m_container )
                        {
                            if( ogreElement )
                            {
                                m_container->removeChild( ogreElement->getName() );
                            }
                        }

                        OverlayElement<IOverlayElementContainer>::removeChild( element );
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( STATE_MESSAGE_REMOVECHILD );
                    message->setObject( element );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->addMessage( stateTask, message );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void COverlayElementContainer::createStateContext()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();
            m_stateContext = stateContext;
            stateContext->setOwner( this );

            auto listener = factoryManager->make_ptr<StateListener>();
            listener->setOwner( this );
            m_stateListener = listener;
            stateContext->addStateListener( listener );

            auto state = factoryManager->make_ptr<OverlayContainerState>();
            stateContext->addState( state );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );

            auto materialStateListener = factoryManager->make_ptr<MaterialStateListener>();
            materialStateListener->setOwner( this );
            m_materialStateListener = materialStateListener;
        }

        void COverlayElementContainer::materialLoaded( SmartPtr<IMaterial> material )
        {
            if( material )
            {
                setMaterial( material );
            }
        }

        SmartPtr<Properties> COverlayElementContainer::getProperties() const
        {
            auto colour = OgreUtil::convert( m_container->getColour() );

            auto properties = OverlayElement<IOverlayElementContainer>::getProperties();
            properties->setProperty( "Type", "OverlayElementContainer" );
            properties->setProperty( "MetricsMode",
                                     StringUtil::toString( m_container->getMetricsMode() ) );
            properties->setProperty( "HorizontalAlignment",
                                     StringUtil::toString( m_container->getHorizontalAlignment() ) );
            properties->setProperty( "VerticalAlignment",
                                     StringUtil::toString( m_container->getVerticalAlignment() ) );
            properties->setProperty( "Left", StringUtil::toString( m_container->getLeft() ) );
            properties->setProperty( "Top", StringUtil::toString( m_container->getTop() ) );
            properties->setProperty( "Width", StringUtil::toString( m_container->getWidth() ) );
            properties->setProperty( "Height", StringUtil::toString( m_container->getHeight() ) );
            properties->setProperty( "Visible", StringUtil::toString( m_container->isVisible() ) );
            properties->setProperty( "MaterialName", m_container->getMaterialName().c_str() );
            properties->setProperty( "Colour", StringUtil::toString( colour ) );
            properties->setProperty( "Caption", m_container->getCaption().c_str() );
            return properties;
        }

        void COverlayElementContainer::setProperties( SmartPtr<Properties> properties )
        {
            OverlayElement<IOverlayElementContainer>::setProperties( properties );
        }

        Array<SmartPtr<ISharedObject>> COverlayElementContainer::getChildObjects() const
        {
            Array<SmartPtr<ISharedObject>> objects;
            objects.reserve( 6 );

            if( m_material )
            {
                objects.push_back( m_material );
            }

            if( auto stateContext = getStateContext() )
            {
                objects.push_back( stateContext );
            }

            //if( m_stateListener )
            //{
            //    objects.push_back( m_stateListener );
            //}

            if( m_materialStateListener )
            {
                objects.push_back( m_materialStateListener );
            }

            //if( m_state )
            //{
            //    objects.push_back( m_state );
            //}

            return objects;
        }

        bool COverlayElementContainer::isValid() const
        {
            const auto &state = getLoadingState();
            switch( state )
            {
            case LoadingState::Unloaded:
            {
                auto stateContext = getStateContext();
                auto stateListener = getStateListener();

                if( stateContext || stateListener || m_materialStateListener )
                {
                    return false;
                }

                return m_container == nullptr;
            }
            break;
            case LoadingState::Loading:
            {
                auto stateContext = getStateContext();
                auto stateListener = getStateListener();

                if( stateContext || stateListener || m_materialStateListener )
                {
                    return false;
                }

                return m_container == nullptr;
            }
            break;
            case LoadingState::Loaded:
            {
                auto stateContext = getStateContext();
                auto stateListener = getStateListener();

                if( stateContext && stateListener && m_materialStateListener )
                {
                    if( stateContext->isValid() && stateListener->isValid() &&
                        m_materialStateListener->isValid() && m_container != nullptr )
                    {
                        return true;
                    }

                    return false;
                }
            }
            break;

            case LoadingState::Unloading:
            {
                return m_container == nullptr;
            }
            break;
            default:
            {
            }
            }

            return OverlayElement<IOverlayElementContainer>::isValid();
        }

        bool COverlayElementContainer::StateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            StateListenerOgre::handleStateChanged( state );
            return false;
        }

        bool COverlayElementContainer::StateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            // auto applicationManager = core::IApplicationManager::instance();
            // auto graphicsSystem = applicationManager->getGraphicsSystem();
            // auto factoryManager = applicationManager->getFactoryManager();

            if( auto owner = workphone::static_pointer_cast<COverlayElementContainer>(
                    StateListenerOgre::getOwner() ) )
            {
                if( message->isExactly<StateMessageMaterial>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageMaterial>( message );
                    WP_ASSERT( objectMessage );

                    owner->setMaterial( objectMessage->getMaterial() );
                }
                else if( message->isExactly<StateMessageMaterialName>() )
                {
                    auto objectMessage =
                        workphone::static_pointer_cast<StateMessageMaterialName>( message );
                    WP_ASSERT( objectMessage );

                    //owner->setMaterialName( objectMessage->getMaterialName() );
                }
                else if( message->isExactly<StateMessageVisible>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageVisible>( message );
                    WP_ASSERT( objectMessage );

                    owner->setVisible( objectMessage->isVisible() );
                }
                else if( message->isExactly<StateMessageObject>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                    WP_ASSERT( objectMessage );

                    auto messageType = objectMessage->getType();
                    auto object = objectMessage->getObject();

                    if( messageType == STATE_MESSAGE_ADDCHILD )
                    {
                        owner->addChild( object );
                    }
                    else if( messageType == STATE_MESSAGE_REMOVECHILD )
                    {
                        owner->removeChild( object );
                    }
                }
                else if( message->isExactly<StateMessageUIntValue>() )
                {
                    auto objectMessage =
                        workphone::static_pointer_cast<StateMessageUIntValue>( message );
                    WP_ASSERT( objectMessage );

                    auto messageType = objectMessage->getType();
                    auto messageValue = objectMessage->getValue();

                    if( messageType == IStateMessage::STATE_MESSAGE_METRICSMODE )
                    {
                        owner->setMetricsMode( static_cast<u8>( messageValue ) );
                    }
                    else if( messageType == IStateMessage::STATE_MESSAGE_ALIGN_HORIZONTAL )
                    {
                        owner->setHorizontalAlignment( static_cast<u8>( messageValue ) );
                    }
                    else if( messageType == IStateMessage::STATE_MESSAGE_ALIGN_VERTICAL )
                    {
                        owner->setVerticalAlignment( static_cast<u8>( messageValue ) );
                    }
                }
                else if( message->isExactly<StateMessageFloatValue>() )
                {
                    auto objectMessage =
                        workphone::static_pointer_cast<StateMessageFloatValue>( message );
                    WP_ASSERT( objectMessage );

                    auto messageType = objectMessage->getType();
                    auto messageValue = objectMessage->getValue();

                    if( messageType == StateMessageFloatValue::WIDTH_HASH )
                    {
                        //owner->setWidth( messageValue );
                    }
                    else if( messageType == StateMessageFloatValue::HEIGHT_HASH )
                    {
                        //owner->setHeight( messageValue );
                    }
                    else if( messageType == StateMessageFloatValue::TOP_HASH )
                    {
                        //owner->setTop( messageValue );
                    }
                    else if( messageType == StateMessageFloatValue::LEFT_HASH )
                    {
                        //owner->setLeft( messageValue );
                    }
                }
            }

            return false;
        }

        COverlayElementContainer::MaterialStateListener::MaterialStateListener(
            COverlayElementContainer *owner ) :
            m_owner( owner )
        {
        }

        COverlayElementContainer::MaterialStateListener::~MaterialStateListener()
        {
        }

        bool COverlayElementContainer::MaterialStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                if( auto stateContent = owner->getStateContext() )
                {
                    stateContent->setDirty( true );
                }
            }

            return false;
        }

        bool COverlayElementContainer::MaterialStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isExactly<StateMessageLoad>() )
                {
                    auto stateMessageLoad = workphone::static_pointer_cast<StateMessageLoad>( message );
                    auto eventType = stateMessageLoad->getType();

                    if( eventType == StateMessageLoad::LOADED_HASH )
                    {
                        owner->materialLoaded( stateMessageLoad->getObject() );
                    }
                }
            }

            return false;
        }

        COverlayElementContainer *COverlayElementContainer::MaterialStateListener::getOwner() const
        {
            return m_owner;
        }

        void COverlayElementContainer::MaterialStateListener::setOwner( COverlayElementContainer *owner )
        {
            m_owner = owner;
        }
    }  // end namespace render
}  // namespace workphone
