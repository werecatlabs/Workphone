#ifndef WORKPHONE_GRAPHICS_MEMORY_H
#define WORKPHONE_GRAPHICS_MEMORY_H

#include "workphone_types.h"

/* Optional allocation admission shared by native asset owners. Non-NULL tokens
 * admit bytes (including allocation headers); NULL rejects. working is zero
 * for retained storage, nonzero for scratch. Callbacks must not throw, and user
 * must outlive all assets created with this policy. */
/* Retain the original mesh struct tag for callers using `struct` in C. The
 * generic typedef lets other asset owners use the same policy and helpers. */
typedef struct wp_mesh_memory_callbacks
{
    void *user;
    void *( *reserve )( void *user, wp_u64 bytes, wp_s32 working );
    void ( *release )( void *reservation );
} wp_graphics_memory_callbacks;

#endif
