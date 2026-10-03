#ifndef SaveProjectJob_h__
#define SaveProjectJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class SaveProjectJob : public Job
        {
        public:
            SaveProjectJob();
            ~SaveProjectJob() override;

            void execute() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace editor
}  // namespace workphone

#endif // SaveProjectJob_h__
