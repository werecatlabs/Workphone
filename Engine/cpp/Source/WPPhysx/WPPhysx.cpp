#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysx.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
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

namespace workphone::physics
{
    SmartPtr<WPPhysx>         WPPhysx::m_sPlugin;
    SmartPtr<IFactoryManager> WPPhysx::m_factoryManager;

    WPPhysx::WPPhysx() = default;
    WPPhysx::~WPPhysx() = default;

    void WPPhysx::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        FactoryUtil::addFactory<PhysxManager>();
    }

    void WPPhysx::unload( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::removeFactory<PhysxManager>();

        setFactoryManager( nullptr );
    }

    SmartPtr<WPPhysx> WPPhysx::instance()
    {
        return m_sPlugin;
    }

    void WPPhysx::setInstance( SmartPtr<WPPhysx> plugin )
    {
        m_sPlugin = plugin;
    }

    SmartPtr<IFactoryManager> WPPhysx::getFactoryManager()
    {
        return m_factoryManager;
    }

    void WPPhysx::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

} // namespace workphone::physics

extern "C"
{
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
        using namespace workphone::physics;

        auto plugin = workphone::make_ptr<WPPhysx>();
        plugin->load( nullptr );
        WPPhysx::setInstance( plugin );
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    unloadPlugin( workphone::core::ApplicationManager *applicationManager )
    {
        using namespace workphone;
        using namespace workphone::physics;

        if( auto plugin = WPPhysx::instance() )
        {
            plugin->unload( nullptr );
            WPPhysx::setInstance( nullptr );
        }
    }
}
