// ---------------------------------------------------------------------------
//  AddComponentGroupWindow.hpp
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorAddComponentGroupWindow_h__
#define AssetDatabaseEditorAddComponentGroupWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Port of the C# AddComponentGroupWindow.  Lets the user add
         *        a new entry to the @c component_groups table.
         */
        class AddComponentGroupWindow : public ISharedObject
        {
        public:
            AddComponentGroupWindow();
            ~AddComponentGroupWindow() override;

            void setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database );
            void show();

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<AssetDatabaseEditorDatabase> m_database;
            SmartPtr<ui::IUIWindow> m_window;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorAddComponentGroupWindow_h__
