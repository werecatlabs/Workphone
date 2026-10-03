#ifndef workphone_script_method_h__
#define workphone_script_method_h__

#include "workphone_script_object.h"

WP_SCRIPT_LIB void wp_script_call_method( wp_script_object *obj, const wp_c8 *name );
WP_SCRIPT_LIB wp_s32 wp_script_call_method_args( wp_script_object *obj, const wp_c8 *name,
                                                 const wp_script_value *args, wp_s32 arg_count,
                                                 wp_script_value *result );
WP_SCRIPT_LIB wp_s32 wp_script_has_method( const wp_script_object *obj, const wp_c8 *name );

#endif /* workphone_script_method_h__ */
