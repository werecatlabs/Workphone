#ifndef LeavePlaymodeJob_h__
#define LeavePlaymodeJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class LeavePlaymodeJob : public Job
        {
        public:
            LeavePlaymodeJob();
            ~LeavePlaymodeJob() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // LeavePlaymodeJob_h__
