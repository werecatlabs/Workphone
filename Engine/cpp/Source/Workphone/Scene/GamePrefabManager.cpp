#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/ApplicationUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GamePrefabManager, IGamePrefabManager );

    namespace
    {
        SmartPtr<Properties> parsePrefabData( const String &dataStr, DataFormat preferredFormat )
        {
            auto data = workphone::make_ptr<Properties>();
            if( StringUtil::isNullOrEmpty( dataStr ) )
            {
                return data;
            }

            auto firstNonWhitespace = dataStr.find_first_not_of( " \t\r\n" );
            auto format = preferredFormat;
            if( firstNonWhitespace != String::npos )
            {
                const auto firstChar = dataStr[firstNonWhitespace];
                if( firstChar == '{' || firstChar == '[' )
                {
                    format = DataFormat::JSON;
                }
                else if( firstChar == '<' )
                {
                    format = DataFormat::XML;
                }
            }

            DataUtil::parse( dataStr, data.get(), format );
            return data;
        }
    }  // namespace

    GamePrefabManager::GamePrefabManager() = default;

    GamePrefabManager::~GamePrefabManager() = default;

    SmartPtr<IGameActor> GamePrefabManager::createInstance( SmartPtr<IGameActor> prefab )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto newActor = sceneManager->createActor();
        auto prefabData = prefab->toData();
        newActor->fromData( prefabData );

        return newActor;
    }

    SmartPtr<IGameActor> GamePrefabManager::loadActor( SmartPtr<Properties> data,
                                                       SmartPtr<IGameActor> parent, bool cascade )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto sceneManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( sceneManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            GameActorUtil::loadFromData( actor, data, cascade );

            if( parent )
            {
                parent->addChild( actor );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IGamePrefab> GamePrefabManager::loadPrefab( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto uuid = StringUtil::getUUID();

        auto prefabResource = create( uuid );
        auto prefab = workphone::static_pointer_cast<IGamePrefab>( prefabResource );

        auto ext = Path::getFileExtension( filePath );
        ext = StringUtil::make_lower( ext );

        if( ApplicationUtil::isSupportedMesh( filePath ) )
        {
            auto meshLoader = applicationManager->getMeshLoader();
            if( !meshLoader )
            {
                WP_LOG_ERROR( "No mesh loader" );
            }

            if( meshLoader )
            {
                auto actor = meshLoader->loadActor( filePath );
                if( actor )
                {
                    auto data = actor->toData();
                    prefab->setData( data );

                    if( sceneManager )
                    {
                        sceneManager->destroyActor( actor );
                    }

                    return prefab;
                }
            }
        }
        else if( ext == ".hda" )
        {
        }
        else if( ext == ".fbmeshbin" )
        {
        }
        else if( ext == ".prefab" )
        {
            auto dataStr = fileSystem->readAllText( filePath );
            auto data = parsePrefabData( dataStr, getDataFormat() );

            prefab->setData( data );
            return prefab;
        }

        return nullptr;
    }

    SmartPtr<IResource> GamePrefabManager::create( const String &uuid )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto prefab = factoryManager->make_ptr<GamePrefab>();

        if( auto handle = prefab->getHandle() )
        {
            handle->setUUID( uuid );
        }

        m_prefabs.emplace_back( prefab );
        return prefab;
    }

    SmartPtr<IResource> GamePrefabManager::create( const String &uuid, const String &name )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto prefab = factoryManager->make_ptr<GamePrefab>();
        prefab->setName( name );

        if( auto handle = prefab->getHandle() )
        {
            handle->setUUID( uuid );
        }

        m_prefabs.emplace_back( prefab );
        return prefab;
    }

    void GamePrefabManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
        resource->saveToFile( filePath );
    }

    SmartPtr<IResource> GamePrefabManager::loadFromFile( const String &filePath )
    {
        auto uuid = StringUtil::getUUID();

        auto prefabResource = create( uuid );
        auto prefab = workphone::static_pointer_cast<IGamePrefab>( prefabResource );

        return prefab;
    }

    SmartPtr<IResource> GamePrefabManager::loadResource( const String &name )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto sceneManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( sceneManager );

            auto uuid = StringUtil::getUUID();

            auto prefabResource = create( uuid );
            auto prefab = workphone::static_pointer_cast<IGamePrefab>( prefabResource );

            auto ext = Path::getFileExtension( name );
            ext = StringUtil::make_lower( ext );

            auto prefabFilePath = ApplicationUtil::getPrefabPath( name );
            if( Path::isExistingFile( prefabFilePath ) )
            {
                auto jsonStr = fileSystem->readAllText( prefabFilePath );
                auto data = parsePrefabData( jsonStr, getDataFormat() );

                prefab->setData( data );

                return prefab;
            }

            if( ApplicationUtil::isSupportedMesh( name ) )
            {
                auto meshLoader = applicationManager->getMeshLoader();
                if( !meshLoader )
                {
                    WP_LOG_ERROR( "No mesh loader" );
                }

                if( meshLoader )
                {
                    auto actor = meshLoader->loadActor( name );
                    if( actor )
                    {
                        auto data = actor->toData();
                        prefab->setData( data );

                        if( sceneManager )
                        {
                            sceneManager->destroyActor( actor );
                        }

                        return prefab;
                    }
                }
            }
            else if( ext == ".hda" || ext == ".HDA" )
            {
            }
            else if( ext == ".fbmeshbin" || ext == ".FBMESHBIN" )
            {
            }
            else if( ext == ".prefab" )
            {
                auto jsonStr = fileSystem->readAllText( name );
                auto data = parsePrefabData( jsonStr, getDataFormat() );

                prefab->setData( data );
                return prefab;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void GamePrefabManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_prefabs.reserve( 1024 );
        setLoadingState( LoadingState::Loaded );
    }

    void GamePrefabManager::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        for( auto prefab : m_prefabs )
        {
            prefab->unload( nullptr );
        }

        m_prefabs.clear();

        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<IResource> GamePrefabManager::getByName( const String &name )
    {
        for( auto prefab : m_prefabs )
        {
            if( prefab->getName() == name )
            {
                return prefab;
            }
        }

        return nullptr;
    }

    SmartPtr<IResource> GamePrefabManager::getById( const String &uuid )
    {
        for( auto prefab : m_prefabs )
        {
            auto handle = prefab->getHandle();
            if( handle->getUUIDAsString() == uuid )
            {
                return prefab;
            }
        }

        return nullptr;
    }

    void GamePrefabManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    void GamePrefabManager::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->lock();
        }
    }

    bool GamePrefabManager::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            return gameManager->try_lock();
        }

        return false;
    }

    void GamePrefabManager::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->unlock();
        }
    }

    Pair<SmartPtr<IResource>, bool> GamePrefabManager::createOrRetrieve( const String &uuid,
                                                                         const String &path,
                                                                         const String &type )
    {
        if( auto prefabResource = getById( uuid ) )
        {
            return Pair<SmartPtr<IResource>, bool>( prefabResource, false );
        }

        auto prefab = create( uuid, path );
        WP_ASSERT( prefab );

        return Pair<SmartPtr<IResource>, bool>( prefab, true );
    }

    Pair<SmartPtr<IResource>, bool> GamePrefabManager::createOrRetrieve( const String &path )
    {
        if( auto prefabResource = getByName( path ) )
        {
            return Pair<SmartPtr<IResource>, bool>( prefabResource, false );
        }

        auto prefab = create( path, path );
        WP_ASSERT( prefab );

        return Pair<SmartPtr<IResource>, bool>( prefab, true );
    }

    void GamePrefabManager::savePrefab( const String &filePath, SmartPtr<IGameActor> prefab )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
            WP_ASSERT( prefab );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            prefab->setFlag( IGameActor::ActorFlagDontSave, false, true );
            prefab->setFlag( IGameActor::ActorFlagPrefab, true, true );

            const auto fmt = getDataFormat();

            auto data = prefab->toData();
            auto dataStr = DataUtil::toString( data.get(), true, fmt );

            fileSystem->writeAllText( filePath, dataStr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GamePrefabManager::destroyAll()
    {
        m_prefabs.clear();
    }

    void GamePrefabManager::destroyResource( SmartPtr<IResource> resource )
    {
        m_prefabs.erase( std::remove_if( m_prefabs.begin(), m_prefabs.end(),
                                         [resource]( const SmartPtr<IResource> &prefab ) {
                                             return prefab == resource;
                                         } ),
                         m_prefabs.end() );
    }

    SmartPtr<IResource> GamePrefabManager::cloneResource( const String &name,
                                                          const String &clonedResourceName )
    {
        auto clone = workphone::make_ptr<GamePrefab>();

        auto resource = getByName( name );
        if( !resource )
        {
            return nullptr;
        }

        auto data = resource->toData();
        clone->fromData( data );
        return clone;
    }

    SmartPtr<IResource> GamePrefabManager::cloneResource( SmartPtr<IResource> resource,
                                                          const String &clonedResourceName )
    {
        auto clone = workphone::make_ptr<GamePrefab>();

        auto data = resource->toData();
        clone->fromData( data );

        return clone;
    }

    void GamePrefabManager::setDataFormat( DataFormat dataFormat )
    {
        m_dataFormat = dataFormat;
    }

    DataFormat GamePrefabManager::getDataFormat() const
    {
        return m_dataFormat;
    }

    void GamePrefabManager::setStateContext( SmartPtr<IStateContext> stateContext )
    {
    }

    SmartPtr<IStateContext> GamePrefabManager::getStateContext() const
    {
        return nullptr;
    }

    IStateContext *GamePrefabManager::getStateContextPtr() const
    {
        return nullptr;
    }

    bool GamePrefabManager::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    bool GamePrefabManager::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

}  // namespace workphone::scene
