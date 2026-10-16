#ifndef ImGuiDragJob_h__
#define ImGuiDragJob_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Memory/WeakPtr.hpp>

namespace workphone
{
    namespace ui
    {

        /** Resolves a tree drag payload on the application task. */
        class WPImGui_API ImGuiDragJob : public Job
        {
        public:
            ImGuiDragJob();

            ~ImGuiDragJob() override;

            void execute() override;

            /** Configure before enqueueing. The element is weak so a pending job cannot retain
                a tree control that has been removed from the UI. */
            void setDragSource( SmartPtr<IUIDragSource> dragSource );
            void setElement( SmartPtr<IUIElement> element );
            void setRoot( bool root );

            /** Read only after the job reaches State::Finish. */
            const String &getPayload() const;

            WP_CLASS_REGISTER_DECL;

        private:
            SmartPtr<IUIDragSource> m_dragSource;
            WeakPtr<IUIElement> m_element;
            String m_payload;
            bool m_root = false;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiDragJob_h__
