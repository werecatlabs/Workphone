// ---------------------------------------------------------------------------
//  AssetDatabaseEditorMainWindow.hpp
//
//  C++17 port of the C# MainWindow.  Hosts the top-level editor UI
//  (tab bar, data grids, menus) and delegates to the dialog classes for
//  model/component/copy operations.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorMainWindow_h__
#define AssetDatabaseEditorMainWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <AssetDatabaseEditorSettings.hpp>
#include <ui/AddComponentGroupWindow.hpp>
#include <ui/CopyDataWindow.hpp>
#include <ui/CopyFixedWingDataWindow.hpp>
#include <ui/HelpSupportWindow.hpp>
#include <ui/MirrorWingDataWindow.hpp>
#include <ui/ModelBrowserWindow.hpp>
#include <ui/ModelObjectWindow.hpp>
#include <ui/WingDataWindow.hpp>
#include <Workphone/Workphone.hpp>

#include <vector>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Top level window of the C++17 asset database editor.
         *
         * Mirrors the C# @c fb.MainWindow:
         *   - @c Open database, set as the working connection
         *   - Tabbed views for Resources / Object Setup / Object Components
         *   - Menus that open the various dialog windows
         */
        class AssetDatabaseEditorMainWindow : public ISharedObject
        {
        public:
            AssetDatabaseEditorMainWindow();
            ~AssetDatabaseEditorMainWindow() override;

            void load( SmartPtr<ISharedObject> data );
            void unload( SmartPtr<ISharedObject> data );

            SmartPtr<ui::IUIWindow> getRootWindow() const { return m_rootWindow; }

            WP_CLASS_REGISTER_DECL;

        protected:
            void createRoot();
            void createMenuBar();
            void createTabs();
            void populateModels();
            void populateComponents();
            void populateComponentGroups();
            void populateResourceAttribs();
            void populateObjectAttribs();
            void onOpenDatabase();
            void onExit();

            SmartPtr<AssetDatabaseEditorDatabase> m_database;
            SmartPtr<AssetDatabaseEditorSettings> m_settings;

            SmartPtr<ui::IUIWindow> m_rootWindow;
            SmartPtr<ui::IUIMenubar> m_menuBar;
            SmartPtr<ui::IUITabBar> m_tabBar;

            SmartPtr<ui::IUIDropdown> m_modelsDropdown;
            SmartPtr<ui::IUIDataGrid> m_modelAttribsGrid;
            SmartPtr<ui::IUIDataGrid> m_componentsGrid;
            SmartPtr<ui::IUIDataGrid> m_componentGroupsGrid;
            SmartPtr<ui::IUITextEntry> m_sqlText;
            SmartPtr<ui::IUIText> m_sqlResultText;

            SmartPtr<WingDataWindow> m_wingDataWindow;
            SmartPtr<CopyDataWindow> m_copyDataWindow;
            SmartPtr<CopyFixedWingDataWindow> m_copyFixedWingDataWindow;
            SmartPtr<MirrorWingDataWindow> m_mirrorWingDataWindow;
            SmartPtr<AddComponentGroupWindow> m_addComponentGroupWindow;
            SmartPtr<HelpSupportWindow> m_helpSupportWindow;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorMainWindow_h__
