#include <WPLua/WPLua.hpp>
#include <Workphone/Workphone.hpp>
#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaObjectData.hpp>

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

    SmartPtr<WPLua> WPLua::m_sPlugin;

    WPLua::WPLua() = default;
    WPLua::~WPLua() = default;

    void WPLua::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            FactoryUtil::addFactory<LuaManager>( factoryManager );
            FactoryUtil::addFactory<LuaObjectData>( factoryManager );

            factoryManager->setPoolSizeByType<LuaObjectData>( 256 );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void WPLua::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                FactoryUtil::removeFactory<LuaManager>();
                FactoryUtil::removeFactory<LuaObjectData>();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<WPLua> WPLua::instance()
    {
        return m_sPlugin;
    }

    void WPLua::setInstance( SmartPtr<WPLua> plugin )
    {
        m_sPlugin = plugin;
    }
}  // namespace workphone

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

    auto plugin = workphone::make_ptr<WPLua>();
    plugin->load( nullptr );
    WPLua::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace physics;

    if( auto plugin = WPLua::instance() )
    {
        plugin->unload( nullptr );
        WPLua::setInstance( nullptr );
    }
}
}
