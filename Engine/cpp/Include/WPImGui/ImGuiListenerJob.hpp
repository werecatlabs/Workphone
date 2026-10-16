#ifndef ImGuiListenerJob_h__
#define ImGuiListenerJob_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/WorkphoneEnums.hpp>

namespace workphone::ui
{
    /** Dispatches an event to a captured listener list on the application task. */
    class WPImGui_API ImGuiListenerJob : public Job
    {
    public:
        ImGuiListenerJob();
        ~ImGuiListenerJob() override;

        void execute() override;

        static void queue( Array<SmartPtr<IEventListener>> listeners, EventType eventType,
                           hash_type eventValue, Array<Parameter> arguments,
                           SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object );

        WP_CLASS_REGISTER_DECL;

    private:
        Array<SmartPtr<IEventListener>> m_listeners;
        Array<Parameter> m_arguments;
        SmartPtr<ISharedObject> m_sender;
        SmartPtr<ISharedObject> m_object;
        EventType m_eventType = EventType::Object;
        hash_type m_eventValue = 0;
    };
}  // namespace workphone::ui

#endif  // ImGuiListenerJob_h__
