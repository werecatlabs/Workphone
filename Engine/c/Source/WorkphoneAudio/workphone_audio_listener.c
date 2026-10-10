#include "workphone_audio_listener.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static wp_vec3f vector(wp_f32 x, wp_f32 y, wp_f32 z) { wp_vec3f v = {x, y, z}; return v; }
static wp_vec3f zero(void) { return vector(0, 0, 0); }
static int valid(wp_vec3f v) { return isfinite(v.x) && isfinite(v.y) && isfinite(v.z); }
static int normalize(wp_vec3f *v) {
    double length = sqrt((double)v->x*v->x + (double)v->y*v->y + (double)v->z*v->z);
    if (!valid(*v) || length < 1e-8) return 0;
    v->x = (wp_f32)(v->x / length); v->y = (wp_f32)(v->y / length); v->z = (wp_f32)(v->z / length);
    return 1;
}
wp_audio_listener *wp_audio_listener_create(void) {
    wp_audio_listener *l = (wp_audio_listener *)calloc(1, sizeof(*l));
    if (l) { l->forward = vector(0, 0, -1); l->up = vector(0, 1, 0); }
    return l;
}
void wp_audio_listener_destroy(wp_audio_listener *l) { free(l); }
const wp_c8 *wp_audio_listener_get_name(const wp_audio_listener *l) { return l ? l->name : NULL; }
void wp_audio_listener_set_name(wp_audio_listener *l, const wp_c8 *name) {
    if (l && name) {
        size_t n = 0;
        while (n < sizeof(l->name)-1 && name[n]) { l->name[n] = name[n]; ++n; }
        l->name[n] = 0;
    }
}
wp_vec3f wp_audio_listener_get_position(const wp_audio_listener *l) { return l ? l->position : zero(); }
void wp_audio_listener_set_position(wp_audio_listener *l, wp_vec3f v) { if (l && valid(v)) l->position = v; }
wp_vec3f wp_audio_listener_get_velocity(const wp_audio_listener *l) { return l ? l->velocity : zero(); }
void wp_audio_listener_set_velocity(wp_audio_listener *l, wp_vec3f v) { if (l && valid(v)) l->velocity = v; }
wp_vec3f wp_audio_listener_get_forward(const wp_audio_listener *l) { return l ? l->forward : zero(); }
wp_vec3f wp_audio_listener_get_up(const wp_audio_listener *l) { return l ? l->up : zero(); }
void wp_audio_listener_set_orientation(wp_audio_listener *l, wp_vec3f forward, wp_vec3f up) {
    wp_f32 dot;
    if (!l || !normalize(&forward) || !valid(up)) return;
    dot = forward.x*up.x + forward.y*up.y + forward.z*up.z;
    up.x -= dot*forward.x; up.y -= dot*forward.y; up.z -= dot*forward.z;
    if (!normalize(&up)) return; /* Reject degenerate basis; retain last good orientation. */
    l->forward = forward; l->up = up;
}
void wp_audio_listener_set_forward(wp_audio_listener *l, wp_vec3f v) {
    if (l) wp_audio_listener_set_orientation(l, v, l->up);
}
void wp_audio_listener_set_up(wp_audio_listener *l, wp_vec3f v) {
    if (l) wp_audio_listener_set_orientation(l, l->forward, v);
}
void wp_audio_listener_get_orientation(const wp_audio_listener *l, wp_vec3f *f, wp_vec3f *u) {
    if (f) *f = wp_audio_listener_get_forward(l);
    if (u) *u = wp_audio_listener_get_up(l);
}
void wp_audio_listener_get_native(const wp_audio_listener *l, void **p) { if (p) *p = l ? l->native : NULL; }
void wp_audio_listener_set_native(wp_audio_listener *l, void *p) { if (l) l->native = p; }
void *wp_audio_listener_get_user_data(const wp_audio_listener *l) { return l ? l->user_data : NULL; }
void wp_audio_listener_set_user_data(wp_audio_listener *l, void *p) { if (l) l->user_data = p; }
