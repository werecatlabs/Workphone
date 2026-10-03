#ifndef _IGUIToggleButton_H
#define _IGUIToggleButton_H

#include <Workphone/Interface/UI/IUIButton.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief Interface for a toggle button UI element.
         */
        class WPCore_API IUIToggle : public IUIButton
        {
        public:
            /** The type of toggle button. */
            enum class ToggleType
            {
                CheckBox,
                RadioButton,
                ToggleButton
            };

            /** The state of the toggle button. */
            enum class ToggleState
            {
                Off,
                On,
                Indeterminate
            };

            IUIToggle() : IUIButton( IUIToggle::typeInfo() )
            {
            }

            IUIToggle( u32 poolTypeId ) : IUIButton( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUIToggle() override;

            /** Sets the toggled state of the toggle button.
             * @param toggled The toggled state to set.
             */
            virtual void setToggled( bool toggled ) = 0;

            /** Gets the toggled state of the toggle button.
             * @return The toggled state of the toggle button.
             */
            virtual bool isToggled() const = 0;

            /** Gets the toggle type of the toggle button.
             * @return The toggle type of the toggle button.
             */
            virtual ToggleType getToggleType() const = 0;

            /** Sets the toggle type of the toggle button.
             * @param toggleType The toggle type to set.
             */
            virtual void setToggleType( ToggleType toggleType ) = 0;

            /** Gets the toggle state of the toggle button.
             * @return The toggle state of the toggle button.
             */
            virtual ToggleState getToggleState() const = 0;

            /** Sets the toggle state of the toggle button.
             * @param toggleState The toggle state to set.
             */
            virtual void setToggleState( ToggleState toggleState ) = 0;

            /** Gets whether the toggle text label should be drawn. */
            virtual bool getShowLabel() const = 0;

            /** Sets whether the toggle text label should be drawn. */
            virtual void setShowLabel( bool showLabel ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
