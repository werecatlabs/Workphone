#ifndef __IUIButton_H
#define __IUIButton_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIButton
         * @brief Interface for a button, responsible for managing button elements in the user interface
         */
        class WPCore_API IUIButton : public IUIElement
        {
        public:
            static const String referenceWidthStr;
            static const String referenceHeightStr;
            static const String textNormalColourStr;
            static const String textHoverColourStr;
            static const String textActiveColourStr;
            static const String textBackgroundColourStr;
            static const String normalColourStr;
            static const String hoverColourStr;
            static const String activeColourStr;
            static const String borderColourStr;
            static const String borderWidthStr;
            static const String roundingStr;
            static const String paddingStr;

            IUIButton();

            IUIButton( u32 poolTypeId );

            /**
             * @brief Virtual destructor
             */
            ~IUIButton() override;

            /**
             * @brief Gets the label text of the button
             * @return The label text as a string
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label text of the button
             * @param label The new label text for the button
             */
            void setLabel( const String &label ) override = 0;

            /**
             * @brief Sets the text size of the button label
             * @param textSize The new text size for the button label
             */
            virtual void setTextSize( f32 textSize ) = 0;

            /**
             * @brief Gets the text size of the button label
             * @return The text size of the button label as a float
             */
            virtual f32 getTextSize() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
