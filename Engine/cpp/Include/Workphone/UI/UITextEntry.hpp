#ifndef UITextEntry_h__
#define UITextEntry_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UITextEntry
         * @brief UI element that allows for text entry and display.
         *
         * This class implements the IUITextEntry interface and provides the functionality
         * to manage and manipulate text content and its visual size within the UI system.
         */
        class UITextEntry : public UIElement<IUITextEntry>
        {
        public:
            /**
             * @brief Constructs a new UITextEntry instance.
             */
            UITextEntry();

            /**
             * @brief Destroys the UITextEntry instance.
             */
            ~UITextEntry() override;

            /**
             * @brief Sets the text content of the entry.
             * @param text The string to be displayed/entered.
             */
            void setText( const String &text ) override;

            /**
             * @brief Retrieves the current text content.
             * @return The current text as a String.
             */
            String getText() const override;

            /**
             * @brief Sets the visual size of the text.
             * @param textSize The size of the text in pixels or points.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Retrieves the current text size.
             * @return The current size of the text.
             */
            f32 getTextSize() const override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UITextEntry_h__
