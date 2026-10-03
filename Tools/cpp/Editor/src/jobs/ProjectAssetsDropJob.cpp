#include <EditorPCH.hpp>
#include <jobs/ProjectAssetsDropJob.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    ProjectAssetsDropJob::ProjectAssetsDropJob() = default;

    ProjectAssetsDropJob::~ProjectAssetsDropJob() = default;

    void ProjectAssetsDropJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );
    }
}  // namespace workphone::editor
