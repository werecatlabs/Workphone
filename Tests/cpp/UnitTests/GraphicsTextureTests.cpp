#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/TextureManager.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( texture_manager_state_listener_type_info )
{
    auto listener = workphone::make_ptr<render::TextureManager::StateListener>();
    BOOST_REQUIRE( listener );
    BOOST_CHECK( listener->isDerived<IStateListener>() );
    BOOST_CHECK( !listener->isDerived<render::ITextureManager>() );
}

BOOST_AUTO_TEST_CASE( graphics_system_texture_manager )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::ApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not available." );
            return;
        }

        auto textureManager = graphicsSystem->getTextureManager();
        BOOST_CHECK( textureManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( graphics_system_rtt )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::ApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "Graphics system is not available." );
            return;
        }

        auto textureManager = graphicsSystem->getTextureManager();
        BOOST_CHECK( textureManager );

        auto renderTarget = textureManager->createRenderTexture();
        BOOST_CHECK( renderTarget );

        if( renderTarget )
        {
            textureManager->destroyRenderTexture( renderTarget );
            BOOST_CHECK( renderTarget->getLoadingState() == LoadingState::Unloaded );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
