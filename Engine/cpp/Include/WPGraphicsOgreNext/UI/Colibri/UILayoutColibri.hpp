#ifndef __UILayoutOgreNext_h__
#define __UILayoutOgreNext_h__

#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <Workphone/UI/UILayout.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @file UILayoutOgreNext.hpp
         * @brief OgreNext UI layout element wrapper.
         *
         * This file declares `UILayoutOgreNext`, a concrete UI layout element
         * that integrates Workphone's `UILayout` with the OgreNext Colibri UI
         * backend. The class is responsible for loading and unloading layout
         * resources, providing access to the associated FSM and UI window,
         * and managing the underlying Colibri window used for rendering.
         */

        /**
         * @class UILayoutOgreNext
         * @brief OgreNext-specific implementation of a UI layout element.
         *
         * `UILayoutOgreNext` extends `UIElementOgreNext<UILayout>` and binds the
         * engine's `UILayout` abstraction to an OgreNext `Colibri::Window`.
         * It handles lifecycle operations (load/unload), exposes the layout's
         * FSM, and forwards/getters/setters for the associated `IUIWindow`.
         *
         * Responsibilities:
         * - Initialize and tear down any OgreNext-specific resources for the layout.
         * - Provide accessors for the layout's FSM and external `IUIWindow`.
         * - Allow the layout to be invalidated, prompting a refresh or rebuild.
         */
        class UILayoutColibri : public UIElementColibri<UILayout>
        {
        public:
            /**
             * @brief Construct a new `UILayoutOgreNext`.
             *
             * Initializes internal state; does not create the underlying
             * Colibri window until `load` is called.
             */
            UILayoutColibri();

            /**
             * @brief Destroy the `UILayoutOgreNext`.
             *
             * Ensures any owned resources are released. The destructor is
             * virtual to allow proper cleanup through base pointers.
             */
            ~UILayoutColibri() override;

            /**
             * @brief Load the layout with optional initialization data.
             *
             * @param data Optional shared object containing initialization data
             *             (e.g., resource descriptors, parent pointers).
             *
             * This function should create or configure the underlying Colibri
             * window and any OgreNext-specific resources required to display
             * the layout.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload the layout and release resources.
             *
             * @param data Optional shared object passed during unload for
             *             contextual cleanup.
             *
             * This will destroy or detach the Colibri window if it was created
             * during `load`, and perform any other necessary cleanup.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the finite state machine (FSM) associated with this layout.
             *
             * @return SmartPtr<IFSM> A smart pointer to the layout's FSM, or
             *         a null/empty pointer if no FSM is associated.
             */
            SmartPtr<IFSM> getFSM();

            /**
             * @brief Retrieve the UI window associated with this layout.
             *
             * @return SmartPtr<IUIWindow> The associated UI window or an empty
             *         smart pointer if none is set.
             */
            SmartPtr<IUIWindow> getParentWindow() const override;

            /**
             * @brief Associate an `IUIWindow` with this layout.
             *
             * @param uiWindow Smart pointer to the UI window to associate.
             *
             * The associated window may be used as a parent or target for
             * rendering and input; implementations should update any backend
             * window references as needed.
             */
            void setParentWindow( SmartPtr<IUIWindow> uiWindow ) override;

            /**
             * @brief Mark the layout as invalid and request a refresh.
             *
             * Calling `invalidate` signals that the layout's visual state has
             * changed and the rendering backend should update / rebuild the
             * layout representation (for example, re-layout controls or
             * recreate the underlying Colibri window contents).
             */
            void invalidate();

            /** Macro used to register the class with the project's runtime type system. */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Pointer to the Colibri window used to render this layout.
             *
             * Non-owning raw pointer: the lifecycle of the `Colibri::Window`
             * is managed externally (created during `load` and destroyed on
             * `unload` or by the renderer). Implementations should ensure the
             * pointer is nullified when the window is destroyed.
             */
            Colibri::Window *m_window = nullptr;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // __UILayoutOgreNext_h__
