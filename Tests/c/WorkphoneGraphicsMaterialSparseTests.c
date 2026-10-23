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

typedef struct callback_state
{
    int syncs;
    int binds;
    int releases;
    int allow_sync;
    int clear_sync;
    int clear_bind;
    wp_graphics_material *clear_release_owner;
    void *last_native;
} callback_state;

static wp_s32 test_sync( const wp_graphics_material *material, void *native, void *user_data )
{
    callback_state *state;
    state = (callback_state *)user_data;
    ++state->syncs;
    state->last_native = native;
    if( state->clear_sync )
        wp_graphics_material_set_sync_func( (wp_graphics_material *)material, NULL, NULL );
    return state->allow_sync;
}

static wp_s32 test_bind( const wp_graphics_material *material, wp_renderer *renderer,
                         void *native, void *user_data )
{
    callback_state *state;
    (void)material;
    (void)renderer;
    state = (callback_state *)user_data;
    ++state->binds;
    state->last_native = native;
    if( state->clear_bind )
        wp_graphics_material_set_bind_func( (wp_graphics_material *)material, NULL, NULL );
    return 1;
}

static void test_release( void *native, void *user_data )
{
    callback_state *state;
    state = (callback_state *)user_data;
    ++state->releases;
    state->last_native = native;
    if( state->clear_release_owner )
        wp_graphics_material_set_release_func( state->clear_release_owner, NULL, NULL );
}

