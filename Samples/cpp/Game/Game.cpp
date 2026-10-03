#include "Game.h"
#include "Types.h"
#include <Workphone/Workphone.hpp>
#include <WPOISInput/WPOISInput.hpp>
#include <WPSQLite/WPSQLite.hpp>

#if WP_BUILD_PHYSX
#    include <WPPhysx/WPPhysx.hpp>
#elif WP_BUILD_ODE
#    include <WPODE3/CPhysicsManagerODE.hpp>
#endif

namespace workphone
{

    Game::Game() = default;

    Game::~Game() = default;

    void Game::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto threadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( threadId );

            auto applicationManager = new core::ApplicationManager;
            core::ApplicationManager::setInstance( applicationManager );

            Application::load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Game::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager )
            {
                applicationManager->unload( nullptr );
                core::ApplicationManager::setInstance( nullptr );
                applicationManager = nullptr;
                WP_ASSERT( core::ApplicationManager::instance() == nullptr );
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Game::createPlugins()
    {
        const auto configFilePath = String( "wp_plugins_samples.cfg" );
        setPluginsConfigFilePath( configFilePath );

        Application::createPlugins();

#ifdef _WP_STATIC_LIB_
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto corePlugin = workphone::make_ptr<WPCore>();
        applicationManager->addPlugin( corePlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
        applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
        auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
        applicationManager->addPlugin( graphicsPlugin );
#    endif
#endif
    }

}  // namespace workphone

int main( int argc, char *argv[] )
{
    using namespace workphone;

    try
    {
        auto typeManager = std::make_shared<TypeManager>();
        typeManager->load();
        TypeManager::setInstance( typeManager.get() );

        auto app = std::make_shared<Game>();
        app->load( nullptr );
        app->run();
        app->unload( nullptr );

        TypeManager::setInstance( nullptr );
    }
    catch( Exception &e )
    {
        std::cout << e.what() << std::endl;
    }
    catch( std::exception &e )
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
