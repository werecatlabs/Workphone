#ifndef ScriptWindow_h__
#define ScriptWindow_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @class ScriptWindow
         * @brief ScriptWindow is a window that displays a script.
         * @ingroup FBEditor
         */
        class ScriptWindow : public EditorWindow
        {
        public:
            static const String classNameStr;  ///< String identifier for the class name.
            static const String
                updateInEditModeStr;  ///< String identifier for update-in-edit-mode property.
            static const String getPropertiesStr;  ///< String identifier for getting properties.
            static const String setPropertiesStr;  ///< String identifier for setting properties.

            ScriptWindow();
            ~ScriptWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void reload( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void setWindowVisible( bool visible ) override;

            SmartPtr<Properties> getProperties() const override;

            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            bool m_changeLoadingState = true;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // ScriptWindow_h__
