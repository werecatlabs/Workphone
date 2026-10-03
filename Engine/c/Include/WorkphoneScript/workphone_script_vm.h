#ifndef workphone_script_vm_h__
#define workphone_script_vm_h__

#include "workphone_script_table.h"
#include "workphone_script_value.h"

WP_SCRIPT_LIB wp_script_table *wp_script_get_globals_table( wp_script_state *state );
WP_SCRIPT_LIB wp_script_value *wp_script_get_stack_top( wp_script_state *state );
WP_SCRIPT_LIB wp_script_value *wp_script_get_stack( wp_script_state *state );

WP_SCRIPT_LIB wp_s32 wp_script_push( wp_script_state *state, wp_script_value v );
WP_SCRIPT_LIB wp_script_value wp_script_pop( wp_script_state *state );
WP_SCRIPT_LIB void wp_script_run( wp_script_state *state );
WP_SCRIPT_LIB void wp_script_run_from( wp_script_state *state, wp_s32 offset );
WP_SCRIPT_LIB wp_s32 wp_script_run_from_ex( wp_script_state *state, wp_s32 offset,
                                            wp_script_value *result );

#endif /* workphone_script_vm_h__ */
