#ifndef ShowDebugJob_h__
#define ShowDebugJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {

        class ShowDebugJob : public Job
        {
        public:
            ShowDebugJob();
            ~ShowDebugJob() override;

            void execute() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace editor
}  // namespace workphone

#endif  // ShowDebugJob_h__
