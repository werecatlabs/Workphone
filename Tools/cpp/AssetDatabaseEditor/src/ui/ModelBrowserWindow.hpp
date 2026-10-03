// ---------------------------------------------------------------------------
//  ModelBrowserWindow.hpp
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorModelBrowserWindow_h__
#define AssetDatabaseEditorModelBrowserWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /** @brief Model browser used by the C# CopyFixedWingDataWindow. */
        class ModelBrowserWindow : public ISharedObject
        {
        public:
            ModelBrowserWindow();
            ~ModelBrowserWindow() override;

            void setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database );
            void setModelObjectType( const String &type );
            void show();

            SmartPtr<Model> getSelectedModel() const { return m_selectedModel; }

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<AssetDatabaseEditorDatabase> m_database;
            String m_modelObjectType;
            SmartPtr<Model> m_selectedModel;
            SmartPtr<ui::IUIWindow> m_window;
            SmartPtr<ui::IUIDropdown> m_modelDropdown;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorModelBrowserWindow_h__
