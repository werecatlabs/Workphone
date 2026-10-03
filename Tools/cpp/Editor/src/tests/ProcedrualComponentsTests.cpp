#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#include <EditorApplication.hpp>
#include <Workphone/Scene/Components/ProceduralRoad.hpp>
#include <Workphone/Scene/Components/ProceduralSky.hpp>
#include <Workphone/Scene/Components/ProceduralSurfaceTexture.hpp>
#include <Workphone/Scene/Components/ProceduralVehicle.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>
#    include <limits>
#    include <stdexcept>

using namespace workphone;

namespace
{
    struct ProceduralComponentFixture
    {
        editor::EditorApplication app;

        ProceduralComponentFixture()
        {
            app.setDebugMode( true );
            app.setActiveThreads( 0 );
            app.load( nullptr );
            BOOST_REQUIRE( app.isLoaded() );
        }

        ~ProceduralComponentFixture()
        {
            if( auto manager = core::IApplicationManager::instancePtr() )
            {
                manager->setQuit( true );
                manager->setRunning( false );
            }
            app.unload( nullptr );
        }
    };

    template <class T>
    T propertyValue( SmartPtr<Properties> properties, const String &name )
    {
        T value{};
        BOOST_REQUIRE( properties->getPropertyValue( name, value ) );
        return value;
    }

    // Records the component/service boundary without modifying renderer state.
    class RecordingAtmosphere : public procedural::ISkyAtmosphere
    {
    public:
        procedural::SkyState state;
        procedural::SkyResults results;
        int updates = 0;
        bool fail = false;

        void setState( const procedural::SkyState &value ) override { state = value; }
        const procedural::SkyState &getState() const override { return state; }
        const procedural::SkyResults &getResults() const override { return results; }
        void update() override
        {
            ++updates;
            if( fail )
                throw std::runtime_error( "atmosphere test failure" );
        }
        void computeCelestialPositions( real_Num, real_Num, real_Num,
                                       Vector3<real_Num> &, Vector3<real_Num> & ) override {}
        procedural::Colour computeSkyColour( const Vector3<real_Num> &,
            const Vector3<real_Num> &, real_Num, real_Num ) override { return {}; }
        procedural::Colour computeSunColour( real_Num, real_Num ) override { return {}; }
        procedural::Colour computeFogColour( real_Num ) override { return {}; }
    };
}

BOOST_FIXTURE_TEST_CASE( procedural_components_road_properties, ProceduralComponentFixture )
{
    scene::ProceduralRoad road;
    auto input = road.getProperties();
    input->setProperty( "Seed", u32( 1234 ) );
    input->setProperty( "Start", Vector3<real_Num>( 2, 3, 4 ) );
    input->setProperty( "End", Vector3<real_Num>( 12, 3, -40 ) );
    input->setProperty( "Road Class", String( "Alley" ) );
    input->setProperty( "Surface", String( "Dirt" ) );
    input->setProperty( "Sidewalks", false );
    input->setProperty( "Kerbs", false );
    input->setProperty( "Markings", false );
    input->setProperty( "Dressing", true );
    road.setProperties( input );

    auto output = road.getProperties();
    BOOST_CHECK_EQUAL( propertyValue<u32>( output, "Seed" ), 1234u );
    BOOST_CHECK( propertyValue<Vector3<real_Num>>( output, "Start" ) == Vector3<real_Num>( 2, 3, 4 ) );
    BOOST_CHECK( propertyValue<Vector3<real_Num>>( output, "End" ) == Vector3<real_Num>( 12, 3, -40 ) );
    BOOST_CHECK_EQUAL( propertyValue<String>( output, "Road Class" ), "Alley" );
    BOOST_CHECK_EQUAL( propertyValue<String>( output, "Surface" ), "Dirt" );
    BOOST_CHECK( !propertyValue<bool>( output, "Sidewalks" ) );
    BOOST_CHECK( !propertyValue<bool>( output, "Kerbs" ) );
    BOOST_CHECK( !propertyValue<bool>( output, "Markings" ) );
    BOOST_CHECK( propertyValue<bool>( output, "Dressing" ) );

    input->setProperty( "Road Class", String( "unknown" ) );
    input->setProperty( "Surface", String( "unknown" ) );
    road.setProperties( input );
    road.setProperties( nullptr );
    BOOST_CHECK_EQUAL( propertyValue<String>( road.getProperties(), "Road Class" ), "Alley" );
    BOOST_CHECK_EQUAL( propertyValue<String>( road.getProperties(), "Surface" ), "Dirt" );
}

