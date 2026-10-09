#include <workphone_physics_bounds.h>
#include <workphone_physics_collisionshape.h>
#include <workphone_physics_narrowphase.h>
#include <workphone_physics_scene.h>
#include <workphone_physics_triangle_mesh.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while(0)

/* Existing native contact-policy setter, absent from the current public header. */
void wp_physics_scene_set_contact_options( wp_physics_scene *scene, const wp_contact_options *options );

static wp_u32 seed = 7919;
static wp_quatf orientation_from_axis_angle( wp_vec3f axis, float angle )
{
    float length = sqrtf(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
    float s = sinf(angle*.5f) / length;
    return (wp_quatf){cosf(angle*.5f), axis.x*s, axis.y*s, axis.z*s};
}
static wp_vec3f rotate_point( wp_quatf q, wp_vec3f v )
{
    /* Independent matrix form for containment checks against the fitted basis. */
    return (wp_vec3f){
        (1-2*(q.y*q.y+q.z*q.z))*v.x + 2*(q.x*q.y-q.w*q.z)*v.y + 2*(q.x*q.z+q.w*q.y)*v.z,
        2*(q.x*q.y+q.w*q.z)*v.x + (1-2*(q.x*q.x+q.z*q.z))*v.y + 2*(q.y*q.z-q.w*q.x)*v.z,
        2*(q.x*q.z-q.w*q.y)*v.x + 2*(q.y*q.z+q.w*q.x)*v.y + (1-2*(q.x*q.x+q.y*q.y))*v.z};
}
static float random_float( float low, float high )
{
    seed = seed * 1664525u + 1013904223u;
    return low + (high - low) * (float)(seed >> 8) / (float)0xffffff;
}
static wp_quatf random_orientation( void )
{
    wp_vec3f axis = { random_float(-1, 1), random_float(-1, 1), random_float(-1, 1) };
    return orientation_from_axis_angle( axis, random_float(-3.14f, 3.14f) );
}
static wp_vec3f add( wp_vec3f a, wp_vec3f b )
{
    return (wp_vec3f){ a.x+b.x, a.y+b.y, a.z+b.z };
}
static wp_vec3f scale( wp_vec3f a, float s )
{
    return (wp_vec3f){ a.x*s, a.y*s, a.z*s };
}
static float dot( wp_vec3f a, wp_vec3f b )
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static void check_contains( const wp_body_obb *obb, wp_vec3f point )
{
    wp_vec3f delta = add( point, scale(obb->center, -1) );
    CHECK(obb && obb->valid);
    CHECK(fabsf(dot(delta, obb->axes[0])) <= obb->half.x + .002f);
    CHECK(fabsf(dot(delta, obb->axes[1])) <= obb->half.y + .002f);
    CHECK(fabsf(dot(delta, obb->axes[2])) <= obb->half.z + .002f);
}
static wp_vec3f shape_point( wp_rigidbody *body, wp_collision_shape *shape, wp_vec3f point )
{
    point = add( wp_collision_shape_get_local_position(shape),
                 rotate_point(wp_collision_shape_get_local_orientation(shape), point) );
    return add( wp_rigidbody_get_position(body),
                rotate_point(wp_rigidbody_get_orientation(body), point) );
}
static void destroy_body( wp_rigidbody *body )
{
    while(wp_rigidbody_get_shape_count(body))
        wp_collision_shape_destroy(wp_rigidbody_get_shape(body, 0));
    wp_rigidbody_destroy(body);
}

static void test_geometry_fit_and_cache( void )
{
    wp_rigidbody *body = wp_rigidbody_create(WORKPHONE_RIGIDBODY_STATIC);
    wp_collision_shape *box = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
    wp_collision_shape *sphere = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_SPHERE);
    wp_collision_shape *capsule = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_CAPSULE);
    const wp_body_obb *obb;
    wp_s32 local, world;
    CHECK(body && box && sphere && capsule);
    wp_collision_shape_set_box_half_extents(box, (wp_vec3f){10, 1, .5f});
    wp_collision_shape_set_local_orientation(box, orientation_from_axis_angle((wp_vec3f){0, 0, 1}, .7f));
    wp_collision_shape_set_local_position(box, (wp_vec3f){3, 2, 1});
    CHECK(wp_rigidbody_add_shape(body, box) >= 0);
    obb = wp_body_get_obb(body, &local, &world);
    CHECK(local == 1 && world == 1 && obb->valid && obb->useful);
    CHECK(fabsf(obb->half.x - 10) < .0001f && fabsf(obb->half.y - 1) < .0001f);
    wp_body_get_obb(body, &local, &world);
    CHECK(local == 0 && world == 0);
    wp_rigidbody_set_position(body, (wp_vec3f){100, -50, 70});
    wp_body_get_obb(body, &local, &world);
    CHECK(local == 0 && world == 1);
    wp_rigidbody_set_orientation(body, random_orientation());
    wp_body_get_obb(body, &local, &world);
    CHECK(local == 0 && world == 1);
    CHECK(wp_rigidbody_add_shape(body, sphere) >= 0);
    CHECK(wp_rigidbody_add_shape(body, capsule) >= 0);
    for(int round = 0; round < 250; ++round)
    {
        wp_collision_shape *shapes[] = {box, sphere, capsule};
        wp_rigidbody_set_position(body, (wp_vec3f){random_float(-100,100), random_float(-100,100), random_float(-100,100)});
        wp_rigidbody_set_orientation(body, random_orientation());
        for(int s = 0; s < 3; ++s)
        {
            wp_collision_shape_set_local_position(shapes[s], (wp_vec3f){random_float(-20,20), random_float(-20,20), random_float(-20,20)});
            wp_collision_shape_set_local_orientation(shapes[s], random_orientation());
        }
        wp_collision_shape_set_sphere_radius(sphere, 2);
        wp_collision_shape_set_capsule(capsule, 1.5f, 5);
        obb = wp_body_get_obb(body, &local, &world);
        CHECK(local == 1 && world == 1 && obb->valid);
        for(int corner = 0; corner < 8; ++corner)
            check_contains(obb, shape_point(body, box, (wp_vec3f){corner&1 ? 10.0f : -10.0f, corner&2 ? 1.0f : -1.0f, corner&4 ? .5f : -.5f}));
        for(int sample = 0; sample < 30; ++sample)
        {
            wp_vec3f unit = rotate_point(random_orientation(), (wp_vec3f){1,0,0});
            check_contains(obb, shape_point(body, sphere, scale(unit, 2)));
            check_contains(obb, shape_point(body, capsule, add(scale(unit, 1.5f), (wp_vec3f){0, sample&1 ? 5.0f : -5.0f, 0})));
        }
    }
    /* Enable, local pose, dimensions, attachment and destruction must invalidate. */
    wp_collision_shape_set_enabled(box, 0);
    wp_body_get_obb(body, &local, &world); CHECK(local && world);
    wp_collision_shape_set_local_position(sphere, (wp_vec3f){40,0,0});
    obb = wp_body_get_obb(body, &local, &world); CHECK(local && world);
    check_contains(obb, shape_point(body, sphere, (wp_vec3f){2,0,0}));
    wp_collision_shape_destroy(capsule);
    wp_body_get_obb(body, &local, &world); CHECK(local && world);
    wp_collision_shape_set_sphere_radius(sphere, 4);
    obb = wp_body_get_obb(body, &local, &world); CHECK(local && world);
    check_contains(obb, shape_point(body, sphere, (wp_vec3f){4,0,0}));
    destroy_body(body);
}

