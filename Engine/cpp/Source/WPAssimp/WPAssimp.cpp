#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/WPAssimp.hpp>
#include <WPAssimp/AssimpLoader.hpp>
#include <WPAssimp/LogStream.hpp>
#include <Workphone/Workphone.hpp>

#if WP_USE_ASSET_IMPORT
#    include <assimp/DefaultLogger.hpp>
#endif

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

#if defined WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
int WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
    return 1;
}
#    endif
#endif

namespace workphone
{
    SmartPtr<WPAssimp> WPAssimp::m_sPlugin;

    WPAssimp::WPAssimp() = default;

    WPAssimp::~WPAssimp() = default;

    void WPAssimp::load( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::addFactory<AssimpLoader>();

        if( Assimp::DefaultLogger::isNullLogger() )
        {
            Assimp::DefaultLogger::create( "" );
        }

        m_logStream = new LogStream();

        auto logger = Assimp::DefaultLogger::get();
        if( logger )
        {
            logger->attachStream( m_logStream, 0 );
        }
    }

    void WPAssimp::unload( SmartPtr<ISharedObject> data )
    {
        if( m_logStream )
        {
            if( !Assimp::DefaultLogger::isNullLogger() )
            {
                if( auto logger = Assimp::DefaultLogger::get() )
                {
                    logger->detachStream( m_logStream, 0 );
                }
            }

            delete m_logStream;
            m_logStream = nullptr;
        }

        FactoryUtil::removeFactory<AssimpLoader>();

#if WP_USE_ASSET_IMPORT
        Assimp::DefaultLogger::kill();
#endif
    }

    SmartPtr<WPAssimp> WPAssimp::instance()
    {
        return m_sPlugin;
    }

    void WPAssimp::setInstance( SmartPtr<WPAssimp> plugin )
    {
        m_sPlugin = plugin;
    }

}  // namespace workphone

#ifndef _WP_STATIC_LIB_
extern "C" {

WP_INTERFACE_EXPORT void WP_INTERFACE_API workphone_get_version( int *major, int *minor, int *patch )
{
    *major = WP_VERSION_MAJOR;
    *minor = WP_VERSION_MINOR;
    *patch = WP_VERSION_PATCH;
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
loadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;

    auto plugin = workphone::make_ptr<WPAssimp>();
    plugin->load( nullptr );
    WPAssimp::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;

    if( auto plugin = WPAssimp::instance() )
    {
        plugin->unload( nullptr );
        WPAssimp::setInstance( nullptr );
    }
}
}
#endif
