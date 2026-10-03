#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <iostream>
#include <stdexcept>
#include <limits>

using namespace workphone;
static void require( bool ok, const char *message )
{
    if( !ok )
        throw std::runtime_error( message );
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
        std::cout << "Mesh imposter bake, depth, alpha, determinism and tree LOD selection passed.\n";
        return 0;
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
