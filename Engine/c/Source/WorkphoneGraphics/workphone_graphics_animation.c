/**
 * @file workphone_graphics_animation.c
 * @brief Implementation of the C89 animation API.
 */

#include "workphone_graphics_animation.h"
#include "workphone_graphics_node.h"

#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_vec3f s_vec3f_lerp( wp_vec3f a, wp_vec3f b, wp_f32 t )
{
    wp_vec3f r;
    r.x = a.x + ( b.x - a.x ) * t;
    r.y = a.y + ( b.y - a.y ) * t;
    r.z = a.z + ( b.z - a.z ) * t;
    return r;
}

static wp_quatf s_quatf_identity( void )
{
    wp_quatf q;
    q.w = 1.0f;
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = 0.0f;
    return q;
}

static wp_f32 s_clampf( wp_f32 v, wp_f32 lo, wp_f32 hi )
{
    if( v < lo )
        return lo;
    if( v > hi )
        return hi;
    return v;
}

static wp_transform_keyframe s_default_transform_keyframe( wp_f32 time )
{
    wp_transform_keyframe kf;
    memset( &kf, 0, sizeof( kf ) );
    kf.time = time;
    kf.rotation = s_quatf_identity();
    kf.scale.x = 1.0f;
    kf.scale.y = 1.0f;
    kf.scale.z = 1.0f;
    return kf;
}

/* =========================================================================
 * Animation lifecycle
 * ====================================================================== */

wp_animation *wp_animation_create( const char *name, wp_f32 length )
{
    wp_animation *anim = (wp_animation *)malloc( sizeof( wp_animation ) );
    if( !anim )
    {
        return NULL;
    }
    memset( anim, 0, sizeof( wp_animation ) );
    if( name )
    {
        strncpy( anim->name, name, WP_ANIMATION_NAME_MAX - 1 );
        anim->name[WP_ANIMATION_NAME_MAX - 1] = '\0';
    }
    anim->length = length;
    anim->interp_mode = WP_ANIM_INTERP_LINEAR;
    anim->rot_interp_mode = WP_ANIM_ROT_INTERP_SPHERICAL;
    return anim;
}

void wp_animation_destroy( wp_animation *anim )
{
    if( !anim )
    {
        return;
    }
    free( anim );
}

/* =========================================================================
 * Animation properties
 * ====================================================================== */

wp_f32 wp_animation_get_length( const wp_animation *anim )
{
    if( !anim )
    {
        return 0.0f;
    }
    return anim->length;
}

void wp_animation_set_length( wp_animation *anim, wp_f32 length )
{
    if( !anim )
    {
        return;
    }
    anim->length = length;
}

const char *wp_animation_get_name( const wp_animation *anim )
{
    if( !anim )
    {
        return NULL;
    }
    return anim->name;
}

void wp_animation_set_interp_mode( wp_animation *anim, wp_anim_interp_mode mode )
{
    if( !anim )
    {
        return;
    }
    anim->interp_mode = mode;
}

void wp_animation_set_rot_interp_mode( wp_animation *anim, wp_anim_rot_interp_mode mode )
{
    if( !anim )
    {
        return;
    }
    anim->rot_interp_mode = mode;
}

/* =========================================================================
 * Node track management
 * ====================================================================== */

wp_node_animation_track *wp_animation_create_node_track( wp_animation *anim, unsigned short handle,
                                                         wp_node *target_node )
{
    wp_node_animation_track *track;

    if( !anim || anim->node_track_count >= WP_ANIMATION_MAX_NODE_TRACKS )
    {
        return NULL;
    }

    track = &anim->node_tracks[anim->node_track_count];
    memset( track, 0, sizeof( wp_node_animation_track ) );
    track->handle = handle;
    track->target_node = target_node;
    track->use_shortest_rotation_path = 1;
    anim->node_track_count++;
    return track;
}

wp_node_animation_track *wp_animation_get_node_track( const wp_animation *anim, unsigned short handle )
{
    unsigned int i;

    if( !anim )
    {
        return NULL;
    }

    for( i = 0; i < anim->node_track_count; ++i )
    {
        if( anim->node_tracks[i].handle == handle )
        {
            return (wp_node_animation_track *)&anim->node_tracks[i];
        }
    }
    return NULL;
}

