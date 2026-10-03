#ifndef __UILayoutCore_h__
#define __UILayoutCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
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
         * that integrates Workphone's `UILayout` with the OgreNext Core UI
         * backend. The class is responsible for loading and unloading layout
         * resources, providing access to the associated FSM and UI window,
         * and managing the underlying Core window used for rendering.
         */

        /**
         * @class UILayoutOgreNext
         * @brief OgreNext-specific implementation of a UI layout element.
         *
         * `UILayoutOgreNext` extends `UIElementOgreNext<UILayout>` and binds the
         * engine's `UILayout` abstraction to an OgreNext `wp_window`.
         * It handles lifecycle operations (load/unload), exposes the layout's
         * FSM, and forwards/getters/setters for the associated `IUIWindow`.
         *
         * Responsibilities:
         * - Initialize and tear down any OgreNext-specific resources for the layout.
         * - Provide accessors for the layout's FSM and external `IUIWindow`.
         * - Allow the layout to be invalidated, prompting a refresh or rebuild.
         */
        class UILayoutCore : public UIElementCore<UILayout>
        {
        public:
            /**
             * @brief Construct a new `UILayoutOgreNext`.
             *
             * Initializes internal state; does not create the underlying
             * Core window until `load` is called.
             */
            UILayoutCore();

            /**
             * @brief Destroy the `UILayoutOgreNext`.
             *
             * Ensures any owned resources are released. The destructor is
             * virtual to allow proper cleanup through base pointers.
             */
            ~UILayoutCore() override;

            /**
             * @brief Load the layout with optional initialization data.
             *
             * @param data Optional shared object containing initialization data
             *             (e.g., resource descriptors, parent pointers).
             *
             * This function should create or configure the underlying Core
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
             * This will destroy or detach the Core window if it was created
             * during `load`, and perform any other necessary cleanup.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            void update() override;

            /** Macro used to register the class with the project's runtime type system. */
            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // __UILayoutOgreNext_h__
