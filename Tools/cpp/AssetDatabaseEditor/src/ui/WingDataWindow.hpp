// ---------------------------------------------------------------------------
//  WingDataWindow.hpp
//
//  Replaces the C# Tools/csharp/AssetDatabaseTool/WingDataWindow.  The
//  original C# class was empty; the C++ port hosts a simple editor for the
//  aerodynamic curves used by the asset database.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorWingDataWindow_h__
#define AssetDatabaseEditorWingDataWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Lightweight standalone wing-curve editor.  Mirrors the
         *        empty C# stub but provides useful edit affordances using the
         *        engine's UI primitives.
         */
        class WingDataWindow : public ISharedObject
        {
        public:
            WingDataWindow();
            ~WingDataWindow() override;

            void setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database );
            void show();

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<AssetDatabaseEditorDatabase> m_database;
            SmartPtr<ui::IUIWindow> m_window;
            SmartPtr<ui::IUIText> m_statusText;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorWingDataWindow_h__
