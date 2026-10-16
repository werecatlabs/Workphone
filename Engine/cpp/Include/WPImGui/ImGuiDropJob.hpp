#ifndef ImGuiDropJob_h__
#define ImGuiDropJob_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone::ui
{
    /** Delivers a copied tree drop payload on the application task. */
    class WPImGui_API ImGuiDropJob : public Job
    {
    public:
        ImGuiDropJob();
        ~ImGuiDropJob() override;

        void execute() override;

        /** Configure before enqueueing. References keep event data alive until delivery. */
        void setDropTarget( SmartPtr<IUIDropTarget> dropTarget );
        void setSender( SmartPtr<ISharedObject> sender );
        void setObject( SmartPtr<ISharedObject> object );
        void setArguments( Array<Parameter> arguments );

        WP_CLASS_REGISTER_DECL;

    private:
        SmartPtr<IUIDropTarget> m_dropTarget;
        SmartPtr<ISharedObject> m_sender;
        SmartPtr<ISharedObject> m_object;
        Array<Parameter> m_arguments;
    };
}  // namespace workphone::ui

#endif  // ImGuiDropJob_h__
