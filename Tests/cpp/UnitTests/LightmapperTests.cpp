#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;

namespace
{
    class MockLightmapperCallback : public render::ILightmapperCallback
    {
    public:
        void onProgress( float progress, const String &message ) override
        {
            ++progressCallCount;
            lastProgress = progress;
            lastMessage = message;
        }

        void onComplete() override
        {
            ++completeCallCount;
        }

        void onError( const String &error ) override
        {
            ++errorCallCount;
            lastError = error;
        }

        void reset()
        {
            progressCallCount = 0;
            completeCallCount = 0;
            errorCallCount = 0;
            lastProgress = 0.0f;
            lastMessage.clear();
            lastError.clear();
        }

        int progressCallCount = 0;
        int completeCallCount = 0;
        int errorCallCount = 0;
        float lastProgress = 0.0f;
        String lastMessage;
        String lastError;
    };

    render::LightmapperSettings makeCustomSettings()
    {
        render::LightmapperSettings settings;
        settings.resolution = 1024;
        settings.samplesPerPixel = 32;
        settings.maxBounces = 5;
        settings.enableAO = false;
        settings.aoDistance = 2.5f;
        settings.enableShadows = false;
        settings.gamma = 1.8f;
        settings.padding = 4;
        return settings;
    }

    void checkSettingsEqual( const render::LightmapperSettings &actual,
                             const render::LightmapperSettings &expected )
    {
        BOOST_CHECK_EQUAL( actual.resolution, expected.resolution );
        BOOST_CHECK_EQUAL( actual.samplesPerPixel, expected.samplesPerPixel );
        BOOST_CHECK_EQUAL( actual.maxBounces, expected.maxBounces );
        BOOST_CHECK_EQUAL( actual.enableAO, expected.enableAO );
        BOOST_CHECK_CLOSE( actual.aoDistance, expected.aoDistance, 0.001f );
        BOOST_CHECK_EQUAL( actual.enableShadows, expected.enableShadows );
        BOOST_CHECK_CLOSE( actual.gamma, expected.gamma, 0.001f );
        BOOST_CHECK_EQUAL( actual.padding, expected.padding );
    }

