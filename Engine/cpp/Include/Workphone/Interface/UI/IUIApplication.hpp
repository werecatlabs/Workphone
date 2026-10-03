//
// Created by Zane Desir on 20/11/2021.
//

#ifndef WP_IUIAPPLICATION_H
#define WP_IUIAPPLICATION_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief Interface for a user interface application.
         *
         * Provides methods for handling window and input events, accessing and setting the menubar and
         * toolbar, and getting and setting the window size.
         */
        class WPCore_API IUIApplication : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IUIApplication() override;

            /**
             * @brief Handles a window event.
             *
             * @param event A smart pointer to the window event to handle.
             */
            virtual void handleWindowEvent( SmartPtr<render::IGraphicsWindowEvent> event ) = 0;

            /**
             * @brief Handles an input event.
             *
             * @param event A smart pointer to the input event to handle.
             * @return True if the event was handled, false otherwise.
             */
            virtual bool handleInputEvent( SmartPtr<IInputEvent> event ) = 0;

            /**
             * @brief Gets the menubar.
             *
             * @return A smart pointer to the current menubar.
             */
            virtual SmartPtr<IUIMenubar> getMenubar() const = 0;

            /**
             * @brief Sets the menubar.
             *
             * @param menubar A smart pointer to the menubar to set.
             */
            virtual void setMenubar( SmartPtr<IUIMenubar> menubar ) = 0;

            /**
             * @brief Gets the toolbar.
             *
             * @return A smart pointer to the current toolbar.
             */
            virtual SmartPtr<IUIToolbar> getToolbar() const = 0;

            /**
             * @brief Sets the toolbar.
             *
             * @param toolbar A smart pointer to the toolbar to set.
             */
            virtual void setToolbar( SmartPtr<IUIToolbar> toolbar ) = 0;

            /**
             * @brief Gets the window size.
             *
             * @return The current size of the window as a Vector2I object.
             */
            virtual Vector2I getWindowSize() const = 0;

            /**
             * @brief Sets the window size.
             *
             * @param size A Vector2I object containing the new size of the window.
             */
            virtual void setWindowSize( const Vector2I &size ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // WP_IUIAPPLICATION_H
