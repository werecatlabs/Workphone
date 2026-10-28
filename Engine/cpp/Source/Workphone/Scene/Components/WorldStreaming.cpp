#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <mutex>

namespace workphone::scene
{
    namespace
    {
        SmartPtr<IGameActor> findObserver( const SmartPtr<IGameActor> &actor, const String &uuid )
        {
            if( actor->getHandle()->getUUIDAsString() == uuid )
            {
                return actor;
            }
            for( const auto &child : actor->getChildren() )
            {
                if( auto found = findObserver( child, uuid ) )
                {
                    return found;
                }
            }
            return nullptr;
        }

        std::mutex &streamerLifetimeMutex( const WorldStreaming *component )
        {
            // Inspector reads only hold this gate while copying the streamer's
            // published statistics. Keep it alive through late factory teardown.
            static auto *gates = new std::array<std::mutex, 64>();
            const auto index = ( reinterpret_cast<std::uintptr_t>( component ) >> 4 ) % gates->size();
            return ( *gates )[index];
        }

        std::shared_ptr<SceneWorldStreamer> replaceStreamer(
            const WorldStreaming *component, std::shared_ptr<SceneWorldStreamer> &slot,
            std::shared_ptr<SceneWorldStreamer> replacement = {} )
        {
            std::shared_ptr<SceneWorldStreamer> retired;
            {
                std::lock_guard<std::mutex> lock( streamerLifetimeMutex( component ) );
                retired = std::move( slot );
                slot = std::move( replacement );
            }
            // Closing a world can pause render or run actor/component callbacks.
            // Return ownership so all of that happens after releasing the gate.
            return retired;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, WorldStreaming, Component );
    
    WorldStreaming::WorldStreaming() = default;

    WorldStreaming::~WorldStreaming()
    {
        m_unloading = true;
        auto retired = replaceStreamer( this, m_streamer );
    }

