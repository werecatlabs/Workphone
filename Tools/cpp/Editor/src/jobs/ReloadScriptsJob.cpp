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
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = applicationManager ? applicationManager->getScriptManager() : nullptr;
        auto queue = applicationManager ? applicationManager->getJobQueue() : nullptr;
        if( !scriptManager || !scriptManager->isLoaded() || !queue ) return;

        // The queue resumes coroutines on Primary. Yield while Application builds
        // the replacement VM so Render, Physics and the Editor can keep progressing.
        queue->startCoroutine( [scriptManager]( ICoroutineData::PullType &yield ) {
            scriptManager->reloadScripts();
            while( scriptManager->reloadPending() ) yield();
            auto app = core::IApplicationManager::instancePtr();
            if( !app || app->getQuit() || app->getScriptManager() != scriptManager ||
                !scriptManager->isLoaded() ) return;
            if( scriptManager->getError() )
            {
                WP_LOG_ERROR( "Script reload failed; the previous Lua state remains active" );
                return;
            }
            auto editor = EditorManager::getSingletonPtr();
            auto ui = editor ? editor->getUI() : nullptr;
            if( !ui ) return;
            if( auto objectWindow = ui->getObjectWindow() ) objectWindow->reload( nullptr );
            if( auto terrainWindow = ui->getTerrainWindow() ) terrainWindow->reload( nullptr );
        } );
    }
}  // namespace workphone::editor
