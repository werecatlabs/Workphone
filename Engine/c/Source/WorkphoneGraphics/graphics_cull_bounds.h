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
    double corners[8][3], edges[12][3], error;
    wp_s32 valid, sat_valid;
} wp_cull_frustum;

static wp_s32 wp_cull_finite( double x )
{
    return x <= DBL_MAX && x >= -DBL_MAX;
}

static double wp_cull_dot( const double a[3], const double b[3] )
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void wp_cull_cross( double out[3], const double a[3], const double b[3] )
{
    wp_s32 i;
    for( i = 0; i < 3; ++i )
    {
        out[i] = a[( i + 1 ) % 3] * b[( i + 2 ) % 3] - a[( i + 2 ) % 3] * b[( i + 1 ) % 3];
    }
}

static wp_s32 wp_cull_unit( double axis[3] )
{
    double length = sqrt( wp_cull_dot( axis, axis ) );
    wp_s32 i;
    if( !wp_cull_finite( length ) || length <= 1.0e-10 )
    {
        return 0;
    }
    for( i = 0; i < 3; ++i )
    {
        axis[i] /= length;
    }
    return 1;
}

/* Intersect one plane from each clip-coordinate pair. Do not refine when the
 * finite corners cannot be reconstructed reliably (including infinite far views). */
static void wp_cull_prepare_sat_frustum( wp_cull_frustum *out )
{
    const double *p[3];
    double cross[3][3], det, min_det = 1, magnitude = 1, distance, terms;
    wp_s32 mask, i, j, k, axis, edge = 0;
    out->sat_valid = 0;
    if( !out->valid )
    {
        return;
    }
    for( mask = 0; mask < 8; ++mask )
    {
        for( i = 0; i < 3; ++i )
        {
            p[i] = out->planes[2 * i + ( ( mask >> i ) & 1 )];
        }
        for( i = 0; i < 3; ++i )
        {
            wp_cull_cross( cross[i], p[( i + 1 ) % 3], p[( i + 2 ) % 3] );
        }
        det = wp_cull_dot( p[0], cross[0] );
        if( !wp_cull_finite( det ) || fabs( det ) < 1.0e-8 )
        {
            return;
        }
        if( fabs( det ) < min_det )
        {
            min_det = fabs( det );
        }
        for( j = 0; j < 3; ++j )
        {
            out->corners[mask][j] = 0;
            for( i = 0; i < 3; ++i )
            {
                out->corners[mask][j] -= p[i][3] * cross[i][j] / det;
            }
            if( !wp_cull_finite( out->corners[mask][j] ) )
            {
                return;
            }
            if( fabs( out->corners[mask][j] ) > magnitude )
            {
                magnitude = fabs( out->corners[mask][j] );
            }
        }
        for( i = 0; i < 6; ++i )
        {
            distance = out->planes[i][3];
            terms = 1 + fabs( distance );
            if( fabs( distance ) > magnitude )
            {
                magnitude = fabs( distance );
            }
            for( j = 0; j < 3; ++j )
            {
                distance += out->planes[i][j] * out->corners[mask][j];
                terms += fabs( out->planes[i][j] * out->corners[mask][j] );
            }
            if( !wp_cull_finite( distance ) || distance < -1.0e-5 * terms )
            {
                return;
            }
            if( ( i & 1 ) == ( ( mask >> ( i / 2 ) ) & 1 ) && fabs( distance ) > 1.0e-5 * terms )
            {
                return;
            }
        }
    }
    /* Roundoff amplified by near-parallel planes enlarges projection intervals. */
    out->error = magnitude * ( 1.0e-5 + 128 * DBL_EPSILON / min_det );
    if( !wp_cull_finite( out->error ) )
    {
        return;
    }
    for( axis = 0; axis < 3; ++axis )
    {
        k = 1 << axis;
        for( mask = 0; mask < 8; ++mask )
        {
            if( !( mask & k ) )
            {
                for( j = 0; j < 3; ++j )
                {
                    out->edges[edge][j] = out->corners[mask | k][j] - out->corners[mask][j];
                }
                if( !wp_cull_unit( out->edges[edge] ) )
                {
                    return;
                }
                ++edge;
            }
        }
    }
    out->sat_valid = 1;
}

/* Relative projections avoid subtracting two large interval centres. Skipped
 * near-zero axes retain candidates; uncertainty always enlarges the intervals. */
static wp_s32 wp_cull_sat_axis( const wp_cull_box *box, const wp_cull_frustum *frustum, double axis[3],
                                wp_u32 *tested )
{
    double low = DBL_MAX, high = -DBL_MAX, delta[3], value, support = 0, tolerance;
    wp_s32 i, j;
    if( !wp_cull_unit( axis ) )
    {
        return 0;
    }
    ++*tested;
    for( i = 0; i < 8; ++i )
    {
        for( j = 0; j < 3; ++j )
        {
            delta[j] = frustum->corners[i][j] - box->center[j];
        }
        value = wp_cull_dot( axis, delta );
        if( !wp_cull_finite( value ) )
        {
            return 0;
        }
        if( value < low )
        {
            low = value;
        }
        if( value > high )
        {
            high = value;
        }
    }
    for( i = 0; i < 3; ++i )
    {
        support += fabs( wp_cull_dot( axis, box->axes[i] ) );
    }
    tolerance = frustum->error +
                1.0e-5 * ( 1 + box->radius + fabs( low ) + fabs( high ) + fabs( box->center[0] ) +
                           fabs( box->center[1] ) + fabs( box->center[2] ) );
    if( !wp_cull_finite( support ) || !wp_cull_finite( tolerance ) )
    {
        return 0;
    }
    return low > support + tolerance || high < -support - tolerance;
}

/* Convex-polyhedron SAT: face normals of both shapes plus edge cross products.
 * Transformed half axes can be sheared; face normals are their cross products. */
static wp_s32 wp_cull_sat_outside( const wp_cull_box *box, const wp_cull_frustum *frustum,
                                   wp_u32 *tested )
{
    double edges[3][3], axis[3];
    wp_s32 i, j, k;
    *tested = 0;
    if( !box->valid || !frustum->sat_valid )
    {
        return 0;
    }
    for( i = 0; i < 6; ++i )
    {
        for( j = 0; j < 3; ++j )
        {
            axis[j] = frustum->planes[i][j];
        }
        if( wp_cull_sat_axis( box, frustum, axis, tested ) )
        {
            return 1;
        }
    }
    for( i = 0; i < 3; ++i )
    {
        for( j = 0; j < 3; ++j )
        {
            edges[i][j] = box->axes[i][j];
        }
        if( !wp_cull_unit( edges[i] ) )
        {
            memset( edges[i], 0, sizeof( edges[i] ) );
        }
    }
    for( i = 0; i < 3; ++i )
    {
        wp_cull_cross( axis, edges[( i + 1 ) % 3], edges[( i + 2 ) % 3] );
        if( wp_cull_sat_axis( box, frustum, axis, tested ) )
        {
            return 1;
        }
        for( k = 0; k < 12; ++k )
        {
            wp_cull_cross( axis, edges[i], frustum->edges[k] );
            if( wp_cull_sat_axis( box, frustum, axis, tested ) )
            {
                return 1;
            }
        }
    }
    return 0;
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
 * 3=box rejected, 4=boundary box retained, 5=box fully inside.
 * Stats/owner policy remain in the cull job. */
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
    inside = 1;
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
        if( distances[i] - support < tolerances[i] )
        {
            inside = 0;
        }
    }
    *stage = inside ? 5 : 4;
    return 0;
}
#endif
