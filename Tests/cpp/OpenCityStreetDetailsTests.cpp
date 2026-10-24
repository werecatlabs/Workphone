#include <Workphone/Interface/Procedural/OpenCityStreetDetails.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>
#include <utility>

using workphone::procedural::OpenCityLayout;
using Details = workphone::procedural::OpenCityStreetDetails;
static void require( bool value, const char *message )
{ if( !value ) throw std::runtime_error( message ); }
static bool close( float a, float b ) { return std::abs( a - b ) < .001f; }
static void rectangle( const Details::Corner &p, const OpenCityLayout &city, float top )
{
    require( std::isfinite( p.x ) && std::isfinite( p.z ) && std::isfinite( p.width ) &&
        std::isfinite( p.depth ) && p.width > 0 && p.depth > 0 && p.bottom == 0 && p.top == top,
        "Street render/collision descriptor must have finite positive dimensions and shared grade" );
    for( int sx : { -1, 1 } ) for( int sz : { -1, 1 } )
    {
        const float x = p.x + sx * p.width * .5f, z = p.z + sz * p.depth * .5f;
        require( std::abs( x ) <= city.extent + Details::cornerExtent + .001f &&
                 std::abs( z ) <= city.extent + Details::cornerExtent + .001f,
                 "Street descriptor escapes the bounded collision footprint" );
        require( city.roadDistance( x, z ) >= Details::roadHalfWidth - .001f ||
                 std::abs( x ) > city.extent + 6 || std::abs( z ) > city.extent + 6,
                 "Pavement encroaches on the carriageway" );
    }
}

