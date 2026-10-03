#include <WPSQLite/WPSQLite.hpp>
#include <WPSQLite/SQLiteDatabase.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

#if defined WP_PLATFORM_WIN32

int WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
    return 1;
}

#endif

namespace workphone
{
    SmartPtr<SQLitePlugin> SQLitePlugin::m_sPlugin;

    SQLitePlugin::SQLitePlugin() = default;

    SQLitePlugin::~SQLitePlugin() = default;

    void SQLitePlugin::load( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::addFactory<SQLiteDatabase>();
    }

    void SQLitePlugin::unload( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::removeFactory<SQLiteDatabase>();
    }

    SmartPtr<SQLitePlugin> SQLitePlugin::instance()
    {
        return m_sPlugin;
    }

    void SQLitePlugin::setInstance( SmartPtr<SQLitePlugin> plugin )
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
    using namespace workphone::physics;

    auto plugin = workphone::make_ptr<SQLitePlugin>();
    plugin->load( nullptr );
    SQLitePlugin::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace workphone::physics;

    if( auto plugin = SQLitePlugin::instance() )
    {
        plugin->unload( nullptr );
        SQLitePlugin::setInstance( nullptr );
    }
}
}
#endif
