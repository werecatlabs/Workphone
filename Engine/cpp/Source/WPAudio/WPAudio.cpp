#include <WPAudio/WPAudio.hpp>
#include <Workphone/Workphone.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <WPAudio/WPAudioManager.hpp>

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

    SmartPtr<WPAudio> WPAudio::m_sPlugin;
    SmartPtr<IFactoryManager> WPAudio::m_factoryManager;

    WPAudio::WPAudio() = default;
    WPAudio::~WPAudio() = default;

    void WPAudio::load( SmartPtr<ISharedObject> data )
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( data );
        setFactoryManager( factoryManager );

        FactoryUtil::addFactory<WPAudioManager>();
        FactoryUtil::addFactory<WPAudioSound>( factoryManager );
    }

    void WPAudio::unload( SmartPtr<ISharedObject> data )
    {
        if( auto factoryManager = getFactoryManager() )
        {
            FactoryUtil::removeFactory<WPAudioManager>();
            FactoryUtil::removeFactory<WPAudioSound>( factoryManager );
            factoryManager->unload( nullptr );
            setFactoryManager( nullptr );
        }
    }

    SmartPtr<WPAudio> WPAudio::instance()
    {
        return m_sPlugin;
    }

    void WPAudio::setInstance( SmartPtr<WPAudio> plugin )
    {
        m_sPlugin = plugin;
    }

    SmartPtr<IFactoryManager> WPAudio::getFactoryManager()
    {
        return m_factoryManager;
    }

    void WPAudio::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
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
loadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace physics;

    auto plugin = workphone::make_ptr<WPAudio>();
    plugin->load( nullptr );
    WPAudio::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace physics;

    if( auto plugin = WPAudio::instance() )
    {
        plugin->unload( nullptr );
        WPAudio::setInstance( nullptr );
    }
}
}
#endif
