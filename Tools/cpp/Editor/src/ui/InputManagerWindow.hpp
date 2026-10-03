#ifndef InputManagerWindow_h__
#define InputManagerWindow_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"

namespace workphone
{
    namespace editor
    {
        class InputManagerWindow : public EditorWindow
        {
        public:
            InputManagerWindow();
            ~InputManagerWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<ui::IUIInputManager> getInputManager() const;

            void setInputManager( SmartPtr<ui::IUIInputManager> inputManager );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUIInputManager> m_inputManager;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // InputManagerWindow_h__
