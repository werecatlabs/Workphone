#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementText.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreFontManager.h>
#include <OgreTextAreaOverlayElement.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayElementText,
                                   COverlayElementOgre<IOverlayElementText> );

        COverlayElementText::COverlayElementText() : COverlayElementOgre<IOverlayElementText>()
        {
            createStateContext();
        }

        COverlayElementText::~COverlayElementText()
        {
            unload( nullptr );
        }

        void COverlayElementText::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto overlayManager = Ogre::OverlayManager::getSingletonPtr();

                static const auto typeName = String( "TextArea" );
                const auto instanceName = getName();
                // Ogre::TextAreaOverlayElement* overlayElement =
                // (Ogre::TextAreaOverlayElement*)m_overlayMgr->createOverlayElement(typeName.c_str(),
                // instanceName.c_str());
                auto text = static_cast<Ogre::TextAreaOverlayElement *>(
                    overlayManager->createOverlayElement( typeName.c_str(), instanceName.c_str() ) );
                m_elementText = text;

                // text->setMetricsMode(Ogre::GuiMetricsMode::GMM_RELATIVE);
                ////text->setMetricsMode(Ogre::GuiMetricsMode::GMM_PIXELS);
                // text->setHorizontalAlignment(Ogre::GuiHorizontalAlignment::GHA_CENTER);
                // text->setVerticalAlignment(Ogre::GuiVerticalAlignment::GVA_CENTER);
                //
                // setFontName("DebugFont");
                //
                // text->setCharHeight(18);
                // text->setCaption("Test");
                // text->setSpaceWidth(9);
                // text->setColour(Ogre::ColourValue::White);

                text->setMetricsMode( Ogre::GuiMetricsMode::GMM_PIXELS );
                //text->setMetricsMode( Ogre::GuiMetricsMode::GMM_RELATIVE_ASPECT_ADJUSTED );
                //text->setHorizontalAlignment( Ogre::GuiHorizontalAlignment::GHA_LEFT );
                //text->setVerticalAlignment( Ogre::GuiVerticalAlignment::GVA_TOP );

                text->setHorizontalAlignment( Ogre::GuiHorizontalAlignment::GHA_CENTER );
                text->setVerticalAlignment( Ogre::GuiVerticalAlignment::GVA_CENTER );

                //text->setFontName( "SdkTrays/Caption" );
                text->setFontName( "default" );
                text->setCharHeight( 18 );
                // text->setCaption("Test");
                text->setSpaceWidth( 9 );

                setColour( ColourF::White );

                text->_setLeft( 100.0f / 1280.0f );
                text->_setTop( 0.0f );
                text->_setWidth( 0.2f );
                text->_setHeight( 0.1f );

                text->initialise();

                // text->setHorizontalAlignment((Ogre::GuiHorizontalAlignment)m_horizontalAlignment);
                // text->setVerticalAlignment((Ogre::GuiVerticalAlignment)m_verticalAlignment);

                setElement( text );

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

                    auto stateManager = applicationManager->getStateManager();
                    WP_ASSERT( stateManager );

                    // if (m_state)
                    //{
                    //	if (m_stateContext)
                    //	{
                    //		m_stateContext->setState(nullptr);
                    //	}

                    //	m_state->unload(nullptr);
                    //	m_state = nullptr;
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

                    if( auto stateListener = getStateListener() )
                    {
                        stateListener->unload( nullptr );
                        setStateListener( nullptr );
                    }

                    auto overlayMgr = Ogre::OverlayManager::getSingletonPtr();
                    WP_ASSERT( overlayMgr );

                    if( m_element )
                    {
                        overlayMgr->destroyOverlayElement( m_element );
                        m_element = nullptr;
                    }

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool COverlayElementText::isContainer() const
        {
            return true;
        }

        void COverlayElementText::_getObject( void **ppObject ) const
        {
            *ppObject = m_element;
        }

        void COverlayElementText::setFontName( const String &fontName )
        {
            try
            {
                auto fontManager = Ogre::FontManager::getSingletonPtr();
                auto font = fontManager->getByName( fontName.c_str() );
                //if( !font )
                //{
                //    auto grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
                //    auto fontResource = fontManager->load( fontName, grp );
                //    font = Ogre::dynamic_pointer_cast<Ogre::Font>(fontResource);
                //}

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
            if( m_elementText )
            {
                m_elementText->setCharHeight( charHeight );
            }
        }

        void COverlayElementText::setVerticalAlignment( u8 alignment )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto overlayTextState = stateContext->invalidateStateData<OverlayTextState>() )
                {
                    overlayTextState->alignment = alignment;
                }
            }
        }

        u8 COverlayElementText::getVerticalAlignment() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto overlayTextState = stateContext->getStateData<OverlayTextState>() )
                {
                    return overlayTextState->alignment;
                }
            }

            return 0;
        }

        void COverlayElementText::setHorizontalAlignment( u8 alignment )
        {
        }

        u8 COverlayElementText::getHorizontalAlignment() const
        {
            return 0;
        }

        void COverlayElementText::setSpaceWidth( f32 width )
        {
            if( m_elementText )
            {
                m_elementText->setSpaceWidth( width );
            }
        }

        f32 COverlayElementText::getSpaceWidth() const
        {
            return 0;
        }

        bool COverlayElementText::StateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( state )
            {
                auto stateData = state->getData();
                if( stateData->isDerived<OverlayTextState>() )
                {
                    auto overlayTextState =
                        workphone::static_pointer_cast<OverlayTextState>( stateData );

                    if( auto owner = workphone::static_pointer_cast<COverlayElementText>(
                            StateListenerOgre::getOwner() ) )
                    {
                        if( auto elementText = owner->getElementText() )
                        {
                            elementText->setAlignment(
                                static_cast<Ogre::TextAreaOverlayElement::Alignment>(
                                    overlayTextState->alignment ) );
                        }
                    }

                    return true;
                }

                return StateListenerOgre::handleStateChanged( state );
            }

            return false;
        }

        bool COverlayElementText::StateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = workphone::static_pointer_cast<COverlayElementText>(
                    StateListenerOgre::getOwner() ) )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

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

        bool COverlayElementText::isValid() const
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

            return COverlayElementOgre<IOverlayElementText>::isValid();
        }

        Ogre::TextAreaOverlayElement *COverlayElementText::getElementText() const
        {
            return m_elementText;
        }

        void COverlayElementText::setElementText( Ogre::TextAreaOverlayElement *elementText )
        {
            m_elementText = elementText;
        }

        void COverlayElementText::createStateContext()
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

            auto listener = factoryManager->make_ptr<StateListener>();
            listener->setOwner( this );
            m_stateListener = listener;
            stateContext->addStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<OverlayTextState>();
            state->setData( state );

            stateContext->setOwner( this );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

    }  // end namespace render
}  // namespace workphone
