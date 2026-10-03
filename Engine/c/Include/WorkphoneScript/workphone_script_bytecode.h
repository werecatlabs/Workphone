#ifndef workphone_script_bytecode_h__
#define workphone_script_bytecode_h__

#include "workphone_script_prerequisites.h"

typedef struct
{
    wp_u8 code[WP_CODE_MAX];
    wp_s32 count;
} workphone_script_chunk;

WP_SCRIPT_LIB wp_s32 wp_script_emit( wp_script_state *state, wp_u8 b );

WP_SCRIPT_LIB wp_s32 wp_script_emit_int( wp_script_state *state, wp_s32 x );
WP_SCRIPT_LIB wp_s32 wp_script_emit_bytes( wp_script_state *state, const void *data, wp_s32 size );
WP_SCRIPT_LIB wp_s32 wp_script_patch_int( wp_script_state *state, wp_s32 offset, wp_s32 x );

#endif /* workphone_script_bytecode_h__ */
