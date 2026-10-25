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
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Graphics/ProceduralSurfaceSourceExport.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <chrono>
#include <fstream>
#include <iterator>
#include <Workphone/Scene/Components/ProceduralRoad.hpp>
#include <Workphone/Scene/Components/ProceduralSky.hpp>
#include <Workphone/Scene/Components/ProceduralSurfaceTexture.hpp>
#include <Workphone/Core/Properties.hpp>
#include <stdexcept>
#include <type_traits>
#include <cmath>
#include <limits>
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
    bool sameVector( const VehiclePhysicsVector3 &a, const VehiclePhysicsVector3 &b )
    { return a.x == b.x && a.y == b.y && a.z == b.z; }
    bool sameEffectState( const VehicleEffectsState &a, const VehicleEffectsState &b )
    {
        if( a.randomState != b.randomState || a.exhaustAccumulator != b.exhaustAccumulator ||
            a.damageSmokeAccumulator != b.damageSmokeAccumulator ) return false;
        for( size_t i = 0; i < 4; ++i )
            if( !sameVector( a.previousContact[i], b.previousContact[i] ) ||
                a.hasPreviousContact[i] != b.hasPreviousContact[i] ||
                a.smokeAccumulator[i] != b.smokeAccumulator[i] || a.sprayAccumulator[i] != b.sprayAccumulator[i] ) return false;
        return true;
    }
    bool sameEffect( const VehicleEffectEvent &a, const VehicleEffectEvent &b )
    {
        return a.type == b.type && a.wheel == b.wheel && sameVector( a.position, b.position ) &&
            sameVector( a.endPosition, b.endPosition ) && sameVector( a.velocity, b.velocity ) &&
            sameVector( a.linearColor, b.linearColor ) && a.intensity == b.intensity &&
            a.sizeM == b.sizeM && a.lifetimeSeconds == b.lifetimeSeconds;
    }
    void checkEffectBudgetContracts( SmartPtr<IVehicleEffects> effects, const VehiclePhysicsConfig &physics )
    {
        auto scheduler = WorldStreamingScheduler::shared();
        constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto original = scheduler->stats().limits[source];
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, original };
        const auto used = [&] { return scheduler->stats().used[source]; };
        const auto limit = [&]( u64 value ) {
            String error; require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ), "Effect Source budget applies" );
        };
        const auto storage = []( size_t count ) { return sourceBudgetStorageBytes( count * sizeof(VehicleEffectEvent), alignof(VehicleEffectEvent) ); };
        VehicleDynamicsState dynamics;
        VehicleDynamicsTelemetry telemetry; telemetry.speedMps = 20;
        for( auto &wheel : telemetry.wheels ) { wheel.grounded = true; wheel.longitudinalSlipRatio = 1; }
        VehiclePresentationState presentation; presentation.exhaustIntensity = 1;
        VehicleDamageTelemetry damage; damage.smoke = 1;
        const auto initial = [&] {
            auto value = effects->reset( 73 ); value.exhaustAccumulator = .9;
            for( size_t i = 0; i < 4; ++i ) { value.hasPreviousContact[i] = true; value.smokeAccumulator[i] = .9; }
            return value;
        }();
        VehicleEffectEvent prefix; prefix.type = VehicleEffectType::BrakeSpark; prefix.intensity = 123;
        const auto fixtureBase = used();
        Array<VehicleEffectEvent> expected; expected.reserve( 31 ); expected.push_back( prefix );
        auto expectedState = initial;
        require( effects->emit( {}, physics, dynamics, telemetry, presentation, damage, {}, .1, expectedState, expected ) &&
                     expected.size() > 1 && expected.size() <= 31 && expected.get_allocator().isSourceBudgeted(),
                 "Effect reference emission owns a bounded admitted generation" );

        Array<VehicleEffectEvent> events; events.reserve( 31 ); events.push_back( prefix );
        auto state = initial;
        const auto baseline = used(); const auto capacityCost = storage( events.capacity() );
        const auto old = events.data();
        limit( baseline + capacityCost - 1 ); bool rejected = false;
        try { effects->emit( {}, physics, dynamics, telemetry, presentation, damage, {}, .1, state, events ); }
        catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected && events.data() == old && events.size() == 1 && events[0].intensity == 123 &&
                     !events.get_allocator().isSourceBudgeted() && sameEffectState( state, initial ) && used() == baseline,
                 "Denied foreign event canonicalization preserves caller buffer, RNG and every accumulator" );
        limit( baseline + capacityCost );
        require( effects->emit( {}, physics, dynamics, telemetry, presentation, damage, {}, .1, state, events ) &&
                     events.size() == expected.size() && events.capacity() == 31 && events.get_allocator().isSourceBudgeted() &&
                     sameEffectState( state, expectedState ) && used() == baseline + capacityCost,
                 "Exact Source admission normalizes event spare capacity and retries the unconsumed frame" );
        for( size_t i = 0; i < events.size(); ++i ) require( sameEffect( events[i], expected[i] ), "Retried effects preserve deterministic event values" );

        events.clear(); state = initial; const auto warm = events.data();
        limit( used() ); const auto denials = sourceBudgetDenialCount(); const auto warmUsed = used();
        require( effects->emit( {}, physics, dynamics, telemetry, presentation, damage, {}, .1, state, events ) &&
                     events.data() == warm && events.size() + 1 == expected.size() && sameEffectState( state, expectedState ) &&
                     used() == warmUsed && sourceBudgetDenialCount() == denials,
                 "Warm Source event appends reuse physical capacity at a full quota" );
        for( size_t i = 0; i < events.size(); ++i ) require( sameEffect( events[i], expected[i + 1] ), "Warm effects match cold generation without advancing RNG twice" );
        events.clear(); auto quietState = effects->reset( 7 ); VehicleEffectConfig quiet;
        quiet.maximumEventsPerStep = std::numeric_limits<size_t>::max();
        require( effects->emit( quiet, physics, dynamics, {}, {}, {}, {}, .1, quietState, events ) && events.empty() &&
                     events.data() == warm && used() == warmUsed && sourceBudgetDenialCount() == denials,
                 "Quiet full-cap frames allocate nothing and do not reserve arbitrary configured maxima" );

        state = initial; VehicleEffectConfig large; large.maximumEventsPerStep = 200; large.smokeEventsPerSecond = 2000;
        rejected = false;
        try { effects->emit( large, physics, dynamics, telemetry, presentation, damage, {}, .1, state, events ); }
        catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected && events.empty() && events.data() == warm && sameEffectState( state, initial ) && used() == warmUsed,
                 "Actual output beyond stack staging requires Source admission before consuming caller state" );

        limit( original ); auto small = makeSourceBudgetSnapshot<VehicleEffectEvent>(); small.reserve( 1 ); small.push_back( prefix );
        state = initial; const auto smallBefore = used(); const auto smallPointer = small.data();
        const auto finalCapacity = expected.size(); limit( smallBefore + storage( finalCapacity ) - 1 ); rejected = false;
        try { effects->emit( {}, physics, dynamics, telemetry, presentation, damage, {}, .1, state, small ); }
        catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected && small.data() == smallPointer && small.size() == 1 && sameEffect( small[0], prefix ) &&
                     sameEffectState( state, initial ) && used() == smallBefore,
                 "Denied admitted-output growth leaves previous output and simulation state unchanged" );
        limit( smallBefore + storage( finalCapacity ) );
        require( effects->emit( {}, physics, dynamics, telemetry, presentation, damage, {}, .1, state, small ) &&
                     small.size() == expected.size() && sameEffectState( state, expectedState ),
                 "Output growth retries after exact old/new overlap admission" );

        limit( original ); Array<VehicleEffectEvent> many; auto manyState = initial; const auto beforeLarge = used();
        require( effects->emit( large, physics, dynamics, telemetry, presentation, damage, {}, .1, manyState, many ) &&
                     many.size() == 200 && many.get_allocator().isSourceBudgeted() && used() == beforeLarge + storage( many.capacity() ),
                 "Admitted output beyond stack staging retires its separate scratch before return" );
        limit( used() ); const auto beforeRetirement = sourceBudgetDenialCount();
        expected = makeSourceBudgetSnapshot<VehicleEffectEvent>(); events = makeSourceBudgetSnapshot<VehicleEffectEvent>();
        small = makeSourceBudgetSnapshot<VehicleEffectEvent>(); many = makeSourceBudgetSnapshot<VehicleEffectEvent>();
        require( used() == fixtureBase && sourceBudgetDenialCount() == beforeRetirement,
                 "Final effect output owners release every physical reservation at the Source cap" );
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
    Array<VehicleEffectEvent> events;
    require( effects->emit( {}, first.physics, state, telemetry, pose, damageTelemetry, {}, 1.0 / 120.0,
                            effectState, events ),
             "Effects adapter failed" );
    checkEffectBudgetContracts( effects, first.physics );
    SurfaceBakeParams params;
    params.size = 16;
    textures->setSeed( 42 );
    auto texture = textures->bakeSurface( SurfaceTag::Concrete, params );
    textures->setSeed( 42 );
    auto repeat = textures->bakeSurface( SurfaceTag::Concrete, params );
    require( texture.albedo.pixels == repeat.albedo.pixels && texture.albedo.width == 16,
             "Seeded texture service changed output" );
    const auto exportDirectory = std::filesystem::temp_directory_path() /
        ( "workphone-asphalt-source-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count() ) );
    std::string exportError;
    require( texture_export::exportAsphaltSource( texture, exportDirectory, 42, exportError ),
             "Generated material source export failed" );
    const auto readExport = [&]( const char *file ) {
        std::ifstream stream( exportDirectory / file, std::ios::binary );
        return std::string( std::istreambuf_iterator<char>( stream ), {} );
    };
    const auto sourceTga = readExport( "asphalt-albedo.tga" );
    require( sourceTga.size() == 18 + texture.albedo.pixels.size() &&
                 static_cast<unsigned char>( sourceTga[17] ) == 0x28 &&
                 static_cast<unsigned char>( sourceTga[18] ) == texture.albedo.pixels[2] &&
                 static_cast<unsigned char>( sourceTga[20] ) == texture.albedo.pixels[0],
             "Source material export lost top-origin RGBA-to-BGRA encoding" );
    require( readExport( "asphalt-normal.texres" ).find( "\"mipFilter\":\"normal\"" ) !=
                 std::string::npos &&
                 readExport( "asphalt-roughness.texres" ).find( "\"mipFilter\":\"roughness\"" ) !=
                 std::string::npos,
             "Source material descriptors lost semantic mip filters" );
    require( readExport( "material-source.json" ).find( "\"seed\":42" ) != std::string::npos &&
                 readExport( "material-source.json" ).find( "\"auxiliaryMapsBoundByCookedMaterial\":true" ) !=
                 std::string::npos,
             "Source material metadata omitted provenance or cooked PBR tier" );
    require( !texture_export::exportAsphaltSource( texture, exportDirectory, 99, exportError ) &&
                 readExport( "asphalt-albedo.tga" ) == sourceTga,
             "Repeated material export overwrote existing source assets" );
    std::filesystem::remove_all( exportDirectory );
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

    auto namedTexture = workphone::make_ptr<render::Texture>();
    namedTexture->setName( "procedural/runtime-leaf-atlas" );
    auto textureProperties = namedTexture->getProperties();
    textureProperties->setProperty( "mipmapFilter", s32( 4 ) );
    namedTexture->setProperties( textureProperties );
    require( namedTexture->getName() == "procedural/runtime-leaf-atlas",
             "Texture mip-property roundtrip cleared the registered atlas name" );
    auto partialTextureProperties = workphone::make_ptr<Properties>();
    partialTextureProperties->setProperty( "mipmapFilter", s32( 4 ) );
    namedTexture->setProperties( partialTextureProperties );
    require( namedTexture->getName() == "procedural/runtime-leaf-atlas",
             "Partial texture property edits must preserve the registered atlas name" );
    partialTextureProperties->setProperty( "name", String( "procedural/replacement-leaf-atlas" ) );
    namedTexture->setProperties( partialTextureProperties );
    require( namedTexture->getName() == "procedural/replacement-leaf-atlas",
             "Explicit resource name property must update the graphics resource name" );

    auto race = workphone::make_ptr<scene::ProceduralRaceScene>();
    require( !race->getFoliageWorldEnabled(), "Legacy foliage selection must remain the default" );
    race->setFoliageWorldEnabled( true );
    auto restoredRace = workphone::make_ptr<scene::ProceduralRaceScene>();
    restoredRace->setProperties( race->getProperties() );
    require( restoredRace->getFoliageWorldEnabled(), "Foliage world selection did not persist" );
    restoredRace->clearGeneratedScene();
    require( restoredRace->getFoliageWorldEnabled() && !restoredRace->getAssets().foliageTerrain,
             "Clearing generated foliage reset authoring selection or retained its owner" );
    auto foliageOwner = workphone::make_ptr<render::Terrain>();
    auto foliage = std::make_shared<render::FoliageWorldData>();
    auto species = std::make_shared<render::FoliageSpecies>();
    scene::FoliagePlantRecipe plant;
    plant.wind.amplitude = .08f;
    String foliageError;
    require( scene::generateFoliagePlant( plant, *species, foliageError ),
             "Racing foliage generator returned invalid topology" );
    require( species->lods[0].indices.size() > species->lods[1].indices.size() &&
                 species->lods[1].indices.size() > species->lods[2].indices.size(),
             "Racing foliage must contain actual reduced LOD meshes" );
    foliage->species.push_back( species );
    render::FoliageInstance instance;
    instance.id = 1;
    foliage->instances.push_back( instance );
    foliage->revision = 1;
    require( foliageOwner->setFoliageSnapshot( foliage, foliageError ) &&
                 foliageOwner->getFoliageSnapshot() == foliage,
             "Foliage attachment did not retain its immutable generation" );
    auto invalidFoliage = std::make_shared<render::FoliageWorldData>( *foliage );
    invalidFoliage->instances[0].species = 99;
    require( !foliageOwner->setFoliageSnapshot( invalidFoliage, foliageError ) &&
                 foliageOwner->getFoliageSnapshot() == foliage,
             "Failed foliage publication replaced the last complete generation" );
    require( foliageOwner->setFoliageSnapshot( {}, foliageError ) &&
                 !foliageOwner->getFoliageSnapshot() &&
                 !foliageOwner->getActiveFoliage().snapshot,
             "Foliage removal retained requested or active generation" );

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
