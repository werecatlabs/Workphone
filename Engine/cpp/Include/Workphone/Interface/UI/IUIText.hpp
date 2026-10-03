#ifndef _IUITEXT_H
#define _IUITEXT_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief Abstract interface for a UI text element.
         */
        class WPCore_API IUIText : public IUIElement
        {
        public:
            /** Property key names */
            static const String textPropertyStr;
            static const String textSizePropertyStr;
            static const String verticalAlignmentPropertyStr;
            static const String horizontalAlignmentPropertyStr;
            static const String textSizeStr;
            static const String verticalAlignmentStr;
            static const String horizontalAlignmentStr;
            static const String textColourRStr;
            static const String textColourGStr;
            static const String textColourBStr;
            static const String textColourAStr;
            static const String backgroundColourRStr;
            static const String backgroundColourGStr;
            static const String backgroundColourBStr;
            static const String backgroundColourAStr;
            static const String textPaddingXStr;
            static const String textPaddingYStr;
            static const String textWrapStr;

            IUIText();

            IUIText( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Virtual destructor. */
            ~IUIText() override;

            /**
             * @brief Sets the text of the UI element.
             * @param text The new text for the element.
             */
            virtual void setText( const String &text ) = 0;

            /**
             * @brief Gets the text of the UI element.
             * @return The current text of the element.
             */
            virtual String getText() const = 0;

            /**
             * @brief Sets the size of the text.
             * @param textSize The new size for the text.
             */
            virtual void setTextSize( f32 textSize ) = 0;

            /**
             * @brief Gets the size of the text.
             * @return The current size of the text.
             */
            virtual f32 getTextSize() const = 0;

            /**
             * Sets the alignment of the text within the element.
             * @param alignment The alignment value to set.
             */
            virtual void setVerticalAlignment( u8 alignment ) = 0;

            /**
             * Gets the alignment of the text within the element.
             * @return The alignment value.
             */
            virtual u8 getVerticalAlignment() const = 0;

            /**
             * Sets the alignment of the text within the element.
             * @param alignment The alignment value to set.
             */
            virtual void setHorizontalAlignment( u8 alignment ) = 0;

            /**
             * Gets the alignment of the text within the element.
             * @return The alignment value.
             */
            virtual u8 getHorizontalAlignment() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
