#ifndef workphone_graphics_skeleton_h__
#define workphone_graphics_skeleton_h__

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
} wp_skeleton;

#endif  // workphone_graphics_skeleton_h__
