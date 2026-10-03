#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIButton.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/State/States/UITextStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIButton, UIElement<IUIButton> );

        UIButton::UIButton() = default;

        UIButton::~UIButton() = default;

        void UIButton::setTextSize( f32 textSize )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->textSize = textSize;
                }
            }
        }

        f32 UIButton::getTextSize() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UITextStateData>() )
                {
                    return state->textSize;
                }
            }

            return 0.0f;
        }

    }  // namespace ui
}  // namespace workphone
