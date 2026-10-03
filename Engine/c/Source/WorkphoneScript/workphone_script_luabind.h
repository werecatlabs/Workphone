#ifndef workphone_script_luabind_h__
#define workphone_script_luabind_h__

#include "workphone_script_state.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_script_lua_initialize( wp_script_state *state );
void wp_script_lua_shutdown( wp_script_state *state );
wp_s32 wp_script_lua_compile( wp_script_state *state, const wp_c8 *code,
                              wp_s32 *chunk_ref );
wp_s32 wp_script_lua_run( wp_script_state *state, wp_s32 chunk_ref,
                          wp_script_value *result );
wp_s32 wp_script_lua_bind_class( wp_script_class *klass );
wp_s32 wp_script_lua_bind_method( wp_script_class *klass, const wp_c8 *name );
wp_script_object *wp_script_lua_new_object( wp_script_state *state,
                                            wp_script_class *klass );
wp_s32 wp_script_lua_call_method( wp_script_object *object, const wp_c8 *name,
                                  const wp_script_value *args, wp_s32 arg_count,
                                  wp_script_value *result );
wp_s32 wp_script_lua_has_method( const wp_script_object *object, const wp_c8 *name );

#ifdef __cplusplus
}
#endif

#endif /* workphone_script_luabind_h__ */
