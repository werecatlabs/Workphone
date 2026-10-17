#ifndef WP_GRAPHICS_CULL_BOUNDS_INTERNAL_H
#define WP_GRAPHICS_CULL_BOUNDS_INTERNAL_H
#include "workphone_graphics_mesh.h"
#include "workphone_matrix.h"
#include <float.h>
#include <math.h>
#include <string.h>

typedef struct wp_cull_box
{
    double center[3], axes[3][3], radius;
    wp_s32 valid, fitted;
} wp_cull_box;

typedef struct wp_cull_frustum
{
    double planes[6][4];
    wp_s32 valid;
} wp_cull_frustum;

static wp_s32 wp_cull_finite( double x )
{
    return x <= DBL_MAX && x >= -DBL_MAX;
}

static void wp_cull_prepare_box( wp_cull_box *out, const wp_aabb3f *aabb, const wp_graphics_obb *obb,
                                 const wp_mat4f *world )
{
    double center[3], axes[3][3], corner[3], value, squared, largest = 0;
    wp_s32 i, j, k, mask;
    memset( out, 0, sizeof( *out ) );
    memset( axes, 0, sizeof( axes ) );
    for( i = 0; i < 16; ++i )
    {
        if( !( ( (const wp_f32 *)world )[i] <= FLT_MAX && ( (const wp_f32 *)world )[i] >= -FLT_MAX ) )
        {
            return;
        }
    }
    if( world->m[3][0] != 0 || world->m[3][1] != 0 || world->m[3][2] != 0 || world->m[3][3] != 1 )
    {
        return;
    }
    if( obb )
    {
        for( i = 0; i < 3; ++i )
        {
            center[i] = ( (const double *)&obb->center )[i];
            for( j = 0; j < 3; ++j )
            {
                axes[i][j] = ( (const double *)&obb->half_axes[i] )[j];
            }
        }
        out->fitted = 1;
    }
    else
    {
        if( !aabb )
        {
            return;
        }
        for( i = 0; i < 3; ++i )
        {
            double low = ( (const wp_f32 *)&aabb->min )[i], high = ( (const wp_f32 *)&aabb->max )[i];
            if( !( low >= -FLT_MAX && high <= FLT_MAX && low <= high ) )
            {
                return;
            }
            center[i] = ( low + high ) * 0.5;
            axes[i][i] = ( high - low ) * 0.5;
        }
        if( axes[0][0] == 0 && axes[1][1] == 0 && axes[2][2] == 0 )
        {
            return;
        }
    }
    for( i = 0; i < 3; ++i )
    {
        value = world->m[i][3];
        for( j = 0; j < 3; ++j )
        {
            value += (double)world->m[i][j] * center[j];
        }
        out->center[i] = value;
        if( !wp_cull_finite( value ) )
        {
            return;
        }
        for( j = 0; j < 3; ++j )
        {
            value = 0;
            for( k = 0; k < 3; ++k )
            {
                value += (double)world->m[i][k] * axes[j][k];
            }
            if( !wp_cull_finite( value ) )
            {
                return;
            }
            out->axes[j][i] = value;
        }
    }
    /* Enumerate the transformed box corners: sqrt(sum squared axis lengths)
     * alone underestimates the enclosing radius when a parent induces shear. */
    for( mask = 0; mask < 8; ++mask )
    {
        squared = 0;
        for( i = 0; i < 3; ++i )
        {
            corner[i] = 0;
            for( j = 0; j < 3; ++j )
            {
                corner[i] += ( mask & ( 1 << j ) ) ? out->axes[j][i] : -out->axes[j][i];
            }
            squared += corner[i] * corner[i];
        }
        if( !wp_cull_finite( squared ) )
        {
            return;
        }
        if( squared > largest )
        {
            largest = squared;
        }
    }
    out->radius = sqrt( largest );
    out->valid = 1;
}

static void wp_cull_prepare_frustum( wp_cull_frustum *out, const wp_mat4f *vp )
{
    wp_s32 i, j, axis, sign;
    double length;
    memset( out, 0, sizeof( *out ) );
    for( i = 0; i < 16; ++i )
    {
        if( !( ( (const wp_f32 *)vp )[i] <= FLT_MAX && ( (const wp_f32 *)vp )[i] >= -FLT_MAX ) )
        {
            return;
        }
    }
    for( i = 0; i < 6; ++i )
    {
        axis = i / 2;
        sign = ( i & 1 ) ? 1 : -1;
        length = 0;
        for( j = 0; j < 4; ++j )
        {
            out->planes[i][j] = (double)vp->m[3][j] + sign * (double)vp->m[axis][j];
        }
        for( j = 0; j < 3; ++j )
        {
            length += out->planes[i][j] * out->planes[i][j];
        }
        length = sqrt( length );
        if( !wp_cull_finite( length ) || length < 1.0e-20 )
        {
            return;
        }
        for( j = 0; j < 4; ++j )
        {
            out->planes[i][j] /= length;
        }
    }
    out->valid = 1;
}

/* stage: 0=unknown/invalid, 1=sphere rejected, 2=sphere fully inside,
 * 3=box rejected, 4=box retained. Stats/owner policy remain in the cull job. */
static wp_s32 wp_cull_box_outside( const wp_cull_box *box, const wp_cull_frustum *frustum, wp_s32 sphere,
                                   wp_s32 *stage )
{
    double distances[6], tolerances[6], radius, support, dot, terms;
    wp_s32 i, j, k, inside = 1;
    *stage = 0;
    if( !box->valid || !frustum->valid )
    {
        return 0;
    }
    radius = box->radius;
    for( i = 0; i < 6; ++i )
    {
        distances[i] = frustum->planes[i][3];
        terms = fabs( frustum->planes[i][3] ) + radius + 1;
        for( j = 0; j < 3; ++j )
        {
            dot = frustum->planes[i][j] * box->center[j];
            distances[i] += dot;
            terms += fabs( dot );
        }
        tolerances[i] = 1.0e-5 * terms;
        if( sphere )
        {
            if( distances[i] + radius < -tolerances[i] )
            {
                *stage = 1;
                return 1;
            }
            if( distances[i] - radius < tolerances[i] )
            {
                inside = 0;
            }
        }
    }
    if( sphere && inside )
    {
        *stage = 2;
        return 0;
    }
    for( i = 0; i < 6; ++i )
    {
        support = 0;
        for( j = 0; j < 3; ++j )
        {
            dot = 0;
            for( k = 0; k < 3; ++k )
            {
                dot += frustum->planes[i][k] * box->axes[j][k];
            }
            support += fabs( dot );
        }
        if( distances[i] + support < -tolerances[i] )
        {
            *stage = 3;
            return 1;
        }
    }
    *stage = 4;
    return 0;
}
#endif