static void test_mesh_refit_and_fallback( void )
{
    wp_rigidbody *body = wp_rigidbody_create(WORKPHONE_RIGIDBODY_STATIC);
    wp_collision_shape *mesh = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_MESH);
    wp_collision_shape *plane = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_PLANE);
    wp_f32 vertices[] = {-10, 0, -1, 10, 0, -1, 10, 0, 1, -10, 0, 1};
    wp_u32 indices[] = {0,1,2, 0,2,3};
    wp_collision_mesh_data data = {vertices, 4, indices, 2};
    wp_s32 local, world;
    const wp_body_obb *obb;
    CHECK(body && mesh && plane);
    wp_collision_shape_set_mesh_data(mesh, &data);
    wp_collision_shape_set_local_position(mesh, (wp_vec3f){3,2,1});
    wp_collision_shape_set_local_orientation(mesh, random_orientation());
    wp_rigidbody_set_orientation(body, random_orientation());
    CHECK(wp_rigidbody_add_shape(body, mesh) >= 0);
    obb = wp_body_get_obb(body, &local, &world);
    CHECK(local && world && obb->valid);
    for(int i = 0; i < 4; ++i)
        check_contains(obb, shape_point(body, mesh, (wp_vec3f){vertices[i*3], vertices[i*3+1], vertices[i*3+2]}));
    vertices[0] = -30;
    wp_triangle_mesh_refit_aabb((wp_triangle_mesh *)wp_collision_shape_get_triangle_mesh(mesh));
    obb = wp_body_get_obb(body, &local, &world);
    CHECK(local && world && obb->valid);
    check_contains(obb, shape_point(body, mesh, (wp_vec3f){-30,0,-1}));
    CHECK(wp_rigidbody_add_shape(body, plane) >= 0);
    CHECK(!wp_body_get_obb(body, &local, &world)->valid);
    wp_collision_shape_set_enabled(plane, 0);
    CHECK(wp_body_get_obb(body, &local, &world)->valid);
    data.indices = NULL; data.triangle_count = 0;
    wp_collision_shape_set_mesh_data(mesh, &data);
    CHECK(!wp_body_get_obb(body, &local, &world)->valid);
    vertices[0] = -50; /* Unobservable borrowed edits keep the fallback. */
    CHECK(!wp_body_get_obb(body, &local, &world)->valid);
    wp_collision_shape_set_mesh_data(mesh, NULL);
    CHECK(!wp_body_get_obb(body, &local, &world)->valid);
    destroy_body(body);
}

