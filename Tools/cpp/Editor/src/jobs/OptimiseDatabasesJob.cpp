#include <EditorPCH.hpp>
#include <jobs/OptimiseDatabasesJob.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    OptimiseDatabasesJob::OptimiseDatabasesJob() = default;

    OptimiseDatabasesJob::~OptimiseDatabasesJob() = default;

    void OptimiseDatabasesJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();

            if( auto resourceDatabase = applicationManager->getResourceDatabase() )
            {
                resourceDatabase->optimise();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
