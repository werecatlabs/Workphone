#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, WorldStreaming, Component );
    WorldStreaming::WorldStreaming() = default;
    WorldStreaming::~WorldStreaming() = default;

    void WorldStreaming::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() ) return;
        Component::load( data );
        auto manager = core::IApplicationManager::instancePtr()->getGameManager();
        manager->registerComponentUpdate( TaskId::Primary, Thread::UpdateState::PreUpdate, this );
        manager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::PreUpdate, this );
        m_reopen = true;
        setLoadingState( LoadingState::Loaded );
    }

    void WorldStreaming::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded ) return;
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = app->getGameManager() ) manager->unregisterAllComponent( this );
        m_streamer.reset(); m_openPath.clear(); m_observer = nullptr;
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void WorldStreaming::preUpdate()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app->getGameManager();
        if( Thread::getCurrentTask() != manager->getSceneTask() || !isEnabled() ) return;
        auto actor = getActor();
        if( !actor || !actor->isEnabledInScene() || !actor->getScene() ) return;
        if( m_reopen )
        {
            m_reopen = false;
            if( m_manifestPath.empty() ) { m_streamer.reset(); return; }
            const auto path = Path::isPathAbsolute( m_manifestPath ) ? m_manifestPath :
                Path::lexically_normal( app->getProjectPath(), m_manifestPath );
            String error;
            if( m_streamer && m_streamer->isOpen() && m_openPath == path )
            {
                if( !m_streamer->open( path, actor->getScene(), m_options, error ) )
                { m_error = error; WP_LOG_ERROR( "WorldStreaming: " + error ); return; }
            }
            else
            {
                auto candidate = std::make_unique<SceneWorldStreamer>();
                if( !candidate->open( path, actor->getScene(), m_options, error ) )
                { m_error = error; WP_LOG_ERROR( "WorldStreaming: " + error ); return; }
                m_streamer = std::move( candidate );
            }
            m_openPath = path;
        }
        if( !m_streamer || !m_streamer->isOpen() ) return;
        auto observer = m_observer.lock();
        if( !observer ) observer = actor;
        if( m_previewEditorCamera && app->isEditor() )
            if( auto cameras = app->getCameraManager() )
                if( auto camera = cameras->getEditorCamera() ) observer = camera;
        const auto p = observer->getPosition();
        String error;
        if( !m_streamer->update( { Vector3F( float( p.x ), float( p.y ), float( p.z ) ) }, error ) && error != m_error )
            WP_LOG_ERROR( "WorldStreaming: " + error );
        m_error = error;
    }

    SmartPtr<Properties> WorldStreaming::getProperties() const
    {
        auto p = Component::getProperties();
        p->setProperty( "World Manifest", m_manifestPath );
        p->setProperty( "Load Radius", m_options.loadRadius );
        p->setProperty( "Retain Radius", m_options.retainRadius );
        p->setProperty( "Maximum Resident Actors", m_options.maximumResidentActors );
        p->setProperty( "Maximum Resident Cells", m_options.maximumResidentCells );
        p->setProperty( "Install Milliseconds", m_options.installMilliseconds );
        p->setProperty( "Preview Editor Camera", m_previewEditorCamera );
        p->setPropertyAsType( "Observer", m_observer.lock() );
        p->setProperty( "Streaming Error", m_error, true );
        auto stats = statistics();
        p->setProperty( "Resident Actors", stats.residentActors, true );
        p->setProperty( "Resident Cells", stats.residentCells, true );
        p->setProperty( "Pending Cells", stats.pendingCells, true );
        p->setProperty( "Resident Source Bytes", stats.residentSourceBytes, true );
        return p;
    }

    void WorldStreaming::setProperties( SmartPtr<Properties> p )
    {
        Component::setProperties( p );
        const auto beforePath = m_manifestPath;
        const auto before = m_options;
        p->getPropertyValue( "World Manifest", m_manifestPath );
        p->getPropertyValue( "Load Radius", m_options.loadRadius );
        p->getPropertyValue( "Retain Radius", m_options.retainRadius );
        p->getPropertyValue( "Maximum Resident Actors", m_options.maximumResidentActors );
        p->getPropertyValue( "Maximum Resident Cells", m_options.maximumResidentCells );
        p->getPropertyValue( "Install Milliseconds", m_options.installMilliseconds );
        p->getPropertyValue( "Preview Editor Camera", m_previewEditorCamera );
        auto observer = m_observer.lock();
        p->getPropertyAsType( "Observer", observer );
        m_observer = observer;
        m_reopen |= beforePath != m_manifestPath || before.loadRadius != m_options.loadRadius ||
            before.retainRadius != m_options.retainRadius || before.maximumResidentActors != m_options.maximumResidentActors ||
            before.maximumResidentCells != m_options.maximumResidentCells || before.installMilliseconds != m_options.installMilliseconds;
    }

    SceneWorldStreamStatistics WorldStreaming::statistics() const
    { return m_streamer ? m_streamer->statistics() : SceneWorldStreamStatistics{}; }
}
