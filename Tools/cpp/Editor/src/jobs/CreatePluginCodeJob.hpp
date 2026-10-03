#ifndef CreatePluginCodeJob_h__
#define CreatePluginCodeJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class CreatePluginCodeJob : public Job
        {
        public:
            CreatePluginCodeJob();
            ~CreatePluginCodeJob() override;

            void execute() override;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // CreatePluginCodeJob_h__