static void test_narrowphase_oracle( void )
{
    wp_narrowphase *np = wp_narrowphase_create(WORKPHONE_NARROWPHASE_SAT);
    int contacts = 0, rejections = 0;
    CHECK(np);
    wp_narrowphase_set_contact_tolerance(np, .001f);
    for(int ta = WORKPHONE_COLLISION_SHAPE_BOX; ta <= WORKPHONE_COLLISION_SHAPE_CAPSULE; ++ta)
        for(int tb = WORKPHONE_COLLISION_SHAPE_BOX; tb <= WORKPHONE_COLLISION_SHAPE_CAPSULE; ++tb)
        {
            wp_rigidbody *a = wp_rigidbody_create(WORKPHONE_RIGIDBODY_DYNAMIC);
            wp_rigidbody *b = wp_rigidbody_create(WORKPHONE_RIGIDBODY_STATIC);
            wp_collision_shape *sa = wp_collision_shape_create((wp_collision_shape_type)ta);
            wp_collision_shape *sb = wp_collision_shape_create((wp_collision_shape_type)tb);
            CHECK(a && b && sa && sb);
            CHECK(wp_rigidbody_add_shape(a, sa) >= 0 && wp_rigidbody_add_shape(b, sb) >= 0);
            for(int round = 0; round < 600; ++round)
            {
                const wp_body_obb *oa, *ob;
                wp_contact_manifold manifold;
                wp_s32 local, world, may_overlap;
                wp_rigidbody *bodies[] = {a,b};
                wp_collision_shape *shapes[] = {sa,sb};
                wp_vec3f common = {random_float(-10000,10000), random_float(-10000,10000), random_float(-10000,10000)};
                for(int i = 0; i < 2; ++i)
                {
                    float spread = round%2 ? 8 : .5f;
                    wp_rigidbody_set_position(bodies[i], add(common, (wp_vec3f){random_float(-spread,spread), random_float(-spread,spread), random_float(-spread,spread)}));
                    wp_rigidbody_set_orientation(bodies[i], random_orientation());
                    wp_collision_shape_set_local_orientation(shapes[i], random_orientation());
                    wp_collision_shape_set_local_position(shapes[i], (wp_vec3f){random_float(-.5f,.5f), random_float(-.5f,.5f), random_float(-.5f,.5f)});
                    wp_collision_shape_set_box_half_extents(shapes[i], (wp_vec3f){random_float(.1f,10), random_float(.1f,2), random_float(.1f,2)});
                    wp_collision_shape_set_capsule(shapes[i], random_float(.2f,2), random_float(.1f,6));
                    wp_collision_shape_set_sphere_radius(shapes[i], random_float(.2f,2));
                }
                oa = wp_body_get_obb(a, &local, &world);
                ob = wp_body_get_obb(b, &local, &world);
                CHECK(oa->valid && ob->valid);
                may_overlap = wp_body_obb_may_overlap(oa, ob, .001f);
                CHECK(may_overlap == wp_body_obb_may_overlap(ob, oa, .001f));
                if(!may_overlap) ++rejections;
                if(wp_narrowphase_test_pair(np, a, sa, b, sb, &manifold))
                {
                    ++contacts;
                    CHECK(may_overlap); /* An independent exact-contact oracle. */
                }
            }
            destroy_body(a); destroy_body(b);
        }
    CHECK(contacts > 1000 && rejections > 1000);
    wp_narrowphase_destroy(np);
}

