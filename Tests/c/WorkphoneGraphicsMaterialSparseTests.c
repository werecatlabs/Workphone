/* Isolate the native implementation to exercise allocator failure and masks
   across 32-slot boundaries without changing the public allocator contract. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int fail_after = -1;
static int live_allocations;
static int allocation_calls;

static void *material_test_alloc( size_t size )
{
    void *result;
    ++allocation_calls;
    if( fail_after == 0 ) return NULL;
    if( fail_after > 0 ) --fail_after;
    result = malloc( size );
    if( result ) ++live_allocations;
    return result;
}

static void material_test_free( void *value )
{
    if( value ) --live_allocations;
    free( value );
}

#define WP_MATERIAL_MAX_TEXTURES 40
#define malloc material_test_alloc
#define free material_test_free
#include "../../Engine/c/Source/WorkphoneGraphics/workphone_graphics_material.c"
#undef malloc
#undef free

/* Device/program behavior is covered by the linked shader/renderer suites.
   These stubs satisfy only the unrelated native submission dependencies. */
void wp_shader_program_retain( wp_shader_program *program ) { (void)program; }
void wp_shader_program_destroy( wp_shader_program *program ) { (void)program; }
wp_s32 wp_shader_program_bind( wp_shader_program *program, wp_renderer *renderer )
{ (void)program; (void)renderer; return 1; }
wp_s32 wp_renderer_reset_program( wp_renderer *renderer ) { (void)renderer; return 1; }
void wp_renderer_set_blend_mode( wp_renderer *renderer, wp_blend_mode mode )
{ (void)renderer; (void)mode; }
void wp_renderer_set_cull_mode( wp_renderer *renderer, wp_cull_mode mode )
{ (void)renderer; (void)mode; }
void wp_renderer_set_depth_test_enabled( wp_renderer *renderer, wp_s32 enabled )
{ (void)renderer; (void)enabled; }
void wp_renderer_set_depth_write_enabled( wp_renderer *renderer, wp_s32 enabled )
{ (void)renderer; (void)enabled; }
void wp_renderer_set_texture_native( wp_renderer *renderer, void *texture_view )
{ (void)renderer; (void)texture_view; }

static int check( int condition, const char *message )
{
    if( condition ) return 1;
    fprintf( stderr, "sparse material test failed: %s\n", message );
    return 0;
}

int main( void )
{
    wp_graphics_material *material;
    wp_graphics_material_desc desired;
    const wp_c8 *borrowed;
    wp_u32 revision, storage;
    int ok, i, calls, allocations;
    material = wp_graphics_material_create();
    if( !material ) return 1;
    wp_graphics_material_set_texture_name( material, 39, "Last" );
    wp_graphics_material_set_texture_name( material, 0, "First" );
    borrowed = wp_graphics_material_get_texture_name( material, 39 );
    wp_graphics_material_clear_dirty( material );
    revision = wp_graphics_material_get_revision( material );
    storage = wp_graphics_material_get_storage_size( material );
    allocations = live_allocations;
    desired.struct_size = sizeof( desired );
    wp_graphics_material_get_desc( material, &desired );
    desired.roughness = 0.75f;
    strcpy( desired.texture_names[0], "Replaced" );
    strcpy( desired.texture_names[32], "New" );
    ok = 1;
    for( i = 0; i < 3; ++i )
    {
        fail_after = i;
        ok &= check( !wp_graphics_material_set_desc( material, &desired ),
                     "descriptor allocation failure is reported" );
        fail_after = -1;
        ok &= check( live_allocations == allocations &&
                     wp_graphics_material_get_storage_size( material ) == storage &&
                     wp_graphics_material_get_revision( material ) == revision &&
                     !wp_graphics_material_is_dirty( material ) &&
                     wp_graphics_material_get_roughness( material ) == 0.5f &&
                     !strcmp( wp_graphics_material_get_texture_name( material, 0 ), "First" ) &&
                     !wp_graphics_material_get_texture_name( material, 32 )[0] &&
                     wp_graphics_material_get_texture_name( material, 39 ) == borrowed,
                     "every descriptor staging failure preserves values, names and allocations" );
    }
    for( i = 0; i < 2; ++i )
    {
        fail_after = i;
        wp_graphics_material_set_texture_name( material, 31, "Inserted" );
        fail_after = -1;
        ok &= check( !wp_graphics_material_get_texture_name( material, 31 )[0] &&
                     wp_graphics_material_get_revision( material ) == revision &&
                     !wp_graphics_material_is_dirty( material ) && live_allocations == allocations,
                     "single-name string/table allocation failures preserve state" );
    }
    fail_after = 0;
    wp_graphics_material_set_texture_name( material, 0, NULL );
    fail_after = -1;
    ok &= check( !strcmp( wp_graphics_material_get_texture_name( material, 0 ), "First" ) &&
                 wp_graphics_material_get_revision( material ) == revision &&
                 live_allocations == allocations, "failed removal compaction preserves old table" );
    calls = allocation_calls;
    desired.struct_size = sizeof( desired );
    wp_graphics_material_get_desc( material, &desired );
    ok &= check( wp_graphics_material_set_desc( material, &desired ), "no-op descriptor succeeds" );
    wp_graphics_material_set_texture_name( material, 39, borrowed );
    ok &= check( allocation_calls == calls, "no-op descriptor and name writes allocate nothing" );
    wp_graphics_material_set_texture_name( material, 32, "Boundary" );
    wp_graphics_material_set_texture_name( material, 31, "Before" );
    wp_graphics_material_set_texture_name( material, 0, NULL );
    wp_graphics_material_set_texture_name( material, 32, NULL );
    ok &= check( !strcmp( wp_graphics_material_get_texture_name( material, 31 ), "Before" ) &&
                 wp_graphics_material_get_texture_name( material, 39 ) == borrowed,
                 "multiword mask ranks survive insertion and first/middle removal" );
    wp_graphics_material_set_texture_name( material, 31, NULL );
    fail_after = 0;
    wp_graphics_material_set_texture_name( material, 39, NULL );
    fail_after = -1;
    ok &= check( live_allocations == 1 && !wp_graphics_material_get_texture_name( material, 39 )[0],
                 "clearing the final name needs no allocation" );
    wp_graphics_material_destroy( material );
    ok &= check( live_allocations == 0, "all material-owned allocations retire" );
    return ok ? 0 : 1;
}