    SmartPtr<render::ILightmapper> tryCreateLightmapper( TestGuard &guard )
    {
        if( !guard.factoryManager )
        {
            return nullptr;
        }

        return guard.factoryManager->make_object<render::ILightmapper>( "Lightmapper" );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( lightmapper_settings_default_values )
{
    render::LightmapperSettings settings;

    BOOST_CHECK_EQUAL( settings.resolution, 512u );
    BOOST_CHECK_EQUAL( settings.samplesPerPixel, 16u );
    BOOST_CHECK_EQUAL( settings.maxBounces, 3u );
    BOOST_CHECK_EQUAL( settings.enableAO, true );
    BOOST_CHECK_CLOSE( settings.aoDistance, 1.0f, 0.001f );
    BOOST_CHECK_EQUAL( settings.enableShadows, true );
    BOOST_CHECK_CLOSE( settings.gamma, 2.2f, 0.001f );
    BOOST_CHECK_EQUAL( settings.padding, 2u );
}

BOOST_AUTO_TEST_CASE( lightmapper_settings_are_copyable_and_assignable )
{
    render::LightmapperSettings original = makeCustomSettings();
    render::LightmapperSettings copied( original );
    render::LightmapperSettings assigned;

    assigned = original;

    checkSettingsEqual( copied, original );
    checkSettingsEqual( assigned, original );
}

BOOST_AUTO_TEST_CASE( lightmapper_settings_preserve_boundary_values )
{
    render::LightmapperSettings settings;
    settings.resolution = 1;
    settings.samplesPerPixel = 1;
    settings.maxBounces = 0;
    settings.aoDistance = 0.0f;
    settings.gamma = 0.1f;
    settings.padding = 0;

    BOOST_CHECK_EQUAL( settings.resolution, 1u );
    BOOST_CHECK_EQUAL( settings.samplesPerPixel, 1u );
    BOOST_CHECK_EQUAL( settings.maxBounces, 0u );
    BOOST_CHECK_CLOSE( settings.aoDistance, 0.0f, 0.001f );
    BOOST_CHECK_CLOSE( settings.gamma, 0.1f, 0.001f );
    BOOST_CHECK_EQUAL( settings.padding, 0u );
}

BOOST_AUTO_TEST_CASE( lightmapper_callback_records_progress_complete_and_error )
{
    MockLightmapperCallback callback;

    callback.onProgress( 0.25f, "Gathering geometry" );
    callback.onProgress( 0.75f, "Baking" );
    callback.onComplete();
    callback.onError( "Example error" );

    BOOST_CHECK_EQUAL( callback.progressCallCount, 2 );
    BOOST_CHECK_CLOSE( callback.lastProgress, 0.75f, 0.001f );
    BOOST_CHECK_EQUAL( callback.lastMessage, "Baking" );
    BOOST_CHECK_EQUAL( callback.completeCallCount, 1 );
    BOOST_CHECK_EQUAL( callback.errorCallCount, 1 );
    BOOST_CHECK_EQUAL( callback.lastError, "Example error" );
}

BOOST_AUTO_TEST_CASE( lightmapper_callback_reset_clears_recorded_state )
{
    MockLightmapperCallback callback;

    callback.onProgress( 1.0f, "Complete" );
    callback.onComplete();
    callback.onError( "Error" );
    callback.reset();

    BOOST_CHECK_EQUAL( callback.progressCallCount, 0 );
    BOOST_CHECK_EQUAL( callback.completeCallCount, 0 );
    BOOST_CHECK_EQUAL( callback.errorCallCount, 0 );
    BOOST_CHECK_CLOSE( callback.lastProgress, 0.0f, 0.001f );
    BOOST_CHECK( callback.lastMessage.empty() );
    BOOST_CHECK( callback.lastError.empty() );
}

BOOST_AUTO_TEST_CASE( lightmap_format_enum_values_are_distinct )
{
    BOOST_CHECK( LightmapFormat::RGB8 != LightmapFormat::RGBA8 );
    BOOST_CHECK( LightmapFormat::RGBA8 != LightmapFormat::RGB16F );
    BOOST_CHECK( LightmapFormat::RGB16F != LightmapFormat::RGBA16F );
    BOOST_CHECK( LightmapFormat::RGBA16F != LightmapFormat::RGB32F );
    BOOST_CHECK( LightmapFormat::RGB32F != LightmapFormat::RGBA32F );
}

BOOST_AUTO_TEST_CASE( lightmapper_runtime_factory_smoke_test )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.applicationManager );

    auto lightmapper = tryCreateLightmapper( guard );
    if( !lightmapper )
    {
        BOOST_TEST_MESSAGE( "Lightmapper factory object not available, skipping runtime smoke test" );
        return;
    }

    BOOST_CHECK( lightmapper );
    BOOST_CHECK( !lightmapper->isBaking() );
}

BOOST_AUTO_TEST_CASE( lightmapper_runtime_settings_round_trip_when_available )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.applicationManager );

    auto lightmapper = tryCreateLightmapper( guard );
    if( !lightmapper )
    {
        BOOST_TEST_MESSAGE( "Lightmapper factory object not available, skipping settings round-trip" );
        return;
    }

    BOOST_REQUIRE( lightmapper->initialize() );

    auto settings = makeCustomSettings();
    lightmapper->setSettings( settings );

    checkSettingsEqual( lightmapper->getSettings(), settings );
}

BOOST_AUTO_TEST_CASE( lightmapper_runtime_callback_accepts_valid_and_null_callback_when_available )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.applicationManager );

    auto lightmapper = tryCreateLightmapper( guard );
    if( !lightmapper )
    {
        BOOST_TEST_MESSAGE( "Lightmapper factory object not available, skipping callback smoke test" );
        return;
    }

    BOOST_REQUIRE( lightmapper->initialize() );

    MockLightmapperCallback callback;
    lightmapper->setCallback( &callback );
    lightmapper->setCallback( nullptr );

    BOOST_CHECK( !lightmapper->isBaking() );
}

BOOST_AUTO_TEST_CASE( lightmapper_load )
{
    TestGuard guard;

    auto manager = core::IApplicationManager::instance();
    BOOST_CHECK( manager );
}
