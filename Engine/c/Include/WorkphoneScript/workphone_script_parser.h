#ifndef workphone_script_parser_h__
#define workphone_script_parser_h__

#include "workphone_script_prerequisites.h"

WP_SCRIPT_LIB const wp_c8 *wp_script_get_source( wp_script_state *state );
WP_SCRIPT_LIB void wp_script_set_source( wp_script_state *state, const wp_c8 *code );
WP_SCRIPT_LIB wp_s32 wp_script_get_pos( wp_script_state *state );

WP_SCRIPT_LIB void wp_script_skip( wp_script_state *state );

WP_SCRIPT_LIB wp_s32 wp_script_match( wp_script_state *state, const wp_c8 *kw );

WP_SCRIPT_LIB wp_s32 wp_script_match_keyword( wp_script_state *state, const wp_c8 *kw );

WP_SCRIPT_LIB wp_c8 *wp_script_identifier( wp_script_state *state );

WP_SCRIPT_LIB void wp_script_parse_class( wp_script_state *state );

WP_SCRIPT_LIB void wp_script_parse_function( wp_script_state *state );

WP_SCRIPT_LIB void wp_script_parse_new( wp_script_state *state );

WP_SCRIPT_LIB void wp_script_parse_call( wp_script_state *state, const wp_c8 *obj );

WP_SCRIPT_LIB void wp_script_parse_assign( wp_script_state *state, const wp_c8 *name );

WP_SCRIPT_LIB void wp_script_compile( wp_script_state *state, const wp_c8 *code );
WP_SCRIPT_LIB wp_s32 wp_script_compile_ex( wp_script_state *state, const wp_c8 *code );

WP_SCRIPT_LIB wp_s32 wp_script_get_compile_offset( wp_script_state *state );

#endif /* workphone_script_parser_h__ */