void wp_animation_destroy_node_track( wp_animation *anim, unsigned short handle )
{
    unsigned int i;
    unsigned int remaining;

    if( !anim )
    {
        return;
    }

    for( i = 0; i < anim->node_track_count; ++i )
    {
        if( anim->node_tracks[i].handle == handle )
        {
            remaining = anim->node_track_count - i - 1;
            if( remaining > 0 )
            {
                memmove( &anim->node_tracks[i], &anim->node_tracks[i + 1],
                         remaining * sizeof( wp_node_animation_track ) );
            }
            anim->node_track_count--;
            return;
        }
    }
}

/* =========================================================================
 * Node track keyframe management
 * ====================================================================== */

wp_transform_keyframe *wp_node_track_create_keyframe( wp_node_animation_track *track, wp_f32 time )
{
    unsigned int i;
    unsigned int insert_pos;
    wp_transform_keyframe kf;

    if( !track || track->keyframe_count >= WP_ANIMATION_MAX_KEYFRAMES )
    {
        return NULL;
    }

    insert_pos = track->keyframe_count;
    for( i = 0; i < track->keyframe_count; ++i )
    {
        if( track->keyframes[i].time > time )
        {
            insert_pos = i;
            break;
        }
    }

    if( insert_pos < track->keyframe_count )
    {
        memmove( &track->keyframes[insert_pos + 1], &track->keyframes[insert_pos],
                 ( track->keyframe_count - insert_pos ) * sizeof( wp_transform_keyframe ) );
    }

    kf = s_default_transform_keyframe( time );
    track->keyframes[insert_pos] = kf;
    track->keyframe_count++;
    return &track->keyframes[insert_pos];
}

wp_transform_keyframe *wp_node_track_get_keyframe( const wp_node_animation_track *track,
                                                   unsigned int index )
{
    if( !track || index >= track->keyframe_count )
    {
        return NULL;
    }
    return (wp_transform_keyframe *)&track->keyframes[index];
}

void wp_node_track_get_keyframes_at_time( const wp_node_animation_track *track, wp_f32 time,
                                          unsigned int *out_k0, unsigned int *out_k1, wp_f32 *out_t )
{
    unsigned int i;
    wp_f32 t0, t1, span;

    if( !track || !out_k0 || !out_k1 || !out_t )
    {
        return;
    }

    *out_k0 = 0;
    *out_k1 = 0;
    *out_t = 0.0f;

    if( track->keyframe_count < 2 )
    {
        return;
    }

    if( time <= track->keyframes[0].time )
    {
        return;
    }

    if( time >= track->keyframes[track->keyframe_count - 1].time )
    {
        *out_k0 = track->keyframe_count - 1;
        *out_k1 = track->keyframe_count - 1;
        return;
    }

    for( i = 0; i < track->keyframe_count - 1; ++i )
    {
        if( track->keyframes[i + 1].time >= time )
        {
            *out_k0 = i;
            *out_k1 = i + 1;
            t0 = track->keyframes[i].time;
            t1 = track->keyframes[i + 1].time;
            span = t1 - t0;
            *out_t = ( span > 1e-7f ) ? ( ( time - t0 ) / span ) : 0.0f;
            return;
        }
    }
}

