#include <WPFMODStudio/WPFMODStudio.hpp>
#include <WPFMODStudio/WPFMODStudioManager.hpp>
#include <WPFMODStudio/WPFMODStudioListener3.hpp>
#include <WPFMODStudio/WPFMODStudioSound.hpp>
#include <Workphone/Workphone.hpp>

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

    SmartPtr<WPFMODStudio> WPFMODStudio::m_sPlugin;
    SmartPtr<IFactoryManager> WPFMODStudio::m_factoryManager;

    WPFMODStudio::~WPFMODStudio() = default;

    WPFMODStudio::WPFMODStudio() = default;

    void WPFMODStudio::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( data );
        setFactoryManager( factoryManager );

        FactoryUtil::addFactory<WPFMODStudioManager>();

        FactoryUtil::addFactory<WPFMODStudioSound>( factoryManager );
        FactoryUtil::addFactory<FMODSoundListener3>( factoryManager );
    }

    void WPFMODStudio::unload( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::removeFactory<WPFMODStudioManager>();

        if( auto factoryManager = getFactoryManager() )
        {
            FactoryUtil::removeFactory<WPFMODStudioSound>( factoryManager );
            FactoryUtil::removeFactory<FMODSoundListener3>( factoryManager );
            factoryManager->unload( nullptr );
        }

        setFactoryManager( nullptr );
    }

    SmartPtr<WPFMODStudio> WPFMODStudio::instance()
    {
        return m_sPlugin;
    }

    void WPFMODStudio::setInstance( SmartPtr<WPFMODStudio> plugin )
    {
        m_sPlugin = plugin;
    }

    SmartPtr<IFactoryManager> WPFMODStudio::getFactoryManager()
    {
        return m_factoryManager;
    }

    void WPFMODStudio::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

}  // namespace workphone

#ifndef _WP_STATIC_LIB_
extern "C" {

WP_INTERFACE_EXPORT void WP_INTERFACE_API getFirebladeVersion( int *major, int *minor, int *patch )
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

    auto plugin = workphone::make_ptr<WPFMODStudio>();
    plugin->load( nullptr );
    WPFMODStudio::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace physics;

    if( auto plugin = WPFMODStudio::instance() )
    {
        plugin->unload( nullptr );
        WPFMODStudio::setInstance( nullptr );
    }
}
}
#endif
