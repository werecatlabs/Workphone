#ifndef WORKPHONE_GRAPHICS_PARTICLE_COMPUTE_DX11_H
#define WORKPHONE_GRAPHICS_PARTICLE_COMPUTE_DX11_H
#include "workphone_graphics_particle_simulation.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wp_particle_compute_dx11 wp_particle_compute_dx11;
/* Retains device/context; callers serialize the immediate context with rendering. */
wp_particle_compute_dx11 *wp_particle_compute_dx11_create( void *device, void *context, wp_u32 capacity );
void wp_particle_compute_dx11_destroy( wp_particle_compute_dx11 *compute );
/* Compatible with wp_particle_integrator; successful updates execute a real compute dispatch.
 * A staging readback preserves the existing billboard renderer and CPU sample API. */
wp_s32 wp_particle_compute_dx11_integrate( void *compute, wp_particle_sample *samples,
    wp_u32 count, const wp_particle_simulation_settings *settings, wp_f32 seconds );
wp_u32 wp_particle_compute_dx11_get_dispatches( const wp_particle_compute_dx11 *compute );
#ifdef __cplusplus
}
#endif
#endif
