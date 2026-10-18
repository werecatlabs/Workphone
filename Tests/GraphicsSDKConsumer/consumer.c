#include <WorkphoneGraphics/workphone_graphics_system.h>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <WorkphoneGraphics/workphone_graphics_mesh_reader.h>
#include <WorkphoneGraphics/workphone_graphics_transform_job.h>
#include <WorkphoneGraphics/workphone_graphics_cull_job.h>
#include <WorkphoneGraphics/workphone_graphics_rtt_job.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #x); return 1; } } while (0)

/* Optional deployment evidence, written by the running application rather than
 * inferred from its import table. Never overwrite an existing report. */
static int report_loaded_modules(void)
{
    const wchar_t *path;
    HANDLE file, snapshot;
    MODULEENTRY32W entry;
    char utf8[MAX_PATH * 4 + 1];
    DWORD written, error;
    int bytes, found, ok = 1;
    path = _wgetenv(L"WP_GRAPHICS_MODULE_REPORT");
    if(!path) return 1;
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE) return 0;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 0);
    if(snapshot == INVALID_HANDLE_VALUE) { CloseHandle(file); return 0; }
    memset(&entry, 0, sizeof(entry)); entry.dwSize = sizeof(entry);
    found = Module32FirstW(snapshot, &entry);
    while(found)
    {
        bytes = WideCharToMultiByte(CP_UTF8, 0, entry.szExePath, -1, utf8, sizeof(utf8), NULL, NULL);
        if(bytes <= 1 || !WriteFile(file, utf8, (DWORD)(bytes - 1), &written, NULL) ||
           written != (DWORD)(bytes - 1) || !WriteFile(file, "\n", 1, &written, NULL) || written != 1)
        { ok = 0; break; }
        found = Module32NextW(snapshot, &entry);
    }
    error = GetLastError();
    if(!found && error != ERROR_NO_MORE_FILES) ok = 0;
    CloseHandle(snapshot); CloseHandle(file);
    return ok;
}

int main(int argc, char **argv)
{
    wp_graphics_system *system;
    wp_graphics_scene *scene;
    wp_graphics_object *object;
    wp_scenenode *node;
    const wp_scenenode *nodes[1];
    wp_graphics_mesh *mesh;
    wp_renderer *renderer;
    wp_threadpool *pool;
    wp_transform_job_system *transforms;
    const wp_mat4f *matrices;
    wp_u32 count;
    wp_u64 generation;
    wp_vec3f position;
    wp_graphics_cull_result *main_view, *shadow_view;
    wp_graphics_system_frame_stats stats;
    wp_graphics_rtt_job *rtt;
    const wp_u8 *pixels;
    int i, coloured;

    if(argc != 2) { fprintf(stderr, "Usage: WorkphoneGraphicsSample <cooked-mesh.wpm>\n"); return 2; }
    mesh = wp_graphics_mesh_read(argv[1]);
    if(!mesh) { fprintf(stderr, "Cannot load cooked mesh: %s\n", argv[1]); return 1; }
    system = wp_graphics_system_create();
    renderer = wp_renderer_create_software(32, 32, WORKPHONE_PIXEL_FORMAT_RGBA8);
    pool = wp_threadpool_create(2);
    CHECK(system && renderer && pool);
    scene = wp_graphics_system_create_scene(system);
    CHECK(scene);
    node = wp_graphics_scene_create_node(scene);
    object = wp_graphics_scene_create_object(scene);
    CHECK(node && object && mesh);
    CHECK(wp_graphics_mesh_get_vertex_count(mesh) == 3);
    CHECK(wp_graphics_mesh_compute_obb(mesh));
    wp_graphics_object_set_mesh(object, mesh);
    wp_scenenode_attach_object(node, object);
    wp_graphics_scene_set_transform_pool(scene, pool);
    memset(&position, 0, sizeof(position)); position.x = 0.125f;
    wp_scenenode_set_position(node, position);
    transforms = wp_transform_job_system_create(pool, 1);
    CHECK(transforms);
    nodes[0] = node;
    CHECK(wp_transform_job_system_dispatch_snapshot(transforms, nodes, 1, 73));
    wp_transform_job_system_wait(transforms);
    matrices = wp_transform_job_system_get_result(transforms, &count, &generation);
    CHECK(matrices && count == 1 && generation == 73);
    CHECK(matrices[0].m[0][3] == position.x);
    wp_transform_job_system_destroy(transforms);

    main_view = wp_graphics_scene_prepare_cull(scene, NULL, WP_GRAPHICS_CULL_PASS_MAIN);
    CHECK(main_view && wp_graphics_cull_result_get_visible_count(main_view) == 1);
    shadow_view = wp_graphics_cull_result_create_view(main_view, NULL, WP_GRAPHICS_CULL_PASS_SHADOW);
    CHECK(shadow_view && wp_graphics_cull_result_get_transform_dispatch_count(shadow_view) == 0);
    wp_graphics_cull_result_destroy(main_view);
    CHECK(wp_graphics_cull_result_get_object(shadow_view, 0) == object);
    CHECK(wp_graphics_cull_result_validate(shadow_view, scene,
          wp_graphics_cull_result_get_scene_generation(shadow_view)) == WP_GRAPHICS_CULL_OK);
    wp_graphics_cull_result_destroy(shadow_view);

    wp_graphics_system_set_renderer(system, renderer);
    wp_renderer_set_clear_color(renderer, 0, 0, 0, 1);
    wp_renderer_clear(renderer, 3);
    wp_graphics_system_update(system);
    wp_graphics_system_render(system);
    CHECK(wp_graphics_system_get_frame_stats(system, &stats));
    CHECK(stats.update_serial == 1 && stats.render_serial == 1);
    pixels = (const wp_u8 *)wp_renderer_get_framebuffer(renderer);
    CHECK(pixels);
    coloured = 0;
    for(i = 0; i < 32 * 32; ++i) if(pixels[i * 4] || pixels[i * 4 + 1] || pixels[i * 4 + 2]) ++coloured;
    CHECK(coloured > 10);
    rtt = wp_graphics_rtt_job_create();
    CHECK(rtt && wp_graphics_rtt_job_status(rtt) == WP_RTT_IDLE);
    CHECK(wp_graphics_rtt_job_destroy(rtt) == WP_RTT_OK);
    wp_graphics_system_destroy(system);
    wp_graphics_mesh_destroy(mesh);
    wp_threadpool_destroy(pool);
    wp_renderer_destroy(renderer);
    CHECK(report_loaded_modules());
    printf("Installed SDK: cooked WPM1 mesh, transforms, shared cull publication, update/render, %d software pixels; RTT linkage OK\n", coloured);
    return 0;
}