wp_transform_keyframe wp_node_track_interpolate( const wp_node_animation_track *track, wp_f32 time,
                                                 wp_anim_interp_mode interp_mode,
                                                 wp_anim_rot_interp_mode rot_interp_mode )
{
    unsigned int k0, k1;
    wp_f32 t, dot;
    wp_transform_keyframe result;
    const wp_transform_keyframe *kf0;
    const wp_transform_keyframe *kf1;
    wp_quatf qa, qb;

    result = s_default_transform_keyframe( time );

    if( !track || track->keyframe_count == 0 )
    {
        return result;
    }

    if( track->keyframe_count == 1 )
    {
        return track->keyframes[0];
    }

    k0 = 0;
    k1 = 0;
    t = 0.0f;
    wp_node_track_get_keyframes_at_time( track, time, &k0, &k1, &t );

    kf0 = &track->keyframes[k0];
    kf1 = &track->keyframes[k1];

    result.time = time;
    result.translation = s_vec3f_lerp( kf0->translation, kf1->translation, t );
    result.scale = s_vec3f_lerp( kf0->scale, kf1->scale, t );

    qa = kf0->rotation;
    qb = kf1->rotation;

    if( track->use_shortest_rotation_path )
    {
        dot = qa.w * qb.w + qa.x * qb.x + qa.y * qb.y + qa.z * qb.z;
        if( dot < 0.0f )
        {
            qb.w = -qb.w;
            qb.x = -qb.x;
            qb.y = -qb.y;
            qb.z = -qb.z;
        }
    }

    if( rot_interp_mode == WP_ANIM_ROT_INTERP_SPHERICAL )
    {
        result.rotation = wp_quatf_slerp( qa, qb, t );
    }
    else
    {
        result.rotation.w = qa.w + ( qb.w - qa.w ) * t;
        result.rotation.x = qa.x + ( qb.x - qa.x ) * t;
        result.rotation.y = qa.y + ( qb.y - qa.y ) * t;
        result.rotation.z = qa.z + ( qb.z - qa.z ) * t;
        result.rotation = wp_quatf_normalize( result.rotation );
    }

    (void)interp_mode; /* spline mode reserved; currently falls through to linear */
    return result;
}

/* =========================================================================
 * Numeric track management
 * ====================================================================== */

wp_numeric_animation_track *wp_animation_create_numeric_track( wp_animation *anim, unsigned short handle,
                                                               wp_f32 *target_value )
{
    wp_numeric_animation_track *track;

    if( !anim || anim->numeric_track_count >= WP_ANIMATION_MAX_NUMERIC_TRACKS )
    {
        return NULL;
    }

    track = &anim->numeric_tracks[anim->numeric_track_count];
    memset( track, 0, sizeof( wp_numeric_animation_track ) );
    track->handle = handle;
    track->target_value = target_value;
    anim->numeric_track_count++;
    return track;
}

wp_numeric_animation_track *wp_animation_get_numeric_track( const wp_animation *anim,
                                                            unsigned short handle )
{
    unsigned int i;

    if( !anim )
    {
        return NULL;
    }

    for( i = 0; i < anim->numeric_track_count; ++i )
    {
        if( anim->numeric_tracks[i].handle == handle )
        {
            return (wp_numeric_animation_track *)&anim->numeric_tracks[i];
        }
    }
    return NULL;
}

void wp_animation_destroy_numeric_track( wp_animation *anim, unsigned short handle )
{
    unsigned int i;
    unsigned int remaining;

    if( !anim )
    {
        return;
    }

    for( i = 0; i < anim->numeric_track_count; ++i )
    {
        if( anim->numeric_tracks[i].handle == handle )
        {
            remaining = anim->numeric_track_count - i - 1;
            if( remaining > 0 )
            {
                memmove( &anim->numeric_tracks[i], &anim->numeric_tracks[i + 1],
                         remaining * sizeof( wp_numeric_animation_track ) );
            }
            anim->numeric_track_count--;
            return;
        }
    }
}

/* =========================================================================
 * Numeric track keyframe management
 * ====================================================================== */

wp_numeric_keyframe *wp_numeric_track_create_keyframe( wp_numeric_animation_track *track, wp_f32 time )
{
    unsigned int i;
    unsigned int insert_pos;

    if( !track || track->keyframe_count >= WP_ANIMATION_MAX_KEYFRAMES )
    {
        return NULL;
    }

    insert_pos = track->keyframe_count;
    for( i = 0; i < track->keyframe_count; ++i )
    {
        if( track->keyframes[i].time > time )
        {
            insert_pos = i;
            break;
        }
    }

    if( insert_pos < track->keyframe_count )
    {
        memmove( &track->keyframes[insert_pos + 1], &track->keyframes[insert_pos],
                 ( track->keyframe_count - insert_pos ) * sizeof( wp_numeric_keyframe ) );
    }

    track->keyframes[insert_pos].time = time;
    track->keyframes[insert_pos].value = 0.0f;
    track->keyframe_count++;
    return &track->keyframes[insert_pos];
}

