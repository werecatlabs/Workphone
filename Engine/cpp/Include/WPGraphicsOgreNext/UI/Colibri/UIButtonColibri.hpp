#ifndef __UIButtonOgreNext_h__
#define __UIButtonOgreNext_h__

#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/UI/UIButton.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UIButtonOgreNext
         * @brief OgreNext implementation of a UI button element.
         *
         * This class provides a concrete implementation of a UI button using the OgreNext
         * rendering engine and the Colibri UI library. It handles button rendering, state
         * management, and user interactions within the OgreNext graphics framework.
         */
        class UIButtonColibri : public UIElementColibri<UIButton>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new instance of the UIButtonOgreNext class with default values.
             */
            UIButtonColibri();

            /**
             * @brief Destructor.
             *
             * Cleans up resources and releases references to the underlying Colibri button.
             */
            ~UIButtonColibri() override;

            /**
             * @brief Loads the button with the specified data.
             *
             * Initializes and configures the button element with the provided shared object data.
             * This method creates the underlying Colibri button widget and sets up its properties.
             *
             * @param data Shared pointer to the initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the button and releases its resources.
             *
             * Destroys the underlying Colibri button widget and releases any associated resources.
             *
             * @param data Shared pointer to the unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Sets the text size for the button label.
             *
             * Updates the font size of the text displayed on the button.
             *
             * @param textSize The new text size as a floating-point value.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Gets the current text size of the button label.
             *
             * @return The current text size as a floating-point value.
             */
            f32 getTextSize() const override;

            /**
             * @brief Handles state change events for the button.
             *
             * Processes state changes such as hover, pressed, disabled, or other button states.
             * This method updates the button's visual appearance based on the new state.
             *
             * @param state Shared pointer to the new state object.
             * @return true if the state change was handled successfully, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Pointer to the underlying Colibri button widget.
            Colibri::Button *m_button = nullptr;

            /// The current text size for the button label.
            f32 m_textSize = 1.0;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UIButton_h__
