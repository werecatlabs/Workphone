#include <WPNetwork/WPNetwork.hpp>
#include <WPNetwork/WPNetworkConfig.hpp>

namespace workphone
{
    SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkServer( const String &filePath )
    {
        const auto config = WPNetworkConfig::load( filePath );
        auto manager = SmartPtr<WPNetworkManager>( new WPNetworkManager( false ) );
        manager->setPort( config.port );
        manager->setMaxClients( config.maxClients );
        manager->setNetIterations( config.eventsPerPoll );
        manager->setVerbose( config.verbose );
        manager->setServer( true );
        return manager;
    }

    SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkClient( const String &filePath )
    {
        const auto config = WPNetworkConfig::load( filePath );
        auto manager = SmartPtr<WPNetworkManager>( new WPNetworkManager( true ) );
        manager->setPort( config.port );
        manager->setMaxClients( config.maxClients );
        manager->setNetIterations( config.eventsPerPoll );
        manager->setVerbose( config.verbose );
        manager->setServer( false );
        return manager;
    }
}  // namespace workphone
