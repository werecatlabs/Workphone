#include <EditorPCH.hpp>
#include "jobs/PlaymodeJob.hpp"
#include "editor/EditorManager.hpp"
#include "ui/UIManager.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/NetworkSceneValidation.hpp>

namespace workphone::editor
{
    PlaymodeJob::PlaymodeJob()
    {
        // Play and stop requests must execute in order, even with worker jobs enabled.
        setPrimary( true );
    }

    PlaymodeJob::~PlaymodeJob() = default;

    void PlaymodeJob::execute()
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
            auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
            if( !taskManager || !scene )
            {
                WP_LOG_ERROR( "Cannot enter play mode without a current scene." );
                return;
            }

            auto renderLock = taskManager->lockTask( TaskId::Render );
            auto physicsLock = taskManager->lockTask( TaskId::Physics );
            auto applicationLock = taskManager->lockTask( TaskId::Application );

            if( applicationManager->isPlaying() || editorManager->getPlayModeSceneData() )
            {
                applicationManager->setPaused( false );
                return;
            }

            const auto loadingState = scene->getSceneLoadingState();
            const auto isUnsavedScene = loadingState == scene::IGameScene::SceneLoadingState::None &&
                                        StringUtil::isNullOrEmpty( scene->getFilePath() );
            if( !scene->isLoaded() ||
                ( loadingState != scene::IGameScene::SceneLoadingState::Loaded && !isUnsavedScene ) )
            {
                // A failed/pending file load must not block the primary job queue.
                WP_LOG_ERROR( "Cannot enter play mode until the scene has loaded." );
                return;
            }

            // Offline scenes remain playable. When a network manager is installed,
            // reject routing/capability errors before capture or runtime flags change.
            if( auto networkManager = applicationManager->getNetworkManager() )
            {
                const auto validation = scene::validateNetworkScene( scene, networkManager );
                if( !validation.issues.empty() )
                {
                    std::size_t shown = 0;
                    for( const auto &issue : validation.issues )
                    {
                        if( shown++ == 16 )
                            break;
                        auto actor = validation.actors[issue.viewIndex];
                        WP_LOG_ERROR( String( "Cannot enter network play: " ) + actor->getName() +
                            ": " + network::networkSceneIssueMessage( issue.code ) );
                    }
                    WP_LOG_ERROR( String( "Network scene validation found " ) +
                        StringUtil::toString( static_cast<u32>( validation.issues.size() ) ) + " errors." );
                    return;
                }
            }

            if( !editorManager->capturePlayModeScene( scene ) )
            {
                WP_LOG_ERROR( "Cannot capture the scene before entering play mode." );
                return;
            }

            // State changes can synchronously reset cameras and initialise components.
            // Publish the runtime flags before those callbacks inspect the application.
            applicationManager->setPaused( false );
            applicationManager->setEditorCamera( false );
            applicationManager->setPlaying( true );
            gameManager->play();

            if( auto cameraManager = applicationManager->getCameraManager() )
            {
                if( cameraManager->getState() == scene::ICameraManager::State::Play )
                {
                    cameraManager->reset();
                }
                else
                {
                    cameraManager->setState( scene::ICameraManager::State::Play );
                }
            }

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
