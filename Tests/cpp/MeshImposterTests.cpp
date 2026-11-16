#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <algorithm>

using namespace workphone;
static void require( bool ok, const char *message )
{
    if( !ok )
        throw std::runtime_error( message );
}

static Array<MeshImposterTriangle> asymmetricBuilding()
{
    Array<MeshImposterTriangle> triangles;
    const auto quad = [&]( Vector3F a, Vector3F b, Vector3F c, Vector3F d, ColourF colour ) {
        triangles.push_back( { { a, b, c }, colour } );
        triangles.push_back( { { a, c, d }, colour } );
    };
    // Different facades make reuse/mirroring of a front chart observable. The
    // pitched roof also distinguishes an elevated chart from a side chart.
    quad( { -1, 0, .6f }, { 1, 0, .6f }, { 1, 2, .6f }, { -1, 2, .6f }, { 1, 0, 0, 1 } );
    quad( { 1, 0, -.6f }, { -1, 0, -.6f }, { -1, 2, -.6f }, { 1, 2, -.6f }, { 0, 0, 1, 1 } );
    quad( { 1, 0, .6f }, { 1, 0, -.6f }, { 1, 2, -.6f }, { 1, 2, .6f }, { 0, 1, 0, 1 } );
    quad( { -1, 0, -.6f }, { -1, 0, .6f }, { -1, 2, .6f }, { -1, 2, -.6f }, { 0, 1, 1, 1 } );
    triangles.push_back( { { Vector3F( -1, 2, .6f ), Vector3F( 1, 2, .6f ), Vector3F( 0, 3, .6f ) }, { 1, 0, 0, 1 } } );
    triangles.push_back( { { Vector3F( 1, 2, -.6f ), Vector3F( -1, 2, -.6f ), Vector3F( 0, 3, -.6f ) }, { 0, 0, 1, 1 } } );
    quad( { 1, 2, .6f }, { 1, 2, -.6f }, { 0, 3, -.6f }, { 0, 3, .6f }, { 1, 1, 0, 1 } );
    quad( { -1, 2, -.6f }, { -1, 2, .6f }, { 0, 3, .6f }, { 0, 3, -.6f }, { 1, 1, 0, 1 } );
    return triangles;
}

