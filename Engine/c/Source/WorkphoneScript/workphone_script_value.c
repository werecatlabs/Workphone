#include "workphone_script_value.h"
#include <string.h>

static wp_script_value wp_script_value_init( wp_script_value_type type )
{
    wp_script_value value;
    memset( &value, 0, sizeof( value ) );
    value.type = type;
    return value;
}

WP_SCRIPT_LIB wp_script_value wp_script_make_number( wp_f64 n )
{
    wp_script_value value = wp_script_value_init( VAL_NUMBER );
    value.as.number = n;
    return value;
}

WP_SCRIPT_LIB wp_script_value wp_script_make_string( const wp_c8 *s )
{
    wp_script_value value = wp_script_value_init( VAL_STRING );
    value.as.string = s;
    return value;
}

WP_SCRIPT_LIB wp_script_value wp_script_make_nil( void )
{
    return wp_script_value_init( VAL_NIL );
}

WP_SCRIPT_LIB wp_script_value wp_script_make_object( wp_script_object *object )
{
    wp_script_value value = wp_script_value_init( VAL_OBJECT );
    value.as.object = object;
    return value;
}

WP_SCRIPT_LIB wp_script_value wp_script_make_class( wp_script_class *klass )
{
    wp_script_value value = wp_script_value_init( VAL_CLASS );
    value.as.klass = klass;
    return value;
}

WP_SCRIPT_LIB wp_script_value wp_script_make_native_method( wp_script_native_method method,
                                                            void *user_data )
{
    wp_script_value value = wp_script_value_init( VAL_NATIVE_FUNC );
    value.native_method = method;
    value.native_user_data = user_data;
    return value;
}

WP_SCRIPT_LIB wp_script_value wp_script_make_script_method( wp_s32 offset )
{
    wp_script_value value = wp_script_value_init( VAL_SCRIPT_FUNC );
    value.as.func_offset = offset;
    return value;
}