BOOST_FIXTURE_TEST_CASE( procedural_components_texture_resolution, ProceduralComponentFixture )
{
    scene::ProceduralSurfaceTexture texture;
    const u32 requested[] = { 0, 15, 16, 31, 63, 128, 1000, 2049 };
    const u32 expected[] = { 16, 16, 16, 16, 32, 128, 512, 2048 };
    for( size_t i = 0; i < sizeof( requested ) / sizeof( requested[0] ); ++i )
    {
        BOOST_TEST_CONTEXT( "requested resolution " << requested[i] )
        {
            auto input = texture.getProperties();
            input->setProperty( "Resolution", requested[i] );
            texture.setProperties( input );
            BOOST_CHECK_EQUAL( propertyValue<u32>( texture.getProperties(), "Resolution" ), expected[i] );
        }
    }
}

BOOST_FIXTURE_TEST_CASE( procedural_components_texture_properties, ProceduralComponentFixture )
{
    scene::ProceduralSurfaceTexture texture;
    auto input = texture.getProperties();
    input->setProperty( "Seed", u32( 99 ) );
    input->setProperty( "Surface", String( "Rubber" ) );
    input->setProperty( "Tile UV", real_Num( 3 ) );
    input->setProperty( "sRGB Albedo", false );
    input->setProperty( "Parallax", real_Num( 0.25 ) );
    input->setProperty( "Detail Scale", real_Num( 4 ) );
    texture.setProperties( input );
    texture.setProperties( nullptr );
    auto output = texture.getProperties();
    BOOST_CHECK_EQUAL( propertyValue<u32>( output, "Seed" ), 99u );
    BOOST_CHECK_EQUAL( propertyValue<String>( output, "Surface" ), "Rubber" );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Tile UV" ), 3 );
    BOOST_CHECK( !propertyValue<bool>( output, "sRGB Albedo" ) );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Parallax" ), real_Num( 0.25 ) );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Detail Scale" ), 4 );
}

BOOST_FIXTURE_TEST_CASE( procedural_components_sky_bounds, ProceduralComponentFixture )
{
    scene::ProceduralSky sky;
    auto input = sky.getProperties();
    input->setProperty( "Time Of Day", real_Num( 30 ) );
    input->setProperty( "Day Of Year", real_Num( -1 ) );
    input->setProperty( "Latitude", real_Num( -100 ) );
    input->setProperty( "Turbidity", real_Num( 50 ) );
    input->setProperty( "Sun Intensity", real_Num( -5 ) );
    input->setProperty( "Weather", String( "Stormy" ) );
    sky.setProperties( input );
    auto output = sky.getProperties();
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Time Of Day" ), 24 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Day Of Year" ), 0 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Latitude" ), -90 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Turbidity" ), 20 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Sun Intensity" ), 0 );
    BOOST_CHECK_EQUAL( propertyValue<String>( output, "Weather" ), "Stormy" );
}

BOOST_FIXTURE_TEST_CASE( procedural_components_sky_nonfinite, ProceduralComponentFixture )
{
    scene::ProceduralSky sky;
    auto input = sky.getProperties();
    const auto nan = std::numeric_limits<real_Num>::quiet_NaN();
    const auto inf = std::numeric_limits<real_Num>::infinity();
    input->setProperty( "Time Of Day", nan );
    input->setProperty( "Day Of Year", inf );
    input->setProperty( "Latitude", -inf );
    input->setProperty( "Turbidity", nan );
    input->setProperty( "Sun Intensity", inf );
    sky.setProperties( input );
    sky.setProperties( nullptr );
    auto output = sky.getProperties();
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Time Of Day" ), 14 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Day Of Year" ), 172 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Latitude" ), 32 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Turbidity" ), 2 );
    BOOST_CHECK_EQUAL( propertyValue<real_Num>( output, "Sun Intensity" ), real_Num( 1.5 ) );
}