static void test_parallel_bars( float angle, float spacing, uint64_t pairs, uint64_t survivors, int simd )
{
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *bodies[128];
    wp_quatf orientation = orientation_from_axis_angle((wp_vec3f){0,0,1}, angle);
    wp_vec3f normal = {-sinf(angle), cosf(angle), 0};
    wp_scene_broadphase_stats stats;
    CHECK(scene && wp_physics_scene_get_broadphase_obb_enabled(scene));
    wp_physics_scene_set_gravity(scene, (wp_vec3f){0,0,0});
    wp_physics_scene_set_broadphase_simd_enabled(scene, simd);
    for(int i = 0; i < 128; ++i)
    {
        wp_collision_shape *shape = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
        bodies[i] = wp_rigidbody_create(i == 64 ? WORKPHONE_RIGIDBODY_DYNAMIC : WORKPHONE_RIGIDBODY_STATIC);
        CHECK(shape && bodies[i]);
        wp_collision_shape_set_box_half_extents(shape, (wp_vec3f){10,1,1});
        wp_collision_shape_set_trigger(shape, 1); /* Count contacts without moving the fixture. */
        CHECK(wp_rigidbody_add_shape(bodies[i], shape) >= 0);
        wp_rigidbody_set_position(bodies[i], scale(normal, (i-64)*spacing));
        wp_rigidbody_set_orientation(bodies[i], orientation);
        CHECK(wp_physics_scene_add_actor(scene, bodies[i]));
    }
    wp_physics_scene_simulate(scene, 1.0f/60);
    stats = wp_physics_scene_get_broadphase_stats(scene);
    CHECK(stats.candidate_pairs == pairs && stats.narrowphase_tests == survivors);
    CHECK(stats.obb_rejections == pairs-survivors);
    wp_physics_scene_simulate(scene, 1.0f/60);
    stats = wp_physics_scene_get_broadphase_stats(scene);
    CHECK(stats.obb_local_rebuilds == 0 && stats.obb_world_updates == 0);
    CHECK(stats.candidate_pairs == pairs && stats.narrowphase_tests == survivors);
    wp_physics_scene_set_broadphase_obb_enabled(scene, 0);
    CHECK(!wp_physics_scene_get_broadphase_obb_enabled(scene));
    wp_physics_scene_simulate(scene, 1.0f/60);
    stats = wp_physics_scene_get_broadphase_stats(scene);
    CHECK(stats.candidate_pairs == pairs && stats.narrowphase_tests == pairs && stats.obb_tests == 0);
    wp_physics_scene_destroy(scene);
    for(int i = 0; i < 128; ++i) destroy_body(bodies[i]);
}

static wp_vec3f correction_case( int enabled, int simd )
{
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *bodies[3];
    wp_quatf orientation = orientation_from_axis_angle((wp_vec3f){0,0,1}, .78539816f);
    wp_vec3f normal = {-.70710678f, .70710678f, 0}, result;
    float offsets[] = {0, -1.5f, 2.1f};
    wp_scene_broadphase_stats stats;
    CHECK(scene);
    wp_physics_scene_set_gravity(scene, (wp_vec3f){0,0,0});
    wp_physics_scene_set_broadphase_obb_enabled(scene, enabled);
    wp_physics_scene_set_broadphase_simd_enabled(scene, simd);
    for(int i = 0; i < 3; ++i)
    {
        wp_collision_shape *shape = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
        bodies[i] = wp_rigidbody_create(i ? WORKPHONE_RIGIDBODY_STATIC : WORKPHONE_RIGIDBODY_DYNAMIC);
        CHECK(shape && bodies[i]);
        wp_collision_shape_set_box_half_extents(shape, (wp_vec3f){10,1,1});
        CHECK(wp_rigidbody_add_shape(bodies[i], shape) >= 0);
        wp_rigidbody_set_orientation(bodies[i], orientation);
        wp_rigidbody_set_position(bodies[i], scale(normal, offsets[i]));
        CHECK(wp_physics_scene_add_actor(scene, bodies[i]));
    }
    /* The first wall pushes the dynamic bar into the initially separated second
     * wall in the same visitor. A snapshot OBB would incorrectly prune it. */
    wp_physics_scene_simulate(scene, 1.0f/60);
    stats = wp_physics_scene_get_broadphase_stats(scene);
    CHECK(stats.candidate_pairs == 2 && stats.narrowphase_tests == 2 && stats.obb_rejections == 0);
    if(enabled) CHECK(stats.obb_tests == 2 && stats.obb_world_updates == 4 && stats.obb_local_rebuilds == 3);
    result = wp_rigidbody_get_position(bodies[0]);
    wp_physics_scene_destroy(scene);
    for(int i = 0; i < 3; ++i) destroy_body(bodies[i]);
    return result;
}