static void spriteSheetContracts()
{
    const auto triangles = asymmetricBuilding();
    const Vector3F lo( -1.1f, -.1f, -.7f ), hi( 1.1f, 3.1f, .7f );
    MeshImposterSpriteSheetOptions options;
    options.tileWidth = 32;
    options.tileHeight = 40;
    options.azimuthViews = 8;
    options.elevationRings = 3;
    options.columns = 5;
    options.gutter = 2;
    auto sheet = bakeMeshImposterSpriteSheet( triangles, lo, hi, options );
    const auto again = bakeMeshImposterSpriteSheet( triangles, lo, hi, options );
    require( sheet.views.size() == 25 && sheet.columns == 5 && sheet.rows == 5 &&
             sheet.width == 160 && sheet.height == 200 && sheet.rgba == again.rgba,
             "Directional sprite sheet must deterministically pack all azimuth/elevation frames" );

    const auto pixel = [&]( size_t frame, u32 x, u32 y ) {
        const auto &view = sheet.views[frame];
        return sheet.rgba.data() + ( size_t( view.tileY + y ) * sheet.width + view.tileX + x ) * 4;
    };
    Array<size_t> roofCoverage;
    for( size_t i = 0; i < sheet.views.size(); ++i )
    {
        const auto &view = sheet.views[i];
        require( view.tileX == ( i % sheet.columns ) * sheet.tileWidth &&
                 view.tileY == ( i / sheet.columns ) * sheet.tileHeight,
                 "Sprite frames must occupy distinct row-major tiles" );
        require( std::abs( view.right.length() - 1.f ) < .0001f &&
                 std::abs( view.up.length() - 1.f ) < .0001f &&
                 std::abs( view.towardCamera.length() - 1.f ) < .0001f &&
                 std::abs( view.right.dotProduct( view.up ) ) < .0001f &&
                 view.right.crossProduct( view.up ).dotProduct( view.towardCamera ) > .999f,
                 "Each capture basis must be orthonormal and right handed, including zenith" );
        require( std::isfinite( view.minimum.x ) && std::isfinite( view.minimum.y ) &&
                 std::isfinite( view.maximum.x ) && std::isfinite( view.maximum.y ) &&
                 view.minimum.x < view.maximum.x && view.minimum.y < view.maximum.y,
                 "All directional charts need finite nondegenerate projected bounds" );
        require( std::isfinite( view.maximumDepth ), "Every directional chart needs a finite front depth" );
        float maximumCornerDepth = -std::numeric_limits<float>::infinity();
        for( unsigned corner = 0; corner < 8; ++corner )
        {
            const Vector3F position( corner & 1 ? hi.x : lo.x, corner & 2 ? hi.y : lo.y,
                                     corner & 4 ? hi.z : lo.z );
            const auto depth = position.dotProduct( view.towardCamera );
            require( depth <= view.maximumDepth + .00001f,
                     "All baked bounds must lie behind the directional front plane" );
            maximumCornerDepth = std::max( maximumCornerDepth, depth );
        }
        require( std::abs( maximumCornerDepth - view.maximumDepth ) < .00001f,
                 "Directional front plane must match the foremost captured bound" );
        const float left = float( view.tileX + sheet.gutter ) / sheet.width;
        const float top = float( view.tileY + sheet.gutter ) / sheet.height;
        const float right = float( view.tileX + sheet.tileWidth - sheet.gutter ) / sheet.width;
        const float bottom = float( view.tileY + sheet.tileHeight - sheet.gutter ) / sheet.height;
        require( view.uvMinimum.x >= left && view.uvMinimum.y >= top &&
                 view.uvMaximum.x <= right && view.uvMaximum.y <= bottom &&
                 view.uvMinimum.x < view.uvMaximum.x && view.uvMinimum.y < view.uvMaximum.y,
                 "Sprite UVs must address their own padded tile, never adjacent frames" );
        size_t covered = 0, roof = 0;
        for( u32 y = 0; y < sheet.tileHeight; ++y ) for( u32 x = 0; x < sheet.tileWidth; ++x )
        {
            const auto colour = pixel( i, x, y );
            if( x < sheet.gutter || y < sheet.gutter || x >= sheet.tileWidth - sheet.gutter ||
                y >= sheet.tileHeight - sheet.gutter )
                require( colour[3] == 0, "Every directional chart must preserve transparent gutters" );
            covered += colour[3] != 0;
            roof += colour[3] > 127 && colour[0] > 32 && colour[1] > 32 && colour[2] == 0;
        }
        require( covered > 40, "Every side and elevated frame must contain the building silhouette" );
        roofCoverage.push_back( roof );
    }
    const auto front = pixel( 0, sheet.tileWidth / 2, sheet.tileHeight / 2 );
    const auto back = pixel( 4, sheet.tileWidth / 2, sheet.tileHeight / 2 );
    const auto side = pixel( 2, sheet.tileWidth / 2, sheet.tileHeight / 2 );
    require( front[0] > 32 && front[1] == 0 && front[2] == 0 && front[3] == 255 &&
             back[2] > 32 && back[0] == 0 && back[1] == 0 && back[3] == 255 &&
             side[1] > 32 && side[0] == 0 && side[2] == 0 && side[3] == 255,
             "Front, rear and side captures must expose their own nearest facade, not a mirrored chart" );
    require( roofCoverage[24] > roofCoverage[0] + 40 && roofCoverage[16] > roofCoverage[0],
             "Elevated captures must show the roof geometry rather than reuse the vertical silhouette" );

    const auto direction = []( float azimuth, float elevation = 0.f ) {
        constexpr float radians = 3.14159265358979323846f / 180.f;
        return Vector3F( std::sin( azimuth * radians ) * std::cos( elevation * radians ),
                         std::sin( elevation * radians ),
                         std::cos( azimuth * radians ) * std::cos( elevation * radians ) );
    };
    require( selectMeshImposterView( sheet.views, { 0, 0, 1 } ) == 0 &&
             selectMeshImposterView( sheet.views, { 1, 0, 0 } ) == 2 &&
             selectMeshImposterView( sheet.views, { 0, 0, -1 } ) == 4 &&
             selectMeshImposterView( sheet.views, { -1, 0, 0 } ) == 6 &&
             selectMeshImposterView( sheet.views, direction( 0, 30 ) ) == 8 &&
             selectMeshImposterView( sheet.views, direction( 0, 60 ) ) == 16 &&
             selectMeshImposterView( sheet.views, { 0, 1, 0 } ) == 24,
             "Direction selection must distinguish opposite sides and elevated rings" );
    require( selectMeshImposterView( sheet.views, direction( 22.6f ), -1, 0.f ) == 1 &&
             selectMeshImposterView( sheet.views, direction( -22.6f ), -1, 0.f ) == 7 &&
             selectMeshImposterView( sheet.views, direction( 22.6f ), 0, .01f ) == 0 &&
             selectMeshImposterView( sheet.views, direction( -22.6f ), 0, .01f ) == 0 &&
             selectMeshImposterView( sheet.views, direction( 35.f ), 0, .01f ) == 1 &&
             selectMeshImposterView( sheet.views, direction( -35.f ), 0, .01f ) == 7,
             "Angular hysteresis must suppress seam chatter, including the last-to-first azimuth seam" );
    const float nan = std::numeric_limits<float>::quiet_NaN();
    require( selectMeshImposterView( sheet.views, {}, 4 ) == 4 &&
             selectMeshImposterView( sheet.views, { nan, 0, 1 }, 4 ) == 4 &&
             selectMeshImposterView( sheet.views, {}, -1 ) == -1 &&
             selectMeshImposterView( {}, { 0, 0, 1 } ) == -1,
             "Missing or invalid camera directions preserve a valid frame and cannot invent one" );

    const auto invalid = [&]( const Array<MeshImposterTriangle> &source,
                              Vector3F minimum, Vector3F maximum,
                              MeshImposterSpriteSheetOptions settings ) {
        bool rejected = false;
        try { auto candidate = bakeMeshImposterSpriteSheet( source, minimum, maximum, settings ); }
        catch( const std::invalid_argument & ) { rejected = true; }
        require( rejected, "Invalid directional input must be rejected before rasterization" );
    };
    auto invalidSource = triangles;
    invalidSource.front().positions[0].x = nan;
    invalid( invalidSource, lo, hi, options );
    invalid( triangles, { nan, 0, 0 }, hi, options );
    invalid( triangles, hi, lo, options );
    auto badOptions = options; badOptions.tileWidth = 0;
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.azimuthViews = 0;
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.columns = 0;
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.gutter = options.tileWidth;
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.supersampling = 0;
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.tileWidth = std::numeric_limits<u32>::max();
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.azimuthViews = std::numeric_limits<u32>::max();
    invalid( triangles, lo, hi, badOptions );
    badOptions = options; badOptions.maximumRasterSamples = 0;
    invalid( triangles, lo, hi, badOptions );
    invalidSource = triangles;
    invalidSource.front().colour.g = nan;
    invalid( invalidSource, lo, hi, options );
    invalidSource = triangles;
    invalidSource.front().positions[0].x = hi.x + 1;
    invalid( invalidSource, lo, hi, options );
    const auto originalPixels = sheet.rgba.data();
    options.maximumRasterSamples = 1;
    bool bounded = false;
    try { sheet = bakeMeshImposterSpriteSheet( triangles, lo, hi, options ); }
    catch( const std::runtime_error & ) { bounded = true; }
    require( bounded && sheet.rgba.data() == originalPixels && sheet.rgba == again.rgba && sheet.views.size() == 25,
             "Exhausted raster work must preserve the previous complete sprite sheet" );
}
int main()
{
    try
    {
        const Array<MeshImposterTriangle> triangles = {
            { { Vector3F( -.5f, 0, -.1f ), Vector3F( .5f, 0, -.1f ), Vector3F( 0, 1, -.1f ) },
              ColourF( 0, 0, 1, 1 ) },
            { { Vector3F( -.5f, 0, .1f ), Vector3F( .5f, 0, .1f ), Vector3F( 0, 1, .1f ) },
              ColourF( 1, 0, 0, 1 ) }
        };
        auto first = generateMeshImposters( triangles, { -.6f, 0, -.2f }, { .6f, 1.1f, .2f }, 32, 64 );
        auto again = generateMeshImposters( triangles, { -.6f, 0, -.2f }, { .6f, 1.1f, .2f }, 32, 64 );
        require( first.width == 96 && first.height == 64 && first.rgba == again.rgba,
                 "Imposter bake must be deterministic and contain three views" );
        const auto centre = ( size_t( 32 ) * first.width + 16 ) * 4;
        require( first.rgba[centre] > 0 && first.rgba[centre + 2] == 0 && first.rgba[centre + 3] == 255,
                 "Frontmost triangle must supply the visible imposter colour" );
        require( first.rgba[3] == 0, "Imposter atlas border must be transparent" );
        bool rejected = false;
        try
        {
            generateMeshImposters( triangles, {}, {}, 0, 0 );
        }
        catch( const std::invalid_argument & )
        {
            rejected = true;
        }
        require( rejected, "Invalid bake dimensions must be rejected" );
        auto invalidTriangles = triangles;
        invalidTriangles[0].positions[0].x = std::numeric_limits<float>::quiet_NaN();
        rejected = false;
        try
        {
            generateMeshImposters( invalidTriangles, { -.6f, 0, -.2f }, { .6f, 1.1f, .2f }, 32, 64 );
        }
        catch( const std::invalid_argument & )
        {
            rejected = true;
        }
        require( rejected, "Non-finite source vertices must be rejected" );
        // A tetrahedron has area in all three projection planes.
        const Array<MeshImposterTriangle> solid = {
            { { Vector3F( -.5f, 0, -.5f ), Vector3F( .5f, 0, -.5f ), Vector3F( 0, 1, 0 ) },
              ColourF::White },
            { { Vector3F( .5f, 0, -.5f ), Vector3F( 0, 0, .5f ), Vector3F( 0, 1, 0 ) }, ColourF::White },
            { { Vector3F( 0, 0, .5f ), Vector3F( -.5f, 0, -.5f ), Vector3F( 0, 1, 0 ) }, ColourF::White }
        };
        const auto allViews =
            generateMeshImposters( solid, { -.6f, 0, -.6f }, { .6f, 1.1f, .6f }, 32, 64 );
        for( size_t view = 0; view < 3; ++view )
        {
            size_t opaque = 0;
            for( size_t y = 0; y < allViews.height; ++y )
                for( size_t x = 0; x < allViews.tileWidth; ++x )
                {
                    const auto alpha = allViews.rgba[( y * allViews.width + view * 32 + x ) * 4 + 3];
                    if( x == 0 || x == 31 || y == 0 || y == 63 )
                        require( alpha == 0, "Each atlas tile needs a transparent gutter" );
                    opaque += alpha != 0;
                }
            require( opaque > 0, "Every imposter view must contain a silhouette" );
        }
        const Array<float> thresholds = { .085f, 0.f };
        using scene::LODSystem;
        require( LODSystem::selectLOD( .09f, thresholds, -1, -1, .15f, false ) == 0,
                 "Nearby tree patches should select meshes" );
        require( LODSystem::selectLOD( .07f, thresholds, 0, -1, .15f, false ) == 1,
                 "Distant tree patches should select imposters" );
        require( LODSystem::selectLOD( .09f, thresholds, 1, -1, .15f, false ) == 1,
                 "Hysteresis should prevent threshold flicker" );
        require( LODSystem::selectLOD( .10f, thresholds, 1, -1, .15f, false ) == 0,
                 "Returning camera should restore meshes" );
        require( LODSystem::selectLOD( 0.f, thresholds, 1, -1, .15f, false ) == 1,
                 "Far trees must remain visible" );
        spriteSheetContracts();
        std::cout << "Mesh imposter bake, directional sprite sheets, depth, alpha, determinism and LOD selection passed.\n";
        return 0;
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
