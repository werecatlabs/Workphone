// ---------------------------------------------------------------------------
//  ModelObjectWindow.hpp
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorModelObjectWindow_h__
#define AssetDatabaseEditorModelObjectWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Picker window for selecting a model + model object.  Port of
         *        the C# ModelObjectWindow used by the copy/mirror dialogs.
         */
        class ModelObjectWindow : public ISharedObject
        {
        public:
            ModelObjectWindow();
            ~ModelObjectWindow() override;

            void setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database );
            void setModelObjectType( const String &type );
            void show();

            SmartPtr<ModelObject> getSelectedModelObject() const { return m_selectedModelObject; }
            SmartPtr<Model> getSelectedModel() const { return m_selectedModel; }

            WP_CLASS_REGISTER_DECL;

        protected:
            void populateModels();

            SmartPtr<AssetDatabaseEditorDatabase> m_database;
            String m_modelObjectType;
            SmartPtr<Model> m_selectedModel;
            SmartPtr<ModelObject> m_selectedModelObject;
            SmartPtr<ui::IUIWindow> m_window;
            SmartPtr<ui::IUIDropdown> m_modelDropdown;
            SmartPtr<ui::IUIDropdown> m_modelObjectDropdown;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorModelObjectWindow_h__
