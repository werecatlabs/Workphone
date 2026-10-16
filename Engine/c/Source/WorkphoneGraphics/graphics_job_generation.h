#ifndef WP_GRAPHICS_JOB_GENERATION_H
#define WP_GRAPHICS_JOB_GENERATION_H
#include <stdint.h>
/* Translation-unit-local identity/revision counter. Mutations remain serialized
 * by the scene owner; atomics only allocate distinct IDs across object creators. */
#ifdef _MSC_VER
#    include <intrin.h>
static __declspec( align( 8 ) ) volatile __int64 wp_job_generation_counter;

static uint64_t wp_job_next_generation( void )
{
    return (uint64_t)_InterlockedIncrement64( &wp_job_generation_counter );
}
#else
static uint64_t wp_job_generation_counter;

static uint64_t wp_job_next_generation( void )
{
    return __atomic_add_fetch( &wp_job_generation_counter, 1, __ATOMIC_RELAXED );
}
#endif
#endif
