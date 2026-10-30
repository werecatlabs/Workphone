#include <Workphone/Scene/ScenePicking.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <WPImGui/SceneViewportToolInput.hpp>
#include <iostream>
#include <stdexcept>
#include <limits>

using namespace workphone;

namespace
{
    struct ViewportCamera
    {
        Matrix4<real_Num> view, projection;
        mutable int legacyRayCalls = 0;
        const Matrix4<real_Num> &getViewMatrix() const { return view; }
        const Matrix4<real_Num> &getProjectionMatrix() const { return projection; }
        Ray3<real_Num> getRay( f32, f32 ) const { ++legacyRayCalls; return {}; }
        s32 getScreenWidth() const { return 0; }
        s32 getScreenHeight() const { return 0; }
    };

    void require( bool value, const char *message )
    {
        if( !value )
        {
            throw std::runtime_error( message );
        }
    }
}

int main()
{
    try
    {
        const Array<Vector3<real_Num>> triangle{ { -1, -1, 0 }, { 1, -1, 0 }, { 0, 1, 0 } };
        const Array<u32> indices{ 0, 1, 2 };
        const Ray3<real_Num> ray( { 0, 0, 0 }, { 0, 0, 1 } );
        Matrix4<real_Num> world;
        world.makeTransform( { 0, 0, 5 }, Vector3<real_Num>::unit(), Quaternion<real_Num>::identity() );
        real_Num nearest = 100;
        require( scene::pickMeshTriangles( ray, world, triangle, indices, nearest ) &&
                     Math<real_Num>::equals( nearest, real_Num( 5 ) ), "translated mesh hit" );
        world.makeTransform( { 0, 0, 10 }, Vector3<real_Num>::unit(), Quaternion<real_Num>::identity() );
        require( !scene::pickMeshTriangles( ray, world, triangle, indices, nearest ), "farther mesh cannot replace nearest" );
        world.makeTransform( { 0, 0, 2 }, { -2, 0.008f, 3 }, Quaternion<real_Num>::identity() );
        require( scene::pickMeshTriangles( ray, world, triangle, indices, nearest ) &&
                     Math<real_Num>::equals( nearest, real_Num( 2 ) ), "mirrored nonuniform import scale" );
        nearest = 100;
        require( !scene::pickMeshTriangles( Ray3<real_Num>( { 1.9f, 0.007f, 0 }, { 0, 0, 1 } ),
                     world, triangle, indices, nearest ), "bounds hit outside triangle is a miss" );
        nearest = 1;
        require( !scene::pickMeshTriangles( ray, world, triangle, indices, nearest ), "far clip limits picking" );
        nearest = 100;
        require( !scene::pickMeshTriangles( Ray3<real_Num>( { 0, 0, 3 }, { 0, 0, 1 } ),
                     world, triangle, indices, nearest ), "geometry behind ray is ignored" );
        require( !scene::pickMeshTriangles( ray, world, triangle, { 0, 1, 99 }, nearest ), "invalid index is safe" );
        require( !scene::pickMeshTriangles( ray, world, triangle, { 0, 0, 0 }, nearest ), "degenerate triangle is ignored" );
        require( !scene::pickMeshTriangles( Ray3<real_Num>( { 0, 0, 0 }, { 0, 0, 0 } ),
                     world, triangle, indices, nearest ), "zero-length ray is safe" );

        auto view = Matrix4<real_Num>::identity();
        Matrix4<real_Num> projection;
        projection.makePerspective( real_Num( 1.0 ), real_Num( 1.5 ), real_Num( 0.1 ), real_Num( 100 ) );
        Ray3<real_Num> cameraRay;
        real_Num range;
        require( scene::makeScenePickingRay( view, projection, { 0.5f, 0.5f }, cameraRay, range ),
                 "normalized viewport ray does not depend on pixel dimensions" );
        require( Math<real_Num>::equals( cameraRay.getOrigin().z, real_Num( -0.1 ) ) &&
                     cameraRay.getDirection().z < -0.999f, "centre ray uses near plane and camera forward" );
        require( scene::makeScenePickingRay( view, projection, { 0.75f, 0.25f }, cameraRay, range ) &&
                     cameraRay.getDirection().x > 0 && cameraRay.getDirection().y > 0,
                 "off-centre viewport ray has correct axes" );

        ViewportCamera camera;
        const Vector3<real_Num> cameraPosition( 0, 80, 100 ), terrainPoint( 18, 0, -22 );
        const auto orientation = MathUtil<real_Num>::getOrientationFromDirection(
            ( -cameraPosition ).normaliseCopy(), -Vector3<real_Num>::unitZ(), true,
            Vector3<real_Num>::unitY() );
        camera.view.makeViewMatrix( cameraPosition, orientation, nullptr );
        camera.projection.makePerspective( real_Num( 1.0 ), real_Num( 1.5 ),
                                            real_Num( 0.1 ), real_Num( 1000 ) );
        const auto clip = camera.projection * camera.view;
        real_Num projected[4]{};
        for( u32 row = 0; row < 4; ++row )
            projected[row] = clip[row][0] * terrainPoint.x + clip[row][1] * terrainPoint.y +
                             clip[row][2] * terrainPoint.z + clip[row][3];
        const Vector2F imagePoint( f32( projected[0] / projected[3] * .5 + .5 ),
                                   f32( .5 - projected[1] / projected[3] * .5 ) );
        require( imagePoint.x > 0 && imagePoint.x < 1 && imagePoint.y > 0 && imagePoint.y < 1,
                 "terrain fixture must project inside the displayed image" );
        render::TerrainData terrain;
        terrain.dimensions = { 65, 65 };
        terrain.origin = { -64, -64 };
        terrain.spacing = { 2, 2 };
        terrain.heights.assign( 65 * 65, 0 );
        const auto originalHeights = terrain.heights;
        Parameters tool;
        for( const auto &rect : { std::pair<Vector2F, Vector2F>( { 8, 80 }, { 384, 300 } ),
                                 std::pair<Vector2F, Vector2F>( { 123.75f, 91.5f }, { 1266.5f, 717.25f } ) } )
        {
            const auto mouse = rect.first + imagePoint * rect.second;
            require( ui::makeSceneViewportToolArguments( camera, rect.first, rect.second, mouse,
                         ui::terrainViewportBrushPhase( false, true, false, true, true ), tool ),
                     "actual viewport pixels must produce a tool ray" );
            require( tool.size() == 7 && tool[0].getStr() == "begin",
                     "viewport boundary must preserve the existing seven-field brush protocol" );
            const Ray3F terrainRay( { f32( tool[1].getF64() ), f32( tool[2].getF64() ), f32( tool[3].getF64() ) },
                                    { f32( tool[4].getF64() ), f32( tool[5].getF64() ), f32( tool[6].getF64() ) } );
            Vector3<real_Num> hit;
            const bool terrainHit = render::intersectTerrain( terrain, {}, terrainRay, hit );
            real_Num hitClip[4]{};
            for( u32 row = 0; row < 4; ++row )
                hitClip[row] = clip[row][0] * hit.x + clip[row][1] * hit.y +
                               clip[row][2] * hit.z + clip[row][3];
            const Vector2F renderedHit( rect.first.x + f32( hitClip[0] / hitClip[3] * .5 + .5 ) * rect.second.x,
                                        rect.first.y + f32( .5 - hitClip[1] / hitClip[3] * .5 ) * rect.second.y );
            const auto pixelError = ( renderedHit - mouse ).length();
            if( !terrainHit || !( pixelError < .25f ) )
                std::cerr << "Viewport diagnostic: hit=" << terrainHit << " point=" << hit.x << ',' << hit.y << ',' << hit.z
                          << " worldError=" << ( hit - terrainPoint ).length() << " pixelError=" << pixelError << " mouse=" << mouse.x << ',' << mouse.y
                          << " ray=" << terrainRay.getOrigin().x << ',' << terrainRay.getOrigin().y << ',' << terrainRay.getOrigin().z
                          << " dir=" << terrainRay.getDirection().x << ',' << terrainRay.getDirection().y << ',' << terrainRay.getDirection().z << '\n';
            // Validate the actual UI boundary in displayed pixels, including
            // fractional DPI geometry, rather than imposing a world-space
            // precision guarantee on the existing float unprojection helper.
            require( terrainHit && pixelError < .25f,
                     "viewport mouse must hit the noncentral terrain surface rendered at that pixel" );
        }
        require( camera.legacyRayCalls == 0,
                 "normalized viewport tools must not enter the legacy pixel-based getRay API" );
        require( ui::makeSceneViewportToolArguments( camera, { 8, 86 }, { 384, 300 },
                     { 230, 70 }, ui::terrainViewportBrushPhase( false, false, true, false, false ), tool ) &&
                     tool[0].getStr() == "cancel",
                 "release outside must cancel even when no held-outside frame was sampled" );
        require( String( ui::terrainViewportBrushPhase( false, true, true, false, false ) ) == "commit" &&
                     String( ui::terrainViewportBrushPhase( true, true, true, false, false ) ) == "cancel" &&
                     String( ui::terrainViewportBrushPhase( false, true, false, false, true ) ) == "update" &&
                     String( ui::terrainViewportBrushPhase( false, true, false, false, false ) ) == "hover",
                 "terrain phase ordering must preserve inside release, forced cancel, drag and hover" );
        require( ui::makeSceneViewportToolArguments( camera, { 8, 80 }, { 384, 300 },
                     { -20, 40 }, "hover", tool ) && tool[0].getStr() == "hover",
                 "finite pointer visits outside the image must retain the city draft hover phase" );
        require( !ui::makeSceneViewportToolArguments( camera, { 8, 80 }, { 0, 300 },
                     { 100, 150 }, "update", tool ) && tool[0].getStr() == "cancel",
                 "invalid viewport geometry must cancel an active tool instead of sending an edit" );
        camera.projection[0][0] = std::numeric_limits<real_Num>::quiet_NaN();
        require( !ui::makeSceneViewportToolArguments( camera, { 8, 80 }, { 384, 300 },
                     { 100, 150 }, "commit", tool ) && tool[0].getStr() == "cancel" &&
                     terrain.heights == originalHeights,
                 "invalid camera projection must cancel without mutating terrain data" );
        std::cout << "PASS: scene picking transforms, nearest hit, triangle precision, clip distance and invalid geometry\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
