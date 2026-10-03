#ifndef CreateCodeProjectJob_h__
#define CreateCodeProjectJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class CreateCodeProjectJob : public Job
        {
        public:
            CreateCodeProjectJob();
            ~CreateCodeProjectJob() override;

            void execute() override;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // CreateCodeProjectJob_h__