static int callback_storage_contracts( void )
{
    wp_graphics_material *material, *clone, *destination;
    callback_state state, other;
    wp_u32 revision, storage;
    int ok, calls, first_native, second_native;
    memset( &state, 0, sizeof( state ) );
    memset( &other, 0, sizeof( other ) );
    state.allow_sync = other.allow_sync = 1;
    first_native = 1;
    second_native = 2;
    material = wp_graphics_material_create();
    destination = wp_graphics_material_create();
    if( !material || !destination )
    {
        wp_graphics_material_destroy( material );
        wp_graphics_material_destroy( destination );
        return 0;
    }
    ok = 1;
    storage = wp_graphics_material_get_storage_size( material );
    revision = wp_graphics_material_get_revision( material );
    calls = allocation_calls;
    wp_graphics_material_set_sync_func( material, NULL, &state );
    wp_graphics_material_set_bind_func( material, NULL, &state );
    wp_graphics_material_set_release_func( material, NULL, &state );
    ok &= check( !material->callbacks && allocation_calls == calls &&
                 wp_graphics_material_get_revision( material ) == revision,
                 "empty callback clears use no allocation or revision" );
    fail_after = 0;
    wp_graphics_material_set_sync_func( material, test_sync, &state );
    wp_graphics_material_set_bind_func( material, test_bind, &state );
    wp_graphics_material_set_release_func( material, test_release, &state );
    fail_after = -1;
    ok &= check( !material->callbacks && wp_graphics_material_get_storage_size( material ) == storage &&
                 wp_graphics_material_get_revision( material ) == revision,
                 "failed first callback registrations preserve storage and revision" );
    wp_graphics_material_set_sync_func( material, test_sync, &state );
    calls = allocation_calls;
    revision = wp_graphics_material_get_revision( material );
    wp_graphics_material_set_sync_func( material, test_sync, &state );
    wp_graphics_material_set_bind_func( material, test_bind, &state );
    wp_graphics_material_set_release_func( material, test_release, &state );
    ok &= check( allocation_calls == calls &&
                 wp_graphics_material_get_storage_size( material ) == storage + sizeof( wp_material_callbacks ) &&
                 wp_graphics_material_get_revision( material ) == revision + 1u,
                 "callbacks share one block; unchanged registration and release registration preserve revision" );
    wp_graphics_material_set_native( material, &first_native );
    state.allow_sync = 0;
    ok &= check( !wp_graphics_material_update( material ) && state.syncs == 1 &&
                 wp_graphics_material_is_dirty( material ), "failed sync retains dirty state" );
    state.allow_sync = 1;
    ok &= check( wp_graphics_material_apply( material, (wp_renderer *)&state ) && state.syncs == 2 &&
                 state.binds == 1 && state.last_native == &first_native &&
                 !wp_graphics_material_is_dirty( material ), "registered sync and bind preserve native/user data" );
    wp_graphics_material_set_native( material, &second_native );
    ok &= check( state.releases == 1 && state.last_native == &first_native,
                 "native replacement releases the outgoing object once" );
    clone = wp_graphics_material_clone( material );
    ok &= check( clone && !clone->callbacks && !wp_graphics_material_get_native( clone ),
                 "clone retains value/program semantics without copying callbacks or native ownership" );
    wp_graphics_material_destroy( clone );
    wp_graphics_material_set_sync_func( destination, test_sync, &other );
    ok &= check( wp_graphics_material_copy( destination, material ) && destination->callbacks &&
                 destination->callbacks->sync_user_data == &other,
                 "value copy preserves destination callback ownership" );
    wp_graphics_material_reset( destination );
    ok &= check( wp_graphics_material_update( destination ) && other.syncs == 1,
                 "reset preserves registered callbacks" );
    wp_graphics_material_destroy( destination );
    calls = allocation_calls;
    fail_after = 0;
    wp_graphics_material_set_sync_func( material, NULL, NULL );
    wp_graphics_material_set_bind_func( material, NULL, NULL );
    fail_after = -1;
    ok &= check( material->callbacks && allocation_calls == calls,
                 "clearing some callbacks preserves release ownership without allocating" );
    state.clear_release_owner = material;
    wp_graphics_material_destroy( material );
    ok &= check( state.releases == 2 && state.last_native == &second_native && live_allocations == 0,
                 "destruction permits self-removal of final release callback and retires storage" );
    state.clear_release_owner = NULL;

    material = wp_graphics_material_create();
    if( !material ) return 0;
    wp_graphics_material_set_sync_func( material, test_sync, &state );
    calls = allocation_calls;
    fail_after = 0;
    wp_graphics_material_set_sync_func( material, NULL, NULL );
    fail_after = -1;
    ok &= check( !material->callbacks && allocation_calls == calls &&
                 wp_graphics_material_get_storage_size( material ) == storage,
                 "clearing final callback frees optional block even under allocation failure" );
    state.clear_sync = 1;
    wp_graphics_material_set_sync_func( material, test_sync, &state );
    ok &= check( wp_graphics_material_update( material ) && !material->callbacks &&
                 wp_graphics_material_is_dirty( material ) && wp_graphics_material_update( material ) &&
                 !wp_graphics_material_is_dirty( material ),
                 "sync may remove its last callback while preserving revision-aware dirty handling" );
    state.clear_bind = 1;
    wp_graphics_material_set_bind_func( material, test_bind, &state );
    ok &= check( wp_graphics_material_apply( material, (wp_renderer *)&state ) && !material->callbacks &&
                 wp_graphics_material_is_dirty( material ) && wp_graphics_material_update( material ),
                 "bind may remove its last callback without retaining the freed optional block" );
    wp_graphics_material_destroy( material );
    return ok;
}

#include "WorkphoneGraphicsMaterialBudgetContracts.h"

int main( void )
{
    wp_graphics_material *material;
    wp_graphics_material_desc desired;
    const wp_c8 *borrowed;
    wp_u32 revision, storage;
    int ok, i, calls, allocations;
    material = wp_graphics_material_create();
    if( !material ) return 1;
    printf( "native_material_object_bytes=%lu native_callback_block_bytes=%lu\n",
            (unsigned long)sizeof( wp_graphics_material ), (unsigned long)sizeof( wp_material_callbacks ) );
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
    ok &= callback_storage_contracts();
    ok &= material_budget_contracts();
    ok &= check( live_allocations == 0, "all material-owned allocations retire" );
    return ok ? 0 : 1;
}
