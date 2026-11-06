#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "workphone_graphics_mesh.h"
#include "workphone_graphics_renderer_dx11.h"

enum { TARGET_SIZE = 32, TARGET_BYTES = TARGET_SIZE * TARGET_SIZE * 4 };

typedef struct section_fixture
{
    wp_renderer_dx11 *renderer;
    wp_mesh_section_dx11 section;
    wp_s32 submitted;
    wp_s32 restored;
    wp_s32 feedback;
} section_fixture;

static int check( int condition, const char *message )
{
    if( !condition ) fprintf( stderr, "%s\n", message );
    return condition;
}

static wp_s32 sentinel_state( const wp_renderer_dx11 *renderer )
{
    return wp_renderer_dx11_get_blend_mode( renderer ) == WORKPHONE_BLEND_MODE_ADDITIVE &&
           wp_renderer_dx11_get_cull_mode( renderer ) == WORKPHONE_CULL_MODE_FRONT &&
           wp_renderer_dx11_get_fill_mode( renderer ) == WORKPHONE_FILL_MODE_WIREFRAME &&
           wp_renderer_dx11_get_depth_func( renderer ) == WORKPHONE_DEPTH_FUNC_NEVER &&
           wp_renderer_dx11_get_depth_test_enabled( renderer ) &&
           wp_renderer_dx11_get_depth_write_enabled( renderer );
}

static wp_rtt_result draw_section( void *user, const wp_graphics_rtt_desc *desc )
{
    section_fixture *fixture = (section_fixture *)user;
    wp_mesh_section_state_dx11 state;
    (void)desc;

    if( !wp_renderer_dx11_begin_mesh_sections( fixture->renderer, &state ) )
        return WP_RTT_BACKEND_FAILED;
    fixture->submitted = wp_renderer_dx11_submit_mesh_section( fixture->renderer, &fixture->section );
    wp_renderer_dx11_end_mesh_sections( fixture->renderer, &state );
    fixture->restored = sentinel_state( fixture->renderer );

    /* Let the RTT transaction report its sticky feedback failure. */
    return fixture->submitted || fixture->feedback ? WP_RTT_OK : WP_RTT_BACKEND_FAILED;
}

typedef struct mesh_batch_fixture
{
    wp_renderer_dx11 *renderer;
    wp_mesh_batch_dx11 command;
    wp_mesh_batch_result_dx11 counts;
    wp_mesh_section_dx11 material;
    wp_s32 resolved, retired, abort_at, restored;
} mesh_batch_fixture;

static wp_s32 resolve_batch_section( wp_s32 index, wp_mesh_section_dx11 *section, void *user )
{
    mesh_batch_fixture *fixture = (mesh_batch_fixture *)user;
    if( index != fixture->resolved || index != fixture->retired ) return -1;
    ++fixture->resolved;
    *section = fixture->material;
    return index == fixture->abort_at ? -1 : 1;
}

static void retire_batch_section( void *user )
{
    mesh_batch_fixture *fixture = (mesh_batch_fixture *)user;
    ++fixture->retired;
}

static wp_rtt_result draw_mesh_batch( void *user, const wp_graphics_rtt_desc *desc )
{
    mesh_batch_fixture *fixture = (mesh_batch_fixture *)user;
    wp_s32 submitted;
    (void)desc;
    submitted = wp_renderer_dx11_submit_mesh_batch( fixture->renderer, &fixture->command, &fixture->counts );
    fixture->restored = sentinel_state( fixture->renderer );
    return submitted && fixture->restored ? WP_RTT_OK : WP_RTT_BACKEND_FAILED;
}

