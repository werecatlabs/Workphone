#ifndef EventListenerWindow_h__
#define EventListenerWindow_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace editor
    {
        class EventListenerWindow : public EditorWindow
        {
        public:
            EventListenerWindow();
            ~EventListenerWindow() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void updateSelection() override;

            SmartPtr<scene::IComponentEventListener> getComponentEventListener() const;
            void setComponentEventListener( SmartPtr<scene::IComponentEventListener> eventListener );

        protected:
            SmartPtr<PropertiesWindow> m_propertiesWindow;

            SmartPtr<scene::IComponentEventListener> m_componentEventListener;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // EventListenerWindow_h__
