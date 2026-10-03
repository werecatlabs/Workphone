#ifndef IUILabelCheckboxPair_h__
#define IUILabelCheckboxPair_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUILabelTogglePair
         * @brief Interface for a UI Label-Checkbox Pair element, extending IUIElement and providing
         * functionality to manage and display a label and a checkbox
         */
        class WPCore_API IUILabelTogglePair : public IUIElement
        {
        public:
            IUILabelTogglePair();

            IUILabelTogglePair( u32 poolTypeId );

            /**
             * @brief Virtual destructor
             */
            ~IUILabelTogglePair() override;

            /**
             * @brief Gets the label's text
             * @return A string representing the label's text
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label's text
             * @param label A string representing the label's text
             */
            void setLabel( const String &label ) override = 0;

            /**
             * @brief Gets the checkbox's value
             * @return A boolean representing the checkbox's value (true if checked, false otherwise)
             */
            virtual bool getValue() const = 0;

            /**
             * @brief Sets the checkbox's value
             * @param value A boolean representing the checkbox's value (true if checked, false
             * otherwise)
             */
            virtual void setValue( bool value ) = 0;

            /**
             * @brief Gets the label's visibility
             * @return A boolean representing the label's visibility (true if visible, false otherwise)
             */
            virtual bool getShowLabel() const = 0;

            /**
             * @brief Sets the label's visibility
             * @param showLabel A boolean representing the label's visibility (true if visible, false
             * otherwise)
             */
            virtual void setShowLabel( bool showLabel ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUILabelCheckboxPair_h__
