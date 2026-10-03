#ifndef ProjectAssetsDropJob_h__
#define ProjectAssetsDropJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {

        class ProjectAssetsDropJob : public Job
        {
        public:
            ProjectAssetsDropJob();
            ~ProjectAssetsDropJob() override;

            void execute() override;
        };

    }  // end namespace editor
}  // namespace workphone

#endif  // ProjectAssetsDropJob_h__
