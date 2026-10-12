#include "workphone_graphics_system.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value) do { if (!(value)) { fprintf(stderr, "frame contract failed at line %d: %s\n", __LINE__, #value); return 1; } } while (0)

/* The isolated build observes traversal and draw calls at the scene boundary.
 * The normal engine build links the real scenes instead. */
#ifdef WP_FRAME_CONTRACT_STUB_SCENES
static int traversal_count;
static int native_draw_count;
wp_graphics_scene *wp_graphics_scene_create(void)
{
    wp_graphics_scene *scene;
    scene = (wp_graphics_scene *)calloc(1, sizeof(*scene));
    if (scene) scene->root_node = (wp_scenenode *)scene;
    return scene;
}
void wp_graphics_scene_destroy(wp_graphics_scene *scene) { free(scene); }
wp_s32 wp_scenenode_get_child_count(const wp_scenenode *node)
{ (void)node; ++traversal_count; return 0; }
wp_scenenode *wp_scenenode_get_child(const wp_scenenode *node, wp_s32 index)
{ (void)node; (void)index; return NULL; }
void wp_graphics_scene_render(wp_graphics_scene *scene, wp_renderer *renderer)
{ (void)scene; (void)renderer; ++native_draw_count; }
#endif

typedef struct trace_state
{
    char events[256];
    int count;
    int accept;
    int recursive;
    int mutation_result;
    wp_f32 delta;
} trace_state;

static void record(trace_state *state, char event, wp_f32 delta)
{
    if (state->count < 255) state->events[state->count++] = event;
    state->events[state->count] = '\0';
    state->delta = delta;
}
static wp_s32 begin_pass(wp_graphics_system *system, wp_f32 delta, void *data)
{
    trace_state *state;
    state = (trace_state *)data;
    record(state, 'B', delta);
    if (state->recursive)
    {
        wp_graphics_system_render_frame(system, 0.75f);
        wp_graphics_system_render(system);
        wp_graphics_system_update(system);
        state->mutation_result = wp_graphics_system_set_render_passes(system, NULL, 0);
        wp_graphics_system_destroy(system);
    }
    return state->accept;
}
static void draw_pass(wp_graphics_system *system, wp_f32 delta, void *data)
{ (void)system; record((trace_state *)data, 'D', delta); }
static void effects_pass(wp_graphics_system *system, wp_f32 delta, void *data)
{ (void)system; record((trace_state *)data, 'F', delta); }
static void ui_pass(wp_graphics_system *system, wp_f32 delta, void *data)
{ (void)system; record((trace_state *)data, 'U', delta); }
static void overlay_pass(wp_graphics_system *system, wp_f32 delta, void *data)
{ (void)system; record((trace_state *)data, 'O', delta); }
static void end_pass(wp_graphics_system *system, wp_f32 delta, void *data)
{ (void)system; record((trace_state *)data, 'E', delta); }
static void legacy_frame(wp_graphics_system *system, wp_f32 delta, void *data)
{
    record((trace_state *)data, 'L', delta);
    wp_graphics_system_render_frame(system, delta);
    wp_graphics_system_render(system);
}
static void reset_trace(trace_state *state)
{ state->count = 0; state->events[0] = '\0'; }

int main(void)
{
    wp_graphics_system *system;
    wp_graphics_system_render_pass passes[2];
    wp_graphics_system_render_pass maximum[WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES];
    trace_state trace;
    volatile wp_f32 zero;
    wp_f32 invalid;
    int i;
    memset(&trace, 0, sizeof(trace));
    memset(passes, 0, sizeof(passes));
    trace.accept = 1;
    system = wp_graphics_system_create();
    CHECK(system != NULL);
    CHECK(wp_graphics_system_create_scene(system) != NULL);
    passes[0].begin = begin_pass;
    passes[0].draw = draw_pass;
    passes[0].effects = effects_pass;
    passes[0].ui = ui_pass;
    passes[0].overlay = overlay_pass;
    passes[0].end = end_pass;
    passes[0].user_data = &trace;
    passes[1] = passes[0];
    CHECK(!wp_graphics_system_set_render_passes(NULL, passes, 1));
    CHECK(!wp_graphics_system_set_render_passes(system, NULL, 1));
    CHECK(!wp_graphics_system_set_render_passes(system, passes, -1));
    CHECK(!wp_graphics_system_set_render_passes(system, passes, WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES + 1));
    CHECK(wp_graphics_system_set_render_passes(system, passes, 2));
    passes[0].draw = NULL; /* Registration must snapshot descriptors. */
    wp_graphics_system_render_frame(system, 0.25f);
    CHECK(strcmp(trace.events, "BDFUOEBDFUOE") == 0);
    CHECK(trace.delta == 0.25f);
#ifdef WP_FRAME_CONTRACT_STUB_SCENES
    CHECK(traversal_count == 1);
#endif
    reset_trace(&trace);
    trace.accept = 0;
    wp_graphics_system_render(system);
    CHECK(strcmp(trace.events, "BB") == 0);
    CHECK(trace.delta == 0.25f);
    trace.accept = 1;
    trace.recursive = 1;
    CHECK(wp_graphics_system_set_render_passes(system, &passes[1], 1));
    reset_trace(&trace);
    wp_graphics_system_render_frame(system, 0.5f);
    CHECK(strcmp(trace.events, "BDFUOE") == 0);
    CHECK(trace.mutation_result == 0);
#ifdef WP_FRAME_CONTRACT_STUB_SCENES
    CHECK(traversal_count == 2);
#endif
    trace.recursive = 0;
    reset_trace(&trace);
    CHECK(!wp_graphics_system_set_render_passes(system, NULL, 1));
    wp_graphics_system_render_frame(system, -1.0f);
    CHECK(strcmp(trace.events, "BDFUOE") == 0 && trace.delta == 0.0f);
    wp_graphics_system_render_frame(system, FLT_MAX);
    CHECK(trace.delta == 1.0f);
    zero = 0.0f;
    invalid = zero / zero;
    wp_graphics_system_render_frame(system, invalid);
    CHECK(trace.delta == 0.0f);
    for (i = 0; i < WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES; ++i)
    {
        memset(&maximum[i], 0, sizeof(maximum[i]));
        maximum[i].draw = draw_pass;
        maximum[i].user_data = &trace;
    }
    CHECK(wp_graphics_system_set_render_passes(system, maximum, WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES));
    reset_trace(&trace);
    wp_graphics_system_render(system);
    CHECK(trace.count == WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES);
    CHECK(wp_graphics_system_set_render_passes(system, NULL, 0));
#ifdef WP_FRAME_CONTRACT_STUB_SCENES
    wp_graphics_system_set_renderer(system, (wp_renderer *)&trace);
    wp_graphics_system_render(system);
    CHECK(native_draw_count == 1);
#endif
    reset_trace(&trace);
    wp_graphics_system_set_frame_func(system, legacy_frame, &trace);
    wp_graphics_system_render_frame(system, 0.125f);
    CHECK(strcmp(trace.events, "L") == 0 && trace.delta == 0.125f);
#ifdef WP_FRAME_CONTRACT_STUB_SCENES
    CHECK(native_draw_count == 2);
#endif
    wp_graphics_system_set_renderer(system, NULL);
    wp_graphics_system_destroy(system);
    wp_graphics_system_destroy(NULL);
    wp_graphics_system_update(NULL);
    wp_graphics_system_render(NULL);
    wp_graphics_system_render_frame(NULL, 0.1f);
    puts("native frame contracts passed");
    return 0;
}
