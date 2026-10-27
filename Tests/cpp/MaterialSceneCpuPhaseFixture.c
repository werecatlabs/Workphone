#define WORKPHONE_MATERIAL_SCENE_NATIVE_IMPLEMENTATION
#include "../../Samples/cpp/VehicleAdvanced/MaterialSceneCpuNativeBridge.h"
static void burn_phase(unsigned count) { volatile unsigned long value=1;while(count--) value=value*1664525+1013904223; }
int wp_scene_test_phase_draw(int cold,int fail)
{
    int phase;
    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_BEGIN,0);
    for(phase=1;phase<=6;++phase) {
        if(!cold && (phase==4 || phase==5)) continue;
        wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN,phase);
        burn_phase(phase==6?200000:1000);
        wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_END,1);
        if(phase==3) {
            wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_BEGIN,0);burn_phase(10000);
            wp_material_scene_native_emit(WP_SCENE_NATIVE_UPLOAD_END,!fail);
            if(fail) { wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_END,0);return 0; }
        }
    }
    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_END,1);
    burn_phase(2000000); /* Excluded IA/MVP/native execution. */
    return 1;
}
void wp_scene_test_phase_events(int cold)
{
    int phase;
    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_BEGIN,0);
    for(phase=1;phase<=6;++phase) {
        if(!cold && (phase==4 || phase==5)) continue;
        wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_BEGIN,phase);
        wp_material_scene_native_emit(WP_SCENE_NATIVE_BIND_END,1);
    }
    wp_material_scene_native_emit(WP_SCENE_NATIVE_DRAW_END,1);
}
