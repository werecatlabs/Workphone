#ifndef OptimiseDatabasesJob_h__
#define OptimiseDatabasesJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class OptimiseDatabasesJob : public Job
        {
        public:
            OptimiseDatabasesJob();
            ~OptimiseDatabasesJob() override;

            void execute() override;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // OptimiseDatabasesJob_h__
