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
#include <Workphone/Interface/Procedural/OpenCityDebugBlocks.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Scene/Components/ProceduralVehicle.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Scene/Components/ProceduralCityScene.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Graphics/ProceduralSurfaceSourceExport.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <chrono>
#include <fstream>
#include <iterator>
#include <set>
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
    require( !race->getAssets().cityStreamingEnabled && !race->getAssets().cityStreaming,
             "The default circuit does not create a city streaming controller" );
    auto city = workphone::make_ptr<scene::ProceduralCityScene>();
    auto cityOptions = city->getProperties();
    s32 buildingDensity = -1;
    require( cityOptions->getPropertyValue( "City Building Density", buildingDensity ) && buildingDensity == 150,
             "New city scenes should pack suitable parcels by default" );
    s32 streetPropDensity = -1;
    require( cityOptions->getPropertyValue( "City Street Prop Density", streetPropDensity ) && streetPropDensity == 150,
             "New city scenes should have the populated street prop preset" );
    bool debugBlocks = true, debugLots = true;
    require( cityOptions->getPropertyValue( "City Debug Blocks", debugBlocks ) && !debugBlocks &&
                 cityOptions->getPropertyValue( "City Debug Lots", debugLots ) && !debugLots,
             "City layout debug geometry should be off by default" );
    OpenCityLayout square;
    square.naturalRoads = true;
    square.roads.nodes = { { 0, 0 }, { 20, 0 }, { 20, 20 }, { 0, 20 } };
    square.roads.edges = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 } };
    const auto squareFaces = openCityDebugBlocks( square );
    require( squareFaces.size() == 1 && squareFaces.front().size() == 4,
             "Natural city debug blocks must select the bounded road face" );
    const auto insetSquare = insetOpenCityDebugBlock( squareFaces.front(), 3.f );
    require( insetSquare.size() == 4 && insetSquare.front().x > 0 && insetSquare.front().z > 0,
             "City debug block outlines must sit inside the road graph face" );
    const auto gridFaces = openCityDebugBlocks( OpenCityLayout::generate( 42, 6, 0, false ) );
    require( gridFaces.size() == 36, "Grid city debug blocks must match the planning cells" );
    size_t sparse = 0, dense = 0, central = 0, peripheral = 0;
    for( u64 id = 1; id <= 1000; ++id )
    {
        const bool low = scene::procedural_scene::selectCityBuilding( id, 42, .5f, 25 );
        const bool high = scene::procedural_scene::selectCityBuilding( id, 42, .5f, 75 );
        require( !low || high, "Increasing density must never remove a selected building" );
        require( !scene::procedural_scene::selectCityBuilding( id, 42, .5f, 0 ) &&
                 scene::procedural_scene::selectCityBuilding( id, 42, .5f, 100 ),
                 "City density endpoints must select no lots or all lots" );
        sparse += low; dense += high;
        central += scene::procedural_scene::selectCityBuilding( id, 42, 0.f, 45 );
        peripheral += scene::procedural_scene::selectCityBuilding( id, 42, 1.f, 45 );
    }
    require( sparse < dense && central > peripheral,
             "Density must change building count while favoring the city centre" );
    auto fullCity = procedural::OpenCityLayout::generate( 42, 8, 0, true );
    auto sparseCity = fullCity;
    const auto originalBuildings = std::count_if( fullCity.lots.begin(), fullCity.lots.end(),
        []( const auto &lot ) { return !lot.park; } );
    scene::procedural_scene::applyCityBuildingDensity( sparseCity, 42, 40 );
    const auto remainingBuildings = std::count_if( sparseCity.lots.begin(), sparseCity.lots.end(),
        []( const auto &lot ) { return !lot.park; } );
    require( remainingBuildings > 0 && remainingBuildings < originalBuildings &&
             sparseCity.roads.edges.size() == fullCity.roads.edges.size() &&
             sparseCity.lots.size() == fullCity.lots.size(),
             "City density must change building occupancy without changing roads or parcels" );
    auto packedCity = fullCity;
    scene::procedural_scene::applyCityBuildingDensity( packedCity, 42, 200 );
    const auto packedBuildings = std::count_if( packedCity.lots.begin(), packedCity.lots.end(),
        []( const auto &lot ) { return !lot.park; } );
    require( packedBuildings > originalBuildings &&
             packedCity.roads.edges.size() == fullCity.roads.edges.size(),
             "Density above 100 must add buildings without rebuilding roads" );
    std::set<u64> packedIds;
    for( const auto &lot : packedCity.lots )
    {
        require( packedIds.insert( lot.id ).second, "Packed city lots need distinct stable identities" );
        if( lot.park ) continue;
        const auto parent = std::find_if( fullCity.lots.begin(), fullCity.lots.end(),
            [&]( const auto &original ) {
                return !original.park &&
                    lot.parcelX - lot.parcelWidth * .5f >= original.parcelX - original.parcelWidth * .5f - .001f &&
                    lot.parcelX + lot.parcelWidth * .5f <= original.parcelX + original.parcelWidth * .5f + .001f &&
                    lot.parcelZ - lot.parcelDepth * .5f >= original.parcelZ - original.parcelDepth * .5f - .001f &&
                    lot.parcelZ + lot.parcelDepth * .5f <= original.parcelZ + original.parcelDepth * .5f + .001f;
            } );
        require( parent != fullCity.lots.end(), "Packed building escaped its original supported parcel" );
    }
    for( size_t i = 0; i < sparseCity.lots.size(); ++i )
        if( !fullCity.lots[i].park && sparseCity.lots[i].park )
            require( sparseCity.lots[i].x == sparseCity.lots[i].parcelX &&
                     sparseCity.lots[i].width == sparseCity.lots[i].parcelWidth,
                     "Vacant building lots must become complete landscape parcels" );
    bool cityStreamingEnabled = false;
    f32 streamLoad = 0, streamRetain = 0;
    s32 cityMemory = 0;
    require( cityOptions->getPropertyValue( "City Streaming", cityStreamingEnabled ) && cityStreamingEnabled &&
                 cityOptions->getPropertyValue( "City Stream Load Radius", streamLoad ) && streamLoad == 300.f &&
                 cityOptions->getPropertyValue( "City Stream Retain Radius", streamRetain ) && streamRetain == 420.f &&
                 cityOptions->getPropertyValue( "City Source Memory MiB", cityMemory ) && cityMemory == 1024,
             "City streaming defaults did not reach the editor properties" );
    s32 foliageResolution = -1, buildingResolution = -1;
    require( cityOptions->getPropertyValue( "City Foliage Impostor Resolution", foliageResolution ) && foliageResolution == 0 &&
             cityOptions->getPropertyValue( "City Building Impostor Resolution", buildingResolution ) && buildingResolution == 0,
             "City imposter resolutions must default to the appearance-quality preset" );
    cityOptions->setProperty( "City Streaming", false );
    cityOptions->setProperty( "City Stream Load Radius", 210.f );
    cityOptions->setProperty( "City Stream Retain Radius", 360.f );
    cityOptions->setProperty( "City Source Memory MiB", s32( 640 ) );
    cityOptions->setProperty( "City Tree LOD0 Distance", 74.f );
    cityOptions->setProperty( "City Tree LOD1 Distance", 310.f );
    cityOptions->setProperty( "City Shrub LOD0 Distance", 24.f );
    cityOptions->setProperty( "City Grass LOD1 Distance", 42.f );
    cityOptions->setProperty( "City Foliage Impostor Resolution", s32( 3 ) );
    cityOptions->setProperty( "City Building Impostor Resolution", s32( 1 ) );
    cityOptions->setProperty( "City Building Density", s32( 160 ) );
    cityOptions->setProperty( "City Street Prop Density", s32( 65 ) );
    cityOptions->setProperty( "City Debug Blocks", true );
    cityOptions->setProperty( "City Debug Lots", true );
    city->setProperties( cityOptions );
    auto restoredCity = workphone::make_ptr<scene::ProceduralCityScene>();
    restoredCity->setProperties( city->getProperties() );
    auto savedOptions = restoredCity->getProperties();
    require( savedOptions->getPropertyValue( "City Streaming", cityStreamingEnabled ) && !cityStreamingEnabled &&
                 savedOptions->getPropertyValue( "City Stream Load Radius", streamLoad ) && streamLoad == 210.f &&
                 savedOptions->getPropertyValue( "City Stream Retain Radius", streamRetain ) && streamRetain == 360.f &&
                 savedOptions->getPropertyValue( "City Source Memory MiB", cityMemory ) && cityMemory == 640,
             "City streaming settings did not round-trip" );
    f32 treeNear=0,treeFar=0,shrubNear=0,grassFar=0;
    require( savedOptions->getPropertyValue( "City Tree LOD0 Distance", treeNear ) && treeNear==74.f &&
             savedOptions->getPropertyValue( "City Tree LOD1 Distance", treeFar ) && treeFar==310.f &&
             savedOptions->getPropertyValue( "City Shrub LOD0 Distance", shrubNear ) && shrubNear==24.f &&
             savedOptions->getPropertyValue( "City Grass LOD1 Distance", grassFar ) && grassFar==42.f,
             "City foliage distances did not survive the component property round-trip" );
    require( savedOptions->getPropertyValue( "City Foliage Impostor Resolution", foliageResolution ) && foliageResolution == 3 &&
             savedOptions->getPropertyValue( "City Building Impostor Resolution", buildingResolution ) && buildingResolution == 1,
             "City imposter resolutions did not survive the component property round-trip" );
    require( savedOptions->getPropertyValue( "City Building Density", buildingDensity ) && buildingDensity == 160,
             "City building density did not survive the component property round-trip" );
    require( savedOptions->getPropertyValue( "City Street Prop Density", streetPropDensity ) && streetPropDensity == 65,
             "City street prop density did not survive the component property round-trip" );
    require( savedOptions->getPropertyValue( "City Debug Blocks", debugBlocks ) && debugBlocks &&
                 savedOptions->getPropertyValue( "City Debug Lots", debugLots ) && debugLots,
             "City layout debug toggles did not survive the component property round-trip" );
    savedOptions->setProperty( "City Street Prop Density", s32( 201 ) );
    restoredCity->setProperties( savedOptions );
    savedOptions = restoredCity->getProperties();
    require( savedOptions->getPropertyValue( "City Street Prop Density", streetPropDensity ) && streetPropDensity == 65,
             "Invalid street prop density must preserve the previous setting" );
    savedOptions->setProperty( "City Building Density", s32( 201 ) );
    restoredCity->setProperties( savedOptions );
    savedOptions = restoredCity->getProperties();
    require( savedOptions->getPropertyValue( "City Building Density", buildingDensity ) && buildingDensity == 160,
             "Invalid building density must preserve the previous setting" );
    savedOptions->setProperty( "City Building Impostor Resolution", s32( 4 ) );
    restoredCity->setProperties( savedOptions );
    savedOptions = restoredCity->getProperties();
    require( savedOptions->getPropertyValue( "City Building Impostor Resolution", buildingResolution ) && buildingResolution == 1,
             "Invalid building imposter resolution must preserve the previous setting" );
    savedOptions->setProperty( "City Tree LOD1 Distance", 10.f );
    restoredCity->setProperties( savedOptions );
    savedOptions = restoredCity->getProperties();
    require( savedOptions->getPropertyValue( "City Tree LOD1 Distance", treeFar ) && treeFar==310.f,
             "Invalid foliage distance must preserve the previous settings" );
    savedOptions->setProperty( "City Stream Retain Radius", 150.f );
    savedOptions->setProperty( "City Source Memory MiB", s32( 8192 ) );
    restoredCity->setProperties( savedOptions );
    savedOptions = restoredCity->getProperties();
    String cityError;
    require( savedOptions->getPropertyValue( "City Stream Retain Radius", streamRetain ) && streamRetain == 360.f &&
                 savedOptions->getPropertyValue( "City Source Memory MiB", cityMemory ) && cityMemory == 4096 &&
                 savedOptions->getPropertyValue( "Generation Error", cityError ) && !cityError.empty(),
             "Invalid city radii must preserve the last valid settings and memory must stay bounded" );
    require( !scene::procedural_scene::prepareCitySourceMemoryBudget( 127, cityError ) && !cityError.empty() &&
                 !scene::procedural_scene::prepareCitySourceMemoryBudget( 4097, cityError ) && !cityError.empty(),
             "Invalid direct city budgets were accepted" );
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
