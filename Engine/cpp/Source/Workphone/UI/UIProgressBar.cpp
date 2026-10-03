#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIProgressBar.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Scene/Directors/TextureResourceDirector.hpp>
#include <Workphone/State/States/UIProgressBarStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIProgressBar, UIElement<IUIProgressBar> );

        UIProgressBar::UIProgressBar() = default;

        UIProgressBar::~UIProgressBar() = default;

        f32 UIProgressBar::getValue() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->value;
                }
            }

            return 0;
        }

        void UIProgressBar::setValue( f32 value )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->value = value;
                }
            }
        }

        f32 UIProgressBar::getMinValue() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->minValue;
                }
            }

            return 0;
        }

        void UIProgressBar::setMinValue( f32 value )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->minValue = value;
                }
            }
        }

        f32 UIProgressBar::getMaxValue() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->maxValue;
                }
            }

            return 0;
        }

        void UIProgressBar::setMaxValue( f32 value )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->maxValue = value;
                }
            }
        }

        bool UIProgressBar::getShowText() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->showText;
                }
            }

            return false;
        }

        void UIProgressBar::setShowText( bool showText )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->showText = showText;
                }
            }
        }

        ColourF UIProgressBar::getFillColour() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->fillColour;
                }
            }

            return {};
        }

        void UIProgressBar::setFillColour( const ColourF &colour )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->fillColour = colour;
                }
            }
        }

        ColourF UIProgressBar::getBackgroundColour() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->backgroundColour;
                }
            }

            return {};
        }

        void UIProgressBar::setBackgroundColour( const ColourF &colour )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->backgroundColour = colour;
                }
            }
        }

        ColourF UIProgressBar::getBorderColour() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->borderColour;
                }
            }

            return {};
        }

        void UIProgressBar::setBorderColour( const ColourF &colour )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->borderColour = colour;
                }
            }
        }

        ColourF UIProgressBar::getTextColour() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->textColour;
                }
            }

            return {};
        }

        void UIProgressBar::setTextColour( const ColourF &colour )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->textColour = colour;
                }
            }
        }

        f32 UIProgressBar::getBorderWidth() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->borderWidth;
                }
            }

            return {};
        }

        void UIProgressBar::setBorderWidth( f32 width )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->borderWidth = width;
                }
            }
        }

        f32 UIProgressBar::getRounding() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UIProgressBarStateData>() )
                {
                    return stateData->rounding;
                }
            }

            return {};
        }

        void UIProgressBar::setRounding( f32 rounding )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UIProgressBarStateData>() )
                {
                    stateData->rounding = rounding;
                }
            }
        }

    }  // namespace ui
}  // namespace workphone