BOOST_FIXTURE_TEST_CASE( procedural_components_sky_service_recovery, ProceduralComponentFixture )
{
    scene::ProceduralSky sky;
    auto service = workphone::make_ptr<RecordingAtmosphere>();
    sky.setAtmosphere( service );
    auto input = sky.getProperties();
    input->setProperty( "Apply Lighting", false );
    input->setProperty( "Apply Fog", false );
    input->setProperty( "Time Of Day", real_Num( 8 ) );
    input->setProperty( "Weather", String( "Overcast" ) );
    sky.setProperties( input );
    service->results.keyIntensity = 7;
    BOOST_REQUIRE( sky.regenerate() );
    BOOST_CHECK_EQUAL( service->updates, 1 );
    BOOST_CHECK_EQUAL( service->state.timeOfDay, 8 );
    BOOST_CHECK( service->state.weather == procedural::Weather::Overcast );
    BOOST_CHECK_EQUAL( sky.getResults().keyIntensity, 7 );

    service->fail = true;
    BOOST_CHECK( !sky.regenerate() );
    BOOST_CHECK_EQUAL( propertyValue<String>( sky.getProperties(), "Generation Error" ),
                       "atmosphere test failure" );
    BOOST_CHECK_EQUAL( sky.getResults().keyIntensity, 7 );
    service->fail = false;
    service->results.keyIntensity = 9;
    BOOST_REQUIRE( sky.regenerate() );
    BOOST_CHECK_EQUAL( sky.getResults().keyIntensity, 9 );
    BOOST_CHECK( propertyValue<String>( sky.getProperties(), "Generation Error" ).empty() );
}

BOOST_FIXTURE_TEST_CASE( procedural_components_vehicle_properties, ProceduralComponentFixture )
{
    scene::ProceduralVehicle vehicle;
    auto input = vehicle.getProperties();
    input->setProperty( "Seed", u32( 456 ) );
    input->setProperty( "Physics Preset", String( "GT" ) );
    input->setProperty( "Appearance Quality", String( "High" ) );
    input->setProperty( "Texture Resolution", u32( 8192 ) );
    input->setProperty( "LOD", u32( 8 ) );
    input->setProperty( "Wheelbase", real_Num( 3 ) );
    input->setProperty( "Three LODs", false );
    vehicle.setProperties( input );
    vehicle.setProperties( nullptr );
    const auto &config = vehicle.getConfig();
    BOOST_CHECK_EQUAL( config.seed, 456u );
    BOOST_CHECK_EQUAL( config.appearance.customMaxResolution, 2048u );
    BOOST_CHECK_EQUAL( config.geometry.wheelbase, 3 );
    BOOST_CHECK( !config.geometry.generateThreeLODs );
    auto output = vehicle.getProperties();
    BOOST_CHECK_EQUAL( propertyValue<u32>( output, "LOD" ), 2u );
    BOOST_CHECK_EQUAL( propertyValue<String>( output, "Physics Preset" ), "GT" );
    BOOST_CHECK_EQUAL( propertyValue<String>( output, "Appearance Quality" ), "High" );
}

BOOST_FIXTURE_TEST_CASE( procedural_components_detached_meshes, ProceduralComponentFixture )
{
    scene::ProceduralRoad road;
    scene::ProceduralVehicle vehicle;
    BOOST_CHECK( !road.regenerate() );
    BOOST_CHECK( !vehicle.regenerate() );
    BOOST_CHECK( road.getResult().roadSurface.indices.empty() );
    BOOST_CHECK( !vehicle.getGeneratedVehicle().geometry.hasGeometry() );
    BOOST_CHECK( !vehicle.stepFixed( {}, {}, 1.0 / 60.0 ) );
    BOOST_CHECK( vehicle.getEffects().empty() );
}

#endif
