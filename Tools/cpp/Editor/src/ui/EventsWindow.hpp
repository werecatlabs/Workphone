#ifndef EventsWindow_h__
#define EventsWindow_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace editor
    {
        class EventsWindow : public EditorWindow
        {
        public:
            EventsWindow();
            ~EventsWindow() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void updateSelection() override;

        protected:
            Array<SmartPtr<EventWindow>> m_eventWindows;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // EventsWindow_h__
