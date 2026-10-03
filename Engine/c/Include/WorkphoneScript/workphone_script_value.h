#ifndef workphone_script_value_h__
#define workphone_script_value_h__

#include "workphone_script_prerequisites.h"

typedef union wp_script_value_data
{
    wp_f64 number;
    wp_s32 func_offset;
    const wp_c8 *string;
    wp_script_object *object;
    wp_script_class *klass;
    void *pointer;
} wp_script_value_data;

struct wp_script_value
{
    wp_script_value_type type;
    wp_script_value_data as;
    wp_script_native_method native_method;
    void *native_user_data;
};

WP_SCRIPT_LIB wp_script_value wp_script_make_number( wp_f64 n );
WP_SCRIPT_LIB wp_script_value wp_script_make_string( const wp_c8 *s );
WP_SCRIPT_LIB wp_script_value wp_script_make_nil( void );
WP_SCRIPT_LIB wp_script_value wp_script_make_object( wp_script_object *object );
WP_SCRIPT_LIB wp_script_value wp_script_make_class( wp_script_class *klass );
WP_SCRIPT_LIB wp_script_value wp_script_make_native_method( wp_script_native_method method,
                                                            void *user_data );
WP_SCRIPT_LIB wp_script_value wp_script_make_script_method( wp_s32 offset );

#endif /* workphone_script_value_h__ */
