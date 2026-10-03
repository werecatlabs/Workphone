#ifndef ReloadScriptsJob_h__
#define ReloadScriptsJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class ReloadScriptsJob : public Job
        {
        public:
            ReloadScriptsJob();
            ~ReloadScriptsJob() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // ReloadScriptsJob_h__