static void test_touching_and_nearly_parallel( void )
{
    wp_rigidbody *a = wp_rigidbody_create(WORKPHONE_RIGIDBODY_DYNAMIC);
    wp_rigidbody *b = wp_rigidbody_create(WORKPHONE_RIGIDBODY_STATIC);
    wp_collision_shape *sa = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
    wp_collision_shape *sb = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
    wp_narrowphase *np = wp_narrowphase_create(WORKPHONE_NARROWPHASE_SAT);
    CHECK(a && b && sa && sb && np);
    CHECK(wp_rigidbody_add_shape(a, sa) >= 0 && wp_rigidbody_add_shape(b, sb) >= 0);
    wp_collision_shape_set_box_half_extents(sa, (wp_vec3f){10,1,1});
    wp_collision_shape_set_box_half_extents(sb, (wp_vec3f){10,1,1});
    wp_narrowphase_set_contact_tolerance(np, .001f);
    for(int round = 0; round < 200; ++round)
    {
        wp_quatf orientation = orientation_from_axis_angle((wp_vec3f){0,0,1}, .78539816f);
        wp_vec3f normal = {-.70710678f,.70710678f,0};
        wp_vec3f origin = round < 100 ? (wp_vec3f){0,0,0} : (wp_vec3f){100000,100000,100000};
        wp_s32 local, world;
        wp_contact_manifold manifold;
        const wp_body_obb *oa, *ob;
        wp_rigidbody_set_orientation(a, orientation);
        wp_rigidbody_set_orientation(b, orientation_from_axis_angle((wp_vec3f){0,0,1}, .78539816f + (round%5)*.0000001f));
        wp_rigidbody_set_position(a, origin);
        wp_rigidbody_set_position(b, add(origin, scale(normal, 2 + (round%2)*.0005f)));
        oa = wp_body_get_obb(a, &local, &world);
        ob = wp_body_get_obb(b, &local, &world);
        CHECK(wp_body_obb_may_overlap(oa, ob, .001f));
        if(round < 100) CHECK(wp_narrowphase_test_pair(np, a, sa, b, sb, &manifold));
    }
    wp_rigidbody_set_position(a, (wp_vec3f){NAN,0,0});
    CHECK(!wp_body_get_obb(a, &(wp_s32){0}, &(wp_s32){0})->valid);
    wp_rigidbody_set_position(a, (wp_vec3f){0,0,0});
    wp_rigidbody_set_orientation(a, (wp_quatf){NAN,0,0,0});
    CHECK(!wp_body_get_obb(a, &(wp_s32){0}, &(wp_s32){0})->valid);
    wp_narrowphase_destroy(np);
    destroy_body(a); destroy_body(b);
}

static void test_large_offset_cancellation( void )
{
    wp_rigidbody *a = wp_rigidbody_create(WORKPHONE_RIGIDBODY_DYNAMIC);
    wp_rigidbody *b = wp_rigidbody_create(WORKPHONE_RIGIDBODY_STATIC);
    wp_collision_shape *sa = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_SPHERE);
    wp_collision_shape *sb = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_SPHERE);
    wp_narrowphase *np = wp_narrowphase_create(WORKPHONE_NARROWPHASE_SAT);
    wp_s32 local, world;
    const wp_body_obb *oa, *ob;
    wp_contact_manifold manifold;
    CHECK(a && b && sa && sb && np);
    wp_collision_shape_set_sphere_radius(sa, .1f);
    wp_collision_shape_set_sphere_radius(sb, .1f);
    wp_collision_shape_set_local_position(sa, (wp_vec3f){10000000,0,0});
    wp_collision_shape_set_local_orientation(sa, orientation_from_axis_angle((wp_vec3f){0,0,1}, .78539816f));
    wp_rigidbody_set_position(a, (wp_vec3f){-10000000,0,0});
    wp_rigidbody_set_position(b, (wp_vec3f){.15f,0,0});
    CHECK(wp_rigidbody_add_shape(a, sa) >= 0 && wp_rigidbody_add_shape(b, sb) >= 0);
    oa = wp_body_get_obb(a, &local, &world);
    ob = wp_body_get_obb(b, &local, &world);
    CHECK(oa->valid && ob->valid);
    CHECK(wp_narrowphase_test_pair(np, a, sa, b, sb, &manifold));
    CHECK(wp_body_obb_may_overlap(oa, ob, .001f));
    wp_narrowphase_destroy(np);
    destroy_body(a); destroy_body(b);
}

