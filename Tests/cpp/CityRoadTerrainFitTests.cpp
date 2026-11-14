#include <Workphone/Scene/Components/CityRoadTerrainFit.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace workphone;
using namespace workphone::procedural;
using namespace workphone::scene;
namespace
{
    void need( bool ok, const char *message ) { if( !ok ) throw std::runtime_error( message ); }
    void check( bool ok, const String &error ) { if( !ok ) throw std::runtime_error( error.c_str() ); }
    SourceBudgetText id( unsigned type, unsigned index )
    {
        char text[40]; std::snprintf( text, sizeof( text ), "%08x-0000-4000-8000-%012x", type, index );
        return SourceBudgetText( text );
    }
    CityRoadSplineSource source()
    {
        CityRoadSplineSource result; result.id = id( 1, 1 ); result.terrainResourceUuid = id( 2, 1 );
        result.revision = 13; result.terrainRevision = 29; return result;
    }
    CityRoadSpline road( unsigned index, Vector3F a, Vector3F b, float width = 8 )
    {
        CityRoadSpline result; result.id = id( 3, index ); result.width = width; result.heightOffset = .025f;
        const auto tangent = ( b-a ) / 3.f;
        result.knots = { { id( 4, index*2 ), a, {}, tangent }, { id( 4, index*2+1 ), b, -tangent, {} } };
        return result;
    }
    std::shared_ptr<render::TerrainData> terrain()
    {
        auto data = std::make_shared<render::TerrainData>();
        data->dimensions = { 129, 129 }; data->spacing = { 2, 2 }; data->origin = { -128, -128 };
        data->heights.resize( 129*129 ); data->layerWeights.assign( 129*129, { .4f, .6f, 0, 0 } );
        data->surfaceMasks.assign( 129*129, { .3f, .7f } ); data->holes.assign( 128*128, 0 );
        data->layers[0].albedo = "hand-authored-ground"; data->appearance.macroStrength = .2f;
        data->revision = 29;
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x )
        {
            const float px = -128.f + x*2, pz = -128.f + z*2;
            data->heights[z*129+x] = .03f*pz + 1.2f*std::sin( px*.21f ) + .4f*std::sin( pz*.3f );
        }
        return data;
    }
    u64 checksum( const render::TerrainData &data )
    {
        u64 hash = 1469598103934665603ull;
        for( float height : data.heights ) { u32 bits; std::memcpy( &bits, &height, sizeof( bits ) ); hash = (hash ^ bits)*1099511628211ull; }
        return hash;
    }
    SourceBudgetText encoded( const CityRoadSplineSource &s )
    {
        SourceBudgetText json; String error; check( serializeCityRoadSplineSource( s, json, error ), error ); return json;
    }
    void preserved( const CityRoadSplineSource &before, const CityRoadTerrainFitResult &after,
                    const render::TerrainData &original )
    {
        need( after.source.revision == before.revision && after.terrain->revision == original.revision,
              "Preparation assigned publication revisions" );
        need( after.source.roads.size() == before.roads.size(), "Fit changed road count" );
        for( size_t r = 0; r < before.roads.size(); ++r )
        {
            need( after.source.roads[r].knots.size() == before.roads[r].knots.size(), "Fit changed knot count" );
            for( size_t k = 0; k < before.roads[r].knots.size(); ++k )
            {
                const auto &a = before.roads[r].knots[k], &b = after.source.roads[r].knots[k];
                need( a.id == b.id && a.position == b.position && a.inHandle == b.inHandle && a.outHandle == b.outHandle,
                      "Fit rewrote editable road controls" );
            }
        }
        need( after.terrain->layers == original.layers && after.terrain->appearance == original.appearance &&
              after.terrain->autoMaterial == original.autoMaterial && after.terrain->layerWeights == original.layerWeights &&
              after.terrain->surfaceMasks == original.surfaceMasks && after.terrain->holes == original.holes,
              "Fit changed terrain appearance, material masks or holes" );
        need( after.terrain->heights.front() == original.heights.front() &&
              after.terrain->heights.back() == original.heights.back(), "Fit modified terrain outside its road corridor" );
        need( after.report.changedSamples > 0 && after.report.changedSamples < original.heights.size() &&
              after.report.maxCut <= 12.00001f && after.report.maxFill <= 12.00001f &&
              after.report.maxRoadGrade <= .08001f && after.report.attempts >= 1 && after.report.attempts <= 5,
              "Fit did not retain bounded earthwork and road-grade evidence" );
    }
    void hillsAndTransforms()
    {
        for( bool transformed : { false, true } )
        {
            auto data = terrain(); auto s = source(); Transform3<real_Num> placement;
            if( transformed )
            {
                placement.setPosition( { 123, 7, -219 } ); placement.setScale( { 1.3f, 1.1f, .8f } );
                placement.setOrientation( { std::cos( .22f ), 0, std::sin( .22f ), 0 } );
            }
            const auto world = [&]( Vector3F local ) {
                return placement.getPosition() + placement.getOrientation() * ( local*placement.getScale() );
            };
            auto r = road( 1, world( { 0, 0, -80 } ), world( { 32, 0, 80 } ) );
            r.knots.front().outHandle = placement.getOrientation() * Vector3F( 35, 0, 50 );
            r.knots.back().inHandle = placement.getOrientation() * Vector3F( -35, 0, -50 );
            r.knots.front().position.y = 31; r.knots.back().position.y = -17;
            s.roads.push_back( std::move( r ) );
            const auto originalHash = checksum( *data );
            CityRoadTerrainFitResult result; String error;
            check( prepareCityRoadTerrainFit( s, data, placement, {}, result, error ), error );
            preserved( s, result, *data ); need( checksum( *data ) == originalHash, "Fit mutated its retained input terrain" );
            CityRoadTerrainMesh mesh;
            check( buildCityRoadTerrainMesh( result.source, result.bake, result.terrain, placement, mesh, error ), error );
            CityRoadTerrainFitResult repeat;
            check( prepareCityRoadTerrainFit( s, data, placement, {}, repeat, error ), error );
            need( checksum( *result.terrain ) == checksum( *repeat.terrain ) && encoded( result.source ) == encoded( repeat.source ),
                  "Identical fit inputs produced different terrain or roundabout profiles" );
        }
    }
    void junctions()
    {
        auto data = terrain(); auto s = source();
        s.roads.push_back( road( 1, { 0, 0, -80 }, { 0, 0, 80 } ) );
        s.roads.push_back( road( 2, { 0, 0, 0 }, { 80, 0, 0 }, 6 ) );
        CityRoadTerrainFitResult result; String error;
        check( prepareCityRoadTerrainFit( s, data, {}, {}, result, error ), error ); preserved( s, result, *data );
        auto circle = source(); circle.version = 2;
        CityRoadRoundaboutSpec junction; junction.junctionId = id( 5, 1 ); junction.center.y = .025f;
        const Vector3F directions[] = { { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 } };
        for( unsigned i = 0; i < 4; ++i )
        {
            circle.roads.push_back( road( i+1, directions[i]*32.f, directions[i]*90.f, 6 ) );
            junction.ports.push_back( { circle.roads.back().id, directions[i], 6, 18 } );
        }
        circle.roundabouts.push_back( std::move( junction ) );
        check( prepareCityRoadTerrainFit( circle, data, {}, {}, result, error ), error ); preserved( circle, result, *data );
        need( std::abs( result.source.roundabouts[0].gradeZ ) > .005f,
              "Roundabout was flattened horizontally instead of fitting a local hill grade plane" );
        for( const auto &r : result.bake.roads )
        {
            const auto &p = r.points.front(); const auto &j = result.source.roundabouts.front();
            for( auto v : { p.position, p.left, p.right } )
            {
                const float plane = j.center.y + j.gradeX*(v.x-j.center.x) + j.gradeZ*(v.z-j.center.z);
                need( std::abs( plane-v.y ) <= .001f, "Fitted roundabout approach cap is not joined within 1 mm" );
            }
        }
    }
    void fixedAndFailureTransactions()
    {
        auto data = terrain(); auto s = source();
        auto r = road( 1, { 0, -.8f, -80 }, { 0, .8f, 80 } ); r.heightMode = CityRoadHeightMode::Fixed;
        s.roads.push_back( std::move( r ) ); CityRoadTerrainFitResult result; String error;
        check( prepareCityRoadTerrainFit( s, data, {}, {}, result, error ), error ); preserved( s, result, *data );
        for( const auto &point : result.bake.roads.front().points )
        {
            float ground = 0;
            need( render::sampleTerrainHeight( *result.terrain, {}, point.position, ground ) &&
                  std::abs( ground + s.roads.front().heightOffset - point.position.y ) <= .001f,
                  "Fixed terrain fitting consumed the authored surface clearance" );
        }
        const auto retained = checksum( *result.terrain ); const auto retainedSource = encoded( result.source );
        const auto reject = [&]( const CityRoadSplineSource &input, render::TerrainSnapshot ground, CityRoadTerrainFitOptions options ) {
            need( !prepareCityRoadTerrainFit( input, ground, {}, options, result, error ) && !error.empty(), "Unsupported fit unexpectedly succeeded" );
            need( checksum( *result.terrain ) == retained && encoded( result.source ) == retainedSource,
                  "Failed fit replaced last-good source or terrain" );
        };
        CityRoadTerrainFitOptions strict; strict.maximumEarthwork = .001f; reject( s, data, strict );
        auto holes = terrain(); holes->holes[64*128+64] = 1; reject( s, holes, {} );
        auto steep = s; steep.roads[0].knots.back().position.y = 100; reject( steep, data, {} );
        auto bridge = s; bridge.roads[0].heightMode = CityRoadHeightMode::Bridge; reject( bridge, data, {} );
        strict = {}; strict.maximumTerrainSamples = 16; reject( s, data, strict );
        const auto scheduler = WorldStreamingScheduler::shared(); const auto stats = scheduler->stats();
        const auto index = size_t( StreamMemoryClass::SourceCpu );
        struct Restore { std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, stats.limits[index] };
        for( u64 headroom : { 0ull, 4096ull, 65536ull, 262144ull } )
        {
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, stats.used[index] + headroom, error ), error );
            const bool ok = prepareCityRoadTerrainFit( s, data, {}, {}, result, error );
            const auto used = scheduler->stats().used[index];
            check( scheduler->setBudget( StreamMemoryClass::SourceCpu, restore.limit, error ), error );
            need( !ok && used == stats.used[index] && checksum( *result.terrain ) == retained,
                  "SourceCpu denial changed retained terrain or leaked staging storage" );
        }
    }
    void earthworkRetry()
    {
        auto data = terrain();
        // The road remains on a low corridor. A higher ridge outside its cut/fill
        // shoulder influences the broad profile filter near one end only.
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x )
        {
            const float px = -128.f+x*2, pz = -128.f+z*2;
            data->heights[z*129+x] = px >= 30 && px <= 90 && pz >= 20 && pz <= 80 ? 20.f : 0.f;
        }
        auto s = source(); s.roads.push_back( road( 1, { 0, 0, -90 }, { 0, 0, 90 } ) );
        CityRoadTerrainFitOptions options; options.smoothingRadius = 80; options.maximumEarthwork = 100;
        CityRoadTerrainFitResult unconstrained, constrained; String error;
        check( prepareCityRoadTerrainFit( s, data, {}, options, unconstrained, error ), error );
        need( unconstrained.report.attempts == 1 && unconstrained.report.maxFill > 1,
              "Earthwork retry fixture did not produce a valid first grade profile" );
        options.maximumEarthwork = unconstrained.report.maxFill * .98f;
        check( prepareCityRoadTerrainFit( s, data, {}, options, constrained, error ), error );
        need( constrained.report.attempts > 1 && constrained.report.maxFill <= options.maximumEarthwork + .00001f,
              "An over-limit first candidate prevented a valid bounded later profile" );
    }
    void extremeVerticalScaleTransactions()
    {
        auto ground = terrain(); auto s = source();
        s.roads.push_back( road( 1, { 0, 0, -80 }, { 0, 0, 80 } ) );
        CityRoadTerrainFitResult retained; String error;
        check( prepareCityRoadTerrainFit( s, ground, {}, {}, retained, error ), error );
        const auto previousTerrain = retained.terrain;
        const auto previousChecksum = checksum( *retained.terrain );
        const auto previousSource = encoded( retained.source );
        const auto scheduler = WorldStreamingScheduler::shared();
        const auto index = size_t( StreamMemoryClass::SourceCpu );
        const auto used = scheduler->stats().used[index];
        struct Extreme { float heightScale, placementScale, sample; };
        const Extreme cases[] = { { 1e30f, 1e20f, 1e-30f }, { 1e-30f, 1e-20f, 1e30f }, { 0, 1, 1 } };
        unsigned rejected = 0;
        for( const auto &extreme : cases )
        {
            ground->heightScale = extreme.heightScale;
            ground->heights.assign( ground->heights.size(), extreme.sample );
            Transform3<real_Num> placement; placement.setScale( { 1, extreme.placementScale, 1 } );
            check( render::validateTerrainPlacement( *ground, placement, error ), error );
            const real_Num product = placement.getScale().y * ground->heightScale;
            // Double-precision builds can represent the first two products;
            // zero remains invalid for the inverse height conversion everywhere.
            if( std::isfinite( product ) && product > 0 ) continue;
            need( !prepareCityRoadTerrainFit( s, ground, placement, {}, retained, error ) &&
                  error.find( "combined vertical scale" ) != String::npos,
                  "Unsupported combined vertical scale was not rejected before fitting" );
            need( scheduler->stats().used[index] == used && retained.terrain == previousTerrain &&
                  checksum( *retained.terrain ) == previousChecksum && encoded( retained.source ) == previousSource,
                  "Extreme-scale rejection changed retained output or leaked SourceCpu staging" );
            ++rejected;
        }
        need( rejected >= 1 && ( sizeof(real_Num) > sizeof(float) || rejected == 3 ),
              "Extreme-scale regression did not exercise overflow, underflow and zero" );
    }
    void localApproachGrades()
    {
        auto data = terrain();
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x )
            data->heights[z*129+x] = .065f*(-128.f+x*2) + .04f*(-128.f+z*2);
        auto s = source();
        s.roads.push_back( road( 1, { -100, 0, 0 }, { 100, 0, 0 }, 12 ) );
        s.roads.push_back( road( 2, { 0, 0, 0 }, { 0, 0, 80 }, 8 ) );
        CityRoadTerrainFitResult result; String error;
        check( prepareCityRoadTerrainFit( s, data, {}, {}, result, error ), error );
        std::cout << "local approaches cut=" << result.report.maxCut << " fill=" << result.report.maxFill <<
            " grade=" << result.report.maxRoadGrade << " height-span=" << result.report.roadHeightSpan <<
            " attempts=" << result.report.attempts << result.report.rejectedAttempts << '\n';
        need( result.report.attempts == 1 && result.report.roadHeightSpan > 12 && result.report.maxRoadGrade <= .08001f,
              "A local junction approach spike required flattening the whole hill profile" );
        CityRoadTerrainMesh mesh;
        check( buildCityRoadTerrainMesh( result.source, result.bake, result.terrain, {}, mesh, error ), error );
    }
    void generatedCity( unsigned seed, int blocks, float relief = 1, bool natural = true )
    {
        const auto city = OpenCityLayout::generate( seed, blocks, 0, natural );
        OpenCityRoadSourceOptions sourceOptions; sourceOptions.seed = seed; sourceOptions.maximumRoundabouts = 4;
        sourceOptions.sourceId = id( 1, 1 ); sourceOptions.terrainResourceUuid = id( 2, 1 );
        OpenCityRoadSourceResult planned; String error;
        check( buildOpenCityRoadSource( city, sourceOptions, []( const Vector3F &, float &h ) { h = 2; return true; }, planned, error ), error );
        const float radius = std::ceil( ( city.extent + 96 ) / 4 ) * 4;
        const int side = int( radius*.5f ) + 1;
        auto data = std::make_shared<render::TerrainData>(); data->dimensions = { side, side };
        data->spacing = { 4, 4 }; data->origin = { -radius, -radius }; data->heights.resize( size_t(side)*side );
        const float phase = float( seed%997 ) * ( 6.28318530718f/997.f );
        const float hillScale = std::max( 1.f, relief );
        for( int z = 0; z < side; ++z ) for( int x = 0; x < side; ++x )
        {
            const float px = -radius+x*4, pz = -radius+z*4;
            const float outside = std::clamp( ( std::max( std::abs( px ), std::abs( pz ) ) - city.extent - 20 ) / 76, 0.f, 1.f );
            data->heights[size_t(z)*side+x] = 2+.005f*px+.003f*pz + relief*(
                14*( std::sin(px/(240*hillScale)+phase)*std::cos(pz/(300*hillScale)-phase)-std::sin(phase)*std::cos(phase) ) +
                5*( std::sin((px+pz)/(180*hillScale)+phase)-std::sin(phase) ) ) +
                outside*outside*3*std::sin(px*.019f)*std::cos(pz*.017f);
        }
        CityRoadTerrainFitResult result;
        check( prepareCityRoadTerrainFit( planned.source, data, {}, {}, result, error ), error );
        SourceBudgetVector<Vector3F> route;
        check( fitOpenCityRoadRoute( city, result.source, [data = result.terrain]( const Vector3F &p, float &h ) {
            return render::sampleTerrainHeight( *data, {}, p, h ); }, route, error ), error );
        need( !route.empty() && result.report.changedSamples < data->heights.size() && result.report.roadHeightSpan > ( relief > 0 ? 6.f : 1.f ),
              "Generated city lost route, untouched hills or meaningful road height variation" );
        std::cout << "seed=" << seed << " blocks=" << blocks << " relief=" << relief << " natural=" << natural << " changed=" << result.report.changedSamples <<
            " cut=" << result.report.maxCut << " fill=" << result.report.maxFill << " grade=" <<
            result.report.maxRoadGrade << " height-span=" << result.report.roadHeightSpan << " attempts=" << result.report.attempts <<
            result.report.rejectedAttempts << '\n';
        if( seed == 42 && blocks == 6 && relief == 2 && natural )
        {
            // Native relief-2 regression: a ray on this retained road/roundabout
            // cap lies only ~5e-8 barycentric units inside an emitted triangle.
            // Require strict coverage; the runtime surface query's epsilon must
            // never hide a microscopic geometry gap at this seam.
            const Vector3F probe{ 26.382787704467773f, 6.160006046295166f, 89.64208984375f };
            CityRoadTerrainMesh mesh;
            check( buildCityRoadTerrainMesh( result.source, result.bake, result.terrain, {}, mesh, error ), error );
            need( result.source.roads.size() > 5 && result.bake.roads.size() > 5,
                  "Relief-2 cap regression lost its short connector road" );
            const auto authored = result.source.roads[5].knots.back().position;
            const auto sampled = result.bake.roads[5].points.back().position;
            need( authored.x == probe.x && authored.z == probe.z && sampled.x == probe.x && sampled.z == probe.z,
                  "Relief-2 terrain fitting moved the canonical road cap" );
            float ground = 0;
            need( render::sampleTerrainHeight( *result.terrain, {}, probe, ground ),
                  "Relief-2 cap regression lost retained terrain support" );
            size_t visits=0, strictCoverage=0;
            for( size_t triangle=0;triangle<mesh.indices.size()/3;++triangle )
            {
                need( ++visits <= 400000, "Relief-2 strict cap coverage exceeded its triangle visit budget" );
                if( mesh.triangleKinds[triangle] != CityRoadTerrainTriangleKind::Road ) continue;
                const auto ia=mesh.indices[triangle*3],ib=mesh.indices[triangle*3+1],ic=mesh.indices[triangle*3+2];
                const auto a=mesh.vertices[ia].position,b=mesh.vertices[ib].position,c=mesh.vertices[ic].position;
                if(probe.x<std::min({a.x,b.x,c.x}) || probe.x>std::max({a.x,b.x,c.x}) ||
                   probe.z<std::min({a.z,b.z,c.z}) || probe.z>std::max({a.z,b.z,c.z}))continue;
                const double denominator=(double(b.z)-c.z)*(double(a.x)-c.x)+(double(c.x)-b.x)*(double(a.z)-c.z);
                if(std::abs(denominator)<=1e-15)continue;
                const double u=((double(b.z)-c.z)*(double(probe.x)-c.x)+(double(c.x)-b.x)*(double(probe.z)-c.z))/denominator;
                const double v=((double(c.z)-a.z)*(double(probe.x)-c.x)+(double(a.x)-c.x)*(double(probe.z)-c.z))/denominator;
                const double w=1-u-v;
                if( u >= 0 && v >= 0 && w >= 0 &&
                    std::abs( u*a.y+v*b.y+w*c.y-ground-result.source.roads[5].heightOffset ) < .001 ) ++strictCoverage;
            }
            need( strictCoverage > 0, "Relief-2 road/roundabout cap has a true gap in emitted triangle coverage" );
        }
    }
}
int main()
{
    std::cout << std::unitbuf;
    int failures = 0;
    const auto run = [&]( const char *label, auto fn ) {
        std::cout << "BEGIN " << label << '\n';
        try { fn(); std::cout << "PASS " << label << '\n'; }
        catch( const std::exception &e ) { ++failures; std::cerr << "FAIL " << label << ": " << e.what() << '\n'; }
    };
    try
    {
        run( "hills, controls, masks, transforms and determinism", hillsAndTransforms );
        run( "ordinary junction and roundabout local planes", junctions );
        run( "Fixed profiles and transactional failure/budget denial", fixedAndFailureTransactions );
        run( "extreme vertical scales reject transactionally", extremeVerticalScaleTransactions );
        run( "earthwork rejection retries gentler candidates", earthworkRetry );
        run( "generated hilly city seed42/6", [] { generatedCity( 42, 6 ); } );
        run( "generated hilly city seed7/8", [] { generatedCity( 7, 8 ); } );
        run( "generated grid city relief0 seed42/6", [] { generatedCity( 42, 6, 0, false ); } );
        run( "generated hilly city relief2 seed42/6", [] { generatedCity( 42, 6, 2 ); } );
        run( "local approach grades retain the broad hill profile", localApproachGrades );
        return failures ? 1 : 0;
    }
    catch( const std::exception &e ) { std::cerr << e.what() << '\n'; return 1; }
}
