#define WORKPHONE_MATERIAL_SCENE_NATIVE_IMPLEMENTATION
#include "../../Samples/cpp/VehicleAdvanced/MaterialSceneCpuNativeBridge.h"

static void native_burn(unsigned count)
{
    volatile unsigned long value=1;
    while(count--) value=value*1664525+1013904223;
}

/* Mirrors the overlay's event boundaries, including early upload failure.
 * The large final interval stands for IA/MVP/draw CPU, outside material timers. */
int wp_material_scene_test_native_draw(int split, int fail_upload)
{
    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_BEGIN,split);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN,1);
    native_burn(20000);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_END,1);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_BEGIN,0);
    native_burn(200000);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_END,!fail_upload);
    if(fail_upload)
    {
        wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_END,0);
        return 0;
    }
    wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_BEGIN,1);
    native_burn(200000);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_END,1);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN,1);
    native_burn(20000);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_END,1);
    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_END,1);
    native_burn(4000000);
    return 1;
}
