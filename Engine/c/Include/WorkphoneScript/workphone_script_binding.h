#ifndef workphone_script_binding_h__
#define workphone_script_binding_h__

/**
 * @file workphone_script_binding.h
 * @brief C89 object-oriented bindings for WorkphoneScript.
 *
 * Bind a native base class with wp_script_bind_class(), then map its methods
 * to ordinary C functions with wp_script_bind_method(). Script classes may
 * inherit from the bound class and instances carry the pointer returned by
 * the native create callback. Retrieve that pointer with
 * wp_script_object_get_native().
 *
 * Native methods receive the VM, script receiver, argument array, result
 * output, and the user-data pointer supplied at registration. They return
 * non-zero on success. Use wp_script_set_error() before returning zero to
 * provide a diagnostic.
 */

#include "workphone_script_method.h"
#include "workphone_script_object.h"
#include "workphone_script_state.h"
#include "workphone_script_value.h"

#endif /* workphone_script_binding_h__ */
