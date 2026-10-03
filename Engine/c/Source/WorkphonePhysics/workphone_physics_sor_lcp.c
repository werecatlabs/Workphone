/**
 * @file workphone_physics_sor_lcp.c
 * @brief Projected successive-over-relaxation solver.
 */

#include "workphone_physics_sor_lcp.h"
#include <stdlib.h>

typedef struct wp_sor_lcp_solver
{
    wp_s32 max_variables;
    wp_s32 iterations;
    wp_f32 relaxation;
    void *user_data;
} wp_sor_lcp_solver;

static wp_f32 clampf( wp_f32 v, wp_f32 lo, wp_f32 hi )
{
    if( lo <= hi )
    {
        if( v < lo )
            return lo;
        if( v > hi )
            return hi;
    }
    return v;
}

wp_sor_lcp_solver *wp_sor_lcp_solver_create( wp_s32 max_variables )
{
    wp_sor_lcp_solver *solver = (wp_sor_lcp_solver *)malloc( sizeof( wp_sor_lcp_solver ) );
    if( !solver )
    {
        return NULL;
    }
    solver->max_variables = max_variables;
    solver->iterations = 32;
    solver->relaxation = 1.0f;
    solver->user_data = NULL;
    return solver;
}

void wp_sor_lcp_solver_destroy( wp_sor_lcp_solver *solver )
{
    free( solver );
}

void wp_sor_lcp_solver_set_iterations( wp_sor_lcp_solver *solver, wp_s32 iterations )
{
    if( solver )
    {
        solver->iterations = iterations > 0 ? iterations : 1;
    }
}

wp_s32 wp_sor_lcp_solver_get_iterations( const wp_sor_lcp_solver *solver )
{
    return solver ? solver->iterations : 0;
}

void wp_sor_lcp_solver_set_relaxation( wp_sor_lcp_solver *solver, wp_f32 omega )
{
    if( solver )
    {
        solver->relaxation = omega;
    }
}

wp_f32 wp_sor_lcp_solver_get_relaxation( const wp_sor_lcp_solver *solver )
{
    return solver ? solver->relaxation : 0.0f;
}

wp_s32 wp_sor_lcp_solve( wp_sor_lcp_solver *solver, const wp_f32 *a, const wp_f32 *b, const wp_f32 *lo,
                         const wp_f32 *hi, wp_f32 *x, wp_s32 n )
{
    wp_s32 iter, i, j;

    if( !solver || !a || !b || !x || n <= 0 || n > solver->max_variables )
    {
        return 0;
    }

    for( iter = 0; iter < solver->iterations; ++iter )
    {
        for( i = 0; i < n; ++i )
        {
            wp_f32 diag = a[i * n + i];
            wp_f32 sigma = 0.0f;
            wp_f32 xi;

            if( diag > -1.0e-7f && diag < 1.0e-7f )
            {
                continue;
            }

            for( j = 0; j < n; ++j )
            {
                if( j != i )
                {
                    sigma += a[i * n + j] * x[j];
                }
            }

            xi = ( b[i] - sigma ) / diag;
            xi = x[i] + solver->relaxation * ( xi - x[i] );
            x[i] = clampf( xi, lo ? lo[i] : 0.0f, hi ? hi[i] : -1.0f );
        }
    }

    return 1;
}

void *wp_sor_lcp_solver_get_user_data( const wp_sor_lcp_solver *solver )
{
    return solver ? solver->user_data : NULL;
}

void wp_sor_lcp_solver_set_user_data( wp_sor_lcp_solver *solver, void *user_data )
{
    if( solver )
    {
        solver->user_data = user_data;
    }
}
