#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/SceneClearJob.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Scene/GameScene.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SceneClearJob, Job );

    SceneClearJob::SceneClearJob() = default;

    SceneClearJob::~SceneClearJob() = default;

    void SceneClearJob::execute()
    {
        if( auto scene = getScene() )
        {
            if( !scene->isLoaded() )
                return;

            auto state = scene->getState();
            switch( state )
            {
            case scene::IGameScene::State::None:
            case scene::IGameScene::State::Edit:
            case scene::IGameScene::State::Play:
            {
                WP_ASSERT( scene->isValid() );
                WP_ASSERT( scene->getLoadingState() == LoadingState::Loaded );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManagerPtr();
                WP_ASSERT( factoryManager );

                auto gameManager = applicationManager->getGameManagerPtr();
                WP_ASSERT( gameManager );

                if( applicationManager->isPlaying() )
                {
                    auto actors = m_actors.snapshot();
                    for( auto &actor : actors )
                    {
                        if( actor )
                        {
                            if( actor->getPerpetual() == false )
                            {
                                gameManager->destroyActor( actor );
                                scene->removeActor( actor );
                            }
                        }
                    }
                }
                else
                {
                    auto actors = m_actors.snapshot();
                    for( auto &actor : actors )
                    {
                        if( actor )
                        {
                            gameManager->destroyActor( actor );
                            scene->removeActor( actor );
                        }
                    }

                    m_actors.clear();
                }

                const auto label = String( "Untitled" );
                auto concrete = workphone::dynamic_pointer_cast<scene::GameScene>( scene );
                if( !concrete || concrete->getLoadGeneration() == m_sceneGeneration )
                    scene->setLabel( label );

                applicationManager->triggerEvent(
                    EventType::Loading, scene::IGameManager::sceneClearHash, Array<Parameter>(), scene,
                    scene, nullptr, false, Thread::Application_Flag );
            }
            break;
            case scene::IGameScene::State::Reset:
            {
            }
            break;
            default:
            {
            }
            break;
            }
        }
    }

    SmartPtr<scene::IGameScene> SceneClearJob::getScene() const
    {
        return m_scene;
    }

    void SceneClearJob::setScene( SmartPtr<scene::IGameScene> scene )
    {
        m_scene = scene;
        auto concrete = workphone::dynamic_pointer_cast<scene::GameScene>( scene );
        m_sceneGeneration = concrete ? concrete->getLoadGeneration() : 0;
    }

    Array<SmartPtr<scene::IGameActor>> SceneClearJob::getActors() const
    {
        return m_actors.snapshot();
    }

    void SceneClearJob::setActors( const Array<SmartPtr<scene::IGameActor>> &actors )
    {
        m_actors = { actors.begin(), actors.end() };
    }
}  // namespace workphone
