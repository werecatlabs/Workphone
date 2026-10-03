#include <EditorPCH.hpp>
#include <jobs/ReloadScriptsJob.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>
#include "ui/MaterialWindow.hpp"
#include "ui/ObjectWindow.hpp"
#include "ui/TerrainWindow.hpp"
#include "ui/UIManager.hpp"

namespace workphone::editor
{
    ReloadScriptsJob::ReloadScriptsJob() = default;

    ReloadScriptsJob::~ReloadScriptsJob() = default;

    void ReloadScriptsJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto threadPool = applicationManager->getThreadPool();

        auto renderLock = taskManager->lockTask( TaskId::Render );
        //auto applicationLock = taskManager->lockTask( TaskId::Application );
        auto physicsLock = taskManager->lockTask( TaskId::Physics );

        auto scriptManager = applicationManager->getScriptManager();
        if( scriptManager )
        {
            scriptManager->reloadScripts();

            if( threadPool->getNumThreads() > 0 )
            {
                while( scriptManager->reloadPending() )
                {
                    Thread::sleep( 3.0 );
                }
            }
        }

        // hack
        auto editorManager = EditorManager::getSingletonPtr();
        auto ui = editorManager->getUI();
        if( auto materialWindow = ui->getObjectWindow() )
        {
            materialWindow->reload( nullptr );
        }

        if( auto terrainWindow = ui->getTerrainWindow() )
        {
            terrainWindow->reload( nullptr );
        }
    }
}  // namespace workphone::editor
