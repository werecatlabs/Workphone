#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UISlider.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Scene/Directors/TextureResourceDirector.hpp>
#include <Workphone/State/States/UISliderStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UISlider, UIElement<IUISlider> );

        UISlider::UISlider() = default;

        UISlider::~UISlider() = default;

        f32 UISlider::getValue() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UISliderStateData>() )
                {
                    return stateData->value;
                }
            }

            return {};
        }

        void UISlider::setValue( f32 value )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UISliderStateData>() )
                {
                    stateData->value = value;
                }
            }
        }

        f32 UISlider::getMinValue() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UISliderStateData>() )
                {
                    return stateData->minValue;
                }
            }

            return 0;
        }

        void UISlider::setMinValue( f32 minValue )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UISliderStateData>() )
                {
                    stateData->minValue = minValue;
                }
            }
        }

        f32 UISlider::getMaxValue() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UISliderStateData>() )
                {
                    return stateData->maxValue;
                }
            }

            return 0;
        }

        void UISlider::setMaxValue( f32 maxValue )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UISliderStateData>() )
                {
                    stateData->maxValue = maxValue;
                }
            }
        }

        Direction UISlider::getDirection() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UISliderStateData>() )
                {
                    return stateData->direction;
                }
            }

            return {};
        }

        void UISlider::setDirection( Direction direction )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UISliderStateData>() )
                {
                    stateData->direction = direction;
                }
            }
        }

        bool UISlider::isDragging() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateData<UISliderStateData>() )
                {
                    return stateData->dragging;
                }
            }

            return false;
        }

        void UISlider::setDragging( bool dragging )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->invalidateStateData<UISliderStateData>() )
                {
                    stateData->dragging = dragging;
                }
            }
        }

        void UISlider::invalidate()
        {
        }
    }  // namespace ui
}  // namespace workphone
