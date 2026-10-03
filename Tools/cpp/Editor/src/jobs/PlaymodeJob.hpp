#ifndef PlaymodeJob_h__
#define PlaymodeJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class PlaymodeJob : public Job
        {
        public:
            PlaymodeJob();
            ~PlaymodeJob() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // PlaymodeJob_h__
