#include <Workphone/Procedural/CityRoadParcelSupport.hpp>
#include <Workphone/Procedural/CityRoadFootways.hpp>
#include <Workphone/Procedural/CityRoadTerrainFit.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
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
    void containedAdaptation( const OpenCityLayout::Lot &original, const OpenCityLayout::Lot &adapted )
    {
        require( original.id == adapted.id && original.variant == adapted.variant && original.frontage == adapted.frontage &&
            original.district == adapted.district && original.park == adapted.park && original.height == adapted.height,
            "Terrain adaptation changed parcel identity, frontage or building style" );
        require( std::abs( adapted.parcelX-original.parcelX ) + adapted.parcelWidth*.5f <= original.parcelWidth*.5f+.001f &&
            std::abs( adapted.parcelZ-original.parcelZ ) + adapted.parcelDepth*.5f <= original.parcelDepth*.5f+.001f &&
            std::abs( adapted.x-adapted.parcelX ) + adapted.width*.5f <= adapted.parcelWidth*.5f+.001f &&
            std::abs( adapted.z-adapted.parcelZ ) + adapted.depth*.5f <= adapted.parcelDepth*.5f+.001f,
            "Adapted parcel escaped its original road setback or its building escaped the supported envelope" );
        using Frontage = OpenCityLayout::Frontage;
        const bool x = original.frontage == Frontage::West || original.frontage == Frontage::East;
        const float sign = original.frontage == Frontage::West || original.frontage == Frontage::North ? -1.f : 1.f;
        const float before = x ? original.parcelX + sign*original.parcelWidth*.5f : original.parcelZ + sign*original.parcelDepth*.5f;
        const float after = x ? adapted.parcelX + sign*adapted.parcelWidth*.5f : adapted.parcelZ + sign*adapted.parcelDepth*.5f;
        require( std::abs( before-after ) < .001f, "Terrain adaptation moved the original street-facing parcel edge" );
        require( std::isfinite( adapted.groundHeight ) && std::isfinite( adapted.groundMinimumHeight ) &&
            adapted.groundHeight >= adapted.groundMinimumHeight && adapted.groundHeight-adapted.groundMinimumHeight <= 1.25001f,
            "Adapted parcel waived the full-footprint1.25m foundation relief bound" );
    }
    void adaptiveFootprints()
    {
        CityRoadParcelSupportOptions options; options.shrinkForRelief = true;
        auto input = city(); input.lots = { lot( 701, 0, 0, 20, 20 ) };
        input.lots[0].frontage = OpenCityLayout::Frontage::North;
        const auto original = input; auto data = terrain(); auto s = source(); const auto bake = sample( s );
        for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x ) data->heights[z*129+x] = .11f*(z-64);
        OpenCityLayout strict, adapted; u32 culled = 0, checks = 0; String error;
        check( prepareCityRoadParcels( input, s, bake, data, {}, strict, culled, checks, error ), error );
        require( strict.lots.empty() && culled == 1 && checks == 1, "Default parcel policy silently enabled terrain resizing" );
        check( prepareCityRoadParcels( input, s, bake, data, {}, adapted, culled, checks, error, options ), error );
        require( adapted.lots.size() == 1 && culled == 0 && checks > 1 && checks <= 25,
            "Supported hill parcel did not recover through bounded adaptation" );
        containedAdaptation( input.lots[0], adapted.lots[0] );
        require( std::abs( adapted.lots[0].parcelWidth-20 ) < .001f && std::abs( adapted.lots[0].parcelDepth-11 ) < .001f &&
            std::abs( adapted.lots[0].groundMinimumHeight+1.1f ) < .001f && std::abs( adapted.lots[0].groundHeight-.11f ) < .001f,
            "Anisotropic adaptation sacrificed safe frontage or missed exact supported extrema" );
        OpenCityLayout repeated; check( prepareCityRoadParcels( input, s, bake, data, {}, repeated, culled, checks, error, options ), error );
        sameCity( adapted, repeated ); sameCity( input, original );
        for( auto facing : { OpenCityLayout::Frontage::West, OpenCityLayout::Frontage::East, OpenCityLayout::Frontage::South } )
        {
            input.lots[0].frontage = facing;
            const bool alongX = facing != OpenCityLayout::Frontage::South;
            for( int z = 0; z < 129; ++z ) for( int x = 0; x < 129; ++x ) data->heights[z*129+x] = .11f*(alongX?x-64:z-64);
            check( prepareCityRoadParcels( input, s, bake, data, {}, repeated, culled, checks, error, options ), error );
            require( repeated.lots.size() == 1, "A rotated street frontage could not adapt to its hill" );
            containedAdaptation( input.lots[0], repeated.lots[0] );
        }
        data = terrain(); data->holes.assign( 128*128, 0 ); data->holes[65*128+65] = 1;
        check( prepareCityRoadParcels( input, s, bake, data, {}, repeated, culled, checks, error, options ), error );
        require( repeated.lots.empty() && culled == 1 && checks == 1, "Adaptive policy hid an unsupported terrain hole" );
        data = terrain(); data->heights[65*129+65] = .7f;
        check( prepareCityRoadParcels( input, s, bake, data, {}, repeated, culled, checks, error, options ), error );
        require( repeated.lots.empty() && culled == 1 && checks == 1, "Adaptive policy waived the exact terrain-cell grade bound" );
    }
    CityRoadTerrainFitResult fitGeneratedHillside( const OpenCityLayout &input )
    {
        OpenCityRoadSourceOptions options; options.seed = 42; options.maximumRoundabouts = 4;
        options.sourceId = source().id; options.terrainResourceUuid = source().terrainResourceUuid;
        OpenCityRoadSourceResult planned; String error;
        check( buildOpenCityRoadSource( input, options, []( const Vector3F &, float &height ) { height = 2; return true; }, planned, error ), error );
        const float radius = std::ceil( (input.extent+96)/4 )*4;
        const int side = int(radius*.5f)+1;
        auto data = std::make_shared<render::TerrainData>(); data->dimensions = {side,side};
        data->origin = {-radius,-radius}; data->spacing = {4,4}; data->heights.resize(size_t(side)*side);
        const float phase = 42.f*(6.28318530718f/997.f);
        for( int z = 0; z < side; ++z ) for( int x = 0; x < side; ++x )
        {
            const float px=-radius+x*4, pz=-radius+z*4;
            const float outside=std::clamp((std::max(std::abs(px),std::abs(pz))-input.extent-20)/76,0.f,1.f);
            data->heights[size_t(z)*side+x]=2+.005f*px+.003f*pz+
                14*(std::sin(px/240+phase)*std::cos(pz/300-phase)-std::sin(phase)*std::cos(phase))+
                5*(std::sin((px+pz)/180+phase)-std::sin(phase))+
                outside*outside*3*std::sin(px*.019f)*std::cos(pz*.017f);
        }
        CityRoadTerrainFitResult fitted;
        check( prepareCityRoadTerrainFit(planned.source,data,{},{},fitted,error),error );
        return fitted;
    }
    void generatedHillsideDensity()
    {
        const auto input = OpenCityLayout::generate( 42, 6, 0, true ); const auto original = input;
        const auto fitted = fitGeneratedHillside( input ); String error;
        OpenCityLayout strict,adapted,verified; u32 culled=0,checks=0;
        check( prepareCityRoadParcels(input,fitted.source,fitted.bake,fitted.terrain,{},strict,culled,checks,error),error );
        CityRoadParcelSupportOptions adaptation; adaptation.shrinkForRelief=true;
        check( prepareCityRoadParcels(input,fitted.source,fitted.bake,fitted.terrain,{},adapted,culled,checks,error,adaptation),error );
        size_t central=0,buildings=0;
        for( const auto &lot:adapted.lots )
        {
            const auto before=std::find_if(input.lots.begin(),input.lots.end(),[&](const auto &item){return item.id==lot.id;});
            require(before!=input.lots.end(),"Adaptive city invented parcel identities");containedAdaptation(*before,lot);
            buildings+=!lot.park;central+=!lot.park && std::hypot(lot.x,lot.z)<input.extent*.34f;
        }
        std::cout << "Hilly seed42 parcels: strict=" << strict.lots.size() << " adapted=" << adapted.lots.size()
                  << " buildings=" << buildings << " central=" << central << " checks=" << checks << '\n';
        require(adapted.lots.size()>=input.lots.size()/2 && adapted.lots.size()>=strict.lots.size()+30 && central>=4,
            "Hilly generated city did not recover substantial building density and a populated centre");
        check(prepareCityRoadParcels(adapted,fitted.source,fitted.bake,fitted.terrain,{},verified,culled,checks,error),error);
        require(culled==0 && checks==adapted.lots.size(),"Adapted city cannot pass unchanged strict terrain support and reservations");
        sameCity(adapted,verified);sameCity(input,original);
    }
    void hillsideGridInfill()
    {
        const auto input = OpenCityLayout::generate( 42, 6, 0, false );
        const auto fitted = fitGeneratedHillside( input );
        CityRoadParcelSupportOptions options;
        options.shrinkForRelief = true;
        OpenCityLayout original, filled, verified;
        CityRoadBlockFaces faces;
        u32 culled = 0, checks = 0;
        String error;
        check( prepareCityRoadParcels( input, fitted.source, fitted.bake, fitted.terrain, {},
                                       original, culled, checks, error, options ), error );
        check( prepareCityRoadParcels( input, fitted.source, fitted.bake, fitted.terrain, {},
                                       filled, culled, checks, error, options, 42u, &faces ), error );
        require( !faces.empty() && filled.lots.size() <= size_t( input.blocks * input.blocks * 4 ) &&
                 filled.lots.size() > original.lots.size() + input.lots.size() / 2,
                 "Hillside grid failed to repopulate enclosed blocks within its lot budget" );
        for( size_t i = 0; i < original.lots.size(); ++i )
            sameLot( original.lots[i], filled.lots[i] );

        size_t commercialFronts = 0;
        float lowestOffice = std::numeric_limits<float>::max(), highestOffice = 0;
        for( size_t i = original.lots.size(); i < filled.lots.size(); ++i )
        {
            const auto &lot = filled.lots[i];
            for( size_t j = 0; j < i; ++j )
                require( lot.id != filled.lots[j].id && !OpenCityLayout::parcelsOverlap( lot, filled.lots[j] ),
                         "Grid infill duplicated a lot or encroached on a neighbouring envelope" );
            if( lot.park )
                continue;
            const bool commercial = lot.district == OpenCityLayout::District::Commercial;
            const float storey = commercial ? 3.6f : 3.2f;
            const float floors = lot.height / storey;
            require( floors >= 2.f && std::abs( floors - std::round( floors ) ) < .001f &&
                     lot.height <= std::min( lot.width, lot.depth ) * ( commercial ? 2.1f : 1.5f ) + .001f,
                     "Grid infill lost whole storeys or outgrew its fitted foundation" );
            if( commercial )
            {
                ++commercialFronts;
                lowestOffice = std::min( lowestOffice, lot.height );
                highestOffice = std::max( highestOffice, lot.height );
            }
        }
        require( commercialFronts >= 4 && highestOffice - lowestOffice >= 3.5f,
                 "Grid centre lacks a mix of commercial building heights" );
        double centreArea = 0, outerArea = 0;
        for( const auto &lot : filled.lots )
        {
            if( lot.park )
                continue;
            const float distance = std::max( std::abs( lot.parcelX ), std::abs( lot.parcelZ ) );
            if( distance < input.spacing )
                centreArea += lot.width * lot.depth;
            else if( distance >= 2 * input.spacing )
                outerArea += lot.width * lot.depth;
        }
        // Four central blocks and twenty perimeter blocks in this six-by-six grid.
        require( outerArea > 0 && centreArea / 4 > 1.25 * outerArea / 20,
                 "Hillside grid lost the transition from denser central blocks to the outskirts" );
        check( prepareCityRoadParcels( filled, fitted.source, fitted.bake, fitted.terrain, {},
                                       verified, culled, checks, error ), error );
        require( culled == 0 && checks == filled.lots.size(),
                 "Grid infill failed strict road, footway or terrain checks" );
        sameCity( filled, verified );
    }
    void largeHillsideInfill( bool naturalRoads )
    {
        const auto input = OpenCityLayout::generate( 42, 10, 0, naturalRoads );
        const auto fitted = fitGeneratedHillside( input );
        CityRoadParcelSupportOptions options;
        options.shrinkForRelief = true;
        OpenCityLayout result;
        u32 culled = 0, checks = 0;
        String error;
        check( prepareCityRoadParcels( input, fitted.source, fitted.bake, fitted.terrain, {},
                                       result, culled, checks, error, options, 42u ), error );
        require( result.lots.size() == 400 && culled > 0,
                 "Largest supported city failed to fill eligible sites within its clearance work budget" );
    }
    void sparseAuthoredBlockInfill()
    {
        auto input = city();  // The saved roads enclose a block that the seed graph does not.
        input.lots = { lot( 901, -25, -25 ) };
        auto data = terrain();
        data->spacing = { 2, 2 };
        data->origin = { -128, -128 };
        auto roads = source();
        constexpr float edge = 88.f;
        const Vector3F corners[] = { { -edge, 0, -edge }, { edge, 0, -edge },
                                     { edge, 0, edge }, { -edge, 0, edge } };
        for( unsigned i = 0; i < 4; ++i )
            addRoad( roads, i, corners[i], corners[( i + 1 ) % 4], 8 );
        const auto bake = sample( roads );
        OpenCityLayout original, filled, repeat, verified;
        u32 culled = 0, checks = 0;
        String error;
        CityRoadParcelSupportOptions options;
        options.shrinkForRelief = true;
        check( prepareCityRoadParcels( input, roads, bake, data, {}, original, culled, checks, error, options ), error );
        require( original.lots.size() == 1, "Sparse authored block fixture lost its original lot" );

        const auto fill = [&]( OpenCityLayout &output ) {
            check( prepareCityRoadParcels( input, roads, bake, data, {}, output, culled, checks, error,
                                           options, 7u ), error );
        };
        fill( filled );
        fill( repeat );
        sameCity( filled, repeat );
        require( filled.lots.size() >= 4 && filled.lots.size() <= size_t( 4 * input.blocks * input.blocks ),
                 "Terrain infill did not populate a sparse authored block within the city parcel budget" );
        sameLot( original.lots.front(), filled.lots.front() );
        float developedArea = 0;
        for( size_t i = 0; i < filled.lots.size(); ++i )
        {
            const auto &parcel = filled.lots[i];
            developedArea += parcel.parcelWidth * parcel.parcelDepth;
            require( std::abs( parcel.parcelX ) + parcel.parcelWidth * .5f < edge - 4.f &&
                     std::abs( parcel.parcelZ ) + parcel.parcelDepth * .5f < edge - 4.f,
                     "Infill escaped its authored block or entered the enclosing carriageway" );
            for( size_t j = 0; j < i; ++j )
            {
                const auto &other = filled.lots[j];
                require( parcel.id != other.id &&
                         ( std::abs( parcel.parcelX - other.parcelX ) >= ( parcel.parcelWidth + other.parcelWidth ) * .5f ||
                           std::abs( parcel.parcelZ - other.parcelZ ) >= ( parcel.parcelDepth + other.parcelDepth ) * .5f ),
                         "Infill duplicated a parcel identity or overlapped an occupied parcel" );
            }
        }
        require( developedArea > original.lots.front().parcelWidth * original.lots.front().parcelDepth * 3.f,
                 "Infill increased the lot count without recovering usable parcel area" );

        // Mark the terrain cell under a known infill site as a hole, then re-run the
        // same seed. No accepted rectangle may touch the interior of that cell.
        const auto &site = filled.lots[1];
        const int cellX = int( std::floor( ( site.parcelX - data->origin.x ) / data->spacing.x ) );
        const int cellZ = int( std::floor( ( site.parcelZ - data->origin.y ) / data->spacing.y ) );
        const float x = data->origin.x + data->spacing.x * cellX;
        const float z = data->origin.y + data->spacing.y * cellZ;
        data->holes.assign( 128 * 128, 0 );
        data->holes[cellZ * 128 + cellX] = 1;
        fill( repeat );
        require( repeat.lots.size() > 1, "One terrain hole emptied a buildable block" );
        sameLot( original.lots.front(), repeat.lots.front() );
        for( const auto &p : repeat.lots )
            require( p.parcelX + p.parcelWidth * .5f <= x || p.parcelX - p.parcelWidth * .5f >= x + data->spacing.x ||
                     p.parcelZ + p.parcelDepth * .5f <= z || p.parcelZ - p.parcelDepth * .5f >= z + data->spacing.y,
                     "Regenerated infill occupied an unsupported terrain cell" );
        check( prepareCityRoadParcels( repeat, roads, bake, data, {}, verified, culled, checks, error ), error );
        require( culled == 0, "Infill bypassed the canonical road, sidewalk or terrain checks" );
        sameCity( repeat, verified );
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
    void ordinaryJunctionReservations()
    {
        for( bool rotated : { false, true } )
        {
            const float cosine = rotated ? .8f : 1.f, sine = rotated ? .6f : 0.f;
            const auto transform = [&]( float x, float z ) {
                return Vector3F( x * cosine - z * sine, 0, x * sine + z * cosine );
            };
            auto s = source();
            addRoad( s, 0, transform( -24, 0 ), transform( 24, 0 ), 12 );
            addRoad( s, 1, transform( 0, 0 ), transform( 0, 24 ), 7 );
            for( auto &road : s.roads ) road.heightMode = CityRoadHeightMode::Drape;
            const CityRoadHeightSampler flat = []( const Vector3F &, float &height ) { height = 0; return true; };
            CityRoadSplineBake bake; String error;
            check( sampleCityRoadSplines( s, {}, flat, bake, error ), error );
            SourceBudgetText originalSource; check( serializeCityRoadSplineSource( s, originalSource, error ), error );
            const auto data = terrain(); CityRoadTerrainMesh footways;
            check( buildCityRoadFootways( s, bake, data, {}, footways, error ), error );
            const auto pavementPoint = transform( 6.38f, 8.88f );
            const auto clearPoint = transform( 7.5f, 10.f );
            auto input = city(); input.lots = {
                lot( 801, pavementPoint.x, pavementPoint.z, .5f, .5f ),
                lot( 802, clearPoint.x, clearPoint.z, .5f, .5f ) };
            const auto original = input;
            // Both parcels are wholly outside the original road+walk capsules.
            // The first still intersects the actual fillet's outer sidewalk;
            // the second sits in the clear pocket a junction disk would erase.
            for( const auto &parcel : input.lots ) for( float x : { -.25f, .25f } ) for( float z : { -.25f, .25f } )
            {
                const float worldX = parcel.x + x, worldZ = parcel.z + z;
                require( worldX * cosine + worldZ * sine > 5.95f &&
                         -worldX * sine + worldZ * cosine > 8.45f,
                         "Ordinary junction regression already intersects a straight-road reservation" );
            }
            const auto covered = [&]( Vector3F p ) {
                const auto cross = []( Vector3F a, Vector3F b, Vector3F c ) {
                    return double( b.x - a.x ) * ( c.z - a.z ) - double( b.z - a.z ) * ( c.x - a.x );
                };
                for( size_t i = 0; i < footways.indices.size(); i += 3 )
                {
                    const auto a = footways.vertices[footways.indices[i]].position,
                               b = footways.vertices[footways.indices[i + 1]].position,
                               c = footways.vertices[footways.indices[i + 2]].position;
                    const double area = cross( a, b, c );
                    if( std::abs( area ) > 1e-8 && cross( a, b, p ) * area >= -1e-9 &&
                        cross( b, c, p ) * area >= -1e-9 && cross( c, a, p ) * area >= -1e-9 ) return true;
                }
                return false;
            };
            require( covered( pavementPoint ) && !covered( clearPoint ),
                     "Mixed-width T regression does not distinguish actual rounded pavement from its clear pocket" );
            OpenCityLayout output; u32 culled = 91, checks = 92;
            check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error ), error );
            require( culled == 1 && checks == 1 && output.lots.size() == 1 && output.lots[0].id == 802,
                     "Ordinary rounded junction pavement overlaps a retained parcel, or a clear neighbour was over-culled" );
            const auto retained = output;
            CityRoadParcelSupportOptions options; options.shrinkForRelief = true;
            check( prepareCityRoadParcels( input, s, bake, data, {}, output, culled, checks, error, options ), error );
            sameCity( retained, output ); sameCity( original, input );
            SourceBudgetText after; check( serializeCityRoadSplineSource( s, after, error ), error );
            require( originalSource == after, "Parcel junction reservations changed canonical editable roads" );
        }
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
    try { canonicalRoads(); fullTerrain(); adaptiveFootprints(); generatedHillsideDensity(); hillsideGridInfill(); largeHillsideInfill( false ); largeHillsideInfill( true ); sparseAuthoredBlockInfill(); junctions(); pavementReservations(); ordinaryJunctionReservations(); transactionalFailures(); std::cout << "City parcels: canonical road/junction/pavement clearances, full terrain support/heights, stable order and transactional budgets PASS\n"; }
    catch( const std::exception &e ) { std::cerr << "City parcel support: " << e.what() << '\n'; result = 1; }
    WorldStreamingScheduler::shutdownShared(); return result;
}
