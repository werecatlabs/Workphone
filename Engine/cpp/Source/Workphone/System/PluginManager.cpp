#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/PluginManager.hpp>
#include <Workphone/System/Plugin.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Jobs/UnloadPluginJob.hpp>

namespace workphone::core
{
    WP_CLASS_REGISTER_DERIVED( workphone::core, PluginManager, IPluginManager );

    PluginManager::PluginManager() = default;

    PluginManager::~PluginManager() = default;

    void PluginManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_plugins.reserve( 10 );
        setLoadingState( LoadingState::Loaded );
    }

    void PluginManager::reload( SmartPtr<ISharedObject> data )
    {
        unload( data );
        load( data );
    }

    void PluginManager::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        auto plugins = getPlugins();
        for( auto &plugin : plugins )
        {
            auto job = workphone::make_ptr<UnloadPluginJob>();
            job->setPlugin( plugin );
            job->execute();
        }

        m_plugins.clear();
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<IPlugin> PluginManager::loadPlugin( const String &filename )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        const auto filePathUTF16 = StringUtil::toUTF8to16( filename );

        auto plugin = workphone::make_ptr<Plugin>();
        plugin->setFilePath( filePathUTF16 );

        m_plugins.emplace_back( plugin );

        return plugin;
    }

    void PluginManager::loadPlugin( SmartPtr<IPlugin> plugin )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_plugins.emplace_back( plugin );
    }

    void PluginManager::unloadPlugin( SmartPtr<IPlugin> plugin )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        m_plugins.erase( std::remove( m_plugins.begin(), m_plugins.end(), plugin ), m_plugins.end() );
    }

    void PluginManager::setPlugins( Array<SmartPtr<ISharedObject>> plugins )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_plugins = plugins;
    }

    Array<SmartPtr<ISharedObject>> PluginManager::getPlugins() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_plugins;
    }

}  // namespace workphone::core
