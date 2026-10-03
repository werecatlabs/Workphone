#ifndef FileViewWindow_h__
#define FileViewWindow_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace editor
    {
        class FileViewWindow : public EditorWindow
        {
        public:
            FileViewWindow( SmartPtr<ui::IUIWindow> parent );
            ~FileViewWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void updateSelection() override;

        protected:
            String m_filePath;
            SmartPtr<ui::IUIText> m_text;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // FileViewWindow_h__
