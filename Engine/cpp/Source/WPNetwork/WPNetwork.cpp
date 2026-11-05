#include <WPNetwork/WPNetwork.hpp>
#include <WPNetwork/WPNetworkConfig.hpp>
#if WP_NETWORK_GNS
#    include <WPNetwork/WPNetworkGNSManager.hpp>
#endif
#include <stdexcept>

namespace workphone
{
    namespace
    {
        SmartPtr<INetworkManager> createGNSDevelopment( const WPNetworkConfig &config, bool server )
        {
#if WP_NETWORK_GNS
            if( config.bindAddress != 0x7f000001u )
            {
                throw std::invalid_argument( "GNS development profiles must bind to loopback" );
            }
            if( server && config.port == 0 )
            {
                throw std::invalid_argument( "GNS development servers require a nonzero port" );
            }
            WPNetworkGNSConfig settings;
            settings.allowUnverifiedLoopback = true;
            settings.bindAddress = "127.0.0.1:" + std::to_string( config.port );
            settings.maxClients = config.maxClients;
            settings.eventsPerPoll = config.eventsPerPoll;
            auto manager = SmartPtr<WPNetworkGNSManager>( new WPNetworkGNSManager( settings ) );
            manager->setVerbose( config.verbose );
            manager->setServer( server );
            return manager;
#else
            (void)config;
            (void)server;
            throw std::logic_error( "WPNetwork was built without GNS support" );
#endif
        }
    }  // namespace

    SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkServer( const String &filePath )
    {
        const auto config = WPNetworkConfig::load( filePath );
        if( config.backend == "gns_loopback" )
        {
            return createGNSDevelopment( config, true );
        }
        auto manager = SmartPtr<WPNetworkManager>( new WPNetworkManager( false ) );
        manager->setPort( config.port );
        manager->setBindAddress( config.bindAddress );
        manager->setMaxClients( config.maxClients );
        manager->setNetIterations( config.eventsPerPoll );
        manager->setVerbose( config.verbose );
        manager->setServer( true );
        return manager;
    }

    SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkClient( const String &filePath )
    {
        const auto config = WPNetworkConfig::load( filePath );
        if( config.backend == "gns_loopback" )
        {
            return createGNSDevelopment( config, false );
        }
        auto manager = SmartPtr<WPNetworkManager>( new WPNetworkManager( true ) );
        manager->setPort( config.port );
        manager->setBindAddress( config.bindAddress );
        manager->setMaxClients( config.maxClients );
        manager->setNetIterations( config.eventsPerPoll );
        manager->setVerbose( config.verbose );
        manager->setServer( false );
        return manager;
    }
}  // namespace workphone
