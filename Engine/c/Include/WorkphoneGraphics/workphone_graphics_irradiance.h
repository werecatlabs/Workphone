#ifndef WORKPHONE_GRAPHICS_IRRADIANCE_H
#define WORKPHONE_GRAPHICS_IRRADIANCE_H
#include "workphone_types.h"
#ifdef __cplusplus
extern "C" {
#endif
/* GPU-compatible 160-byte SH9 probe. Coefficients are cosine-convolved irradiance. */
typedef struct wp_irradiance_probe
{
    wp_f32 position[3], padding;
    wp_f32 sh[9][4];
} wp_irradiance_probe;
typedef struct wp_irradiance_hit
{
    wp_f32 position[3], normal[3], albedo[3], emission[3], distance;
} wp_irradiance_hit;
typedef struct wp_irradiance_light
{
    wp_f32 position[3], direction[3], radiance[3], range;
    wp_s32 directional, dynamic;
} wp_irradiance_light;
typedef wp_s32 (*wp_irradiance_trace)( void *context, const wp_f32 origin[3],
    const wp_f32 direction[3], wp_f32 maximum_distance, wp_irradiance_hit *hit );
typedef void (*wp_irradiance_environment)( void *context, const wp_f32 direction[3], wp_f32 radiance[3] );
typedef struct wp_irradiance_scene
{
    void *context;
    wp_irradiance_trace trace;
    wp_irradiance_environment environment;
    const wp_irradiance_light *lights;
    wp_u32 light_count;
} wp_irradiance_scene;
/* light_selection: 0 all, 1 static, 2 dynamic. Hybrid combines static bake + dynamic refresh.
 * Every ray evaluates actual visible surface albedo, emission and shadowed light bounce.
 * Identical inputs produce identical coefficients; no global RNG state is consumed. */
wp_s32 wp_irradiance_bake_probe( const wp_irradiance_scene *scene, const wp_f32 position[3],
    wp_u32 rays, wp_f32 maximum_distance, wp_s32 light_selection, wp_s32 include_environment,
    wp_irradiance_probe *probe );
void wp_irradiance_evaluate( const wp_irradiance_probe *probe, const wp_f32 normal[3], wp_f32 irradiance[3] );
void wp_irradiance_combine( const wp_irradiance_probe *static_probe,
    const wp_irradiance_probe *dynamic_probe, wp_irradiance_probe *output );
#ifdef __cplusplus
}
#endif
#endif
