#include "HelloWorld.h"
#include <Workphone/Workphone.hpp>

#ifdef _WP_STATIC_LIB_
#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.h>
#    endif
#endif

namespace workphone
{

    HelloWorld::HelloWorld() = default;

    HelloWorld::~HelloWorld() = default;

    void HelloWorld::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto threadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( threadId );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            applicationManager->load( data );
            core::ApplicationManager::setInstance( applicationManager );
            m_applicationManager = applicationManager;

            applicationManager->setApplication( this );

            setCreateFrameStatistics( true );
            Application::load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void HelloWorld::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            Application::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void HelloWorld::update()
    {
        Application::update();

        if( Thread::getCurrentTask() != TaskId::Render )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        if( !timer )
        {
            return;
        }

        const auto now = timer->getTime();
        if( m_fpsSampleStart <= 0.0 )
        {
            m_fpsSampleStart = now;
        }

        ++m_fpsFrameCount;
        const auto elapsed = now - m_fpsSampleStart;
        if( elapsed >= 1.0 )
        {
            const auto fps = static_cast<u32>( static_cast<f64>( m_fpsFrameCount ) / elapsed + 0.5 );
            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                if( auto window = graphicsSystem->getDefaultWindow() )
                {
                    window->setTitle( "Workphone - Hello world! | Render FPS: " +
                                      StringUtil::toString( fps ) );
                }
            }

            m_fpsSampleStart = now;
            m_fpsFrameCount = 0;
        }
    }

    void HelloWorld::createScene()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "HelloWorld::createScene: graphics system not available." );
            return;
        }

        if( auto debug = graphicsSystem->getDebug() )
        {
            debug->drawText( 0, Vector2F::unit() * 0.5f, "Hello world!", 0 );
        }

        // WPGraphics uses the native window title as a backend-independent fallback
        // while its debug-text adapter is still intentionally lightweight.
        if( auto window = graphicsSystem->getDefaultWindow() )
        {
            window->setTitle( "Workphone - Hello world!" );
        }
    }

    void HelloWorld::createPlugins()
    {
        const auto configFilePath = String( "wp_plugins_samples.cfg" );
        setPluginsConfigFilePath( configFilePath );

        Application::createPlugins();

#ifdef _WP_STATIC_LIB_
        auto applicationManager = core::IApplicationManager::instance();
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

#ifdef WP_ENABLE_TRACE
    s32 HelloWorld::addReference()
    {
        //auto debugStr = getDebugStr() + DebugUtil::getStackTrace() + "\n";
        //setDebugStr( debugStr );

        return ISharedObject::addReference();
    }

    bool HelloWorld::removeReference()
    {
        return ISharedObject::removeReference();
    }
#endif

}  // namespace workphone

int main( int argc, char *argv[] )
{
    using namespace workphone;

    TypeManager *typeManager = nullptr;
    bool ownsTypeManager = false;
    SmartPtr<HelloWorld> app;

    auto unloadApplication = [&app]() {
        if( app )
        {
            if( app->getLoadingState() != LoadingState::Unloaded )
            {
                app->unload( nullptr );
            }

            WP_ASSERT( app->getLoadingState() == LoadingState::Unloaded );
            app = nullptr;
        }

        core::IApplicationManager::setInstance( nullptr );
    };

    auto unloadTypeManager = [&typeManager, &ownsTypeManager]() {
        if( ownsTypeManager && typeManager )
        {
            typeManager->unload();
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
            ownsTypeManager = false;
        }
    };

    try
    {
        typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
            ownsTypeManager = true;
        }

        app = workphone::make_ptr<HelloWorld>();
        app->setActiveThreads( 4 );
        app->load( nullptr );
        app->run();

        unloadApplication();
        unloadTypeManager();
    }
    catch( Exception &e )
    {
        unloadApplication();
        unloadTypeManager();

        MessageBoxUtil::show( e.what() );
    }
    catch( std::exception &e )
    {
        unloadApplication();
        unloadTypeManager();

        MessageBoxUtil::show( e.what() );
    }
    catch( ... )
    {
        unloadApplication();
        unloadTypeManager();

        MessageBoxUtil::show( "Unknown error" );
    }

    return 0;
}
