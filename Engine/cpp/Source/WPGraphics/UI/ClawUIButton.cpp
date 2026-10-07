#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIButton.hpp>
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIButton::ClawUIButton()
    {
        m_type = "Button";
    }

    ClawUIButton::~ClawUIButton()
    {
        unload( nullptr );
    }

    void ClawUIButton::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        if( auto stateContext = getStateContext() )
        {
            stateContext->setDirty( true );
        }
        setLoadingState( LoadingState::Loaded );
    }

    void ClawUIButton::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        ClawUIElement<IUIButton>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    bool ClawUIButton::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void ClawUIButton::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    String ClawUIButton::getLabel() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<UIElementStateData>() )
            {
                return !state->label.empty() ? state->label : state->caption;
            }
        }
        return ClawUIElement::getLabel();
    }

    void ClawUIButton::setLabel( const String &label )
    {
        ClawUIElement::setLabel( label );
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateData<UIElementStateData>() )
            {
                state->label = label;
                state->caption = label;
            }
        }
    }

    void ClawUIButton::setTextSize( f32 textSize )
    {
        m_textSize = MathF::clamp( textSize, 0.0f, 255.0f );
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateData<UIElementStateData>() )
            {
                state->textSize = static_cast<u8>( m_textSize );
            }
        }
    }

    f32 ClawUIButton::getTextSize() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<UIElementStateData>() )
            {
                return state->textSize;
            }
        }
        return m_textSize;
    }

    bool ClawUIButton::isSimpleButton() const
    {
        return m_isSimpleButton;
    }

    void ClawUIButton::setSimpleButton( bool simpleButton )
    {
        m_isSimpleButton = simpleButton;
    }

    bool ClawUIButton::handleStateChanged( SmartPtr<IState> &state )
    {
        return ClawUIElement<IUIButton>::handleStateChanged( state );
    }

    void ClawUIButton::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto label = getLabel();
        if( label.empty() )
        {
            label = "Button";
        }

        // Scene buttons can provide a separate Text child with their own font,
        // alignment and colour. Do not draw a second label underneath it.
        const bool hasTextChild = std::any_of( m_children.begin(), m_children.end(),
            []( const SmartPtr<IUIElement> &child ) { return child && child->isDerived<IUIText>(); } );
        if( wp_button_label( ctx, hasTextChild ? "" : label.c_str() ) )
        {
            handleButtonClick();
        }

        drawWorkphoneChildren( ctx );
    }

    void ClawUIButton::handleButtonClick()
    {
        if( auto layoutElement = getLayout() )
        {
            layoutElement->onActivate( this );
        }

        Array<Parameter> arguments;
        for( auto &listener : getObjectListeners() )
        {
            if( listener )
            {
                listener->handleEvent( EventType::UI, IEvent::CLICK_HASH, arguments, this, nullptr,
                                       nullptr );
            }
        }
    }
}  // namespace workphone::ui
