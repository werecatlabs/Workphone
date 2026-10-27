#ifndef WORKPHONE_MATERIAL_SCENE_NATIVE_BRIDGE_H
#define WORKPHONE_MATERIAL_SCENE_NATIVE_BRIDGE_H
/* Test-overlay bridge. Plain C: no C++ types, allocation, or renderer ABI change. */
#ifdef __cplusplus
extern "C" {
#endif
enum wp_material_scene_native_event
{
    WP_SCENE_NATIVE_DRAW_BEGIN, WP_SCENE_NATIVE_DRAW_END,
    WP_SCENE_NATIVE_UPLOAD_BEGIN, WP_SCENE_NATIVE_UPLOAD_END,
    WP_SCENE_NATIVE_BIND_BEGIN, WP_SCENE_NATIVE_BIND_END
};
typedef void (*wp_material_scene_native_callback)(int event, int value);
void wp_material_scene_native_set_callback(wp_material_scene_native_callback callback);
#ifdef WORKPHONE_MATERIAL_SCENE_NATIVE_IMPLEMENTATION
static wp_material_scene_native_callback wp_scene_native_callback = 0;
void wp_material_scene_native_set_callback(wp_material_scene_native_callback callback)
{ wp_scene_native_callback = callback; }
static void wp_material_scene_native_emit(int event, int value)
{ if(wp_scene_native_callback) wp_scene_native_callback(event, value); }
#endif
#ifdef __cplusplus
}
#endif
#endif
