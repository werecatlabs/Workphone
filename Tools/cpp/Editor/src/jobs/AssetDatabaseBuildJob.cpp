#include <EditorPCH.hpp>
#include "jobs/AssetDatabaseBuildJob.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    AssetDatabaseBuildJob::AssetDatabaseBuildJob() = default;

    AssetDatabaseBuildJob::~AssetDatabaseBuildJob() = default;

    void AssetDatabaseBuildJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        WP_ASSERT( resourceDatabase );

        resourceDatabase->build();
    }
}  // namespace workphone::editor
