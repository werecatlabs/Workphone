#ifndef ProjectCleanJob_h__
#define ProjectCleanJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class ProjectCleanJob : public Job
        {
        public:
            ProjectCleanJob();
            ~ProjectCleanJob() override;

            void execute() override;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // ProjectCleanJob_h__
