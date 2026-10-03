// ---------------------------------------------------------------------------
//  HelpSupportWindow.hpp
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorHelpSupportWindow_h__
#define AssetDatabaseEditorHelpSupportWindow_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /**
         * @brief Lightweight help/about window.  Mirrors the C#
         *        HelpSupportWindow (which is also a stub).
         */
        class HelpSupportWindow : public ISharedObject
        {
        public:
            HelpSupportWindow();
            ~HelpSupportWindow() override;

            void show();

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUIWindow> m_window;
            SmartPtr<ui::IUIText> m_text;
            bool m_visible = false;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorHelpSupportWindow_h__
