#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GamePrefab, Resource<IGamePrefab> );

    GamePrefab::GamePrefab() = default;

    GamePrefab::~GamePrefab() = default;

    auto GamePrefab::createActor() -> SmartPtr<IGameActor>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto data = workphone::dynamic_pointer_cast<Properties>( getData() );
        if( !data )
            return nullptr;
        auto instanceData = GameActorUtil::createInstanceData( data );
        auto actors = GameActorUtil::loadSceneActors( { instanceData } );
        return actors.empty() ? nullptr : actors.front();
    }

    void GamePrefab::save()
    {
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
        m_data = data;
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
