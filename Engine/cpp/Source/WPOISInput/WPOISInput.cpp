#include <WPOISInput/WPOISInput.hpp>
#include <WPOISInput/WPOISInputManager.hpp>
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
    SmartPtr<OISInput> OISInput::m_sPlugin;

    OISInput::OISInput() = default;

    OISInput::~OISInput() = default;

    void OISInput::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        FactoryUtil::addFactory<OISInputManager>();
    }

    void OISInput::unload( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::removeFactory<OISInputManager>();
    }

    auto OISInput::createInputManager( SmartPtr<render::IGraphicsWindow> window )
        -> SmartPtr<IInputDeviceManager>
    {
        return workphone::make_ptr<OISInputManager>( window );
    }

    auto OISInput::instance() -> SmartPtr<OISInput>
    {
        return m_sPlugin;
    }

    void OISInput::setInstance( SmartPtr<OISInput> plugin )
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

    auto plugin = workphone::make_ptr<OISInput>();
    plugin->load( nullptr );
    OISInput::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;

    if( auto plugin = OISInput::instance() )
    {
        plugin->unload( nullptr );
        OISInput::setInstance( nullptr );
    }
}
}
#endif
