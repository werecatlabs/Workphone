#include <EditorPCH.hpp>
#include "jobs/AssetImportJob.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    AssetImportJob::AssetImportJob() = default;

    AssetImportJob::~AssetImportJob() = default;

    void AssetImportJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        WP_ASSERT( resourceDatabase );

        resourceDatabase->importAssets();
    }
}  // namespace workphone::editor
