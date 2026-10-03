#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UITextColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <ColibriGui/ColibriWindow.h>
#include <ColibriGui/ColibriLabel.h>
#include <ColibriGui/ColibriManager.h>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {

        WP_CLASS_REGISTER_DERIVED( workphone::ui, UITextColibri, UIElementColibri<IUIText> );

        UITextColibri::UITextColibri()
        {
            createStateContext();
        }

        UITextColibri::~UITextColibri()
        {
            unload( nullptr );
        }

        void UITextColibri::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( this );

                auto ui = workphone::static_pointer_cast<UIManagerColibri>(
                    applicationManager->getRenderUI() );

                auto window = ui->getLayoutWindow();
                if( !window )
                {
                    WP_LOG_ERROR( "Layout window is not available." );
                    setLoadingState( LoadingState::Loaded );
                    return;
                }

                auto colibriManager = ui->getColibriManager();
                if( !colibriManager )
                {
                    WP_LOG_ERROR( "Colibri manager is not available." );
                    setLoadingState( LoadingState::Loaded );
                    return;
                }

                m_labelText = colibriManager->createWidget<Colibri::Label>( window );
                m_labelText->setText( "Text" );

                m_labelText->m_minSize = Ogre::Vector2( 350, 32 );
                m_labelText->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 1920, 1080 ) );
                m_labelText->sizeToFit();

                setWidget( m_labelText );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITextColibri::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    WP_ASSERT( graphicsSystem );

                    m_labelText = nullptr;
                    UIElementColibri<ui::UIText>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool UITextColibri::handleStateChanged( SmartPtr<IState> &state )
        {
            auto retValue = UIElementColibri<UIText>::handleStateChanged( state );

            auto stateData = state->getData();
            if( stateData->isDerived<UITextStateData>() )
            {
                if( m_labelText )
                {
                    auto textStateData = workphone::static_pointer_cast<UITextStateData>( stateData );

                    auto fontSize = static_cast<Colibri::FontSize>( textStateData->textSize );
                    m_labelText->setDefaultFontSize( fontSize );

                    m_labelText->setText( textStateData->text.str() );

                    auto vertAlignment = static_cast<Colibri::TextVertAlignment::TextVertAlignment>(
                        textStateData->verticalAlignment );
                    m_labelText->setTextVertAlignment( vertAlignment );

                    auto horizAlignment = static_cast<Colibri::TextHorizAlignment::TextHorizAlignment>(
                        textStateData->horizontalAlignment );
                    m_labelText->setTextHorizAlignment( horizAlignment );

                    retValue = true;
                }
            }

            return retValue;
        }

        SmartPtr<Properties> UITextColibri::getProperties() const
        {
            auto properties = UIElementColibri<UIText>::getProperties();
            properties->setProperty( IUIText::textPropertyStr, getText() );
            properties->setProperty( IUIText::textSizePropertyStr, getTextSize() );
            properties->setProperty( IUIText::verticalAlignmentPropertyStr, getVerticalAlignment() );
            properties->setProperty( IUIText::horizontalAlignmentPropertyStr, getHorizontalAlignment() );
            return properties;
        }

        void UITextColibri::setProperties( SmartPtr<Properties> properties )
        {
            auto text = getText();
            auto textSize = getTextSize();
            u32 verticalAlignment = 0;
            u32 horizontalAlignment = 0;

            UIElementColibri<UIText>::setProperties( properties );
            properties->getPropertyValue( IUIText::textPropertyStr, text );
            properties->getPropertyValue( IUIText::textSizePropertyStr, textSize );
            properties->getPropertyValue( IUIText::verticalAlignmentPropertyStr, verticalAlignment );
            properties->getPropertyValue( IUIText::horizontalAlignmentPropertyStr, horizontalAlignment );

            setText( text );
            setTextSize( textSize );
            setVerticalAlignment( verticalAlignment );
            setHorizontalAlignment( horizontalAlignment );
        }

        Colibri::Label *UITextColibri::getLabelText() const
        {
            return m_labelText;
        }

        void UITextColibri::setLabelText( Colibri::Label *labelText )
        {
            m_labelText = labelText;
        }

        void UITextColibri::createStateContext()
        {
            WP_ASSERT( getStateContext() == nullptr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateTask = graphicsSystem->getStateTask();

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );
            setStateContext( stateContext );
            stateContext->setTaskId( stateTask );

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );
            setStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            auto transformState = factoryManager->make_ptr<State>();
            transformState->setId( getId() );
            transformState->setOwner( this );
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transformState->setData( transformStateData );

            auto textState = factoryManager->make_ptr<State>();
            textState->setId( getId() );
            textState->setOwner( this );
            stateContext->addState( textState );

            auto textStateData = factoryManager->make_ptr<UITextStateData>();
            textState->setData( textStateData );
        }

    }  // namespace ui
}  // namespace workphone
