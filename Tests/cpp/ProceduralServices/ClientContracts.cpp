// This translation unit deliberately includes no WPProcedural implementation header.
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Interface/Procedural/IVehicleAppearance.hpp>
#include <Workphone/Interface/Procedural/IVehicleDamage.hpp>
#include <Workphone/Interface/Procedural/IVehicleDynamics.hpp>
#include <Workphone/Interface/Procedural/IVehicleEffects.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/IVehicleGeometry.hpp>
#include <Workphone/Interface/Procedural/IVehiclePhysics.hpp>
#include <Workphone/Interface/Procedural/IVehiclePresentation.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Scene/Components/ProceduralVehicle.hpp>
#include <Workphone/Scene/Components/ProceduralRoad.hpp>
#include <Workphone/Scene/Components/ProceduralSky.hpp>
#include <Workphone/Scene/Components/ProceduralSurfaceTexture.hpp>
#include <Workphone/Core/Properties.hpp>
#include <stdexcept>
#include <type_traits>
#include <cmath>
using namespace workphone;
using namespace workphone::procedural;
namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }
    template <class T>
    SmartPtr<T> obtain( SmartPtr<IFactoryManager> factories, const String &name )
    {
        static_assert( std::is_abstract_v<T>, "Procedural service must be abstract" );
        auto result = factories->createObjectFromType<T>( name );
        require( static_cast<bool>( result ), "Missing default procedural service factory" );
        return result;
    }
}  // namespace
void checkClientContracts( SmartPtr<IFactoryManager> factories )
{
    auto roads = obtain<IRoadSystem>( factories, "IRoadSystem" );
    auto sky = obtain<ISkyAtmosphere>( factories, "ISkyAtmosphere" );
    auto textures = obtain<ITextureForge>( factories, "ITextureForge" );
    auto appearance = obtain<IVehicleAppearance>( factories, "IVehicleAppearance" );
    auto damage = obtain<IVehicleDamage>( factories, "IVehicleDamage" );
    auto dynamics = obtain<IVehicleDynamics>( factories, "IVehicleDynamics" );
    auto effects = obtain<IVehicleEffects>( factories, "IVehicleEffects" );
    auto generator = obtain<IVehicleGenerator>( factories, "IVehicleGenerator" );
    auto geometry = obtain<IVehicleGeometry>( factories, "IVehicleGeometry" );
    auto physics = obtain<IVehiclePhysics>( factories, "IVehiclePhysics" );
    auto presentation = obtain<IVehiclePresentation>( factories, "IVehiclePresentation" );
    VehicleGenerationConfig request;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    auto first = generator->generate( request );
    auto second = generator->generate( request );
    require( first.isValid() && first.geometry.hasGeometry(), "Generated vehicle is invalid" );
    require( generator->validate( first ).empty(), "Generated vehicle validation failed" );
    require( first.contentHash == second.contentHash, "Service generation is not deterministic" );
    const auto generatedGeometry = geometry->generate();
    require( generatedGeometry.hasGeometry(), "Geometry service returned no geometry" );
    for( const auto &lod : generatedGeometry.lods )
    {
        std::array<size_t, 4> tyreTriangles{};
        for( const auto &section : lod.sections )
        {
            if( section.material != VehicleMaterial::Tyre )
                continue;
            for( size_t i = 0; i < section.indices.size(); i += 3 )
            {
                const auto centre =
                    ( section.vertices.at( section.indices.at( i ) ).position +
                      section.vertices.at( section.indices.at( i + 1 ) ).position +
                      section.vertices.at( section.indices.at( i + 2 ) ).position ) / 3.f;
                const auto midAxleZ =
                    ( generatedGeometry.frontAxle.z + generatedGeometry.rearAxle.z ) * .5f;
                const size_t wheel = ( centre.z > midAxleZ ? 2u : 0u ) +
                                     ( centre.x > 0.f ? 1u : 0u );
                ++tyreTriangles[wheel];
            }
        }
        require( tyreTriangles[0] > 0 && tyreTriangles[1] == tyreTriangles[0] &&
                     tyreTriangles[2] == tyreTriangles[0] && tyreTriangles[3] == tyreTriangles[0],
                 "Indexed tyre triangles must cover all four wheels at every LOD" );
    }
    require( physics->validate( first.physics ).isValid(),
             "Physics service returned invalid configuration" );
    require( appearance->validate( first.appearance ).valid,
             "Appearance service returned invalid assets" );
    const auto &livery=first.appearance.textures.bodyLivery;
    bool hasPaint=false,hasContrast=false;
    for(size_t i=0;i<livery.pixels.size();i+=4)
    {
        hasPaint |= livery.pixels[i]>livery.pixels[i+1]+20;
        hasContrast |= livery.pixels[i]<80;
    }
    require(hasPaint && hasContrast,"Generated livery lost its paint colour or contrast");
    auto state = dynamics->reset( first.physics );
    VehicleDynamicsTelemetry telemetry;
    std::array<VehicleSurfaceSample, 4> surfaces;
    VehicleControlInput input;
    input.throttle = 0.8;
    require( dynamics->stepFixed( first.physics, {}, input, surfaces, 1.0 / 120.0, state, telemetry ),
             "Dynamics adapter failed" );
    auto damageState = damage->reset( 17 );
    require( damage->update( {}, 1.0 / 120.0, damageState ), "Damage adapter failed" );
    auto damageTelemetry = damage->telemetry( {}, damageState );
    auto pose = presentation->reset( first.physics );
    require( presentation->update( {}, first.physics, state, telemetry, damageTelemetry, {}, 1.0 / 120.0,
                                   pose ),
             "Presentation adapter failed" );
    auto effectState = effects->reset( 17 );
    std::vector<VehicleEffectEvent> events;
    require( effects->emit( {}, first.physics, state, telemetry, pose, damageTelemetry, {}, 1.0 / 120.0,
                            effectState, events ),
             "Effects adapter failed" );
    SurfaceBakeParams params;
    params.size = 16;
    textures->setSeed( 42 );
    auto texture = textures->bakeSurface( SurfaceTag::Concrete, params );
    textures->setSeed( 42 );
    auto repeat = textures->bakeSurface( SurfaceTag::Concrete, params );
    require( texture.albedo.pixels == repeat.albedo.pixels && texture.albedo.width == 16,
             "Seeded texture service changed output" );
    sky->setState( {} );
    sky->update();
    require( std::isfinite( sky->getResults().sunAltitude ), "Sky service produced invalid results" );
    RoadSegmentSpec segment;
    segment.end = { 0, 0, -10 };
    segment.generateDressing = false;
    auto road = roads->generateSegment( segment );
    require( !road.roadSurface.indices.empty(), "Road service returned no triangles" );
    for( auto index : road.roadSurface.indices )
        require( index < road.roadSurface.vertices.size(),
                 "Road service returned an out-of-range index" );

    auto vehicleComponent = workphone::make_ptr<scene::ProceduralVehicle>();
    auto properties = vehicleComponent->getProperties();
    properties->setProperty( "Seed", u32( 123 ) );
    properties->setProperty( "Appearance Quality", String( "Preview" ) );
    vehicleComponent->setProperties( properties );
    require( vehicleComponent->getConfig().seed == 123 &&
                 vehicleComponent->getConfig().appearance.quality == VehicleAppearanceQuality::Preview,
             "Vehicle editor properties did not deserialize" );
    auto clonedVehicle = workphone::make_ptr<scene::ProceduralVehicle>();
    clonedVehicle->setProperties( vehicleComponent->getProperties() );
    require( clonedVehicle->getConfig().seed == 123, "Vehicle editor properties did not round-trip" );
    require( !clonedVehicle->regenerate(), "Detached component attempted mesh publication" );
    auto textureComponent = workphone::make_ptr<scene::ProceduralSurfaceTexture>();
    properties = textureComponent->getProperties();
    properties->setProperty( "Resolution", u32( 31 ) );
    properties->setProperty( "Surface", String( "Concrete" ) );
    textureComponent->setProperties( properties );
    require( textureComponent->regenerate() && textureComponent->getResult().albedo.width == 16,
             "Surface editor resolution was not bounded to a power of two" );
    auto skyComponent = workphone::make_ptr<scene::ProceduralSky>();
    skyComponent->setAtmosphere( sky );
    properties = skyComponent->getProperties();
    properties->setProperty( "Latitude", real_Num( 1000 ) );
    skyComponent->setProperties( properties );
    require( skyComponent->regenerate() && std::isfinite( skyComponent->getResults().sunAltitude ),
             "Sky editor properties were not bounded" );
    auto roadComponent = workphone::make_ptr<scene::ProceduralRoad>();
    properties = roadComponent->getProperties();
    properties->setProperty( "Road Class", String( "Highway" ) );
    roadComponent->setProperties( properties );
    String roadClass;
    roadComponent->getProperties()->getPropertyValue( "Road Class", roadClass );
    require( roadClass == "Highway", "Road editor dropdown did not round-trip" );
}
