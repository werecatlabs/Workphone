#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Database/AssetDocumentSave.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Path.hpp>
#include <stdexcept>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GamePrefab, Resource<IGamePrefab> );

    GamePrefab::GamePrefab() = default;

    GamePrefab::~GamePrefab() = default;

    auto GamePrefab::createActor() -> SmartPtr<IGameActor>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );
        SmartPtr<ITask> renderTask;
        if(auto tasks=applicationManager->getTaskManager())
            if(auto task=tasks->getTask(TaskId::Render);task&&task->isLoaded())renderTask=task;
        TaskLock renderPause(renderTask);
        ScopedLock managerLock(sceneManager);

        auto data = workphone::dynamic_pointer_cast<Properties>( getData() );
        if( !data )
        {
            return nullptr;
        }
        String error;
        if(m_runtimeAssets&&!resource::validateCookedActorGraphData(data,false,m_runtimeAssets,error))
            throw std::runtime_error(error.c_str());
        struct ResolutionScope {
            Properties::ObjectResolver previous;
            bool fallback=true,active=false;
            explicit ResolutionScope(const std::shared_ptr<const resource::RuntimeAssetContext> &context) {
                if(context){previous=Properties::exchangeObjectResolver(context->objects);fallback=Properties::exchangeObjectResolverFallback(false);active=true;}
            }
            ~ResolutionScope(){if(active){Properties::exchangeObjectResolver(std::move(previous));Properties::exchangeObjectResolverFallback(fallback);}}
        } resolution(m_runtimeAssets);
        auto instanceData = GameActorUtil::createInstanceData( data );
        auto graphics=applicationManager->getGraphicsSystem();ScopedLock graphicsLock(graphics);
        auto actors = GameActorUtil::loadSceneActors( { instanceData },nullptr,m_runtimeAssets );
        return actors.empty() ? nullptr : actors.front();
    }

    void GamePrefab::save()
    {
        auto properties = dynamic_pointer_cast<Properties>( getData() );
        const auto path = getFilePath();
        String error;
        const auto format =
            Path::getFileExtension( path ) == ".fbscenexml" ? DataFormat::XML : DataFormat::JSON;
        if( !saveAssetDocumentText( path, properties,
                                    DataUtil::toString( properties.get(), true, format ), error ) )
        {
            throw std::runtime_error( error.c_str() );
        }
    }

    void GamePrefab::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        setLoadingState( LoadingState::Loaded );
    }

    void GamePrefab::unload( SmartPtr<ISharedObject> data )
    {
        ScopedLoadstateWait loadstateWait( this );

        setLoadingState( LoadingState::Unloading );

        setData( nullptr );
        m_runtimeAssets.reset();

        setLoadingState( LoadingState::Unloaded );
    }

    auto GamePrefab::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Resource<IGamePrefab>::getProperties();
        return properties;
    }

    void GamePrefab::setProperties( SmartPtr<Properties> properties )
    {
    }

    void GamePrefab::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    auto GamePrefab::getData() const -> SmartPtr<ISharedObject>
    {
        ScopedLock lock( this );
        return m_data;
    }

    void GamePrefab::setData( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock( this );
        if(m_runtimeAssets&&data&&data!=m_data)throw std::logic_error("Unload the cooked prefab before replacing its document");
        m_data = data;
    }

    void GamePrefab::setRuntimeAssets( std::shared_ptr<const resource::RuntimeAssetContext> context )
    {
        ScopedLock lock(this);
        if(m_runtimeAssets)throw std::logic_error("Cooked prefab context is already sealed");
        m_runtimeAssets=std::move(context);
    }

    void GamePrefab::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->lock();
        }
    }

    bool GamePrefab::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            return gameManager->try_lock();
        }

        return false;
    }

    void GamePrefab::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->unlock();
        }
    }

}  // namespace workphone::scene
