#include <Workphone/Scene/ScenePicking.hpp>
#include <iostream>
#include <stdexcept>

using namespace workphone;

namespace
{
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
        std::cout << "PASS: scene picking transforms, nearest hit, triangle precision, clip distance and invalid geometry\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
