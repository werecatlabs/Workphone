#ifndef AssetImportJob_h__
#define AssetImportJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class AssetImportJob : public Job
        {
        public:
            AssetImportJob();
            ~AssetImportJob() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // AssetImportJob_h__
