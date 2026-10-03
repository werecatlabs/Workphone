#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIToggle.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/State/States/UIToggleStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIToggle, UIElement<IUIToggle> );

        UIToggle::UIToggle() = default;

        UIToggle::~UIToggle() = default;

        void UIToggle::setToggled( bool toggled )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->checked = toggled;
                    state->toggleState = static_cast<u8>( toggled ? ToggleState::On : ToggleState::Off );
                }
            }
        }

        bool UIToggle::isToggled() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->checked;
                }
            }

            return false;
        }

        IUIToggle::ToggleType UIToggle::getToggleType() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return static_cast<ToggleType>( state->toggleType );
                }
            }

            return ToggleType::CheckBox;
        }

        void UIToggle::setToggleType( ToggleType toggleType )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->toggleType = static_cast<u8>( toggleType );
                }
            }
        }

        IUIToggle::ToggleState UIToggle::getToggleState() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return static_cast<ToggleState>( state->toggleState );
                }
            }

            return ToggleState::Off;
        }

        void UIToggle::setToggleState( ToggleState toggleState )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->toggleState = static_cast<u8>( toggleState );
                    state->checked = toggleState == ToggleState::On;
                }
            }
        }

        bool UIToggle::getShowLabel() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->showLabel;
                }
            }

            return true;
        }

        void UIToggle::setShowLabel( bool showLabel )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->showLabel = showLabel;
                }
            }
        }

        String UIToggle::getLabel() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->label.c_str();
                }
            }

            return {};
        }

        void UIToggle::setLabel( const String &label )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->label = label.c_str();
                }
            }
        }

        void UIToggle::setTextSize( f32 textSize )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->textSize = textSize;
                }
            }
        }

        f32 UIToggle::getTextSize() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->textSize;
                }
            }

            return 0.0f;
        }

        void UIToggle::invalidate()
        {
            if( auto stateContext = getStateContext() )
            {
                stateContext->setDirty( true );
            }
        }
    }  // namespace ui
}  // namespace workphone
