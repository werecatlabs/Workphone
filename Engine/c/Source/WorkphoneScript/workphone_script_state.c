#include "workphone_script_state.h"
#include "workphone_script_object.h"
#include "workphone_script_luabind.h"
#include <stdlib.h>
#include <string.h>

WP_SCRIPT_LIB wp_script_state *wp_script_create_state( void )
{
    wp_script_state *s = (wp_script_state *)calloc( 1, sizeof( wp_script_state ) );
    if( !s )
        return NULL;

    if( !wp_script_lua_initialize( s ) )
    {
        wp_script_destroy_state( s );
        return NULL;
    }

    return s;
}

WP_SCRIPT_LIB void wp_script_destroy_state( wp_script_state *state )
{
    wp_script_object *object;
    wp_script_object *next_object;
    wp_script_class *klass;
    wp_script_class *next_class;

    if( !state )
        return;

    object = state->objects;
    while( object )
    {
        next_object = object->next;
        if( object->native_object && object->native_destroy )
            object->native_destroy( object->native_object, object->native_user_data );
        wp_script_table_clear( &object->fields );
        free( object );
        object = next_object;
    }

    klass = state->classes;
    while( klass )
    {
        next_class = klass->next;
        wp_script_table_clear( &klass->methods );
        free( klass->parent_name );
        free( klass->name );
        free( klass );
        klass = next_class;
    }

    wp_script_table_clear( &state->globals );
    wp_script_lua_shutdown( state );
    free( state );
}

WP_SCRIPT_LIB void *wp_script_get_lua_state( const wp_script_state *state )
{
    return state ? state->lua_state : NULL;
}

WP_SCRIPT_LIB const wp_c8 *wp_script_get_last_error( const wp_script_state *state )
{
    if( !state || !state->had_error )
        return "";
    return state->last_error;
}

WP_SCRIPT_LIB void wp_script_clear_error( wp_script_state *state )
{
    if( !state )
        return;
    state->had_error = 0;
    state->last_error[0] = '\0';
}

WP_SCRIPT_LIB void wp_script_set_error( wp_script_state *state, const wp_c8 *message )
{
    size_t length;

    if( !state || state->had_error )
        return;

    state->had_error = 1;
    if( !message )
        message = "unknown script error";
    length = strlen( message );
    if( length >= WP_SCRIPT_ERROR_MAX )
        length = WP_SCRIPT_ERROR_MAX - 1u;
    memcpy( state->last_error, message, length );
    state->last_error[length] = '\0';
}
