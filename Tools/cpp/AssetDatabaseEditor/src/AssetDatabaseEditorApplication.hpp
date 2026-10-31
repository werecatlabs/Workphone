// ---------------------------------------------------------------------------
//  AssetDatabaseEditorApplication.hpp
//
//  Application wrapper that hosts the ported AssetDatabaseWindow from the
//  in-progress Editor project.  It reuses the engine's core::Application
//  class so that the renderer, ImGui-based UI and database interfaces are
//  brought up by the regular Workphone bootstrap.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorApplication_h__
#define AssetDatabaseEditorApplication_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <Workphone/Application.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Stand-alone application for the C++17 asset database editor.
         *
         * The application derives from the engine's @c core::Application and
         * adds the editor window.  It is intentionally lightweight so that
         * the rest of the engine subsystems (graphics, UI, asset import, ...)
         * continue to be configured the same way as for the main editor.
         */
        class AssetDatabaseEditorApplication : public core::Application
        {
        public:
            enum class ElementId
            {
                Open,
                Exit,

                Count
            };

            AssetDatabaseEditorApplication();
            ~AssetDatabaseEditorApplication() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void run() override;
            void iterate() override;
            void createPlugins();
            void createScene();
            void createUI();
            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<AssetDatabaseEditorMainWindow> m_mainWindow;
            SmartPtr<AssetDatabaseWindow> m_assetDatabaseWindow;
            SmartPtr<ui::IUIApplication> m_application;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorApplication_h__
