#include <EditorPCH.hpp>
#include "jobs/LeavePlaymodeJob.hpp"
#include "editor/EditorManager.hpp"
#include "ui/UIManager.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    namespace
    {
        void retainEditorCamera( SmartPtr<scene::IGameScene> scene,
                                 SmartPtr<scene::ICameraManager> cameraManager )
        {
            auto editorCamera = cameraManager ? cameraManager->getEditorCamera() : nullptr;
            if( editorCamera && editorCamera->getScene() == scene )
            {
                if( auto parent = editorCamera->getParent() )
                {
                    parent->removeChild( editorCamera );
                }
                scene->removeActor( editorCamera );
            }
        }
    }

    LeavePlaymodeJob::LeavePlaymodeJob()
    {
        setPrimary( true );
    }

    LeavePlaymodeJob::~LeavePlaymodeJob() = default;

    void LeavePlaymodeJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto editorManager = EditorManager::getSingletonPtr();
            if( !applicationManager || !editorManager )
            {
                return;
            }

            auto taskManager = applicationManager->getTaskManager();
            auto gameManager = applicationManager->getGameManager();
            if( !taskManager || !gameManager )
            {
                return;
            }

            auto renderLock = taskManager->lockTask( TaskId::Render );
            auto physicsLock = taskManager->lockTask( TaskId::Physics );
            auto applicationLock = taskManager->lockTask( TaskId::Application );

            applicationManager->setPaused( false );
            applicationManager->setPlaying( false );
            applicationManager->setEditorCamera( true );

            if( auto savedScene = editorManager->getPlayModeScene() )
            {
                if( auto selectionManager = applicationManager->getSelectionManager() )
                {
                    selectionManager->clearSelection();
                }

                auto cameraManager = applicationManager->getCameraManager();
                auto runtimeScene = gameManager->getCurrentScene();
                if( runtimeScene && runtimeScene != savedScene && runtimeScene->isLoaded() )
                {
                    // Runtime scene switches can leave cameras and perpetual actors
                    // alive after returning to the captured editor scene.
                    retainEditorCamera( runtimeScene, cameraManager );
                    runtimeScene->setState( scene::IGameScene::State::Edit );
                    runtimeScene->clear( true );
                }

                retainEditorCamera( savedScene, cameraManager );
                gameManager->setCurrentScene( savedScene );
                editorManager->restorePlayModeScene();
            }

            gameManager->edit();
            if( auto cameraManager = applicationManager->getCameraManager() )
            {
                if( cameraManager->getState() == scene::ICameraManager::State::Edit )
                {
                    cameraManager->reset();
                }
                else
                {
                    cameraManager->setState( scene::ICameraManager::State::Edit );
                }
            }

            editorManager->clearPlayModeScene();
            if( auto uiManager = editorManager->getUI() )
            {
                uiManager->rebuildSceneTree();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
