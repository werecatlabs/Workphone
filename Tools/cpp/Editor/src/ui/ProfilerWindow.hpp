#ifndef ProfilerWindow_h__
#define ProfilerWindow_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"

namespace workphone
{
    namespace editor
    {
        /**
         * @class ProfilerWindow
         * @brief Editor window wrapper that owns and manages the profiler UI element.
         *
         * The window creates a parent UI container and hosts a profiler-specific UI element
         * inside it while the editor window is loaded.
         */
        class ProfilerWindow : public EditorWindow
        {
        public:
            /**
             * @brief Creates a new profiler editor window.
             */
            ProfilerWindow();

            /**
             * @brief Destroys the window and releases its UI resources.
             */
            ~ProfilerWindow() override;

            /**
             * @brief Loads the profiler window and creates the underlying UI elements.
             * @param data Optional shared object payload passed in by the editor framework.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the profiler window and removes its UI elements.
             * @param data Optional shared object payload passed in by the editor framework.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the profiler window each frame.
             */
            void update() override;

            /**
             * @brief Gets the hosted profiler UI element.
             * @return The profiler UI element if it has been created; otherwise, a null pointer.
             */
            SmartPtr<ui::IUIProfilerWindow> getProfilerWindow() const;

            /**
             * @brief Sets the hosted profiler UI element.
             * @param profilerWindow The profiler UI element to track.
             */
            void setProfilerWindow( SmartPtr<ui::IUIProfilerWindow> profilerWindow );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Cached pointer to the profiler UI element owned by this window.
             */
            SmartPtr<ui::IUIProfilerWindow> m_profilerWindow;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // ProfilerWindow_h__
