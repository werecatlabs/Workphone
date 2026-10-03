#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementContainer.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlay.h>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreRoot.h>
#include <OgreResourceGroupManager.h>
#include <OgreGpuProgramParams.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsManager.h>
#include <OgreTextureGpuManager.h>
#include "OgreUtil.hpp"

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayElementContainer,
                               COverlayElementOgreNext<IOverlayElementContainer> );

    COverlayElementContainer::COverlayElementContainer() :
        COverlayElementOgreNext<IOverlayElementContainer>()
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

        destroyStateContext();
    }

    void COverlayElementContainer::load( SmartPtr<ISharedObject> data )
    {
        using namespace Ogre;

        try
        {
            if( !getStateContext() )
            {
                createStateContext();
            }

            setLoadingState( LoadingState::Loading );

            auto overlayMgr = Ogre::v1::OverlayManager::getSingletonPtr();
            WP_ASSERT( overlayMgr );
            if( !overlayMgr )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto name = getName();
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );
            if( StringUtil::isNullOrEmpty( name ) )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto panel =
                (Ogre::v1::OverlayContainer *)overlayMgr->createOverlayElement( "Panel", name.c_str() );
            WP_ASSERT( panel );
            if( !panel )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            panel->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            panel->initialise();
            //panel->updatePositionGeometry();
            //panel->updateTextureGeometry();

            panel->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            panel->setLeft( 0 );
            panel->setTop( 0 );
            panel->setWidth( 100.0 );
            panel->setHeight( 100.0 );
            panel->show();

            setElement( panel );
            setContainerElement( panel );
            WP_ASSERT( getElement() );
            WP_ASSERT( getContainerElement() );

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

    void COverlayElementContainer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager ? applicationManager->getStateManager() : nullptr;
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

                m_material = nullptr;

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

                if( auto stateListener = getStateListener() )
                {
                    stateListener->unload( nullptr );
                    setStateListener( nullptr );
                }

                if( auto container = getContainerElement() )
                {
                    auto children = getChildren();
                    for( auto child : children )
                    {
                        WP_ASSERT( child );
                        if( child )
                        {
                            Ogre::v1::OverlayElement *childElement = nullptr;
                            child->_getObject( reinterpret_cast<void **>( &childElement ) );

                            if( childElement && childElement->getParent() == container )
                            {
                                container->removeChild( childElement->getName() );
                            }

                            child->setParent( nullptr );
                            child->setOverlay( nullptr );
                            COverlayElementOgreNext<IOverlayElementContainer>::removeChild( child );
                        }
                    }

                    container->_setNullDatablock();

                    auto overlayMgr = Ogre::v1::OverlayManager::getSingletonPtr();
                    WP_ASSERT( overlayMgr );

                    if( auto parent = container->getParent() )
                    {
                        parent->removeChild( container->getName() );
                    }

                    container->_notifyParent( nullptr, nullptr );
                    if( overlayMgr )
                    {
                        overlayMgr->destroyOverlayElement( container );
                    }

                    setElement( nullptr );
                    setContainerElement( nullptr );
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
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                return;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );
            if( !factoryManager )
            {
                return;
            }

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                WP_ASSERT( m_container );
                if( !m_container )
                {
                    return;
                }

                if( m_material )
                {
                    auto materialStateObject = m_material->getStateContext();
                    if( materialStateObject && m_materialStateListener )
                    {
                        materialStateObject->removeStateListener( m_materialStateListener );
                    }
                }

                m_material = material;

                if( m_material )
                {
                    auto materialStateObject = m_material->getStateContext();
                    if( materialStateObject && m_materialStateListener )
                    {
                        materialStateObject->addStateListener( m_materialStateListener );
                    }
                }

                if( m_material )
                {
                    if( m_material->getLoadingState() == LoadingState::Loaded )
                    {
                        auto pMaterial = workphone::static_pointer_cast<CMaterialOgreNext>( m_material );
                        WP_ASSERT( pMaterial );
                        if( !pMaterial )
                        {
                            return;
                        }

                        auto datablock = pMaterial->getHlmsDatablock();
                        WP_ASSERT( datablock );
                        if( datablock )
                        {
                            auto materialName = pMaterial->getDatablockName();

                            WP_ASSERT( !StringUtil::isNullOrEmpty( materialName ) );

                            m_container->setMaterialName( materialName.c_str() );

                            //m_container->setDatablock(datablock);
                            //m_container->updatePositionGeometry();
                            //m_container->updateTextureGeometry();

                            m_container->setDatablock( datablock );
                        }
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageMaterial>();
                        message->setMaterial( material );
                        message->setSender( this );

                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->addMessage( renderTask, message );
                        }
                    }
                }
                else
                {
                    m_container->_setNullDatablock();
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageMaterial>();
                message->setMaterial( material );
                message->setSender( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
                }
            }
        }
        catch( Ogre::Exception &e )
        {
            auto errorMessage = String( e.getFullDescription().c_str() );
            WP_LOG_ERROR( errorMessage );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto COverlayElementContainer::isContainer() const -> bool
    {
        return true;
    }

    void COverlayElementContainer::addChild( SmartPtr<IOverlayElement> element )
    {
        try
        {
            WP_ASSERT( element );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                return;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                return;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );
            if( !factoryManager )
            {
                return;
            }

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                if( !element->isLoaded() )
                {
                    graphicsSystem->loadObject( element );
                }

                const auto &childLoadingState = element->getLoadingState();
                WP_ASSERT( childLoadingState == LoadingState::Loaded );

                if( childLoadingState == LoadingState::Loaded )
                {
                    if( auto parent = element->getParent() )
                    {
                        if( parent.get() != this )
                        {
                            parent->removeChild( element );
                        }
                    }

                    element->setParent( this );
                    if( auto overlay = getOverlay() )
                    {
                        element->setOverlay( overlay );
                    }
                    else if( auto parent = getParent() )
                    {
                        element->setOverlay( parent->getOverlay() );
                    }

                    Ogre::v1::OverlayElement *ogreElement = nullptr;
                    element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

                    WP_ASSERT( m_container );
                    WP_ASSERT( ogreElement );

                    if( m_container )
                    {
                        if( ogreElement )
                        {
                            if( auto ogreParent = ogreElement->getParent() )
                            {
                                if( ogreParent != m_container )
                                {
                                    ogreParent->removeChild( ogreElement->getName() );
                                }
                            }

                            if( ogreElement->getParent() != m_container )
                            {
                                m_container->addChild( ogreElement );
                            }
                        }
                    }

                    auto children = getChildren();
                    if( std::find( children.begin(), children.end(), element ) == children.end() )
                    {
                        COverlayElementOgreNext<IOverlayElementContainer>::addChild( element );
                    }

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( STATE_MESSAGE_ADDCHILD );
                    message->setObject( element );
                    message->setSender( this );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->addMessage( renderTask, message );
                    }
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setType( STATE_MESSAGE_ADDCHILD );
                message->setObject( element );
                message->setSender( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
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
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                return;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                return;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );
            if( !factoryManager )
            {
                return;
            }

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                if( element )
                {
                    element->setParent( nullptr );
                    element->setOverlay( nullptr );

                    Ogre::v1::OverlayElement *ogreElement = nullptr;
                    element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

                    if( m_container )
                    {
                        if( ogreElement && ogreElement->getParent() == m_container )
                        {
                            m_container->removeChild( ogreElement->getName() );
                        }
                    }

                    COverlayElementOgreNext<IOverlayElementContainer>::removeChild( element );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setType( STATE_MESSAGE_REMOVECHILD );
                message->setObject( element );
                message->setSender( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
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
        if( !applicationManager )
        {
            return;
        }

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );
        if( !stateManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        if( !factoryManager )
        {
            return;
        }

        auto stateContext = stateManager->addStateContext();
        WP_ASSERT( stateContext );
        if( !stateContext )
        {
            return;
        }

        m_stateContext = stateContext;
        stateContext->setOwner( this );

        auto listener = factoryManager->make_ptr<StateListener>();
        WP_ASSERT( listener );
        listener->setOwner( this );
        m_stateListener = listener;
        stateContext->addStateListener( listener );

        auto state = factoryManager->make_ptr<State>();
        WP_ASSERT( state );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<OverlayContainerState>();
        WP_ASSERT( stateData );
        state->setData( stateData );

        stateContext->setTaskId( TaskId::Render );

        auto materialStateListener = factoryManager->make_ptr<MaterialStateListener>();
        WP_ASSERT( materialStateListener );
        materialStateListener->setOwner( this );
        m_materialStateListener = materialStateListener;
    }

    void COverlayElementContainer::materialLoaded( SmartPtr<IMaterial> material )
    {
        WP_ASSERT( material );
        if( material )
        {
            setMaterial( material );
        }
    }

    auto COverlayElementContainer::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = COverlayElementOgreNext<IOverlayElementContainer>::getProperties();
        WP_ASSERT( properties );
        properties->setProperty( "materialName", m_materialName );

        if( auto element = getElement() )
        {
            auto position = Vector2F( element->getLeft(), element->getTop() );
            properties->setProperty( "position", position );

            auto size = Vector2F( element->getWidth(), element->getHeight() );
            properties->setProperty( "size", size );
        }

        properties->setProperty( "Type", "OverlayElementContainer" );

        WP_ASSERT( m_container || !isLoaded() );
        if( m_container )
        {
            auto colour = OgreUtil::convert( m_container->getColour() );
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
        }

        return properties;
    }

    void COverlayElementContainer::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            return;
        }

        COverlayElementOgreNext<IOverlayElementContainer>::setProperties( properties );
    }

    auto COverlayElementContainer::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 6 );

        if( m_material )
        {
            objects.emplace_back( m_material );
        }

        if( auto stateContext = getStateContext() )
        {
            objects.emplace_back( stateContext );
        }

        if( auto stateListener = getStateListener() )
        {
            objects.emplace_back( stateListener );
        }

        if( m_materialStateListener )
        {
            objects.emplace_back( m_materialStateListener );
        }

        return objects;
    }

    auto COverlayElementContainer::isValid() const -> bool
    {
        auto stateContext = getStateContext();
        auto stateListener = getStateListener();

        const auto &state = getLoadingState();
        switch( state )
        {
        case LoadingState::Unloaded:
        {
            if( stateContext || stateListener || m_materialStateListener )
            {
                return false;
            }

            return m_container == nullptr;
        }
        break;
        case LoadingState::Loading:
        {
            if( stateContext || stateListener || m_materialStateListener )
            {
                return false;
            }

            return m_container == nullptr;
        }
        break;
        case LoadingState::Loaded:
        {
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
        };

        return COverlayElementOgreNext<IOverlayElementContainer>::isValid();
    }

    auto COverlayElementContainer::getContainerElement() const -> Ogre::v1::OverlayContainer *
    {
        return m_container;
    }

    void COverlayElementContainer::setContainerElement( Ogre::v1::OverlayContainer *container )
    {
        m_container = container;
    }

    bool COverlayElementContainer::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        StateListenerOgre::handleStateChanged( state );
        return false;
    }

    bool COverlayElementContainer::StateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        WP_ASSERT( message );
        if( !message )
        {
            return false;
        }

        StateListenerOgre::handleStateMessage( message );

        if( auto owner = getOwner() )
        {
            // auto applicationManager = core::IApplicationManager::instance();
            // auto graphicsSystem = applicationManager->getGraphicsSystem();
            // auto factoryManager = applicationManager->getFactoryManager();

            if( message->isExactly<StateMessageMaterial>() )
            {
                auto objectMessage = workphone::static_pointer_cast<StateMessageMaterial>( message );
                WP_ASSERT( objectMessage );

                auto material = objectMessage->getMaterial();

                owner->setMaterial( material );
            }
            else if( message->isExactly<StateMessageMaterialName>() )
            {
                auto objectMessage = workphone::static_pointer_cast<StateMessageMaterialName>( message );
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
                WP_ASSERT( object );

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
                auto objectMessage = workphone::static_pointer_cast<StateMessageUIntValue>( message );
                WP_ASSERT( objectMessage );

                auto messageType = objectMessage->getType();
                auto messageValue = objectMessage->getValue();

                if( messageType == STATE_MESSAGE_METRICSMODE )
                {
                    owner->setMetricsMode( static_cast<u8>( messageValue ) );
                }
                else if( messageType == STATE_MESSAGE_ALIGN_HORIZONTAL )
                {
                    owner->setHorizontalAlignment( static_cast<u8>( messageValue ) );
                }
                else if( messageType == STATE_MESSAGE_ALIGN_VERTICAL )
                {
                    owner->setVerticalAlignment( static_cast<u8>( messageValue ) );
                }
            }
            else if( message->isExactly<StateMessageFloatValue>() )
            {
                auto objectMessage = workphone::static_pointer_cast<StateMessageFloatValue>( message );
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
        COverlayElementContainer *owner )
    {
        m_owner = owner;
    }

    COverlayElementContainer::MaterialStateListener::~MaterialStateListener() = default;

    bool COverlayElementContainer::MaterialStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        WP_ASSERT( state );
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
        WP_ASSERT( message );
        if( !message )
        {
            return false;
        }

        if( auto owner = getOwner() )
        {
            if( owner->isLoaded() )
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
        }

        return false;
    }

    auto COverlayElementContainer::MaterialStateListener::getOwner() const
        -> SmartPtr<COverlayElementContainer>
    {
        return m_owner;
    }

    void COverlayElementContainer::MaterialStateListener::setOwner(
        SmartPtr<COverlayElementContainer> owner )
    {
        m_owner = owner;
    }

}  // namespace workphone::render
