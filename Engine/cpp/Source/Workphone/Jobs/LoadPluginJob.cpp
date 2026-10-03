#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/LoadPluginJob.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/PluginMacros.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/System/IPluginManager.hpp>
#include <Workphone/Interface/System/IPlugin.hpp>

DECLARE_FUNCTION_ARG1( loadPlugin, void, workphone::core::IApplicationManager * );
DECLARE_FUNCTION_ARG3( workphone_get_version, void, int *, int *, int * );

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, LoadPluginJob, Job );

    LoadPluginJob::LoadPluginJob() = default;

    LoadPluginJob::~LoadPluginJob() = default;

    void LoadPluginJob::execute()
    {
        try
        {
            int exeMajor = WP_VERSION_MAJOR;
            int exeMinor = WP_VERSION_MINOR;
            int exePatch = WP_VERSION_PATCH;

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto pluginManager = applicationManager->getPluginManager();
            if( pluginManager )
            {
                auto pluginPath = getPluginPath();

#if defined WP_PLATFORM_WIN32
                if( pluginPath.find( ".dll" ) == String::npos )
                {
                    pluginPath += ".dll";
                }
#elif defined WP_PLATFORM_APPLE
                pluginPath = StringUtil::replace( pluginPath, '\r', ' ' );
                pluginPath = StringUtil::trim( pluginPath );
                pluginPath = StringUtil::cleanupPath( pluginPath );

                if( pluginPath.find( ".dylib" ) == String::npos )
                {
                    pluginPath += ".dylib";
                }

                if( !Path::isExistingFile( pluginPath ) )
                {
                    pluginPath = "lib" + pluginPath;
                }
#elif defined WP_PLATFORM_LINUX
                if( pluginPath.find( ".so" ) == String::npos )
                {
                    pluginPath += ".so";
                }
#endif

                auto plugin = pluginManager->loadPlugin( pluginPath );
                if( plugin )
                {
                    plugin->load( nullptr );

                    auto libraryHandle = plugin->getLibraryHandle();

                    init_workphone_get_version( libraryHandle );

                    int dllMajor = 0;
                    int dllMinor = 0;
                    int dllPatch = 0;

                    if( hworkphone_get_version )
                    {
                        hworkphone_get_version( &dllMajor, &dllMinor, &dllPatch );
                    }

                    // Compare versions
                    if( exeMajor != dllMajor || exeMinor != dllMinor || exePatch != dllPatch )
                    {
                        auto msg =
                            "DLL version mismatch. Recompile required. DLL version: " +
                            StringUtil::toString( dllMajor ) + "." + StringUtil::toString( dllMinor ) +
                            "." + StringUtil::toString( dllPatch ) +
                            " Host version: " + StringUtil::toString( exeMajor ) + "." +
                            StringUtil::toString( exeMinor ) + "." + StringUtil::toString( exePatch );
                        WP_LOG_ERROR( msg );

                        // Perform actions for version mismatch, e.g., recompiling
                    }
                    else
                    {
                        auto msg =
                            "DLL version matches the host executable version. DLL version: " +
                            StringUtil::toString( dllMajor ) + "." + StringUtil::toString( dllMinor ) +
                            "." + StringUtil::toString( dllPatch ) +
                            " Host version: " + StringUtil::toString( exeMajor ) + "." +
                            StringUtil::toString( exeMinor ) + "." + StringUtil::toString( exePatch );
                        WP_LOG( msg.c_str() );

                        init_loadPlugin( libraryHandle );

                        if( hloadPlugin )
                        {
                            auto fLoadPlugin = hloadPlugin;
                            fLoadPlugin( applicationManager );
                        }

                        setPlugin( plugin );
                    }
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String LoadPluginJob::getPluginPath() const
    {
        return m_pluginPath;
    }

    void LoadPluginJob::setPluginPath( const String &pluginPath )
    {
        m_pluginPath = pluginPath;
    }

    void LoadPluginJob::setPlugin( SmartPtr<IPlugin> plugin )
    {
        m_plugin = plugin;
    }

    SmartPtr<IPlugin> LoadPluginJob::getPlugin() const
    {
        return m_plugin;
    }
}  // namespace workphone
