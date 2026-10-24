#ifndef workphone_graphics_skeleton_h__
#define workphone_graphics_skeleton_h__

#include "workphone_graphics_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BONES 1024
#define ANIMBLEND_AVERAGE 0

typedef struct
{
    unsigned short handle;
    char name[64];
} wp_bone;

typedef struct
{
    unsigned short next_auto_handle;
    int blend_state;
    int manual_bones_dirty;
    wp_bone *bone_list[MAX_BONES];
    wp_graphics_memory_callbacks memory;
} wp_skeleton;

/* Initialize only fresh or destroyed storage. The caller owns the inline
 * skeleton and bone pointer inventory. A budgeted
 * policy covers separately allocated bone bodies through their destruction. */
void wp_skeleton_init( wp_skeleton *self );
wp_s32 wp_skeleton_init_budgeted( wp_skeleton *self, const wp_graphics_memory_callbacks *callbacks );
void wp_skeleton_destroy( wp_skeleton *self );
wp_bone *wp_skeleton_create_bone( wp_skeleton *self, const char *name, unsigned short handle );
wp_bone *wp_skeleton_create_bone_auto( wp_skeleton *self );
wp_bone *wp_skeleton_create_bone_named( wp_skeleton *self, const char *name );

#ifdef __cplusplus
}
#endif

#endif  // workphone_graphics_skeleton_h__
