#include "workphone_script_bytecode.h"
#include "workphone_script_state.h"
#include <string.h>

WP_SCRIPT_LIB wp_s32 wp_script_emit( wp_script_state *state, wp_u8 b )
{
    if( !state || state->chunk.count >= WP_CODE_MAX )
    {
        wp_script_set_error( state, "script bytecode capacity exceeded" );
        return 0;
    }
    state->chunk.code[state->chunk.count++] = b;
    return 1;
}

WP_SCRIPT_LIB wp_s32 wp_script_emit_int( wp_script_state *state, wp_s32 x )
{
    return wp_script_emit_bytes( state, &x, (wp_s32)sizeof( x ) );
}

WP_SCRIPT_LIB wp_s32 wp_script_emit_bytes( wp_script_state *state, const void *data, wp_s32 size )
{
    if( !state || !data || size < 0 || state->chunk.count > WP_CODE_MAX - size )
    {
        wp_script_set_error( state, "script bytecode capacity exceeded" );
        return 0;
    }
    memcpy( &state->chunk.code[state->chunk.count], data, (size_t)size );
    state->chunk.count += size;
    return 1;
}

WP_SCRIPT_LIB wp_s32 wp_script_patch_int( wp_script_state *state, wp_s32 offset, wp_s32 x )
{
    if( !state || offset < 0 || offset > state->chunk.count - (wp_s32)sizeof( x ) )
    {
        wp_script_set_error( state, "invalid bytecode patch offset" );
        return 0;
    }
    memcpy( &state->chunk.code[offset], &x, sizeof( x ) );
    return 1;
}
