#ifndef workphone_script_object_h__
#define workphone_script_object_h__

#include "workphone_script_table.h"

typedef struct wp_script_class
{
    wp_c8 *name;
    wp_c8 *parent_name;
    struct wp_script_class *parent;
    wp_script_state *state;
    wp_script_table methods;
    wp_script_native_create native_create;
    wp_script_native_destroy native_destroy;
    void *native_user_data;
    struct wp_script_class *next;
} wp_script_class;

typedef struct wp_script_object
{
    wp_script_class *klass;
    wp_script_table fields;
    void *native_object;
    wp_script_native_destroy native_destroy;
    void *native_user_data;
    void *lua_identity;
    wp_s32 lua_ref;
    struct wp_script_object *next;
} wp_script_object;

WP_SCRIPT_LIB wp_script_class *wp_script_new_class( wp_script_state *state, const wp_c8 *name );
WP_SCRIPT_LIB wp_script_class *wp_script_create_class( wp_script_state *state, const wp_c8 *name );
WP_SCRIPT_LIB wp_script_object *wp_script_new_object( wp_script_state *state, wp_script_class *c );
WP_SCRIPT_LIB wp_script_class *wp_script_find_class( wp_script_state *state, const wp_c8 *name );
WP_SCRIPT_LIB wp_s32 wp_script_class_set_parent( wp_script_class *klass,
                                                 const wp_c8 *parent_name );

WP_SCRIPT_LIB wp_script_value wp_script_class_get_method( wp_script_class *c, const wp_c8 *name );
WP_SCRIPT_LIB wp_script_value wp_script_class_get_super_method( wp_script_class *c, const wp_c8 *name );
WP_SCRIPT_LIB wp_s32 wp_script_call_super( wp_script_state *state, const wp_c8 *name,
                                           const wp_script_value *args, wp_s32 arg_count,
                                           wp_script_value *result );

WP_SCRIPT_LIB wp_script_class *wp_script_bind_class( wp_script_state *state, const wp_c8 *name,
                                                     const wp_c8 *parent_name,
                                                     wp_script_native_create create_fn,
                                                     wp_script_native_destroy destroy_fn,
                                                     void *user_data );
WP_SCRIPT_LIB wp_s32 wp_script_bind_method( wp_script_class *klass, const wp_c8 *name,
                                            wp_script_native_method method, void *user_data );
WP_SCRIPT_LIB void *wp_script_object_get_native( const wp_script_object *object );
WP_SCRIPT_LIB void wp_script_object_set_native( wp_script_object *object, void *native_object,
                                                wp_script_native_destroy destroy_fn,
                                                void *user_data );

#endif /* workphone_script_object_h__ */
