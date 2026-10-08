#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/Directors/LightingDirector.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskLock.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/Jobs/CameraManagerReset.hpp>
#include <Workphone/Jobs/SceneClearJob.hpp>
#include <Workphone/Jobs/SceneLoadJob.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Thread/TryLockGuard.hpp>
#include <Workphone/ApplicationUtil.hpp>

namespace workphone::scene
{

    class SpatialPartitioner
    {
    public:
        virtual ~SpatialPartitioner() = default;
        virtual void addActor( SmartPtr<IGameActor> actor ) = 0;
        virtual void removeActor( SmartPtr<IGameActor> actor ) = 0;
        virtual void updateActor( SmartPtr<IGameActor> actor ) = 0;
        virtual void queryActors( const Vector3F &position, real_Num radius,
                                  Array<SmartPtr<IGameActor>> &outActors ) = 0;
        virtual void clear() = 0;
        virtual void update( const Vector3F &referencePoint, float deltaTime,
                             const GameScene::SpatialPartitioningConfig &config ) = 0;
    };

    class UniformGridPartitioner : public SpatialPartitioner
    {
    public:
        UniformGridPartitioner( real_Num cellSize = 10.0f ) : m_cellSize( cellSize )
        {
        }
        void addActor( SmartPtr<IGameActor> actor ) override
        {
            if( !actor )
                return;
            Vector3 pos = actor->getWorldTransform().getPosition();
            int x = static_cast<int>( std::floor( pos.x / m_cellSize ) );
            int y = static_cast<int>( std::floor( pos.y / m_cellSize ) );
            int z = static_cast<int>( std::floor( pos.z / m_cellSize ) );
            m_grid[{ x, y, z }].push_back( actor );
        }
        void removeActor( SmartPtr<IGameActor> actor ) override
        {
            if( !actor )
                return;
            for( auto &pair : m_grid )
            {
                auto &actors = pair.second;
                actors.erase( std::remove( actors.begin(), actors.end(), actor ), actors.end() );
            }
        }
        void updateActor( SmartPtr<IGameActor> actor ) override
        {
            removeActor( actor );
            addActor( actor );
        }
        void queryActors( const Vector3F &position, real_Num radius,
                          Array<SmartPtr<IGameActor>> &outActors ) override
        {
            int minX = static_cast<int>( std::floor( ( position.x - radius ) / m_cellSize ) );
            int maxX = static_cast<int>( std::floor( ( position.x + radius ) / m_cellSize ) );
            int minY = static_cast<int>( std::floor( ( position.y - radius ) / m_cellSize ) );
            int maxY = static_cast<int>( std::floor( ( position.y + radius ) / m_cellSize ) );
            int minZ = static_cast<int>( std::floor( ( position.z - radius ) / m_cellSize ) );
            int maxZ = static_cast<int>( std::floor( ( position.z + radius ) / m_cellSize ) );
            for( int x = minX; x <= maxX; ++x )
            {
                for( int y = minY; y <= maxY; ++y )
                {
                    for( int z = minZ; z <= maxZ; ++z )
                    {
                        auto it = m_grid.find( { x, y, z } );
                        if( it != m_grid.end() )
                        {
                            for( auto &actor : it->second )
                                outActors.push_back( actor );
                        }
                    }
                }
            }
        }

        void clear() override
        {
            m_grid.clear();
            m_cellUpdates.clear();
        }

        void update( const Vector3F &referencePoint, float deltaTime,
                     const GameScene::SpatialPartitioningConfig &config ) override
        {
            for( auto &[key, actors] : m_grid )
            {
                Vector3 cellCenter( ( key.x + 0.5f ) * m_cellSize, ( key.y + 0.5f ) * m_cellSize,
                                    ( key.z + 0.5f ) * m_cellSize );
                float distSq = ( cellCenter - referencePoint ).lengthSquared();
                float targetRate = 0.0f;
                if( distSq < config.nearDistance * config.nearDistance )
                    targetRate = config.nearUpdateRate;
                else if( distSq < config.midDistance * config.midDistance )
                    targetRate = config.midUpdateRate;
                else if( distSq < config.farDistance * config.farDistance )
                    targetRate = config.farUpdateRate;
                else if( distSq < config.sleepDistance * config.sleepDistance )
                    targetRate = config.farUpdateRate * 0.1f;
                auto &accumulator = m_cellUpdates[key];
                accumulator += deltaTime;
                if( targetRate > 0 && accumulator >= ( 1.0f / targetRate ) )
                {
                    accumulator = 0.0f;
                }
            }
        }

    private:
        struct GridKey
        {
            int x, y, z;
            bool operator<( const GridKey &other ) const
            {
                if( x != other.x )
                    return x < other.x;
                if( y != other.y )
                    return y < other.y;
                return z < other.z;
            }
            bool operator==( const GridKey &other ) const
            {
                return x == other.x && y == other.y && z == other.z;
            }
        };
        real_Num m_cellSize;
        std::map<GridKey, std::vector<SmartPtr<IGameActor>>> m_grid;
        std::map<GridKey, float> m_cellUpdates;
    };