static void test_rejected_manifold_is_invalidated( int simd )
{
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *a = wp_rigidbody_create(WORKPHONE_RIGIDBODY_DYNAMIC);
    wp_rigidbody *b = wp_rigidbody_create(WORKPHONE_RIGIDBODY_STATIC);
    wp_collision_shape *sa = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
    wp_collision_shape *sb = wp_collision_shape_create(WORKPHONE_COLLISION_SHAPE_BOX);
    wp_contact_options options = {WP_CONTACT_STRATEGY_FIXED, 1000, 100, 1};
    wp_quatf orientation = orientation_from_axis_angle((wp_vec3f){0,0,1}, .78539816f);
    wp_vec3f normal = {-.70710678f,.70710678f,0};
    wp_scene_broadphase_stats stats;
    CHECK(scene && a && b && sa && sb);
    wp_physics_scene_set_gravity(scene, (wp_vec3f){0,0,0});
    wp_physics_scene_set_broadphase_simd_enabled(scene, simd);
    wp_physics_scene_set_contact_options(scene, &options);
    wp_collision_shape_set_box_half_extents(sa, (wp_vec3f){10,1,1});
    wp_collision_shape_set_box_half_extents(sb, (wp_vec3f){10,1,1});
    wp_collision_shape_set_trigger(sa, 1);
    CHECK(wp_rigidbody_add_shape(a, sa) >= 0 && wp_rigidbody_add_shape(b, sb) >= 0);
    wp_rigidbody_set_orientation(a, orientation); wp_rigidbody_set_orientation(b, orientation);
    wp_rigidbody_set_position(b, scale(normal, 1.5f));
    CHECK(wp_physics_scene_add_actor(scene, a) && wp_physics_scene_add_actor(scene, b));
    wp_physics_scene_simulate(scene, 1.0f/60);
    CHECK(wp_physics_scene_get_broadphase_stats(scene).narrowphase_tests == 1);
    wp_physics_scene_simulate(scene, 1.0f/60);
    CHECK(wp_physics_scene_get_broadphase_stats(scene).narrowphase_tests == 0); /* cache reuse */
    wp_rigidbody_set_position(b, scale(normal, 3));
    wp_physics_scene_simulate(scene, 1.0f/60);
    stats = wp_physics_scene_get_broadphase_stats(scene);
    CHECK(stats.candidate_pairs == 1 && stats.obb_rejections == 1 && stats.narrowphase_tests == 0);
    wp_rigidbody_set_position(b, scale(normal, 1.5f));
    wp_physics_scene_simulate(scene, 1.0f/60);
    CHECK(wp_physics_scene_get_broadphase_stats(scene).narrowphase_tests == 1);
    wp_physics_scene_destroy(scene);
    destroy_body(a); destroy_body(b);
}

int main( void )
{
    test_geometry_fit_and_cache();
    test_mesh_refit_and_fallback();
    test_narrowphase_oracle();
    test_touching_and_nearly_parallel();
    test_large_offset_cancellation();
    for(int simd = 0; simd < 2; ++simd)
    {
        wp_vec3f filtered, reference;
        test_parallel_bars(0, 3, 0, 0, simd);
        test_parallel_bars(.78539816f, 3, 14, 0, simd);
        test_parallel_bars(.78539816f, 1.5f, 28, 2, simd);
        filtered = correction_case(1, simd);
        reference = correction_case(0, simd);
        CHECK(fabsf(filtered.x-reference.x) < .00001f && fabsf(filtered.y-reference.y) < .00001f);
        test_rejected_manifold_is_invalidated(simd);
    }
    puts("Oriented bounds: containment, cache invalidation, mesh refit/fallback, exact-contact oracle, scalar/SSE pairs and live solver corrections passed.");
    return 0;
}
