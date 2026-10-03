/**
 * @file workphone_physics_sor_lcp.h
 * @brief Small projected SOR solver for boxed linear complementarity systems.
 */

#ifndef WORKPHONE_PHYSICS_SOR_LCP_H
#define WORKPHONE_PHYSICS_SOR_LCP_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_sor_lcp_solver wp_sor_lcp_solver;

wp_sor_lcp_solver *wp_sor_lcp_solver_create( wp_s32 max_variables );
void wp_sor_lcp_solver_destroy( wp_sor_lcp_solver *solver );

void wp_sor_lcp_solver_set_iterations( wp_sor_lcp_solver *solver, wp_s32 iterations );
wp_s32 wp_sor_lcp_solver_get_iterations( const wp_sor_lcp_solver *solver );

void wp_sor_lcp_solver_set_relaxation( wp_sor_lcp_solver *solver, wp_f32 omega );
wp_f32 wp_sor_lcp_solver_get_relaxation( const wp_sor_lcp_solver *solver );

wp_s32 wp_sor_lcp_solve( wp_sor_lcp_solver *solver, const wp_f32 *a, const wp_f32 *b, const wp_f32 *lo,
                         const wp_f32 *hi, wp_f32 *x, wp_s32 n );

void *wp_sor_lcp_solver_get_user_data( const wp_sor_lcp_solver *solver );
void wp_sor_lcp_solver_set_user_data( wp_sor_lcp_solver *solver, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_SOR_LCP_H */
