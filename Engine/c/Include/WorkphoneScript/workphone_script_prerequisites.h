#ifndef workphone_script_prerequisites_h__
#define workphone_script_prerequisites_h__

#include "workphone_types.h"

#ifndef WP_SCRIPT_LIB
#    ifdef WORKPHONE_SINGLE_FILE
#        define WP_SCRIPT_LIB static
#    else
#        define WP_SCRIPT_LIB extern
#    endif
#endif

#define WP_CODE_MAX 65536
#define STACK_MAX 256
#define TABLE_SIZE 64
#define WP_SCRIPT_MAX_ARGS 16
#define WP_SCRIPT_MAX_CALL_DEPTH 64
#define WP_SCRIPT_ERROR_MAX 256

typedef struct wp_script_class wp_script_class;
typedef struct wp_script_object wp_script_object;
typedef struct wp_script_state wp_script_state;
typedef struct wp_script_value wp_script_value;

/*
 * Native methods use an explicit C ABI.  Returning the value through an
 * output pointer keeps the declaration valid in C89 while wp_script_value is
 * still an incomplete type here.
 */
typedef wp_s32 ( *wp_script_native_method )( wp_script_state *state,
                                             wp_script_object *self,
                                             const wp_script_value *args,
                                             wp_s32 arg_count,
                                             wp_script_value *result,
                                             void *user_data );
typedef void *( *wp_script_native_create )( wp_script_state *state,
                                            wp_script_object *self,
                                            void *user_data );
typedef void ( *wp_script_native_destroy )( void *native_object, void *user_data );

typedef enum
{
    OP_PUSH_NUM,
    OP_PUSH_STR,
    OP_PUSH_NIL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_SELF,
    OP_GET_ARG,
    OP_NEW_OBJECT,
    OP_CALL_METHOD,
    OP_CALL_SUPER,
    OP_POP,
    OP_RETURN,
    OP_JUMP,
    OP_END
} workphone_script_opcode;

typedef enum
{
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_OBJECT,
    VAL_CLASS,
    VAL_NATIVE,
    VAL_NATIVE_FUNC,
    VAL_SCRIPT_FUNC
} wp_script_value_type;

#endif /* workphone_script_prerequisites_h__ */
