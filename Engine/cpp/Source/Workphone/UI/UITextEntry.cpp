#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UITextEntry.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/State/States/UITextStateData.hpp>

namespace workphone
{
    namespace ui
    {

        WP_CLASS_REGISTER_DERIVED( workphone::ui, UITextEntry, UIElement<IUITextEntry> );

        UITextEntry::UITextEntry() = default;

        UITextEntry::~UITextEntry() = default;

        void UITextEntry::setText( const String &text )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->text = text.c_str();
                }
            }
        }

        String UITextEntry::getText() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UITextStateData>() )
                {
                    auto &text = state->text;
                    return String( text.c_str(), text.size() );
                }
            }

            return {};
        }

        void UITextEntry::setTextSize( f32 textSize )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->textSize = textSize;
                }
            }
        }

        f32 UITextEntry::getTextSize() const
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