    void WorldStreaming::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }
        m_unloading = false;
        Component::load( data );
        auto manager = core::IApplicationManager::instancePtr()->getGameManager();
        manager->registerComponentUpdate( TaskId::Primary, Thread::UpdateState::PreUpdate, this );
        manager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::PreUpdate, this );
        m_reopen = true;
        setLoadingState( LoadingState::Loaded );
    }

    void WorldStreaming::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded || m_unloading )
        {
            return;
        }
        m_unloading = true;
        if( auto app = core::IApplicationManager::instancePtr() )
        {
            if( auto manager = app->getGameManager() )
            {
                manager->unregisterAllComponent( this );
            }
        }
        auto retired = replaceStreamer( this, m_streamer );
        retired.reset();
        m_openPath.clear();
        m_observer = nullptr;
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void WorldStreaming::preUpdate()
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app )
        {
            return;
        }
        auto manager = app->getGameManager();
        if( !manager || Thread::getCurrentTask() != manager->getSceneTask() || !isEnabled() )
        {
            return;
        }
        if( m_updating || m_unloading )
        {
            return;
        }
        // Actor installation can invoke user callbacks which unload this
        // component or request another update. Retain both active objects until
        // this operation returns, and never publish a second streamer on reentry.
        auto keepAlive = SmartPtr<WorldStreaming>( this );

        struct Updating
        {
            bool &value;

            ~Updating()
            {
                value = false;
            }
        } updating{ m_updating };

        m_updating = true;
        auto actor = getActor();
        if( !actor || !actor->isEnabledInScene() || !actor->getScene() )
        {
            return;
        }
        auto active = m_streamer;
        try
        {
            if( m_reopen )
            {
                m_reopen = false;
                if( m_manifestPath.empty() )
                {
                    auto retired = replaceStreamer( this, m_streamer );
                    return;
                }
                const auto path = Path::isPathAbsolute( m_manifestPath )
                                      ? m_manifestPath
                                      : String( ( std::filesystem::u8path( app->getProjectPath().c_str() ) /
                                                  std::filesystem::u8path( m_manifestPath.c_str() ) )
                                                    .lexically_normal()
                                                    .generic_u8string()
                                                    .c_str() );
                String error;
                const auto admissionEpoch = sourceBudgetDenialCount();
                if( active && active->isOpen() && m_openPath == path )
                {
                    if( !active->open( path, actor->getScene(), m_options, m_instanceWorld ? actor : nullptr,
                                       error ) )
                    {
                        if( sourceBudgetDenialCount() != admissionEpoch ) throw std::bad_alloc();
                        m_error = error;
                        WP_LOG_ERROR( "WorldStreaming: " + error );
                        return;
                    }
                }
                else
                {
                    std::shared_ptr<SceneWorldStreamer> candidate;
                    try
                    {
                        candidate = std::allocate_shared<SceneWorldStreamer>(
                            SourceBudgetAllocator<SceneWorldStreamer>{} );
                    }
                    catch( const std::bad_alloc & )
                    {
                        throw;
                    }
                    catch( const std::exception &e )
                    {
                        m_error = e.what();
                        WP_LOG_ERROR( "WorldStreaming: " + m_error );
                        return;
                    }
                    if( !candidate->open( path, actor->getScene(), m_options,
                                          m_instanceWorld ? actor : nullptr, error ) )
                    {
                        if( sourceBudgetDenialCount() != admissionEpoch ) throw std::bad_alloc();
                        m_error = error;
                        WP_LOG_ERROR( "WorldStreaming: " + error );
                        return;
                    }
                    if( m_unloading )
                    {
                        return;
                    }
                    auto retired = replaceStreamer( this, m_streamer, std::move( candidate ) );
                }
                m_openPath = path;
            }
        }
        catch( const std::bad_alloc & )
        {
            m_reopen = true;
            m_error = "Out of memory";
            return;
        }
        // Replacement retirement must finish before the new world installs
        // identities in this scene. The local only protects reentrant open().
        active.reset();
        auto streamer = m_streamer;
        if( m_unloading || !streamer || !streamer->isOpen() )
        {
            return;
        }
        auto observer = m_observer.lock();
        if( !observer && m_previewEditorCamera && app->isEditor() && !app->isPlaying() )
        {
            if( auto cameras = app->getCameraManager() )
            {
                observer = cameras->getEditorCamera();
            }
        }
        if( !observer && m_followActiveCamera )
        {
            for( const auto &camera : actor->getScene()->getComponents<Camera>() )
            {
                if( camera && camera->isEnabled() && camera->isActive() )
                {
                    if( auto cameraActor = camera->getActor();
                        cameraActor && cameraActor->isEnabledInScene() )
                    {
                        observer = cameraActor;
                        break;
                    }
                }
            }
        }
        if( !observer )
        {
            observer = actor;
        }
        const auto p = observer->getWorldTransform().getPosition();
        String error;
        if( !streamer->update( { Vector3F( float( p.x ), float( p.y ), float( p.z ) ) }, error ) &&
            error != m_error && error != "World retirement pending" )
        {
            WP_LOG_ERROR( "WorldStreaming: " + error );
        }
        m_error = error;
    }

    SmartPtr<Properties> WorldStreaming::getProperties() const
    {
        auto p = Component::getProperties();
        p->setProperty( "World Manifest", m_manifestPath );
        p->setProperty( "Load Radius", m_options.loadRadius );
        p->setProperty( "Retain Radius", m_options.retainRadius );
        p->setProperty( "Enable HLOD", m_options.enableHlod );
        p->setProperty( "Proxy Load Radius", m_options.proxyLoadRadius );
        p->setProperty( "Proxy Retain Radius", m_options.proxyRetainRadius );
        p->setProperty( "Maximum Resident Actors", m_options.maximumResidentActors );
        p->setProperty( "Maximum Resident Cells", m_options.maximumResidentCells );
        p->setProperty( "Install Milliseconds", m_options.installMilliseconds );
        p->setProperty( "Preview Editor Camera", m_previewEditorCamera );
        p->setProperty( "Follow Active Camera", m_followActiveCamera );
        p->setProperty( "Instance World", m_instanceWorld );
        p->setPropertyAsType( "Observer", m_observer.lock() );
        p->setProperty( "Streaming Error", m_error, true );
        auto stats = statistics();
        p->setProperty( "Resident Actors", stats.residentActors, true );
        p->setProperty( "Resident Cells", stats.residentCells, true );
        p->setProperty( "Resident Proxy Cells", stats.residentProxyCells, true );
        p->setProperty( "Visible Proxy Cells", stats.visibleProxyCells, true );
        p->setProperty( "Representation Switches", stats.representationSwitches, true );
        p->setProperty( "HLOD Admission Failures", stats.hlodAdmissionFailures, true );
        p->setProperty( "Graphics Retry Requests", stats.graphicsRetryRequests, true );
        p->setProperty( "Pending Cells", stats.pendingCells, true );
        p->setProperty( "Resident Source Bytes", stats.residentSourceBytes, true );
        p->setProperty( "Released CPU Mesh Resources", stats.releasedMeshResources, true );
        p->setProperty( "Released Material Resources", stats.releasedMaterialResources, true );
        p->setProperty( "Released Texture Resources", stats.releasedTextureResources, true );
        p->setProperty( "Index Nodes Visited", stats.indexNodesVisited, true );
        p->setProperty( "Index Cells Tested", stats.indexCellsTested, true );
        return p;
    }

    void WorldStreaming::setProperties( SmartPtr<Properties> p )
    {
        Component::setProperties( p );
        const auto beforePath = m_manifestPath;
        const auto before = m_options;
        const auto beforeInstance = m_instanceWorld;
        p->getPropertyValue( "World Manifest", m_manifestPath );
        p->getPropertyValue( "Load Radius", m_options.loadRadius );
        p->getPropertyValue( "Retain Radius", m_options.retainRadius );
        p->getPropertyValue( "Enable HLOD", m_options.enableHlod );
        p->getPropertyValue( "Proxy Load Radius", m_options.proxyLoadRadius );
        p->getPropertyValue( "Proxy Retain Radius", m_options.proxyRetainRadius );
        p->getPropertyValue( "Maximum Resident Actors", m_options.maximumResidentActors );
        p->getPropertyValue( "Maximum Resident Cells", m_options.maximumResidentCells );
        p->getPropertyValue( "Install Milliseconds", m_options.installMilliseconds );
        p->getPropertyValue( "Preview Editor Camera", m_previewEditorCamera );
        p->getPropertyValue( "Follow Active Camera", m_followActiveCamera );
        p->getPropertyValue( "Instance World", m_instanceWorld );
        auto observer = m_observer.lock();
        if( p->hasProperty( "Observer" ) )
        {
            const auto uuid = p->getPropertyObject( "Observer" ).getValue();
            if( uuid.empty() )
            {
                observer = nullptr;
            }
            else
            {
                // Serialized scene batches resolve references before actors
                // are published. Inspector updates instead need the owning
                // scene graph: scene actors are not asset-catalog resources.
                p->getPropertyAsType( "Observer", observer );
                if( auto actor = getActor() )
                {
                    if( auto scene = actor->getScene() )
                    {
                        const auto canonical = StringUtil::toString( StringUtil::parseUUID( uuid ) );
                        for( const auto &root : scene->getActors() )
                        {
                            if( auto found = findObserver( root, canonical ) )
                            {
                                observer = found;
                                break;
                            }
                        }
                    }
                }
            }
        }
        m_observer = observer;
        m_reopen |= beforePath != m_manifestPath || beforeInstance != m_instanceWorld ||
                    before.loadRadius != m_options.loadRadius ||
                    before.retainRadius != m_options.retainRadius ||
                    before.maximumResidentActors != m_options.maximumResidentActors ||
                    before.enableHlod != m_options.enableHlod ||
                    before.proxyLoadRadius != m_options.proxyLoadRadius ||
                    before.proxyRetainRadius != m_options.proxyRetainRadius ||
                    before.maximumResidentCells != m_options.maximumResidentCells ||
                    before.installMilliseconds != m_options.installMilliseconds;
    }

    SceneWorldStreamStatistics WorldStreaming::statistics() const
    {
        std::lock_guard<std::mutex> lock( streamerLifetimeMutex( this ) );
        return m_streamer ? m_streamer->statistics() : SceneWorldStreamStatistics{};
    }
}  // namespace workphone::scene
