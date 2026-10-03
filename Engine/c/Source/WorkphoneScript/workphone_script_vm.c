#include "workphone_script_vm.h"
#include "workphone_script_luabind.h"
#include "workphone_script_state.h"

WP_SCRIPT_LIB wp_script_table *wp_script_get_globals_table( wp_script_state *state )
{
    return state ? &state->globals : NULL;
}

WP_SCRIPT_LIB wp_script_value *wp_script_get_stack_top( wp_script_state *state )
{
    if( !state || state->sp < 0 || state->sp > STACK_MAX )
        return NULL;
    return &state->stack[state->sp];
}

WP_SCRIPT_LIB wp_script_value *wp_script_get_stack( wp_script_state *state )
{
    return state ? state->stack : NULL;
}

WP_SCRIPT_LIB wp_s32 wp_script_push( wp_script_state *state, wp_script_value value )
{
    if( !state || state->sp >= STACK_MAX )
    {
        wp_script_set_error( state, "script stack overflow" );
        return 0;
    }
    state->stack[state->sp++] = value;
    return 1;
}

WP_SCRIPT_LIB wp_script_value wp_script_pop( wp_script_state *state )
{
    if( !state || state->sp <= 0 )
    {
        wp_script_set_error( state, "script stack underflow" );
        return wp_script_make_nil();
    }
    --state->sp;
    return state->stack[state->sp];
}

WP_SCRIPT_LIB wp_s32 wp_script_run_from_ex( wp_script_state *state, wp_s32 offset,
                                            wp_script_value *result )
{
    return wp_script_lua_run( state, offset, result );
}

WP_SCRIPT_LIB void wp_script_run_from( wp_script_state *state, wp_s32 offset )
{
    wp_script_value result;
    wp_script_run_from_ex( state, offset, &result );
}

WP_SCRIPT_LIB void wp_script_run( wp_script_state *state )
{
    if( state )
        wp_script_run_from( state, state->compile_offset );
}
