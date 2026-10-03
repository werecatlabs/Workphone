#ifndef IUILabelTextInputPair_h__
#define IUILabelTextInputPair_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class IUILabelTextInputPair
         * @brief Interface for a UI element that consists of a label and a text input box
         */
        class WPCore_API IUILabelTextInputPair : public IUIElement
        {
        public:
            IUILabelTextInputPair();

            IUILabelTextInputPair( u32 poolTypeId );

            /**
             * @brief Destructor
             */
            ~IUILabelTextInputPair() override;

            /**
             * @brief Gets the label associated with the input box
             * @return The label associated with the input box
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label associated with the input box
             * @param label The label to associate with the input box
             */
            void setLabel( const String &label ) override = 0;

            /**
             * @brief Gets the current value of the text input box
             * @return The current value of the text input box
             */
            virtual String getValue() const = 0;

            /**
             * @brief Sets the value of the text input box
             * @param value The value to set for the text input box
             */
            virtual void setValue( const String &value ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUILabelTextInputPair_h__
