// ---------------------------------------------------------------------------
//  MirrorWingDataWindow.hpp
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorMirrorWingDataWindow_h__
#define AssetDatabaseEditorMirrorWingDataWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorDatabase.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Mirrors the position/scale of a wing's attributes along
         *        the x axis.  This is a direct port of the C#
         *        MirrorWingDataWindow helper.
         */
        class MirrorWingDataWindow : public ISharedObject
        {
        public:
            MirrorWingDataWindow();
            ~MirrorWingDataWindow() override;

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

#endif  // AssetDatabaseEditorMirrorWingDataWindow_h__
