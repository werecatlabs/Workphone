#include "workphone_script_method.h"
#include "workphone_script_luabind.h"
#include "workphone_script_state.h"
#include <stdio.h>

WP_SCRIPT_LIB wp_s32 wp_script_call_method_args( wp_script_object *object, const wp_c8 *name,
                                                 const wp_script_value *args, wp_s32 arg_count,
                                                 wp_script_value *result )
{
    return wp_script_lua_call_method( object, name, args, arg_count, result );
}

WP_SCRIPT_LIB void wp_script_call_method( wp_script_object *object, const wp_c8 *name )
{
    wp_script_value result;

    if( !wp_script_call_method_args( object, name, NULL, 0, &result ) && object &&
        object->klass && object->klass->state && !object->klass->state->had_error )
    {
        fprintf( stderr, "Method '%s' not found\n", name ? name : "" );
    }
}

WP_SCRIPT_LIB wp_s32 wp_script_has_method( const wp_script_object *object, const wp_c8 *name )
{
    return wp_script_lua_has_method( object, name );
}

WP_SCRIPT_LIB wp_s32 wp_script_call_super( wp_script_state *state, const wp_c8 *name,
                                           const wp_script_value *args, wp_s32 arg_count,
                                           wp_script_value *result )
{
    wp_script_value method;
    wp_script_class *klass;
    wp_script_class *saved_class;
    wp_s32 ok;

    if( result )
        *result = wp_script_make_nil();
    if( !state || !state->current_object || !state->current_class || !name )
        return 0;

    klass = state->current_class->parent;
    while( klass )
    {
        method = wp_script_table_get( &klass->methods, name );
        if( method.type == VAL_NATIVE_FUNC && method.native_method )
        {
            saved_class = state->current_class;
            state->current_class = klass;
            ok = method.native_method( state, state->current_object, args, arg_count, result,
                                       method.native_user_data );
            state->current_class = saved_class;
            return ok && !state->had_error;
        }
        klass = klass->parent;
    }
    return 0;
}
