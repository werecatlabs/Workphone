#include "Terrain.hpp"
#include <Workphone/Workphone.hpp>

#ifdef _WP_STATIC_LIB_
#    include <WPSQLite/WPSQLite.hpp>
#    include <FBOISInput/FBOISInput.hpp>
#endif

namespace workphone
{

    Terrain::Terrain()
    {
    }

    Terrain::~Terrain()
    {
        unload( nullptr );
    }

    void Terrain::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto threadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( threadId );

        auto applicationManager = new core::ApplicationManager;
        core::ApplicationManager::setInstance( applicationManager );

        const auto configFilePath = String( "wp_plugins_samples.cfg" );
        setPluginsConfigFilePath( configFilePath );

        Application::load( data );

        setLoadingState( LoadingState::Loaded );
    }

    void Terrain::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        auto applicationManager = core::ApplicationManager::instance();

        applicationManager->unload( nullptr );
        core::ApplicationManager::setInstance( nullptr );
        applicationManager = nullptr;
        WP_ASSERT( core::ApplicationManager::instance() == nullptr );

        setLoadingState( LoadingState::Unloaded );
    }

    void Terrain::update()
    {
        Application::update();
    }

    void Terrain::createScene()
    {
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
    }

    void Terrain::createPlugins()
    {
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );
    }

}  // namespace workphone
