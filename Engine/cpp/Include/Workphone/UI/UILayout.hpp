#ifndef UILayout_h__
#define UILayout_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UILayout
         * @brief Concrete UI layout element implementing IUILayoutWindow.
         *
         * UILayout is a UI element that encapsulates layout-specific behavior. It provides access
         * to an FSM (finite state machine) associated with the element, and maintains a reference
         * to an owning UI window. Position and size are applied via the layout so that derived UI
         * systems can respond and recompute layout, rendering, and hit-testing as required.
         *
         * Responsibilities:
         * - Provide access to the element's FSM.
         * - Store and expose a reference to the parent UI window.
         * - Allow consumers to set position and size of the layout.
         * - Allow invalidation to notify the system that layout or visual state changed.
         */
        class WPCore_API UILayout : public UIElement<IUILayoutWindow>
        {
        public:
            /**
             * @brief Construct a new UILayout instance.
             *
             * Performs minimal initialization. Any heavy initialization should be done by the
             * UI system when the element is added to a window or scene.
             */
            UILayout();

            UILayout( u32 poolTypeId );

            /**
             * @brief Destroy the UILayout instance.
             *
             * Releases any resources owned by the layout. Destructor does not throw.
             */
            ~UILayout() override;

            /**
             * @brief Get the current layout state.
             *
             * Returns the current state of the layout's finite state machine (FSM).
             *
             * @return LayoutStates The current layout state.
             */
            LayoutStates getState() override;

            /**
             * @brief Set the current layout state.
             *
             * Updates the state of the layout's finite state machine (FSM).
             *
             * @param state The new layout state to set.
             */
            void setState( LayoutStates state ) override;

            /**
             * @brief Get the UI window that owns this layout.
             *
             * Returns the window that this layout is attached to. Caller should check the
             * returned SmartPtr for validity before use.
             *
             * @return SmartPtr<IUIWindow> Smart pointer to the owning UI window (or null).
             */
            SmartPtr<IUIWindow> getParentWindow() const override;

            /**
             * @brief Set the UI window that owns this layout.
             *
             * Attaching a layout to a window allows the layout to participate in window-level
             * layout passes and rendering. Passing a null SmartPtr detaches the layout from any
             * window.
             *
             * @param uiWindow Smart pointer to the UI window to attach.
             */
            void setParentWindow( SmartPtr<IUIWindow> uiWindow ) override;

            /**
             * @brief Invalidate the layout.
             *
             * Marks the layout as dirty so that the UI system will recompute layout, redraw or
             * otherwise update the element's visual/state representation on the next update pass.
             * This method should be inexpensive to call and idempotent.
             */
            void invalidate() override;

            /** @copydoc IUILayoutWindow::getWindowFlags */
            WindowFlags getWindowFlags() const override;

            /** @copydoc IUILayoutWindow::setWindowFlags */
            void setWindowFlags( WindowFlags flags ) override;

            /** @copydoc IUILayoutWindow::hasWindowFlag */
            bool hasWindowFlag( WindowFlags flag ) const override;

            /**
             * @brief Macro for class registration with the Workphone UI system.
             *
             * Ensures the UILayout class is registered for reflection or factory usage.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Smart pointer to the UI window that owns this layout.
             *
             * May be null if the layout is not attached to any window.
             */
            SmartPtr<IUIWindow> m_uiWindow;

            /**
             * @brief Current state of the layout's finite state machine (FSM).
             */
            LayoutStates m_state = LayoutStates::None;

            /**
             * @brief Window decoration and behaviour flags applied each update.
             *
             * Defaults to the combination of Border, Movable, Scalable, Minimizable
             * and Title — matching the previous hardcoded behaviour.
             */
            WindowFlags m_windowFlags = WindowFlags::Default;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UILayout_h__