static void compactCollision( const OpenCityLayout &city, const Details &details )
{
    const auto compact = details.collisionSurfaces( city );
    const size_t n = size_t( city.blocks );
    require( compact.size() == n * n + 12 * n + 4,
             "Compact collision count does not match interior blocks and retained perimeter pieces" );
    std::vector<Details::Corner> original( details.corners.begin(), details.corners.end() );
    original.insert( original.end(), details.sidewalks.begin(), details.sidewalks.end() );
    const int half = city.blocks / 2;
    for( int x = -half; x < half; ++x ) for( int z = -half; z < half; ++z )
        original.push_back( { ( x + .5f ) * city.spacing, ( z + .5f ) * city.spacing,
            city.spacing - 2 * Details::cornerExtent, city.spacing - 2 * Details::cornerExtent,
            0, Details::sidewalkTop } );
    for( size_t i = 0; i < compact.size(); ++i )
    {
        const auto &a = compact[i];
        rectangle( a, city, Details::sidewalkTop );
        for( size_t j = 0; j < i; ++j )
        {
            const auto &b = compact[j];
            const float overlapX = std::min( a.x + a.width * .5f, b.x + b.width * .5f ) -
                std::max( a.x - a.width * .5f, b.x - b.width * .5f );
            const float overlapZ = std::min( a.z + a.depth * .5f, b.z + b.depth * .5f ) -
                std::max( a.z - a.depth * .5f, b.z - b.depth * .5f );
            require( overlapX <= .001f || overlapZ <= .001f, "Compact surface interiors overlap" );
        }
    }
    double beforeArea = 0, afterArea = 0;
    std::vector<float> edges;
    for( const auto &p : original )
    {
        beforeArea += double( p.width ) * p.depth;
        edges.push_back( p.z - p.depth * .5f ); edges.push_back( p.z + p.depth * .5f );
    }
    for( const auto &p : compact )
    {
        afterArea += double( p.width ) * p.depth;
        edges.push_back( p.z - p.depth * .5f ); edges.push_back( p.z + p.depth * .5f );
    }
    require( std::abs( beforeArea - afterArea ) < .001, "Compact colliders changed occupied surface area" );
    std::sort( edges.begin(), edges.end() );
    edges.erase( std::unique( edges.begin(), edges.end() ), edges.end() );
    std::vector<float> rows = edges;
    for( size_t i = 1; i < edges.size(); ++i ) rows.push_back( ( edges[i - 1] + edges[i] ) * .5f );
    rows.push_back( edges.front() - 1 ); rows.push_back( edges.back() + 1 );
    const auto profile = []( const auto &pieces, float z ) {
        std::vector<std::pair<float, float>> intervals, merged;
        for( const auto &p : pieces )
            if( z >= p.z - p.depth * .5f && z <= p.z + p.depth * .5f )
                intervals.push_back( { p.x - p.width * .5f, p.x + p.width * .5f } );
        std::sort( intervals.begin(), intervals.end() );
        for( const auto &interval : intervals )
        {
            if( merged.empty() || interval.first > merged.back().second + .001f ) merged.push_back( interval );
            else merged.back().second = std::max( merged.back().second, interval.second );
        }
        return merged;
    };
    // Rectangles change coverage only at their edge coordinates. Comparing the
    // complete X union on every boundary and intervening row proves the 2D union.
    for( float z : rows )
    {
        const auto expected = profile( original, z ), actual = profile( compact, z );
        require( expected.size() == actual.size(), "Compaction bridged a road gap or lost a pavement strip" );
        for( size_t i = 0; i < expected.size(); ++i )
            require( close( expected[i].first, actual[i].first ) && close( expected[i].second, actual[i].second ),
                     "Compaction changed the original sidewalk/corner/plaza union" );
    }
    const auto covered = [&]( float x, float z ) {
        for( const auto &p : compact )
            if( x >= p.x - p.width * .5f && x <= p.x + p.width * .5f &&
                z >= p.z - p.depth * .5f && z <= p.z + p.depth * .5f ) return true;
        return false;
    };
    for( int grid = -half; grid <= half; ++grid )
    {
        const float line = grid * city.spacing;
        require( !covered( line, 0 ) && !covered( 0, line ) &&
                 !covered( line + 5.99f, city.spacing * .5f ) &&
                 !covered( city.spacing * .5f, line + 5.99f ),
                 "Compact colliders obstruct an open carriageway" );
        for( int side : { -1, 1 } )
            require( !covered( side * ( city.extent + 7.25f ), line ) &&
                     !covered( line, side * ( city.extent + 7.25f ) ),
                     "Perimeter compaction filled an outer dead-end road mouth" );
    }
}
int main()
{
    for( unsigned seed : { 0u, 42u, 0xffffffffu } ) for( int blocks : { 6, 8, 10 } )
    {
        const auto city = OpenCityLayout::generate( seed, blocks, 0 );
        const auto details = Details::generate( city, seed );
        const auto repeated = Details::generate( city, seed );
        const auto preview = Details::generate( city, seed, false );
        const auto route = Details::generate( OpenCityLayout::generate( seed, blocks, 2 ), seed );
        compactCollision( city, details );
        const size_t junctions = size_t( blocks + 1 ) * ( blocks + 1 );
        const size_t segments = size_t( 2 ) * blocks * ( blocks + 1 );
        require( details.corners.size() == junctions * 4 && details.sidewalks.size() == segments * 2 &&
                 details.furniture.size() == segments * 4,
                 "Street detail counts must stay bounded by the grid" );
        for( const auto &p : details.corners ) rectangle( p, city, Details::sidewalkTop );
        for( const auto &p : details.sidewalks ) rectangle( p, city, Details::sidewalkTop );
        const auto pavementCount = details.corners.size() + details.sidewalks.size();
        const auto pavement = [&]( size_t i ) -> const Details::Corner & {
            return i < details.corners.size() ? details.corners[i] : details.sidewalks[i - details.corners.size()];
        };
        for( size_t i = 0; i < pavementCount; ++i ) for( size_t j = 0; j < i; ++j )
        {
            const auto &a = pavement( i ), &b = pavement( j );
            const float overlapX = std::min( a.x + a.width * .5f, b.x + b.width * .5f ) -
                std::max( a.x - a.width * .5f, b.x - b.width * .5f );
            const float overlapZ = std::min( a.z + a.depth * .5f, b.z + b.depth * .5f ) -
                std::max( a.z - a.depth * .5f, b.z - b.depth * .5f );
            require( overlapX <= .001f || overlapZ <= .001f,
                     "Perpendicular pavement descriptors overlap instead of sharing an edge" );
        }
        // The NE origin corner joins both perpendicular straight walks without
        // an overlapping top surface or an uncovered square at the junction.
        bool corner = false, horizontal = false, vertical = false;
        for( const auto &p : details.corners )
            corner |= close( p.x - p.width * .5f, 6 ) && close( p.z - p.depth * .5f, 6 ) &&
                      close( p.x + p.width * .5f, 8.5f ) && close( p.z + p.depth * .5f, 8.5f );
        for( const auto &p : details.sidewalks )
        {
            horizontal |= close( p.x - p.width * .5f, 8.5f ) && close( p.z - p.depth * .5f, 6 );
            vertical |= close( p.z - p.depth * .5f, 8.5f ) && close( p.x - p.width * .5f, 6 );
        }
        require( corner && horizontal && vertical, "Corner pavement does not join both sidewalks at shared grade" );
        std::set<std::uint64_t> ids;
        size_t previewLamp = 0;
        for( size_t i = 0; i < details.furniture.size(); ++i )
        {
            const auto &a = details.furniture[i], &b = repeated.furniture[i], &c = route.furniture[i];
            require( ids.insert( a.id ).second && a.id == b.id && a.id == c.id && a.x == b.x &&
                     a.z == b.z && a.yaw == b.yaw && a.seed == b.seed && a.x == c.x && a.z == c.z,
                     "Furniture seed replay or route-independent identity failed" );
            require( std::isfinite( a.x ) && std::isfinite( a.z ) && std::isfinite( a.yaw ) &&
                     a.baseY == Details::sidewalkTop, "Street furniture has an invalid contact transform" );
            const float dx = std::abs( a.x - std::round( a.x / city.spacing ) * city.spacing );
            const float dz = std::abs( a.z - std::round( a.z / city.spacing ) * city.spacing );
            require( std::min( dx, dz ) > 6.3f && std::min( dx, dz ) < 8.2f &&
                     std::max( dx, dz ) >= 14, "Furniture intrudes into the carriageway or junction crossing" );
            if( a.kind == Details::FurnitureKind::Lamp )
            {
                const auto &low = preview.furniture[previewLamp++];
                require( low.kind == a.kind && low.id == a.id && low.x == a.x && low.z == a.z,
                         "Quality reduction reshuffled retained lamps" );
            }
            for( size_t j = 0; j < i; ++j )
                require( std::hypot( a.x - details.furniture[j].x, a.z - details.furniture[j].z ) > 1,
                         "Street furniture footprints overlap" );
        }
        require( previewLamp == preview.furniture.size(), "Preview must contain only retained lamps" );
    }
    auto invalid = OpenCityLayout::generate( 42 );
    invalid.spacing = std::numeric_limits<float>::quiet_NaN();
    bool rejected = false;
    try { (void)Details::generate( invalid, 42 ); } catch( const std::invalid_argument & ) { rejected = true; }
    require( rejected, "Non-finite city grid accepted for street detail publication" );
    std::cout << "Open-city shared pavement/collision grade, corner joins, bounded deterministic street furniture: PASS\n";
}
