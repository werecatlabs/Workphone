#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementText.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>
#include <OgreFontManager.h>
#include <OgreTextAreaOverlayElement.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayElementText,
                               COverlayElementOgreNext<IOverlayElementText> );

    u32 COverlayElementText::m_extId = 0;

    COverlayElementText::COverlayElementText() : COverlayElementOgreNext<IOverlayElementText>()
    {
        createStateContext();
    }

    COverlayElementText::~COverlayElementText()
    {
        
    }

    void COverlayElementText::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( !getStateContext() )
            {
                createStateContext();
            }

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto fontManager = graphicsSystem->getFontManager();
            WP_ASSERT( fontManager );

            ScopedLock lock( this );

            auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
            WP_ASSERT( overlayManager );
            if( !overlayManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            static const auto typeName = String( "TextArea" );
            auto instanceName = typeName + "_" + getName() + StringUtil::toString( m_extId++ );
            WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
            // Ogre::TextAreaOverlayElement* overlayElement =
            // (Ogre::TextAreaOverlayElement*)m_overlayMgr->createOverlayElement(typeName.c_str(),
            // instanceName.c_str());
            auto text = (Ogre::v1::TextAreaOverlayElement *)overlayManager->createOverlayElement(
                typeName.c_str(), instanceName.c_str() );
            WP_ASSERT( text );
            if( !text )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            setElement( text );
            setElementText( text );
            WP_ASSERT( getElement() );
            WP_ASSERT( getElementText() );

            // text->setMetricsMode(Ogre::v1::GuiMetricsMode::GMM_RELATIVE);
            ////text->setMetricsMode(Ogre::v1::GuiMetricsMode::GMM_PIXELS);
            // text->setHorizontalAlignment(Ogre::v1::GuiHorizontalAlignment::GHA_CENTER);
            // text->setVerticalAlignment(Ogre::v1::GuiVerticalAlignment::GVA_CENTER);
            //
            // setFontName("DebugFont");
            //
            // text->setCharHeight(18);
            // text->setCaption("Test");
            // text->setSpaceWidth(9);
            // text->setColour(Ogre::ColourValue::White);

            // text->setMetricsMode(Ogre::v1::GuiMetricsMode::GMM_RELATIVE);
            text->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            text->setHorizontalAlignment( Ogre::v1::GuiHorizontalAlignment::GHA_LEFT );
            text->setVerticalAlignment( Ogre::v1::GuiVerticalAlignment::GVA_TOP );

            if( fontManager && fontManager->isLoaded() )
            {
                static const auto defaultFontName = String( "default" );
                text->setFontName( defaultFontName.c_str() );
            }

            text->setCharHeight( 18 );
            // text->setCaption("Test");
            text->setSpaceWidth( 9 );
            text->setColour( Ogre::ColourValue::White );
            text->initialise();
            if( isVisible() )
            {
                text->show();
            }
            else
            {
                text->hide();
            }

            if( auto stateContext = getStateContext() )
            {
                stateContext->setDirty( true );
            }

            // text->setHorizontalAlignment((Ogre::v1::GuiHorizontalAlignment)m_horizontalAlignment);
            // text->setVerticalAlignment((Ogre::v1::GuiVerticalAlignment)m_verticalAlignment);

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void COverlayElementText::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( const auto &loadingState = getLoadingState(); loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto overlayMgr = Ogre::v1::OverlayManager::getSingletonPtr();
                WP_ASSERT( overlayMgr );

                if( auto overlay = getOverlay() )
                {
                    overlay->removeElement( this );
                    setOverlay( nullptr );
                }

                if( auto element = getElement() )
                {
                    element->hide();
                    if( auto parent = element->getParent() )
                    {
                        parent->removeChild( element->getName() );
                    }

                    element->_notifyParent( nullptr, nullptr );

                    if( overlayMgr )
                    {
                        overlayMgr->destroyOverlayElement( element );
                    }

                    setElement( nullptr );
                }

                setElementText( nullptr );

                COverlayElementOgreNext<IOverlayElementText>::unload( data );

                destroyStateContext();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto COverlayElementText::isContainer() const -> bool
    {
        return false;
    }

    void COverlayElementText::setFontName( const String &fontName )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( fontName ) );
            if( StringUtil::isNullOrEmpty( fontName ) )
            {
                return;
            }

            auto fontManager = Ogre::FontManager::getSingletonPtr();
            WP_ASSERT( fontManager );
            if( !fontManager )
            {
                return;
            }

            auto font = fontManager->getByName( fontName.c_str() );
            WP_ASSERT( font );
            if( font )
            {
                if( m_elementText )
                {
                    m_elementText->setFontName( fontName.c_str() );
                }
            }
        }
        catch( Ogre::Exception &e )
        {
            auto errorMessage = e.getFullDescription();
            WP_LOG_ERROR( errorMessage.c_str() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void COverlayElementText::setCharHeight( f32 charHeight )
    {
        WP_ASSERT( charHeight > 0.0f );
        WP_ASSERT( m_elementText );
        if( m_elementText )
        {
            m_elementText->setCharHeight( charHeight );
        }
    }

    void COverlayElementText::setAlignment( u8 alignment )
    {
        WP_ASSERT( alignment <= static_cast<u8>( Ogre::v1::TextAreaOverlayElement::Center ) );
        WP_ASSERT( m_elementText );
        if( m_elementText )
        {
            m_elementText->setAlignment(
                static_cast<Ogre::v1::TextAreaOverlayElement::Alignment>( alignment ) );
        }
    }

    auto COverlayElementText::getAlignment() const -> u8
    {
        if( m_elementText )
        {
            return static_cast<u8>( m_elementText->getAlignment() );
        }

        return 0;
    }

    void COverlayElementText::setSpaceWidth( f32 width )
    {
        WP_ASSERT( width >= 0.0f );
        WP_ASSERT( m_elementText );
        if( m_elementText )
        {
            m_elementText->setSpaceWidth( width );
        }
    }

    auto COverlayElementText::getSpaceWidth() const -> f32
    {
        WP_ASSERT( m_elementText );
        if( m_elementText )
        {
            return m_elementText->getSpaceWidth();
        }

        return 0;
    }

    COverlayElementText::StateListener::StateListener() = default;
    COverlayElementText::StateListener::~StateListener() = default;

    bool COverlayElementText::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        WP_ASSERT( state );
        if( !state )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        WP_ASSERT( graphicsSystem );

        auto fontManager = graphicsSystem ? graphicsSystem->getFontManager() : nullptr;
        WP_ASSERT( fontManager );

        if( fontManager && fontManager->isLoaded() )
        {
            auto stateData = state->getData();
            WP_ASSERT( stateData );
            if( stateData && stateData->isDerived<OverlayTextState>() )
            {
                auto overlayTextState = workphone::static_pointer_cast<OverlayTextState>( stateData );
                WP_ASSERT( overlayTextState );

                auto pOwner = getOwner();
                WP_ASSERT( pOwner );
                auto owner = workphone::static_pointer_cast<COverlayElementText>( pOwner );
                if( owner )
                {
                    if( auto elementText = owner->getElementText() )
                    {
                        auto ogreAlignment = static_cast<Ogre::v1::TextAreaOverlayElement::Alignment>(
                            overlayTextState->alignment );
                        elementText->setAlignment( ogreAlignment );

                        static const auto defaultFontName = String( "default" );
                        elementText->setFontName( defaultFontName.c_str() );

                        elementText->_positionsOutOfDate();
                        elementText->_notifyViewport();
                        elementText->_update();
                    }
                }
            }
        }

        StateListenerOgre::handleStateChanged( state );

        return false;
    }

    bool COverlayElementText::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        //StateListenerOgre::handleStateChanged( message );

        WP_ASSERT( message );
        if( !message )
        {
            return false;
        }

        if( auto owner =
                workphone::static_pointer_cast<COverlayElementText>( StateListenerOgre::getOwner() ) )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                return false;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                return false;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );
            if( !factoryManager )
            {
                return false;
            }

            auto renderTask = graphicsSystem->getRenderTask();
            auto currentTaskId = Thread::getCurrentTask();

            // WP_ASSERT(currentTaskId == renderTask);

            if( currentTaskId == renderTask )
            {
                if( message->isExactly<StateMessageText>() )
                {
                    auto stateMessage = workphone::static_pointer_cast<StateMessageText>( message );
                    auto messageType = stateMessage->getType();
                    auto value = stateMessage->getText();

                    if( messageType == STATE_MESSAGE_TEXT )
                    {
                        owner->setCaption( value );
                    }
                }
                else if( message->isExactly<StateMessageFloatValue>() )
                {
                    auto stateMessage =
                        workphone::static_pointer_cast<StateMessageFloatValue>( message );
                    auto messageType = stateMessage->getType();
                    auto value = stateMessage->getValue();

                    if( messageType == STATE_MESSAGE_LEFT )
                    {
                        //owner->setLeft( value );
                    }
                    else if( messageType == STATE_MESSAGE_TOP )
                    {
                        //owner->setTop( value );
                    }
                    else if( messageType == STATE_MESSAGE_WIDTH )
                    {
                        //owner->setWidth( value );
                    }
                    else if( messageType == STATE_MESSAGE_HEIGHT )
                    {
                        //owner->setHeight( value );
                    }
                }
                else if( message->isExactly<StateMessageUIntValue>() )
                {
                    auto objectMessage =
                        workphone::static_pointer_cast<StateMessageUIntValue>( message );
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
            }
        }

        return false;
    }

    auto COverlayElementText::isValid() const -> bool
    {
        const auto &loadingState = getLoadingState();
        switch( loadingState )
        {
        case LoadingState::Unloaded:
        {
            if( m_element )
            {
                return false;
            }
        }
        break;
        case LoadingState::Loading:
        {
            if( m_element )
            {
                return false;
            }
        }
        break;
        case LoadingState::Loaded:
        {
            if( !m_element )
            {
                return false;
            }
        }
        break;
        case LoadingState::Unloading:
        {
            if( !m_element )
            {
                return false;
            }
        }
        break;
        default:
            break;
        }

        return COverlayElementOgreNext<IOverlayElementText>::isValid();
    }

    auto COverlayElementText::getElementText() const -> Ogre::v1::TextAreaOverlayElement *
    {
        return m_elementText;
    }

    void COverlayElementText::setElementText( Ogre::v1::TextAreaOverlayElement *elementText )
    {
        m_elementText = elementText;
    }

    void COverlayElementText::createStateContext()
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

        auto listener = factoryManager->make_ptr<StateListener>();
        WP_ASSERT( listener );
        listener->setOwner( this );
        m_stateListener = listener;
        stateContext->addStateListener( listener );

        auto state = factoryManager->make_ptr<State>();
        WP_ASSERT( state );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<OverlayTextState>();
        WP_ASSERT( stateData );
        state->setData( stateData );

        stateContext->setOwner( this );
        stateContext->setTaskId( TaskId::Render );
    }

}  // namespace workphone::render
