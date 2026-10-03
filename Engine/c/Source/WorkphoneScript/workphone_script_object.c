#include "workphone_script_object.h"
#include "workphone_script_state.h"
#include "workphone_script_luabind.h"
#include <stdlib.h>
#include <string.h>

static wp_c8 *wp_script_strdup( const wp_c8 *source )
{
    size_t length;
    wp_c8 *copy;

    if( !source )
        return NULL;
    length = strlen( source ) + 1u;
    copy = (wp_c8 *)malloc( length );
    if( copy )
        memcpy( copy, source, length );
    return copy;
}

static wp_s32 wp_script_parent_is_valid( wp_script_class *klass, wp_script_class *parent )
{
    wp_s32 depth;

    depth = 0;
    while( parent && depth < TABLE_SIZE )
    {
        if( parent == klass )
            return 0;
        parent = parent->parent;
        depth++;
    }
    return parent == NULL;
}

static void wp_script_resolve_parents( wp_script_state *state )
{
    wp_script_class *klass;
    wp_script_class *parent;

    if( !state )
        return;
    klass = state->classes;
    while( klass )
    {
        if( !klass->parent && klass->parent_name )
        {
            parent = wp_script_find_class( state, klass->parent_name );
            if( parent && wp_script_parent_is_valid( klass, parent ) )
                klass->parent = parent;
        }
        klass = klass->next;
    }
}

WP_SCRIPT_LIB wp_script_class *wp_script_find_class( wp_script_state *state, const wp_c8 *name )
{
    wp_script_value value;

    if( !state || !name )
        return NULL;
    value = wp_script_table_get( &state->globals, name );
    if( value.type != VAL_CLASS )
        return NULL;
    return value.as.klass;
}

WP_SCRIPT_LIB wp_s32 wp_script_class_set_parent( wp_script_class *klass,
                                                 const wp_c8 *parent_name )
{
    wp_c8 *copy;
    wp_script_class *parent;

    if( !klass )
        return 0;
    copy = NULL;
    parent = NULL;
    if( parent_name && parent_name[0] )
    {
        copy = wp_script_strdup( parent_name );
        if( !copy )
        {
            wp_script_set_error( klass->state, "unable to allocate parent class name" );
            return 0;
        }
        parent = wp_script_find_class( klass->state, parent_name );
        if( parent && !wp_script_parent_is_valid( klass, parent ) )
        {
            free( copy );
            wp_script_set_error( klass->state, "script inheritance cycle detected" );
            return 0;
        }
    }
    free( klass->parent_name );
    klass->parent_name = copy;
    klass->parent = parent;
    return 1;
}

WP_SCRIPT_LIB wp_script_class *wp_script_new_class( wp_script_state *state, const wp_c8 *name )
{
    wp_script_class *klass;

    if( !state || !name || !name[0] )
        return NULL;

    klass = wp_script_find_class( state, name );
    if( klass )
        return klass;

    klass = (wp_script_class *)calloc( 1, sizeof( wp_script_class ) );
    if( !klass )
    {
        wp_script_set_error( state, "unable to allocate script class" );
        return NULL;
    }

    klass->name = wp_script_strdup( name );
    if( !klass->name )
    {
        free( klass );
        wp_script_set_error( state, "unable to allocate script class name" );
        return NULL;
    }
    klass->state = state;
    klass->next = state->classes;
    state->classes = klass;

    if( !wp_script_table_set( &state->globals, name, wp_script_make_class( klass ) ) )
    {
        state->classes = klass->next;
        free( klass->name );
        free( klass );
        wp_script_set_error( state, "script global table is full" );
        return NULL;
    }

    wp_script_resolve_parents( state );
    return klass;
}

WP_SCRIPT_LIB wp_script_class *wp_script_create_class( wp_script_state *state, const wp_c8 *name )
{
    return wp_script_new_class( state, name );
}

WP_SCRIPT_LIB wp_script_object *wp_script_new_object( wp_script_state *state, wp_script_class *klass )
{
    if( !state || !klass )
        return NULL;
    return wp_script_lua_new_object( state, klass );
}

WP_SCRIPT_LIB wp_script_value wp_script_class_get_method( wp_script_class *klass,
                                                           const wp_c8 *name )
{
    wp_script_value value;
    wp_s32 depth;

    depth = 0;
    while( klass && depth < TABLE_SIZE )
    {
        value = wp_script_table_get( &klass->methods, name );
        if( value.type != VAL_NIL )
            return value;
        klass = klass->parent;
        depth++;
    }
    return wp_script_make_nil();
}

WP_SCRIPT_LIB wp_script_value wp_script_class_get_super_method( wp_script_class *klass,
                                                                 const wp_c8 *name )
{
    if( !klass )
        return wp_script_make_nil();
    return wp_script_class_get_method( klass->parent, name );
}

WP_SCRIPT_LIB wp_script_class *wp_script_bind_class( wp_script_state *state, const wp_c8 *name,
                                                     const wp_c8 *parent_name,
                                                     wp_script_native_create create_fn,
                                                     wp_script_native_destroy destroy_fn,
                                                     void *user_data )
{
    wp_script_class *klass;

    klass = wp_script_new_class( state, name );
    if( !klass )
        return NULL;

    if( !wp_script_class_set_parent( klass, parent_name ) )
        return NULL;
    klass->native_create = create_fn;
    klass->native_destroy = destroy_fn;
    klass->native_user_data = user_data;
    wp_script_resolve_parents( state );
    if( !wp_script_lua_bind_class( klass ) )
        return NULL;
    return klass;
}

WP_SCRIPT_LIB wp_s32 wp_script_bind_method( wp_script_class *klass, const wp_c8 *name,
                                            wp_script_native_method method, void *user_data )
{
    if( !klass || !name || !method )
        return 0;
    if( !wp_script_table_set( &klass->methods, name,
                              wp_script_make_native_method( method, user_data ) ) )
    {
        wp_script_set_error( klass->state, "script method table is full" );
        return 0;
    }
    return wp_script_lua_bind_method( klass, name );
}

WP_SCRIPT_LIB void *wp_script_object_get_native( const wp_script_object *object )
{
    return object ? object->native_object : NULL;
}

WP_SCRIPT_LIB void wp_script_object_set_native( wp_script_object *object, void *native_object,
                                                wp_script_native_destroy destroy_fn,
                                                void *user_data )
{
    if( !object )
        return;
    if( object->native_object && object->native_destroy &&
        object->native_object != native_object )
    {
        object->native_destroy( object->native_object, object->native_user_data );
    }
    object->native_object = native_object;
    object->native_destroy = destroy_fn;
    object->native_user_data = user_data;
}
