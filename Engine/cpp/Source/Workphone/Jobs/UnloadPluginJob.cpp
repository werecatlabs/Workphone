#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/UnloadPluginJob.hpp>
#include <Workphone/Core/PluginMacros.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/IPluginManager.hpp>
#include <Workphone/Interface/System/IPlugin.hpp>

DECLARE_FUNCTION_ARG1( unloadPlugin, void, workphone::core::IApplicationManager * );

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UnloadPluginJob, Job );

    UnloadPluginJob::UnloadPluginJob() = default;

    UnloadPluginJob::~UnloadPluginJob() = default;

    void UnloadPluginJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto pluginManager = applicationManager->getPluginManager();
            if( pluginManager )
            {
                auto plugin = getPlugin();
                if( plugin )
                {
                    auto libraryHandle = plugin->getLibraryHandle();
                    if( libraryHandle )
                    {
                        init_unloadPlugin( libraryHandle );

                        if( hunloadPlugin )
                        {
                            auto funloadPlugin = hunloadPlugin;
                            funloadPlugin( applicationManager.get() );
                        }

                        plugin->unload( nullptr );
                    }

                    pluginManager->unloadPlugin( plugin );
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto UnloadPluginJob::getPlugin() const -> SmartPtr<IPlugin>
    {
        return m_plugin;
    }

    void UnloadPluginJob::setPlugin( SmartPtr<IPlugin> plugin )
    {
        m_plugin = plugin;
    }
}  // namespace workphone
