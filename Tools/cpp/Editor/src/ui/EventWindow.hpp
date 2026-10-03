#ifndef EventWindow_h__
#define EventWindow_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {
        class EventWindow : public EditorWindow
        {
        public:
            enum class WidgetId
            {
                Label,

                Enabled,
                Visible,
                Static,

                AddComponent,
                RemoveComponent,

                Count
            };

            EventWindow();
            ~EventWindow() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void updateSelection() override;

            SmartPtr<scene::IComponentEvent> getEvent() const;
            void setEvent( SmartPtr<scene::IComponentEvent> event );

        protected:
            class UIElementListener : public IEventListener
            {
            public:
                UIElementListener();
                ~UIElementListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                EventWindow *getOwner() const;
                void setOwner( EventWindow *owner );

            private:
                EventWindow *m_owner = nullptr;
            };

            SmartPtr<scene::IComponentEvent> m_event;

            Array<SmartPtr<ui::IUIEventWindow>> m_eventWindows;

            Array<SmartPtr<EventListenerWindow>> m_eventListenerWindows;

            SmartPtr<ui::IUIButton> m_addComponentButton;
            SmartPtr<ui::IUIButton> m_removeComponentButton;

            SmartPtr<IEventListener> m_uiListener;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // EventsWindow_h__