wp_numeric_keyframe *wp_numeric_track_get_keyframe( const wp_numeric_animation_track *track,
                                                    unsigned int index )
{
    if( !track || index >= track->keyframe_count )
    {
        return NULL;
    }
    return (wp_numeric_keyframe *)&track->keyframes[index];
}

/* =========================================================================
 * Apply / evaluate
 * ====================================================================== */

void wp_node_track_apply( const wp_node_animation_track *track, wp_f32 time_pos, wp_f32 weight,
                          wp_anim_interp_mode interp_mode, wp_anim_rot_interp_mode rot_interp_mode )
{
    wp_transform_keyframe kf;
    wp_vec3f base_pos, base_scale;
    wp_quatf base_rot;

    if( !track || !track->target_node || track->keyframe_count == 0 )
    {
        return;
    }

    kf = wp_node_track_interpolate( track, time_pos, interp_mode, rot_interp_mode );

    if( weight >= 1.0f )
    {
        wp_node_set_position( track->target_node, kf.translation );
        wp_node_set_orientation( track->target_node, kf.rotation );
        wp_node_set_scale( track->target_node, kf.scale );
    }
    else
    {
        base_pos = wp_node_get_position( track->target_node );
        base_rot = wp_node_get_orientation( track->target_node );
        base_scale = wp_node_get_scale( track->target_node );

        base_pos = s_vec3f_lerp( base_pos, kf.translation, weight );
        base_scale = s_vec3f_lerp( base_scale, kf.scale, weight );
        base_rot = wp_quatf_slerp( base_rot, kf.rotation, weight );

        wp_node_set_position( track->target_node, base_pos );
        wp_node_set_orientation( track->target_node, base_rot );
        wp_node_set_scale( track->target_node, base_scale );
    }
}

void wp_numeric_track_apply( const wp_numeric_animation_track *track, wp_f32 time_pos, wp_f32 weight )
{
    unsigned int i;
    unsigned int k0, k1;
    wp_f32 t, v0, v1, interpolated;

    if( !track || !track->target_value || track->keyframe_count == 0 )
    {
        return;
    }

    if( track->keyframe_count == 1 )
    {
        *track->target_value = track->keyframes[0].value;
        return;
    }

    if( time_pos <= track->keyframes[0].time )
    {
        *track->target_value = track->keyframes[0].value;
        return;
    }

    if( time_pos >= track->keyframes[track->keyframe_count - 1].time )
    {
        *track->target_value = track->keyframes[track->keyframe_count - 1].value;
        return;
    }

    k0 = 0;
    k1 = 1;
    t = 0.0f;
    for( i = 0; i < track->keyframe_count - 1; ++i )
    {
        if( track->keyframes[i + 1].time >= time_pos )
        {
            wp_f32 t0 = track->keyframes[i].time;
            wp_f32 t1 = track->keyframes[i + 1].time;
            wp_f32 span = t1 - t0;
            k0 = i;
            k1 = i + 1;
            t = ( span > 1e-7f ) ? ( ( time_pos - t0 ) / span ) : 0.0f;
            break;
        }
    }

    v0 = track->keyframes[k0].value;
    v1 = track->keyframes[k1].value;
    interpolated = v0 + ( v1 - v0 ) * t;

    if( weight >= 1.0f )
    {
        *track->target_value = interpolated;
    }
    else
    {
        *track->target_value = *track->target_value + ( interpolated - *track->target_value ) * weight;
    }
}

void wp_animation_apply( const wp_animation *anim, wp_f32 time_pos, wp_f32 weight )
{
    unsigned int i;

    if( !anim )
    {
        return;
    }

    time_pos = s_clampf( time_pos, 0.0f, anim->length );
    weight = s_clampf( weight, 0.0f, 1.0f );

    for( i = 0; i < anim->node_track_count; ++i )
    {
        wp_node_track_apply( &anim->node_tracks[i], time_pos, weight, anim->interp_mode,
                             anim->rot_interp_mode );
    }

    for( i = 0; i < anim->numeric_track_count; ++i )
    {
        wp_numeric_track_apply( &anim->numeric_tracks[i], time_pos, weight );
    }
}