int main( void )
{
    static const wp_vertex_pntc vertices[] = {
        { { -0.75f, -0.75f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, 0xffffffffu },
        { { 0.75f, -0.75f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, 0xffffffffu },
        { { 0.75f, 0.75f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, 0xffffffffu },
        { { -0.75f, 0.75f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, 0xffffffffu }
    };
    static const wp_u16 indices[] = { 0, 1, 2, 0, 2, 3 };
    static const unsigned char texel[] = { 0, 255, 0, 255 };
    HWND window = NULL;
    wp_renderer_dx11 *renderer = NULL;
    wp_render_texture_dx11 *target = NULL;
    wp_geometry_dx11 *geometry = NULL;
    wp_graphics_mesh *mesh = NULL;
    void *texture = NULL;
    wp_mat4f identity;
    wp_graphics_rtt_desc desc;
    section_fixture fixture;
    mesh_batch_fixture batch;
    wp_instance_pntc_dx11 instances[3];
    wp_render_statistics_dx11 before, after;
    unsigned char pixels[TARGET_BYTES], rollback[TARGET_BYTES];
    const unsigned char *center = pixels + ( ( TARGET_SIZE / 2 ) * TARGET_SIZE + TARGET_SIZE / 2 ) * 4;
    int i, ok = 0;

    window = CreateWindowExW( 0, L"STATIC", L"Workphone C90 DX11 section", WS_OVERLAPPEDWINDOW,
                              0, 0, TARGET_SIZE, TARGET_SIZE, NULL, NULL, GetModuleHandleW( NULL ), NULL );
    if( !check( window != NULL, "could not create a native test window" ) ) goto done;
    renderer = wp_renderer_dx11_create( window, TARGET_SIZE, TARGET_SIZE );
    if( !check( renderer != NULL, "could not create the native DX11 renderer" ) ) goto done;
    target = wp_renderer_dx11_create_render_texture( renderer, TARGET_SIZE, TARGET_SIZE );
    geometry = wp_renderer_dx11_create_indexed_geometry_pntc( renderer, vertices, 4, indices, 6, 0 );
    texture = wp_renderer_dx11_create_texture_native( renderer, texel, 1, 1, WORKPHONE_PIXEL_FORMAT_RGBA8 );
    if( !check( target && geometry && texture, "could not allocate native section resources" ) ) goto done;

    memset( &identity, 0, sizeof( identity ) );
    for( i = 0; i < 4; ++i ) identity.m[i][i] = 1.0f;
    wp_renderer_dx11_set_world_matrix( renderer, &identity );
    wp_renderer_dx11_set_view_matrix( renderer, &identity );
    wp_renderer_dx11_set_projection_matrix( renderer, &identity );
    wp_renderer_dx11_set_blend_mode( renderer, WORKPHONE_BLEND_MODE_ADDITIVE );
    wp_renderer_dx11_set_cull_mode( renderer, WORKPHONE_CULL_MODE_FRONT );
    wp_renderer_dx11_set_fill_mode( renderer, WORKPHONE_FILL_MODE_WIREFRAME );
    wp_renderer_dx11_set_depth_func( renderer, WORKPHONE_DEPTH_FUNC_NEVER );
    wp_renderer_dx11_set_depth_test_enabled( renderer, 1 );
    wp_renderer_dx11_set_depth_write_enabled( renderer, 1 );

    memset( &desc, 0, sizeof( desc ) );
    desc.format = WP_RTT_RGBA8_UNORM;
    desc.colour_space = WP_RTT_LEGACY_LDR;
    desc.renderer_generation = wp_renderer_dx11_get_rtt_generation( renderer );
    desc.target_generation = wp_renderer_dx11_get_render_texture_generation( target );
    desc.width = desc.viewport.width = TARGET_SIZE;
    desc.height = desc.viewport.height = TARGET_SIZE;
    desc.clear_colour = desc.clear_depth = 1;
    desc.colour[3] = desc.depth = 1.0f;

    memset( &fixture, 0, sizeof( fixture ) );
    fixture.renderer = renderer;
    fixture.section.struct_size = (wp_u32)sizeof( fixture.section );
    fixture.section.geometry = geometry;
    fixture.section.index_count = 6;
    fixture.section.base_texture = texture;
    fixture.section.anisotropy = 1;
    fixture.section.material.base_color.x = fixture.section.material.base_color.y =
        fixture.section.material.base_color.z = fixture.section.material.base_color.w = 1.0f;
    fixture.section.material.surface.z = fixture.section.material.surface.w = 1.0f;
    fixture.section.material.controls.z = -1.0f;
    fixture.section.blend_mode = WORKPHONE_BLEND_MODE_NONE;
    fixture.section.cull_mode = WORKPHONE_CULL_MODE_NONE;
    fixture.section.fill_mode = WORKPHONE_FILL_MODE_SOLID;
    fixture.section.depth_func = WORKPHONE_DEPTH_FUNC_ALWAYS;

    wp_renderer_dx11_get_statistics_counters( renderer, &before );
    fixture.section.index_count = 7;
    if( !check( wp_renderer_dx11_submit_mesh_section( renderer, &fixture.section ) == 0 &&
                sentinel_state( renderer ), "invalid index range modified renderer state" ) ) goto done;
    fixture.section.index_count = 6;
    wp_renderer_dx11_get_statistics_counters( renderer, &after );
    if( !check( after.draws == before.draws && after.triangles == before.triangles,
                "invalid section submitted geometry" ) ) goto done;

    if( !check( wp_renderer_dx11_execute_rtt_job( renderer, target, &desc, NULL, NULL,
                                                 draw_section, &fixture ) == WP_RTT_OK &&
                fixture.submitted == 1 && fixture.restored && sentinel_state( renderer ),
                "native section failed to draw or restore its scoped state" ) ) goto done;
    wp_renderer_dx11_get_statistics_counters( renderer, &after );
    if( !check( after.draws == before.draws + 1 && after.triangles == before.triangles + 2,
                "native section did not submit exactly one indexed draw" ) ) goto done;
    if( !check( wp_renderer_dx11_copy_render_texture_rgba8( target, pixels, TARGET_BYTES ) &&
                center[0] < 5 && center[1] > 240 && center[2] < 5 && center[3] > 240 &&
                pixels[0] < 5 && pixels[1] < 5 && pixels[2] < 5,
                "native section did not produce the expected textured RTT pixels" ) ) goto done;

    fixture.section.base_texture = wp_renderer_dx11_get_render_texture_view( target );
    fixture.feedback = 1;
    desc.colour[0] = 1.0f;
    if( !check( wp_renderer_dx11_execute_rtt_job( renderer, target, &desc, NULL, NULL,
                                                 draw_section, &fixture ) == WP_RTT_HAZARD &&
                fixture.submitted == 0 && fixture.restored && sentinel_state( renderer ),
                "native section accepted a feedback texture or left stale state" ) ) goto done;
    wp_renderer_dx11_get_statistics_counters( renderer, &after );
    if( !check( after.draws == before.draws + 1 && after.triangles == before.triangles + 2 &&
                wp_renderer_dx11_copy_render_texture_rgba8( target, rollback, TARGET_BYTES ) &&
                memcmp( pixels, rollback, sizeof( pixels ) ) == 0,
                "feedback submitted a draw or replaced the last good RTT image" ) ) goto done;

    before = after;
    for( i = 0; i < 3; ++i )
    {
        instances[i].transform = identity;
        instances[i].tint.x = instances[i].tint.y = instances[i].tint.z = instances[i].tint.w = 1.0f;
    }
    fixture.feedback = 0;
    fixture.section.base_texture = texture;
    fixture.section.instances = instances;
    fixture.section.instance_count = 3;
    fixture.section.instance_batch_size = 2;
    instances[2].transform.m[0][0] = 0.0f;
    if( !check( wp_renderer_dx11_submit_mesh_section( renderer, &fixture.section ) == 0 &&
                sentinel_state( renderer ), "invalid instance in a later batch changed renderer state" ) ) goto done;
    wp_renderer_dx11_get_statistics_counters( renderer, &after );
    if( !check( after.draws == before.draws && after.instances == before.instances,
                "invalid instance in a later batch submitted earlier instances" ) ) goto done;
    instances[2].transform = identity;
    if( !check( wp_renderer_dx11_execute_rtt_job( renderer, target, &desc, NULL, NULL,
                                                 draw_section, &fixture ) == WP_RTT_OK &&
                fixture.submitted == 3 && fixture.restored && sentinel_state( renderer ),
                "batched native section failed after the feedback rejection" ) ) goto done;
    wp_renderer_dx11_get_statistics_counters( renderer, &after );
    if( !check( after.draws == before.draws + 2 && after.instanced_draws == before.instanced_draws + 2 &&
                after.instances == before.instances + 3 && after.triangles == before.triangles + 6,
                "batched native section submitted the wrong draw or instance count" ) ) goto done;

    mesh = wp_graphics_mesh_create();
    if( !check( mesh && wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_PNTC, vertices, 4 ) &&
                wp_graphics_mesh_set_indices_u16( mesh, indices, 6 ) &&
                wp_graphics_mesh_add_submesh( mesh, 0, 3, 0 ) == 0 &&
                wp_graphics_mesh_add_submesh( mesh, 3, 3, 1 ) == 1,
                "native mesh batch fixtures" ) ) goto done;
    memset( &batch, 0, sizeof( batch ) );
    batch.renderer = renderer;
    batch.material = fixture.section;
    batch.abort_at = -1;
    batch.command.mesh = mesh;
    batch.command.world = batch.command.view = batch.command.projection = &identity;
    batch.command.resolve = resolve_batch_section;
    batch.command.retire = retire_batch_section;
    batch.command.user = &batch;
    desc.colour[0] = 0.0f;
    if( !check( wp_renderer_dx11_execute_rtt_job( renderer, target, &desc, NULL, NULL,
                                                 draw_mesh_batch, &batch ) == WP_RTT_OK &&
                batch.resolved == 2 && batch.retired == 2 && batch.counts.submissions == 2 &&
                batch.counts.triangles == 2 && batch.restored && sentinel_state( renderer ) &&
                wp_renderer_dx11_copy_render_texture_rgba8( target, rollback, TARGET_BYTES ) &&
                memcmp( pixels, rollback, sizeof( pixels ) ) == 0,
                "native mesh batch lost section order, state, counts or pixel parity" ) ) goto done;

    batch.abort_at = 1;
    batch.resolved = batch.retired = batch.restored = 0;
    desc.colour[0] = 1.0f;
    if( !check( wp_renderer_dx11_execute_rtt_job( renderer, target, &desc, NULL, NULL,
                                                 draw_mesh_batch, &batch ) == WP_RTT_BACKEND_FAILED &&
                batch.resolved == 2 && batch.retired == 2 && batch.counts.submissions == 1 &&
                batch.counts.triangles == 1 && batch.restored && sentinel_state( renderer ) &&
                wp_renderer_dx11_copy_render_texture_rgba8( target, rollback, TARGET_BYTES ) &&
                memcmp( pixels, rollback, sizeof( pixels ) ) == 0,
                "late section rejection must retire and roll back the full RTT batch" ) ) goto done;
    ok = 1;

done:
    wp_renderer_dx11_destroy_texture_native( texture );
    wp_renderer_dx11_destroy_geometry( geometry );
    wp_renderer_dx11_destroy_render_texture( target );
    wp_renderer_dx11_destroy( renderer );
    wp_graphics_mesh_destroy( mesh );
    if( window ) DestroyWindow( window );
    return ok ? 0 : 1;
}
