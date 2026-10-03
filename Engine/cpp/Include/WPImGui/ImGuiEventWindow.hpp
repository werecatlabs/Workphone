#ifndef ImGuiEventWindow_h__
#define ImGuiEventWindow_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiWindowT.hpp>
#include <Workphone/Interface/UI/IUIEventWindow.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiEventWindow : public ImGuiWindowT<IUIEventWindow>
        {
        public:
            ImGuiEventWindow();
            ~ImGuiEventWindow() override;

            /** @copydoc IUIEventWindow::getEvents */
            Array<SmartPtr<IEvent>> getEvents() const override;

            /** @copydoc IUIEventWindow::setEvents */
            void setEvents( const Array<SmartPtr<IEvent>> &events ) override;

            void update() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<IEvent>> m_events;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiWindow_h__
