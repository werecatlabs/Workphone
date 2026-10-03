#ifndef SetupMaterialJob_h__
#define SetupMaterialJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class SetupMaterialJob : public Job
        {
        public:
            SetupMaterialJob();
            ~SetupMaterialJob() override;

            void execute() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // SetupMaterialJob_h__
