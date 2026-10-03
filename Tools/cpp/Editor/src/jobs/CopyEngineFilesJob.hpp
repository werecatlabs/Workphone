#ifndef CopyEngineFilesJob_h__
#define CopyEngineFilesJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class CopyEngineFilesJob : public Job
        {
        public:
            CopyEngineFilesJob();
            ~CopyEngineFilesJob() override;

            void execute() override;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // CreateCodeProjectJob_h__
