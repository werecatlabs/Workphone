#ifndef AssetDatabaseBuildJob_h__
#define AssetDatabaseBuildJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class AssetDatabaseBuildJob : public Job
        {
        public:
            AssetDatabaseBuildJob();
            ~AssetDatabaseBuildJob() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // AssetDatabaseBuildJob_h__
