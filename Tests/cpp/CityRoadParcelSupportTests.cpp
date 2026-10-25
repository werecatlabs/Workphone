#include <Workphone/Scene/Components/CityRoadParcelSupport.hpp>
#include <Workphone/Scene/Components/CityRoadFootways.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace workphone;
using namespace workphone::procedural;
using namespace workphone::scene;
namespace
{
    void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error( error.c_str() ); }
    std::shared_ptr<render::TerrainData> terrain()
    {
        auto data = std::make_shared<render::TerrainData>();
        data->dimensions = { 129, 129 }; data->spacing = { 1, 1 }; data->origin = { -64, -64 };
        data->heights.assign( 129 * 129, 0 ); data->revision = 23; return data;
    }
    OpenCityLayout::Lot lot( u64 id, float x, float z, float width = 8, float depth = 8 )
    {
        OpenCityLayout::Lot result{}; result.id = id; result.variant = u32( id * 17 );
        result.x = result.parcelX = x; result.z = result.parcelZ = z;
        result.width = result.parcelWidth = width; result.depth = result.parcelDepth = depth;
        result.height = 12.8f; result.groundHeight = 3; result.groundMinimumHeight = -2;
        return result;
    }
    OpenCityLayout city()
    {
        OpenCityLayout result; result.blocks = 6; result.spacing = 96; result.extent = 288; result.naturalRoads = true;
        result.route = { { 0, 0 }, { 0, -1 } }; result.roads.nodes = { { 40, 40 }, { 40, 50 } };
        result.roads.edges = { { 0, 1 } }; return result;
    }
    CityRoadSplineSource source()
    {
        CityRoadSplineSource result; result.version = 2; result.id = "10000000-0000-4000-8000-000000000001";
        result.terrainResourceUuid = "20000000-0000-4000-8000-000000000001"; result.revision = 7;
        return result;
    }
    void addRoad( CityRoadSplineSource &s, unsigned index, Vector3F a, Vector3F b, float width )
    {
        CityRoadSpline road; road.id = "30000000-0000-4000-8000-000000000001";
        road.id.back() = char( '1' + index ); road.width = width; road.heightMode = CityRoadHeightMode::Fixed;
        SourceBudgetText first = "40000000-0000-4000-8000-000000000001", second = first;
        first.back() = char( '1' + index * 2 ); second.back() = char( '2' + index * 2 );
        const auto tangent = ( b - a ) / 3.f;
        road.knots = { { first, a, {}, tangent }, { second, b, -tangent, {} } };
        s.roads.push_back( std::move( road ) );
    }
    CityRoadSplineBake sample( const CityRoadSplineSource &s )
    { CityRoadSplineBake bake; String error; check( sampleCityRoadSplines( s, {}, {}, bake, error ), error ); return bake; }
    void sameLot( const OpenCityLayout::Lot &a, const OpenCityLayout::Lot &b )
    {
        require( a.id == b.id && a.variant == b.variant && a.x == b.x && a.z == b.z && a.width == b.width &&
            a.depth == b.depth && a.height == b.height && a.park == b.park && a.district == b.district &&
            a.frontage == b.frontage && a.parcelX == b.parcelX && a.parcelZ == b.parcelZ &&
            a.parcelWidth == b.parcelWidth && a.parcelDepth == b.parcelDepth &&
            a.groundHeight == b.groundHeight && a.groundMinimumHeight == b.groundMinimumHeight,
            "Parcel output lost stable identity/profile or failed to retain last-good output" );
    }
    void sameCity( const OpenCityLayout &a, const OpenCityLayout &b )
    {
        require( a.blocks == b.blocks && a.spacing == b.spacing && a.extent == b.extent &&
            a.naturalRoads == b.naturalRoads && a.route.size() == b.route.size() &&
            a.roads.nodes.size() == b.roads.nodes.size() && a.roads.edges.size() == b.roads.edges.size() &&
            a.lots.size() == b.lots.size(), "Parcel preparation changed city metadata/counts" );
        for( size_t i = 0; i < a.route.size(); ++i ) require( a.route[i].x == b.route[i].x && a.route[i].z == b.route[i].z, "Parcel preparation changed route" );
        for( size_t i = 0; i < a.roads.nodes.size(); ++i ) require( a.roads.nodes[i].x == b.roads.nodes[i].x && a.roads.nodes[i].z == b.roads.nodes[i].z, "Parcel preparation changed graph" );
        for( size_t i = 0; i < a.roads.edges.size(); ++i ) require( a.roads.edges[i].a == b.roads.edges[i].a && a.roads.edges[i].b == b.roads.edges[i].b, "Parcel preparation changed graph edges" );
        for( size_t i = 0; i < a.lots.size(); ++i ) sameLot( a.lots[i], b.lots[i] );
    }
    void canonicalRoads()
    {
        auto input = city(); input.lots = { lot( 201, 0, 0, 12, 12 ), lot( 202, 0, 20 ), lot( 203, 30, 0 ) };
        const auto unchanged = input;
        auto s = source(); addRoad( s, 0, { -20, 0, 0 }, { 20, 0, 0 }, 4 ); auto bake = sample( s );
        OpenCityLayout output; u32 culled = 91, checks = 92; String error;
        check( prepareCityRoadParcels( input, s, bake, terrain(), {}, output, culled, checks, error ), error );
        require( culled == 1 && checks == 2 && output.lots.size() == 2 && output.lots[0].id == 202 && output.lots[1].id == 203,
            "Canonical road crossing a parcel interior was missed, or lot order changed" );
        require( output.lots[0].groundHeight == 0 && output.lots[0].groundMinimumHeight == 0,
            "Flat parcel support did not replace stale foundation heights" );
        sameCity( input, unchanged );
        require( output.route[1].z == input.route[1].z && output.roads.nodes[0].x == 40 && output.blocks == 6,
            "Canonical road filtering rewrote unrelated generated topology" );
        ++s.revision;
        for( auto &knot : s.roads[0].knots ) knot.position.z = 20;
        bake = sample( s );
        check( prepareCityRoadParcels( input, s, bake, terrain(), {}, output, culled, checks, error ), error );
        require( culled == 1 && output.lots.size() == 2 && output.lots[0].id == 201 && output.lots[1].id == 203,
            "Moved authored road retained stale generated-road parcel exclusions" );
        auto tangent = input; tangent.lots = { lot( 204, 0, 26, 6, 6 ) };
        check( prepareCityRoadParcels( tangent, s, bake, terrain(), {}, output, culled, checks, error ), error );
        require( culled == 1 && checks == 0 && output.lots.empty(), "Parcel inside the road and pedestrian reservation was accepted" );
    }
    void fullTerrain()
    {
        auto input = city(); input.lots = { lot( 301, 0, 0, 10, 8 ) }; auto s = source(); const auto bake = sample( s );
        auto data = terrain();
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x ) data->heights[z * 129 + x] = ( x - 64 ) * .02f + ( z - 64 ) * .01f;
        OpenCityLayout output; u32 culled = 0, checks = 0; String error;
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 0 && checks == 1 && output.lots.size() == 1 &&
            std::abs( output.lots[0].groundHeight - .14f ) < .0001f &&
            std::abs( output.lots[0].groundMinimumHeight + .14f ) < .0001f,
            "Sloped full-parcel minimum/maximum foundation heights are wrong" );
        data = terrain(); data->heights[65 * 129 + 65] = .2f;
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        std::cout << "Interior ridge support: lots=" << output.lots.size() << " culled=" << culled
            << " checks=" << checks << " error=" << error;
        if( !output.lots.empty() ) std::cout << " minimum=" << output.lots[0].groundMinimumHeight
            << " maximum=" << output.lots[0].groundHeight;
        std::cout << '\n';
        require( culled == 0 && checks == 1 && output.lots.size() == 1 &&
            std::abs( output.lots[0].groundHeight - .2f ) < .0001f &&
            std::abs( output.lots[0].groundMinimumHeight ) < .0001f,
            "Full support missed an interior ridge between centre/edge queries" );
        // A supported ridge must retain its interior extremum, while a sharper
        // spike must still fail the same exact terrain-cell grade policy.
        data->heights[65 * 129 + 65] = .7f;
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 1 && checks == 1 && output.lots.empty(), "Steep interior terrain spike bypassed parcel grade rejection" );
        data = terrain(); data->holes.assign( 128 * 128, 0 ); data->holes[65 * 128 + 65] = 1;
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 1 && checks == 1 && output.lots.empty(), "Interior terrain hole between coarse queries did not cull parcel" );
        data = terrain();
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x ) data->heights[z * 129 + x] = ( x - 64 ) * .15f;
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 1 && output.lots.empty(), "Relief above1.25m produced an oversized foundation" );
        auto outside = input; outside.lots[0] = lot( 302, 64, 0 );
        check( prepareCityRoadParcels( outside, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 1 && output.lots.empty(), "Parcel partly outside terrain retained unsupported corners" );
        data = terrain(); Transform3<real_Num> placement;
        placement.setPosition( { 10, 3, -5 } ); placement.setScale( { 1.5f, 2, .75f } );
        placement.setOrientation( { std::cos( .155f ), 0, std::sin( .155f ), 0 } );
        check( prepareCityRoadParcels( input, s, bake, data, placement, output, culled, checks, error ), error );
        require( output.lots.size() == 1 && std::abs( output.lots[0].groundHeight - 3 ) < .0001f &&
            std::abs( output.lots[0].groundMinimumHeight - 3 ) < .0001f, "Transformed terrain support used a second coordinate frame" );
    }
    void junctions()
    {
        auto s = source(); CityRoadRoundaboutSpec junction;
        junction.junctionId = "50000000-0000-4000-8000-000000000001";
        const float q = std::sqrt( .5f ); const Vector3F directions[] = { { q, 0, q }, { -q, 0, q }, { 0, 0, -1 } };
        for( unsigned i = 0; i < 3; ++i )
        {
            addRoad( s, i, directions[i] * 32.f, directions[i] * 48.f, 6 );
            junction.ports.push_back( { s.roads.back().id, directions[i], 6, 18 } );
        }
        s.roundabouts.push_back( junction ); const auto bake = sample( s );
        auto input = city();
        input.lots = { lot( 401, 16, 0, 4, 4 ), lot( 402, 22 * q, 22 * q, 4, 4 ),
            lot( 403, 12 * q, 32 * q, 4, 4 ) };
        OpenCityLayout output; u32 culled = 0, checks = 0; String error;
        check( prepareCityRoadParcels( input, s, bake, terrain(), {}, output, culled, checks, error ), error );
        require( culled == 2 && checks == 1 && output.lots.size() == 1 && output.lots[0].id == 403,
            "Roundabout outer-circle or rotated approach-strip parcel reservation is wrong" );
        const auto retained = output; auto invalid = s; invalid.roundabouts[0].ports[0].outward = {};
        culled = 91; checks = 92;
        require( !prepareCityRoadParcels( input, invalid, bake, terrain(), {}, output, culled, checks, error ) && !error.empty(),
            "Zero-direction junction port was silently converted into parcel culling" );
        require( culled == 91 && checks == 92, "Malformed junction input published partial parcel counters" );
        sameCity( retained, output );
    }
    void pavementReservations()
    {
        auto data = terrain(); auto input = city();
        input.lots = { lot( 601, 0, 9.75f, 4, 4 ), lot( 602, 0, 13, 4, 4 ) };
        auto s = source(); addRoad( s, 0, { -20, 0, -3 }, { 20, 0, -3 }, 12 );
        OpenCityLayout output; u32 culled = 0, checks = 0; String error;
        check( prepareCityRoadParcels( input, s, sample( s ), data, {}, output, culled, checks, error ), error );
        require( culled == 0 && output.lots.size() == 2, "Initial authored road rejected clear pavement envelopes" );
        ++s.revision; for( auto &knot : s.roads[0].knots ) knot.position.z = 0;
        check( prepareCityRoadParcels( input, s, sample( s ), data, {}, output, culled, checks, error ), error );
        require( culled == 1 && checks == 1 && output.lots.size() == 1 && output.lots[0].id == 602,
            "Moved road admitted a foundation7.75m from its centre inside its8.45m sidewalk envelope" );

        // The actual generated grid has parcel corners(11,11): radius15.556m.
        // They clear anR14 road+1m shoulder but intersect its fitted outer walk.
        auto grid = OpenCityLayout::generate( 42, 6 ); input = grid; input.lots.clear();
        for( const auto &parcel : grid.lots )
        {
            const float x = std::max( std::abs( parcel.parcelX ) - parcel.parcelWidth * .5f, 0.f );
            const float z = std::max( std::abs( parcel.parcelZ ) - parcel.parcelDepth * .5f, 0.f );
            if( std::abs( x - 11 ) < .001f && std::abs( z - 11 ) < .001f ) input.lots.push_back( parcel );
        }
        require( input.lots.size() == 4, "Default-grid fixture did not select the four real corner parcels" );
        for( const auto &parcel : grid.lots ) if( parcel.parcelX > 60 && parcel.parcelX < 80 && parcel.parcelZ > 60 && parcel.parcelZ < 80 )
        { input.lots.push_back( parcel ); break; }
        require( input.lots.size() == 5, "Default-grid fixture lacks its clear control parcel" );
        const auto clearId = input.lots.back().id;
        s = source(); CityRoadRoundaboutSpec junction; junction.junctionId = "50000000-0000-4000-8000-000000000001";
        const Vector3F directions[] = { { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 } };
        for( unsigned i = 0; i < 4; ++i )
        { addRoad( s, i, directions[i] * 32.f, directions[i] * 60.f, 12 ); junction.ports.push_back( { s.roads.back().id, directions[i], 12, 18 } ); }
        s.roundabouts.push_back( junction ); const auto bake = sample( s );
        data->dimensions = { 257, 257 }; data->origin = { -128, -128 }; data->heights.assign( 257 * 257, 0 );
        CityRoadTerrainMesh footways; check( buildCityRoadFootways( s, bake, data, {}, footways, error ), error );
        for( size_t parcel = 0; parcel < 4; ++parcel )
        {
            const auto &p = input.lots[parcel]; bool intersection = false;
            for( const auto &v : footways.vertices ) if( v.normal.y > .5f &&
                std::abs( v.position.x - p.parcelX ) < p.parcelWidth * .5f &&
                std::abs( v.position.z - p.parcelZ ) < p.parcelDepth * .5f ) { intersection = true; break; }
            require( intersection, "Generated-grid corner is not an actual fitted-pavement collision regression" );
        }
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 4 && checks == 1 && output.lots.size() == 1 && output.lots[0].id == clearId,
            "Default-gridR14 roundabout pavement still intersects a retained full parcel foundation" );

        input = city(); input.lots = { lot( 603, 22, 7.75f, 2, 1 ), lot( 604, 22, 10.5f, 2, 1 ) };
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        require( culled == 1 && checks == 1 && output.lots.size() == 1 && output.lots[0].id == 604,
            "Roundabout port admitted a parcel inside its real sidewalk shoulder beyond the circle" );
    }
    void transactionalFailures()
    {
        auto input = city(); input.lots = { lot( 501, 0, 0, 20, 20 ) }; auto s = source(); auto bake = sample( s );
        const auto data = terrain(); OpenCityLayout output; u32 culled = 0, checks = 0; String error;
        check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
        const auto old = output;
        const auto reject = [&]( const OpenCityLayout &badCity, const CityRoadSplineSource &badSource,
            const CityRoadSplineBake &badBake, render::TerrainSnapshot badTerrain ) {
            culled = 91; checks = 92;
            require( !prepareCityRoadParcels( badCity, badSource, badBake, badTerrain, {}, output, culled, checks, error ) && !error.empty(),
                "Invalid parcel/source/terrain input was accepted" );
            require( culled == 91 && checks == 92, "Failed parcel operation published partial counters" ); sameCity( old, output );
        };
        auto stale = bake; ++stale.sourceRevision; reject( input, s, stale, data ); reject( input, s, bake, {} );
        auto bad = input; bad.lots[0].parcelWidth = std::numeric_limits<float>::quiet_NaN(); reject( bad, s, bake, data );
        bad = input; bad.lots.push_back( bad.lots[0] ); reject( bad, s, bake, data );
        auto invalid = s; invalid.id = "not-a-guid"; reject( input, invalid, bake, data );
        const auto scheduler = WorldStreamingScheduler::shared(); constexpr auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, scheduler->stats().limits[index] };
        const auto baseline = scheduler->stats().used[index]; bool admitted = false; u64 largestDenial = 0;
        for( u64 additional = 0; additional <= 2097152; additional += 16384 )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline + additional, error ), error );
            culled = 91; checks = 92;
            const bool ok = prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error );
            const auto resident = scheduler->stats().used[index];
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            require( resident == baseline, "Parcel support leaked SourceCpu storage after denied preparation" ); sameCity( old, output );
            if( ok ) { admitted = true; break; }
            require( culled == 91 && checks == 92, "Budget denial was swallowed as a successful parcel cull" ); largestDenial = additional;
        }
        require( admitted && largestDenial >= 32768, "Parcel denial sweep did not reach partial exact terrain mesh allocations" );
    }
}
int main()
{
    int result = 0;
    try { canonicalRoads(); fullTerrain(); junctions(); pavementReservations(); transactionalFailures(); std::cout << "City parcels: canonical road/junction/pavement clearances, full terrain support/heights, stable order and transactional budgets PASS\n"; }
    catch( const std::exception &e ) { std::cerr << "City parcel support: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