    class OctreePartitioner : public SpatialPartitioner
    {
    public:
        OctreePartitioner() : m_root( std::make_unique<OctreeNode>( Vector3F( 0, 0, 0 ), 1000.0f ) )
        {
        }
        void addActor( SmartPtr<IGameActor> actor ) override
        {
            if( actor )
                m_root->insert( actor );
        }
        void removeActor( SmartPtr<IGameActor> actor ) override
        {
            if( actor )
                m_root->remove( actor );
        }
        void updateActor( SmartPtr<IGameActor> actor ) override
        {
            removeActor( actor );
            addActor( actor );
        }
        void queryActors( const Vector3F &position, real_Num radius,
                          Array<SmartPtr<IGameActor>> &outActors ) override
        {
            m_root->query( position, radius, outActors );
        }
        void clear() override
        {
            m_root = std::make_unique<OctreeNode>( Vector3F( 0, 0, 0 ), 1000.0f );
        }
        void update( const Vector3F &referencePoint, float deltaTime,
                     const GameScene::SpatialPartitioningConfig &config ) override
        {
            m_root->update( referencePoint, deltaTime, config );
        }

    private:
        struct OctreeNode
        {
            Vector3F center;
            real_Num halfSize;
            std::vector<SmartPtr<IGameActor>> actors;
            std::unique_ptr<OctreeNode> children[8];
            bool isLeaf = true;
            float updateAccumulator = 0.0f;

            OctreeNode( Vector3F c, real_Num s ) : center( c ), halfSize( s )
            {
            }

            void insert( SmartPtr<IGameActor> actor )
            {
                if( isLeaf && actors.size() < 8 )
                {
                    actors.push_back( actor );
                    return;
                }
                if( isLeaf )
                    subdivide();
                children[getChildIndex( actor->getWorldTransform().getPosition() )]->insert( actor );
            }
            void remove( SmartPtr<IGameActor> actor )
            {
                auto it = std::remove( actors.begin(), actors.end(), actor );
                if( it != actors.end() )
                    actors.erase( it, actors.end() );
                else if( !isLeaf )
                    children[getChildIndex( actor->getWorldTransform().getPosition() )]->remove( actor );
            }
            void query( const Vector3F &pos, real_Num radius, Array<SmartPtr<IGameActor>> &out )
            {
                if( !intersects( pos, radius ) )
                    return;
                for( auto &actor : actors )
                    out.push_back( actor );
                if( !isLeaf )
                {
                    for( int i = 0; i < 8; ++i )
                        children[i]->query( pos, radius, out );
                }
            }
            void update( const Vector3F &referencePoint, float deltaTime,
                         const GameScene::SpatialPartitioningConfig &config )
            {
                float distSq = ( center - referencePoint ).lengthSquared();
                float targetRate = 0.0f;
                if( distSq < config.nearDistance * config.nearDistance )
                    targetRate = config.nearUpdateRate;
                else if( distSq < config.midDistance * config.midDistance )
                    targetRate = config.midUpdateRate;
                else if( distSq < config.farDistance * config.farDistance )
                    targetRate = config.farUpdateRate;
                else if( distSq < config.sleepDistance * config.sleepDistance )
                    targetRate = config.farUpdateRate * 0.1f;
                updateAccumulator += deltaTime;
                if( targetRate > 0 && updateAccumulator >= ( 1.0f / targetRate ) )
                {
                    updateAccumulator = 0.0f;
                }
                if( !isLeaf )
                {
                    for( int i = 0; i < 8; ++i )
                        children[i]->update( referencePoint, deltaTime, config );
                }
            }
            bool intersects( const Vector3F &pos, real_Num radius )
            {
                real_Num distSq = 0;
                if( pos.x < center.x - halfSize )
                    distSq += std::pow( pos.x - ( center.x - halfSize ), 2 );
                else if( pos.x > center.x + halfSize )
                    distSq += std::pow( pos.x - ( center.x + halfSize ), 2 );
                if( pos.y < center.y - halfSize )
                    distSq += std::pow( pos.y - ( center.y - halfSize ), 2 );
                else if( pos.y > center.y + halfSize )
                    distSq += std::pow( pos.y - ( center.y + halfSize ), 2 );
                if( pos.z < center.z - halfSize )
                    distSq += std::pow( pos.z - ( center.z - halfSize ), 2 );
                else if( pos.z > center.z + halfSize )
                    distSq += std::pow( pos.z - ( center.z + halfSize ), 2 );
                return distSq <= radius * radius;
            }
            int getChildIndex( const Vector3F &pos )
            {
                int index = 0;
                if( pos.x >= center.x )
                    index |= 1;
                if( pos.y >= center.y )
                    index |= 2;
                if( pos.z >= center.z )
                    index |= 4;
                return index;
            }
            void subdivide()
            {
                isLeaf = false;
                real_Num nextHalf = halfSize * 0.5f;
                for( int i = 0; i < 8; ++i )
                {
                    Vector3 nextCenter = center;
                    nextCenter.x += ( i & 1 ) ? nextHalf : -nextHalf;
                    nextCenter.y += ( i & 2 ) ? nextHalf : -nextHalf;
                    nextCenter.z += ( i & 4 ) ? nextHalf : -nextHalf;
                    children[i] = std::make_unique<OctreeNode>( nextCenter, nextHalf );
                }
                for( auto &actor : actors )
                    children[getChildIndex( actor->getWorldTransform().getPosition() )]->insert( actor );
                actors.clear();
            }
        };
        std::unique_ptr<OctreeNode> m_root;
    };

    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameScene, Resource<IGameScene> );

    GameScene::GameScene()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
        setEventTaskFlags( Thread::Application_Flag );
    }

    GameScene::~GameScene() = default;

    String GameScene::getLabel() const
    {
        return m_label.load();
    }

    void GameScene::setLabel( const String &name )
    {
        m_label = name;
    }

    void GameScene::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this, true );

            setLoadingState( LoadingState::Loading );

            m_updateObjects.resize( static_cast<s32>( Thread::UpdateState::Count ) );

            for( u32 x = 0; x < static_cast<s32>( Thread::UpdateState::Count ); ++x )
            {
                m_updateObjects[x].resize( static_cast<s32>( TaskId::Count ) );
            }

            for( u32 x = 0; x < static_cast<s32>( Thread::UpdateState::Count ); ++x )
            {
                for( u32 y = 0; y < static_cast<s32>( Thread::UpdateState::Count ); ++y )
                {
                    m_updateObjects[x][y].reserve( WP_MAX_ACTORS );
                }
            }

            WP_ASSERT( isValid() );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

