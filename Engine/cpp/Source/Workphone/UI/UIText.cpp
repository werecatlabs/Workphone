#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIText.hpp>
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

        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIText, UIElement<IUIText> );

        UIText::UIText() = default;

        UIText::~UIText() = default;

        void UIText::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                UIElement::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIText::setText( const String &text )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->text = text.c_str();
                }
            }
        }

        String UIText::getText() const
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

        void UIText::setTextSize( f32 textSize )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->textSize = textSize;
                }
            }
        }

        f32 UIText::getTextSize() const
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

        void UIText::setVerticalAlignment( u8 alignment )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->verticalAlignment = alignment;
                }
            }
        }

        u8 UIText::getVerticalAlignment() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UITextStateData>() )
                {
                    return state->verticalAlignment;
                }
            }

            return 0u;
        }

        void UIText::setHorizontalAlignment( u8 alignment )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UITextStateData>() )
                {
                    state->horizontalAlignment = alignment;
                }
            }
        }

        u8 UIText::getHorizontalAlignment() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UITextStateData>() )
                {
                    return state->horizontalAlignment;
                }
            }

            return 0u;
        }

        SmartPtr<Properties> UIText::getProperties() const
        {
            auto properties = UIElement::getProperties();
            if( !properties )
            {
                properties = workphone::make_ptr<Properties>();
            }

            auto textStateData = getStateContext()->getStateData<UITextStateData>();
            auto text = textStateData->text.c_str();
            auto textSize = textStateData->textSize;
            auto verticalAlignment = textStateData->verticalAlignment;
            auto horizontalAlignment = textStateData->horizontalAlignment;

            // Add text-specific properties
            properties->setProperty( IUIText::textPropertyStr, text );
            properties->setProperty( IUIText::textSizeStr, textSize );
            properties->setProperty( IUIText::verticalAlignmentStr,
                                     static_cast<s32>( verticalAlignment ) );
            properties->setProperty( IUIText::horizontalAlignmentStr,
                                     static_cast<s32>( horizontalAlignment ) );

            return properties;
        }

        void UIText::setProperties( SmartPtr<Properties> properties )
        {
            UIElement::setProperties( properties );

            if( properties )
            {
                auto textStateData = getStateContext()->getStateData<UITextStateData>();
                auto text = String();
                auto textSize = textStateData->textSize;
                auto verticalAlignment = (u32)textStateData->verticalAlignment;
                auto horizontalAlignment = (u32)textStateData->horizontalAlignment;

                // Set text property
                if( properties->getPropertyValue( IUIText::textPropertyStr, text ) )
                {
                    setText( text );
                }

                // Set text size property
                if( properties->getPropertyValue( IUIText::textSizeStr, textSize ) )
                {
                    setTextSize( textSize );
                }

                // Set vertical alignment property
                if( properties->getPropertyValue( IUIText::verticalAlignmentStr, verticalAlignment ) )
                {
                    setVerticalAlignment( static_cast<u8>( verticalAlignment ) );
                }

                // Set horizontal alignment property
                if( properties->getPropertyValue( IUIText::horizontalAlignmentStr,
                                                  horizontalAlignment ) )
                {
                    setHorizontalAlignment( static_cast<u8>( horizontalAlignment ) );
                }
            }
        }

        Array<SmartPtr<ISharedObject>> UIText::getChildObjects() const
        {
            auto childObjects = UIElement::getChildObjects();
            // UIText doesn't have additional child objects beyond base UI element children
            return childObjects;
        }

        void UIText::onChangedState()
        {
            UIElement::onChangedState();

            // Additional state change handling for text-specific properties
            // This could include updating the visual representation, font rendering, etc.
        }

    }  // namespace ui
}  // namespace workphone
