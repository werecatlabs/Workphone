#include <workphone_physics_scene.h>
#include <workphone_physics_rigidbody.h>
#include <cmath>
#include <cstdio>

int main()
{
    // A 90-degree yaw maps world X torque onto the body's Z principal axis.
    // Compare the same torque in two poses with deliberately unequal moments.
    for( int rotated = 0; rotated < 2; ++rotated )
    {
        auto scene = wp_physics_scene_create();
        auto body = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
        wp_physics_scene_set_gravity( scene, { 0, 0, 0 } );
        wp_rigidbody_set_inertia_tensor( body, { 100, 200, 10 } );
        wp_rigidbody_set_angular_damping( body, 0 );
        wp_rigidbody_set_sleep_threshold( body, 0 );
        wp_rigidbody_set_orientation(
            body, rotated ? wp_quatf{ 0, .707106781f, 0, .707106781f } : wp_quatf{ 0, 0, 0, 1 } );
        wp_physics_scene_add_actor( scene, body );
        wp_rigidbody_add_torque( body, { 10, 0, 0 }, WORKPHONE_FORCE_MODE_FORCE );
        wp_physics_scene_simulate( scene, .01f );
        const auto angular = wp_rigidbody_get_angular_velocity( body );
        const auto expected = rotated ? .01f : .001f;
        const bool passed = std::abs( angular.x - expected ) < 1e-5f && std::abs( angular.y ) < 1e-5f &&
                            std::abs( angular.z ) < 1e-5f;
        wp_physics_scene_remove_actor( scene, body );
        wp_rigidbody_destroy( body );
        wp_physics_scene_destroy( scene );
        if( !passed )
        {
            std::fprintf( stderr, "Rotated inertia regression failed: pose %d, wx=%f\n", rotated,
                          angular.x );
            return 1;
        }
    }
    std::puts( "World-space torque uses rotated principal inertia: passed." );
    return 0;
}
