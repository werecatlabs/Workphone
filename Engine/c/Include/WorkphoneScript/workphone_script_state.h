#ifndef workphone_script_state_h__
#define workphone_script_state_h__

#include "workphone_script_prerequisites.h"
#include "workphone_script_bytecode.h"
#include "workphone_script_table.h"

typedef struct wp_script_state
{
    void *lua_state;
    wp_script_object *current_object;
    wp_script_class *current_class;
    const wp_script_value *current_args;
    wp_s32 current_arg_count;
    const wp_c8 *src;
    wp_s32 pos;
    wp_s32 compile_offset;
    wp_s32 sp;
    wp_s32 call_depth;
    wp_s32 compile_param_count;
    wp_c8 compile_params[WP_SCRIPT_MAX_ARGS][64];
    wp_s32 had_error;
    wp_c8 last_error[WP_SCRIPT_ERROR_MAX];
    wp_script_table globals;
    workphone_script_chunk chunk;
    wp_script_value stack[STACK_MAX];
    wp_script_class *classes;
    wp_script_object *objects;
} wp_script_state;

WP_SCRIPT_LIB wp_script_state *wp_script_create_state( void );
WP_SCRIPT_LIB void wp_script_destroy_state( wp_script_state *state );
WP_SCRIPT_LIB const wp_c8 *wp_script_get_last_error( const wp_script_state *state );
WP_SCRIPT_LIB void wp_script_clear_error( wp_script_state *state );
WP_SCRIPT_LIB void wp_script_set_error( wp_script_state *state, const wp_c8 *message );
WP_SCRIPT_LIB void *wp_script_get_lua_state( const wp_script_state *state );

#endif /* workphone_script_state_h__ */
