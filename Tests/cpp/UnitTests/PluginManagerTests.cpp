#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::core;

BOOST_AUTO_TEST_CASE( plugin_manager_tests )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto pluginPath = String( "" );

#if defined WP_PLATFORM_WIN32
        pluginPath = "PluginUnity.dll";
#elif defined WP_PLATFORM_APPLE
        pluginPath = "PluginUnity.dll";
#elif defined WP_PLATFORM_LINUX
        pluginPath = "PluginUnity.dll";
#endif

        auto plugin = pluginManager->loadPlugin( pluginPath );
        if( plugin )
        {
            BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
            plugin->load( nullptr );

            auto loadingState = plugin->getLoadingState();
            BOOST_CHECK( loadingState == LoadingState::Loaded );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