#ifdef _DEBUG
    s32 GameScene::addReference()
    {
        return ISharedObject::addReference();
    }

    bool GameScene::removeReference()
    {
        return ISharedObject::removeReference();
    }
#endif

    void GameScene::loadScene( const String &path, bool async )
    {
        beginSceneLoad();

        if( isLoaded() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto jobQueue = applicationManager->getJobQueuePtr();
            WP_ASSERT( jobQueue );

            if( auto job = factoryManager->make_ptr<SceneLoadJob>() )
            {
                job->setScene( this );
                job->setFilePath( path );
                job->setCreateActorJobs( async );

                if( async )
                {
                    setSceneLoadingState( SceneLoadingState::Loading );
                    job->queuePrepare();
                }
                else
                {
                    job->execute();

                    applicationManager->triggerEvent( EventType::Loading, IEvent::sceneChanged,
                                                      Array<Parameter>(), this, this, nullptr );
                }
            }
        }
    }

    void GameScene::loadSceneDataStr( const String &data, bool async )
    {
        beginSceneLoad();

        if( isLoaded() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto jobQueue = applicationManager->getJobQueuePtr();
            WP_ASSERT( jobQueue );

            if( auto job = factoryManager->make_ptr<SceneLoadJob>() )
            {
                job->setScene( this );
                job->setDataStr( data );

                if( async )
                {
                    setSceneLoadingState( SceneLoadingState::Loading );
                    job->queuePrepare();
                }
                else
                {
                    job->execute();
                }
            }
        }
    }

    void GameScene::saveScene( const String &path )
    {
        // WP_ASSERT( !Path::isPathAbsolute( path ) );

        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto data = toData();
        WP_ASSERT( data );

        if( data )
        {
            setFilePath( path );

            auto name = Path::getFileNameWithoutExtension( path );
            setLabel( name );

            auto scenePath = StringUtil::cleanupPath( path );
            WP_ASSERT( !StringUtil::isNullOrEmpty( scenePath ) );

            auto sceneFileExt = Path::getFileExtension( scenePath );

            if( StringUtil::isNullOrEmpty( sceneFileExt ) )
            {
                scenePath += ApplicationUtil::builtinSceneExt;
            }

            if( sceneFileExt == ApplicationUtil::builtinBinarySceneExt )
            {
                auto properties = workphone::static_pointer_cast<Properties>( data );
                auto binaryData = PropertiesBinarySerializer::serialize( *properties );
                WP_ASSERT( !binaryData.empty() );
                fileSystem->writeAllBytes( scenePath, std::move( binaryData ) );
            }
            else
            {
                auto format = DataFormat::JSON;
                if( sceneFileExt == ApplicationUtil::builtinXmlSceneExt )
                {
                    format = DataFormat::XML;
                }
                else if( sceneFileExt == ApplicationUtil::builtinUsdSceneExt )
                {
                    format = DataFormat::USD;
                }

                auto dataStr = DataUtil::toString( data.get(), true, format );
                WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );
                fileSystem->writeAllText( scenePath, dataStr );
            }
        }
    }

    void GameScene::saveScene()
    {
        ScopedLock lock( this );

        auto filePath = getFilePath();
        WP_ASSERT( StringUtil::isNullOrEmpty( filePath ) == false );

        if( !StringUtil::isNullOrEmpty( filePath ) )
        {
            saveScene( filePath );
        }
    }

    void GameScene::reload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto cachePath = applicationManager->getCachePath();
        auto tempScenePath = cachePath + "/tmp.fbscene";

        WP_ASSERT( !Path::isPathAbsolute( tempScenePath ) );
        saveScene( tempScenePath );
        clear();
        loadScene( tempScenePath );
    }

    void GameScene::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            {
                ScopedLock sceneLock( this );
                beginSceneLoad();
                setLoadingState( LoadingState::Unloading );
            }
            // In-flight updates may need the scene mutex before releasing their load lock.
            ScopedLoadstateWait loadstateWait( this );
            ScopedLock sceneLock( this );

            SmartPtr<IGameActor> actor;
            while( m_playQueue.try_pop( actor ) )
            {
                actor = nullptr;
            }

            while( m_editQueue.try_pop( actor ) )
            {
                actor = nullptr;
            }

            m_playQueue.clear();
            m_editQueue.clear();

            for( u32 x = 0; x < static_cast<int>( Thread::UpdateState::Count ); ++x )
            {
                for( u32 y = 0; y < static_cast<int>( TaskId::Count ); ++y )
                {
                    m_updateObjects[x][y].clear();
                }
            }

            m_actors.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameScene::preUpdate()
    {
        ScopedLock sceneLock( this );
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( isLoaded() )
            {
                ScopedLoadLock loadLock( this );

                WP_DEBUG_TRACE;
                WP_ASSERT( isValid() );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );
                WP_ASSERT( applicationManager->isValid() );

                auto sceneManager = applicationManager->getGameManagerPtr();
                WP_ASSERT( sceneManager );

                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    if( !m_editQueue.empty() )
                    {
                        Array<SmartPtr<IGameActor>> actors;
                        actors.reserve( 256 );

                        SmartPtr<IGameActor> actor;
                        while( m_editQueue.try_pop( actor ) )
                        {
                            actors.push_back( actor );
                        }

                        for( auto &actor : actors )
                        {
                            if( actor && actor->isLoaded() && getState() == State::Edit )
                            {
                                actor->setState( IGameActor::State::Edit );
                            }
                        }
                    }

                    if( !m_playQueue.empty() )
                    {
                        Array<SmartPtr<IGameActor>> actors;
                        actors.reserve( 256 );

                        SmartPtr<IGameActor> actor;
                        while( m_playQueue.try_pop( actor ) )
                        {
                            actors.push_back( actor );
                        }

                        for( auto &actor : actors )
                        {
                            if( actor && actor->isLoaded() && getState() == State::Play )
                            {
                                actor->setState( IGameActor::State::Play );
                            }
                        }
                    }

                    auto &updateObjects = getRegisteredObjects( Thread::UpdateState::PreUpdate, task );

                    for( auto &actor : updateObjects )
                    {
                        if( actor )
                        {
                            actor->preUpdate();
                        }
                    }

                    WP_ASSERT( isValid() );
                }
            }
        }
        break;
        default:
        {
        }
        }
    }

    void GameScene::update()
    {
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( isLoaded() )
            {
                TryLockGuard lock( this );
                if( lock.locked() && isLoaded() )
                {
                    ScopedLoadLock loadLock( this );
                    if( m_partitioner )
                    {
                        Vector3F referencePos( 0, 0, 0 );
                        float deltaTime = 0.016f;
                        m_partitioner->update( referencePos, deltaTime, m_partitioningConfig.load() );
                    }

                    WP_DEBUG_TRACE;
                    WP_ASSERT( isValid() );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto &updateObjects = getRegisteredObjects( Thread::UpdateState::Update, task );

                    for( auto actor : updateObjects )
                    {
                        if( actor )
                        {
                            actor->update();
                        }
                    }

                    WP_ASSERT( isValid() );
                }
            }
        }
        break;
        default:
        {
        }
        }
    }

    void GameScene::postUpdate()
    {
        ScopedLock sceneLock( this );
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( isLoaded() )
            {
                ScopedLoadLock loadLock( this );

                WP_DEBUG_TRACE;
                WP_ASSERT( isValid() );

                TryLockGuard lock( this );
                if( lock.locked() )
                {
                    auto &updateObjects = getRegisteredObjects( Thread::UpdateState::PostUpdate, task );

                    for( auto &actor : updateObjects )
                    {
                        if( actor )
                        {
                            actor->postUpdate();
                        }
                    }
                }

                WP_ASSERT( isValid() );
            }
        }
        break;
        default:
        {
        }
        }
    }

    void GameScene::addActor( SmartPtr<IGameActor> actor )
    {
        ScopedLock sceneLock( this );
        try
        {
            WP_ASSERT( isValid() );

            if( actor )
            {
                auto it = std::find( m_actors.begin(), m_actors.end(), actor );
                if( it == m_actors.end() )
                {
                    auto handle = actor->getHandle();
                    WP_ASSERT( handle );

#if _DEBUG

                    auto id = handle->getId();
                    //WP_ASSERT( id != 0 );

                    WP_ASSERT( StringUtil::isNullOrEmpty( handle->getUUIDAsString() ) == false );
#endif

                    actor->setScene( this );

                    if( m_actors.size() >= m_actors.capacity() )
                        throw std::length_error( "Scene actor capacity exhausted" );
                    m_actors.push_back( actor );
                    // A cancelled load leaves an empty scene available for manual editing.
                    // Publishing a new actor makes that scene ready for state updates again.
                    if( getSceneLoadingState() == SceneLoadingState::Cancelled )
                        m_sceneLoadingState = SceneLoadingState::Loaded;
                    if( m_partitioner )
                        m_partitioner->addActor( actor );

                    // Scene ownership includes update ownership.  In particular, loaded
                    // rigidbodies rely on their actor update to copy the simulated transform
                    // back into the scene graph.  Camera actors happened to receive an extra
                    // update from CameraManager, which masked missing registration for them.
                    registerAllUpdates( actor );

                    auto actorIndex = handle->getInstanceId();
                    WP_ASSERT( actorIndex != std::numeric_limits<hash64>::max() );

                    actor->setFlag( IGameActor::ActorFlagInScene, true );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    applicationManager->triggerEvent( EventType::Scene, IEvent::addActor,
                                                      Array<Parameter>(), this, actor, nullptr );
                }
            }
            else
            {
                static const auto message = String( "Scene::addActor null" );
                WP_LOG_ERROR( message );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            throw;
        }
    }

    void GameScene::removeActor( SmartPtr<IGameActor> actor )
    {
        ScopedLock sceneLock( this );
        try
        {
            WP_ASSERT( isValid() );

            if( actor )
            {
                if( m_partitioner )
                    m_partitioner->removeActor( actor );

                unregisterAll( actor );
                m_actors.erase( std::remove( m_actors.begin(), m_actors.end(), actor ), m_actors.end() );
                actor->setFlag( IGameActor::ActorFlagInScene, false );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( applicationManager->isRunning() )
                {
                    if( !applicationManager->getQuit() )
                    {
                        applicationManager->triggerEvent( EventType::Scene, IEvent::removeActor,
                                                          Array<Parameter>(), this, actor, nullptr );
                    }
                }
            }
            else
            {
                static const auto message = String( "CScene::removeActor null" );
                WP_LOG_ERROR( message );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameScene::removeAllActors()
    {
        ScopedLock sceneLock( this );
        m_actors.clear();
    }

    SmartPtr<IGameActor> GameScene::findActorByName( const String &name ) const
    {
        WP_ASSERT( getLoadingState() == LoadingState::Loaded );
        WP_ASSERT( isValid() );

        auto actors = getActors();
        for( auto actor : actors )
        {
            if( actor->getName() == name )
            {
                return actor;
            }
        }

        return nullptr;
    }

    SmartPtr<IGameActor> GameScene::findActorById( s32 id ) const
    {
        WP_ASSERT( getLoadingState() == LoadingState::Loaded );

        WP_ASSERT( isValid() );
        WP_ASSERT( id != 0 );

        auto actors = getActors();
        for( auto actor : actors )
        {
            auto handle = actor->getHandle();
            if( handle->getId() == id )
            {
                return actor;
            }
        }

        return nullptr;
    }

    Array<SmartPtr<IGameActor>> GameScene::getActors() const
    {
        auto actors = m_actors.readLocked();
        return { actors.begin(), actors.end() };
    }

    void GameScene::setActors( const Array<SmartPtr<IGameActor>> &actors )
    {
        ScopedLock sceneLock( this );
        m_actors = { actors.begin(), actors.end() };
    }

    void GameScene::clear( bool clearNow /*= true*/ )
    {
        ScopedLock lock( this );
        beginSceneLoad();
        setSceneLoadingState( SceneLoadingState::Cancelled );
        m_playQueue.clear();
        m_editQueue.clear();

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto jobQueue = applicationManager->getJobQueuePtr();
        auto job = factoryManager->make_ptr<SceneClearJob>();
        job->setScene( this );

        auto actors = getActors();
        job->setActors( actors );

        if( clearNow )
        {
            job->execute();
        }
        else
        {
            // Scene clearing is deferred, but it must not depend on the application task's
            // frame-rate gate: callers may explicitly drain the main job queue without enough
            // wall-clock time elapsing for another application tick.
            job->setPrimary( true );
            jobQueue->addJob( job );
        }
    }

    void GameScene::destroyOnLoad()
    {
        WP_ASSERT( isValid() );

        auto actors = getActors();

        Array<SmartPtr<IGameActor>> removeQueue;
        removeQueue.reserve( actors.size() );

        for( auto actor : actors )
        {
            if( actor )
            {
                if( actor->getPerpetual() )
                {
                    actor->unload( nullptr );
                    removeQueue.push_back( actor );
                }
            }
        }

        for( auto actor : removeQueue )
        {
            removeActor( actor );
        }
    }

    void GameScene::registerAllUpdates( SmartPtr<IGameActor> actor )
    {
        ScopedLock sceneLock( this );
        try
        {
            WP_ASSERT( isValid() );
            WP_ASSERT( getLoadingState() == LoadingState::Loaded );

            for( u32 x = 0; x < static_cast<u32>( TaskId::Count ); ++x )
            {
                for( u32 y = 0; y < static_cast<u32>( Thread::UpdateState::Count ); ++y )
                {
                    registerUpdate( static_cast<TaskId>( x ), static_cast<Thread::UpdateState>( y ),
                                    actor );
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GameScene::registerUpdates( TaskId taskId, SmartPtr<IGameActor> actor )
    {
        ScopedLock sceneLock( this );
        registerUpdate( taskId, Thread::UpdateState::PreUpdate, actor );
        registerUpdate( taskId, Thread::UpdateState::Update, actor );
        registerUpdate( taskId, Thread::UpdateState::PostUpdate, actor );
    }

    void GameScene::registerUpdate( TaskId taskId, Thread::UpdateState updateType,
                                    SmartPtr<IGameActor> object )
    {
        ScopedLock sceneLock( this );
        WP_ASSERT( isValid() );
        WP_ASSERT( getLoadingState() == LoadingState::Loaded );

        auto &updateObjects = getRegisteredObjects( updateType, taskId );

        auto it = std::find( updateObjects.begin(), updateObjects.end(), object );

        if( it == updateObjects.end() )
        {
            updateObjects.push_back( object );
            // Only this list changed. Sorting every task/phase list for each
            // registration makes procedural scenes with many actors costly.
            std::sort( updateObjects.begin(), updateObjects.end() );
        }
    }

    void GameScene::sortObjects()
    {
        ScopedLock sceneLock( this );
        WP_ASSERT( isValid() );

        for( u32 x = 0; x < static_cast<u32>( Thread::UpdateState::Count ); ++x )
        {
            for( u32 y = 0; y < static_cast<u32>( TaskId::Count ); ++y )
            {
                auto &updateObjects = getRegisteredObjects( static_cast<Thread::UpdateState>( x ),
                                                            static_cast<TaskId>( y ) );

                std::sort( updateObjects.begin(), updateObjects.end() );
            }
        }
    }

    Parameter GameScene::handleEvent( EventType eventType, hash_type eventValue,
                                      const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                      SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( Thread::getTaskFlag( Thread::Application_Flag ) )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto target = event ? event->getTarget() : nullptr;
            if( target && target->isDerived<IComponent>() )
            {
                auto component = workphone::static_pointer_cast<IComponent>( target );
                if( component->isLoaded() )
                {
                    component->handleEvent( eventType, eventValue, arguments, sender, object, event );
                }
            }
            else if( target && target->isDerived<IGameActor>() )
            {
                auto actor = workphone::static_pointer_cast<IGameActor>( target );
                if( actor->getLoadingState() != LoadingState::Unloaded )
                {
                    actor->handleEvent( eventType, eventValue, arguments, sender, object, event );
                }
            }
            else if( eventValue == IEvent::componentLoaded && object && object->isDerived<IComponent>() )
            {
                // Sibling components may depend on the newly loaded component, but components on
                // unrelated actors do not need this notification.
                auto component = workphone::static_pointer_cast<IComponent>( object );
                if( auto actor = component->getActor() )
                {
                    actor->handleEvent( eventType, eventValue, arguments, sender, object, event );
                }
            }
            else if( !target )
            {
                auto actors = getActors();
                for( auto &actor : actors )
                {
                    if( actor )
                    {
                        actor->handleEvent( eventType, eventValue, arguments, sender, object, event );
                    }
                }
            }

            if( eventValue == IEvent::actorLoaded )
            {
                if( applicationManager->isEditor() )
                {
                    if( object )
                    {
                        if( object->isDerived<IGameActor>() )
                        {
                            auto actor = workphone::static_pointer_cast<IGameActor>( object );

                            if( applicationManager->isPlaying() )
                            {
                                m_playQueue.push( actor );
                            }
                            else
                            {
                                m_editQueue.push( actor );
                            }
                        }
                    }
                }
            }

            if( eventValue == IEvent::componentLoaded )
            {
                WP_ASSERT( object );

                if( object->isDerived<IComponent>() )
                {
                    auto component = workphone::static_pointer_cast<IComponent>( object );

                    if( arguments.size() >= 1 )
                    {
                        auto data = arguments[0].object;
                        if( data->isDerived<Properties>() )
                        {
                            component->fromData( data );
                        }
                    }
                }
            }

            if( eventValue == scene::IGameManager::sceneLoadedHash )
            {
                if( applicationManager->isEditor() )
                {
                    if( applicationManager->isPlaying() )
                    {
                        setState( IGameScene::State::Play );
                    }
                    else
                    {
                        setState( IGameScene::State::Edit );
                    }
                }
                else
                {
                    setState( IGameScene::State::Play );
                }
            }
        }

        return {};
    }

    void GameScene::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->lock();
        }
    }

    void GameScene::lock_shared()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->lock_shared();
        }
    }

    bool GameScene::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            return gameManager->try_lock();
        }

        return false;
    }

    void GameScene::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->unlock();
        }
    }

    void GameScene::unlock_shared()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManager->getGameManagerPtr();
        if( gameManager )
        {
            gameManager->unlock_shared();
        }
    }

    void GameScene::unregisterUpdate( TaskId taskId, Thread::UpdateState updateType,
                                      SmartPtr<IGameActor> object )
    {
        ScopedLock sceneLock( this );
        WP_ASSERT( isValid() );

        auto &updateObjects = getRegisteredObjects( updateType, taskId );
        updateObjects.erase( std::remove( updateObjects.begin(), updateObjects.end(), object ),
                             updateObjects.end() );

        sortObjects();
    }

    void GameScene::unregisterAll( SmartPtr<IGameActor> object )
    {
        ScopedLock sceneLock( this );
        WP_ASSERT( isValid() );

        for( u32 x = 0; x < static_cast<int>( Thread::UpdateState::Count ); ++x )
        {
            for( u32 y = 0; y < static_cast<int>( TaskId::Count ); ++y )
            {
                auto &updateObjects = getRegisteredObjects( static_cast<Thread::UpdateState>( x ),
                                                            static_cast<TaskId>( y ) );
                updateObjects.erase( std::remove( updateObjects.begin(), updateObjects.end(), object ),
                                     updateObjects.end() );
            }
        }

        sortObjects();
    }

    ConcurrentArray<SmartPtr<IGameActor>> &GameScene::getRegisteredObjects(
        Thread::UpdateState updateState, TaskId task )
    {
        WP_ASSERT( static_cast<size_t>( updateState ) < m_updateObjects.size() );
        WP_ASSERT( static_cast<size_t>( task ) <
                   m_updateObjects[static_cast<s32>( updateState )].size() );

        return m_updateObjects[static_cast<int>( updateState )][static_cast<int>( task )];
    }

    const ConcurrentArray<SmartPtr<IGameActor>> &GameScene::getRegisteredObjects(
        Thread::UpdateState updateState, TaskId task ) const
    {
        WP_ASSERT( static_cast<size_t>( updateState ) < m_updateObjects.size() );
        WP_ASSERT( static_cast<size_t>( task ) <
                   m_updateObjects[static_cast<s32>( updateState )].size() );

        return m_updateObjects[static_cast<int>( updateState )][static_cast<int>( task )];
    }

    void GameScene::setRegisteredObjects( Thread::UpdateState updateState, TaskId task,
                                          const ConcurrentArray<SmartPtr<IGameActor>> &objects )
    {
        WP_ASSERT( static_cast<s32>( updateState ) < static_cast<s32>( m_updateObjects.size() ) );
        WP_ASSERT( static_cast<s32>( task ) <
                   static_cast<s32>( m_updateObjects[static_cast<s32>( updateState )].size() ) );

        m_updateObjects[static_cast<s32>( updateState )][static_cast<s32>( task )] = objects;
    }

    bool GameScene::isValid() const
    {
        return true;
    }

    SmartPtr<ISharedObject> GameScene::toData() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto properties = factoryManager->make_ptr<Properties>();

        auto lightingDirector = getLightingDirector();
        properties->setPropertyAsType( ApplicationUtil::lightingStr, lightingDirector );

        for( auto &actor : m_actors )
        {
            if( actor && !actor->getFlag( IGameActor::ActorFlagIsEditor ) )
            {
                auto pActorData = actor->toData();
                auto actorProperties = workphone::static_pointer_cast<Properties>( pActorData );
                actorProperties->setName( ApplicationUtil::actorsStr );
                properties->addChild( actorProperties );
            }
        }

        auto cameraManager = applicationManager->getCameraManager();
        if( cameraManager )
        {
            auto editorCamera = cameraManager->getEditorCamera();
            if( editorCamera )
            {
                if( auto cameraData = editorCamera->toData() )
                {
                    if( cameraData->isDerived<Properties>() )
                    {
                        auto cameraProperties = workphone::static_pointer_cast<Properties>( cameraData );
                        cameraProperties->setName( "editorCamera" );

                        properties->addChild( cameraData );
                    }
                }
            }
        }

        return properties;
    }

    void GameScene::fromData( SmartPtr<ISharedObject> data )
    {
        auto properties = workphone::static_pointer_cast<Properties>( data );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto lightingDirector = getLightingDirector();
        properties->getPropertyAsType( ApplicationUtil::lightingStr, lightingDirector );

        //graphicsSceneManager->setAmbientLight( ColourF::White * 0.5f );
        //graphicsSceneManager->setEnableShadows( true );

        auto actorsData = properties->getChildrenByName( ApplicationUtil::actorsStr );
        auto cameraManager = applicationManager->getCameraManager();
        auto editorCamera = cameraManager ? cameraManager->getEditorCamera() : nullptr;
        auto editorCameraData = properties->getChild( "editorCamera" );
        for( auto actorData : actorsData )
        {
            // Older scenes saved the editor camera among the game actors. Restore
            // its settings on the existing editor camera instead of creating a
            // second camera that will become active in playmode.
            if( GameActorUtil::isEditorCameraData( editorCamera, actorData ) )
            {
                if( !editorCameraData )
                {
                    editorCameraData = actorData;
                }
                continue;
            }
            auto actor = prefabManager->loadActor( actorData, nullptr );
            addActor( actor );
        }

        if( cameraManager )
        {
            if( auto cameraData = editorCameraData )
            {
                if( editorCamera && cameraData->isDerived<Properties>() )
                {
                    auto cameraProperties = workphone::static_pointer_cast<Properties>( cameraData );
                    GameActorUtil::restoreEditorCameraData( editorCamera, cameraProperties );
                }
            }
        }
    }

    void GameScene::setState( State state )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto threadPool = applicationManager->getThreadPoolPtr();
        auto taskManager = applicationManager->getTaskManagerPtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        // A queued transition can outlive a rapid play/stop request. Discard
        // superseded requests before queuing the actors for the latest mode.
        m_playQueue.clear();
        m_editQueue.clear();
        m_state = state;

        switch( m_state )
        {
        case State::None:
        {
        }
        break;
        case State::Edit:
        {
            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                auto actors = m_actors.snapshot();
                for( auto &actor : actors )
                {
                    m_editQueue.push( actor );
                }

                auto applicationTask = taskManager->getTask( TaskId::Application );
                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                applicationTask->addJob( cameraManagerResetJob );
            }
            else if( Thread::getTaskFlag( Thread::Application_Flag ) )
            {
                for( auto &actor : m_actors )
                {
                    actor->setState( IGameActor::State::Edit );
                }

                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                cameraManagerResetJob->execute();
            }
            else
            {
                for( auto &actor : m_actors )
                {
                    actor->setState( IGameActor::State::Edit );
                }

                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                cameraManagerResetJob->execute();
            }
        }
        break;
        case State::Play:
        {
            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                for( auto &actor : m_actors )
                {
                    m_playQueue.push( actor );
                }

                auto applicationTask = taskManager->getTask( TaskId::Application );
                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                applicationTask->addJob( cameraManagerResetJob );
            }
            else
            {
                for( auto &actor : m_actors )
                {
                    actor->setState( IGameActor::State::Play, true );
                }

                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                cameraManagerResetJob->execute();
            }
        }
        break;
        case State::Reset:
        {
        }
        break;
        default:
        {
        }
        break;
        }
    }

    GameScene::State GameScene::getState() const
    {
        return m_state;
    }

    void GameScene::setSceneLoadingState( SceneLoadingState state )
    {
        ScopedLock lock( this );

        const auto generation = getLoadGeneration();

        switch( state )
        {
        case SceneLoadingState::Loaded:
        {
            updateLighting();

            auto actors = getActors();
            for( auto actor : actors )
            {
                if( actor )
                {
                    actor->levelWasLoaded( this );
                }
            }
        }
        break;
        default:
        {
        }
        break;
        }
        // Publish completion after lighting and level callbacks have returned.
        if( state != SceneLoadingState::Loaded || generation == getLoadGeneration() )
            m_sceneLoadingState = state;
    }

    GameScene::SceneLoadingState GameScene::getSceneLoadingState() const
    {
        return m_sceneLoadingState;
    }

    SmartPtr<Properties> GameScene::getProperties() const
    {
        auto properties = Resource<IGameScene>::getProperties();

        auto lightingDirector = getLightingDirector();
        properties->setPropertyAsType( ApplicationUtil::lightingStr, lightingDirector );
        return properties;
    }

    void GameScene::setProperties( SmartPtr<Properties> properties )
    {
        Resource<IGameScene>::setProperties( properties );

        auto lightingDirector = getLightingDirector();
        properties->getPropertyAsType( ApplicationUtil::lightingStr, lightingDirector );

        setLightingDirector( lightingDirector );

        updateLighting();
    }

    void GameScene::updateLighting()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        if( !graphicsScene )
        {
            return;
        }

        if( m_lightingDirector )
        {
            graphicsScene->setAmbientLight( m_lightingDirector->getAmbientColour() );
            graphicsScene->setUpperHemisphere( m_lightingDirector->getUpperHemisphere() );
            graphicsScene->setLowerHemisphere( m_lightingDirector->getLowerHemisphere() );
        }
    }

    void GameScene::setLightingDirector( SmartPtr<LightingDirector> lightingDirector )
    {
        m_lightingDirector = lightingDirector;
    }

    SmartPtr<LightingDirector> GameScene::getLightingDirector() const
    {
        return m_lightingDirector;
    }

    StringPool<c8> *GameScene::getStringPool() const
    {
        return m_stringPool;
    }

    void GameScene::setStringPool( StringPool<c8> *pool )
    {
        m_stringPool = pool;
    }

    void GameScene::setSpatialPartitioningMethod( SpatialPartitioningMethod method )
    {
        ScopedLock lock( this );
        m_partitioningMethod = method;

        switch( method )
        {
        case SpatialPartitioningMethod::UniformGrid:
            m_partitioner = std::make_unique<UniformGridPartitioner>();
            break;
        case SpatialPartitioningMethod::Octree:
            m_partitioner = std::make_unique<OctreePartitioner>();
            break;
        default:
            m_partitioner = nullptr;
            break;
        }

        if( m_partitioner )
        {
            for( auto actor : m_actors )
            {
                m_partitioner->addActor( actor );
            }
        }
    }

    IGameScene::SpatialPartitioningMethod GameScene::getSpatialPartitioningMethod() const
    {
        return m_partitioningMethod.load();
    }

    void GameScene::setSpatialPartitioningConfig( const SpatialPartitioningConfig &config )
    {
        ScopedLock lock( this );
        m_partitioningConfig = config;
    }

    IGameScene::SpatialPartitioningConfig GameScene::getSpatialPartitioningConfig() const
    {
        return m_partitioningConfig.load();
    }
}  // namespace workphone::scene
