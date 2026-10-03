#include "workphone_script_parser.h"
#include "workphone_script_bytecode.h"
#include "workphone_script_object.h"
#include "workphone_script_state.h"
#include "workphone_script_value.h"
#include "workphone_script_vm.h"
#include "workphone_script_luabind.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static wp_c8 *wp_script_parser_strndup( const wp_c8 *source, wp_s32 length )
{
    wp_c8 *copy;

    if( !source || length < 0 )
        return NULL;
    copy = (wp_c8 *)malloc( (size_t)length + 1u );
    if( !copy )
        return NULL;
    if( length > 0 )
        memcpy( copy, source, (size_t)length );
    copy[length] = '\0';
    return copy;
}

static wp_s32 wp_script_is_identifier_start( wp_c8 value )
{
    return isalpha( (unsigned char)value ) || value == '_';
}

static wp_s32 wp_script_is_identifier_part( wp_c8 value )
{
    return isalnum( (unsigned char)value ) || value == '_';
}

static void wp_script_parser_error( wp_script_state *state, const wp_c8 *message )
{
    wp_script_set_error( state, message );
}

WP_SCRIPT_LIB const wp_c8 *wp_script_get_source( wp_script_state *state )
{
    return state ? state->src : NULL;
}

WP_SCRIPT_LIB void wp_script_set_source( wp_script_state *state, const wp_c8 *code )
{
    if( !state )
        return;
    state->src = code;
    state->pos = 0;
}

WP_SCRIPT_LIB wp_s32 wp_script_get_pos( wp_script_state *state )
{
    return state ? state->pos : 0;
}

WP_SCRIPT_LIB void wp_script_skip( wp_script_state *state )
{
    wp_s32 skipped;

    if( !state || !state->src )
        return;

    do
    {
        skipped = 0;
        while( state->src[state->pos] &&
               isspace( (unsigned char)state->src[state->pos] ) )
        {
            state->pos++;
            skipped = 1;
        }
        if( state->src[state->pos] == '-' && state->src[state->pos + 1] == '-' )
        {
            while( state->src[state->pos] && state->src[state->pos] != '\n' )
                state->pos++;
            skipped = 1;
        }
        else if( state->src[state->pos] == '#' )
        {
            while( state->src[state->pos] && state->src[state->pos] != '\n' )
                state->pos++;
            skipped = 1;
        }
    } while( skipped );
}

WP_SCRIPT_LIB wp_s32 wp_script_match( wp_script_state *state, const wp_c8 *text )
{
    size_t length;

    if( !state || !state->src || !text )
        return 0;
    wp_script_skip( state );
    length = strlen( text );
    if( strncmp( &state->src[state->pos], text, length ) != 0 )
        return 0;
    state->pos += (wp_s32)length;
    return 1;
}

WP_SCRIPT_LIB wp_s32 wp_script_match_keyword( wp_script_state *state, const wp_c8 *keyword )
{
    size_t length;
    wp_c8 next;

    if( !state || !state->src || !keyword )
        return 0;
    wp_script_skip( state );
    length = strlen( keyword );
    if( strncmp( &state->src[state->pos], keyword, length ) != 0 )
        return 0;
    next = state->src[state->pos + (wp_s32)length];
    if( wp_script_is_identifier_part( next ) )
        return 0;
    state->pos += (wp_s32)length;
    return 1;
}

WP_SCRIPT_LIB wp_c8 *wp_script_identifier( wp_script_state *state )
{
    wp_s32 start;
    wp_s32 length;

    if( !state || !state->src )
        return NULL;
    wp_script_skip( state );
    start = state->pos;
    if( !wp_script_is_identifier_start( state->src[state->pos] ) )
        return wp_script_parser_strndup( "", 0 );
    state->pos++;
    while( wp_script_is_identifier_part( state->src[state->pos] ) )
        state->pos++;
    length = state->pos - start;
    return wp_script_parser_strndup( &state->src[start], length );
}

static wp_s32 wp_script_emit_name( wp_script_state *state, const wp_c8 *name )
{
    wp_s32 length;
    wp_c8 terminator;

    if( !name )
        return 0;
    length = (wp_s32)strlen( name );
    if( length <= 0 || length >= 128 )
    {
        wp_script_parser_error( state, "script identifier is too long" );
        return 0;
    }
    terminator = '\0';
    return wp_script_emit_int( state, length ) &&
           wp_script_emit_bytes( state, name, length ) &&
           wp_script_emit_bytes( state, &terminator, 1 );
}

static wp_s32 wp_script_find_parameter( wp_script_state *state, const wp_c8 *name )
{
    wp_s32 i;

    for( i = 0; i < state->compile_param_count; ++i )
    {
        if( strcmp( state->compile_params[i], name ) == 0 )
            return i;
    }
    return -1;
}

static wp_s32 wp_script_parse_expression( wp_script_state *state );

static wp_s32 wp_script_parse_arguments( wp_script_state *state, wp_s32 *arg_count )
{
    wp_s32 count;

    count = 0;
    if( !wp_script_match( state, "(" ) )
    {
        *arg_count = 0;
        return 1;
    }
    wp_script_skip( state );
    if( wp_script_match( state, ")" ) )
    {
        *arg_count = 0;
        return 1;
    }

    while( !state->had_error )
    {
        if( count >= WP_SCRIPT_MAX_ARGS )
        {
            wp_script_parser_error( state, "too many script call arguments" );
            return 0;
        }
        if( !wp_script_parse_expression( state ) )
            return 0;
        count++;
        if( wp_script_match( state, ")" ) )
            break;
        if( !wp_script_match( state, "," ) )
        {
            wp_script_parser_error( state, "expected ',' or ')' in argument list" );
            return 0;
        }
    }
    *arg_count = count;
    return !state->had_error;
}

static wp_s32 wp_script_parse_string( wp_script_state *state )
{
    wp_c8 quote;
    wp_c8 buffer[1024];
    wp_s32 count;
    wp_c8 value;
    wp_c8 terminator;

    wp_script_skip( state );
    quote = state->src[state->pos];
    if( quote != '\'' && quote != '"' )
        return 0;
    state->pos++;
    count = 0;
    while( state->src[state->pos] && state->src[state->pos] != quote )
    {
        value = state->src[state->pos++];
        if( value == '\\' && state->src[state->pos] )
        {
            value = state->src[state->pos++];
            if( value == 'n' )
                value = '\n';
            else if( value == 'r' )
                value = '\r';
            else if( value == 't' )
                value = '\t';
        }
        if( count >= (wp_s32)sizeof( buffer ) - 1 )
        {
            wp_script_parser_error( state, "script string literal is too long" );
            return 0;
        }
        buffer[count++] = value;
    }
    if( state->src[state->pos] != quote )
    {
        wp_script_parser_error( state, "unterminated script string" );
        return 0;
    }
    state->pos++;
    terminator = '\0';
    return wp_script_emit( state, OP_PUSH_STR ) &&
           wp_script_emit_int( state, count ) &&
           ( count == 0 || wp_script_emit_bytes( state, buffer, count ) ) &&
           wp_script_emit_bytes( state, &terminator, 1 );
}

static wp_s32 wp_script_parse_primary( wp_script_state *state )
{
    wp_c8 *end;
    wp_f64 number;
    wp_c8 *name;
    wp_s32 parameter;
    wp_s32 arg_count;

    wp_script_skip( state );
    if( state->src[state->pos] == '\'' || state->src[state->pos] == '"' )
        return wp_script_parse_string( state );

    if( isdigit( (unsigned char)state->src[state->pos] ) ||
        ( state->src[state->pos] == '-' &&
          isdigit( (unsigned char)state->src[state->pos + 1] ) ) )
    {
        number = strtod( &state->src[state->pos], &end );
        if( end == &state->src[state->pos] )
            return 0;
        state->pos = (wp_s32)( end - state->src );
        return wp_script_emit( state, OP_PUSH_NUM ) &&
               wp_script_emit_bytes( state, &number, (wp_s32)sizeof( number ) );
    }

    if( wp_script_match_keyword( state, "nil" ) )
        return wp_script_emit( state, OP_PUSH_NIL );
    if( wp_script_match_keyword( state, "true" ) )
    {
        number = 1.0;
        return wp_script_emit( state, OP_PUSH_NUM ) &&
               wp_script_emit_bytes( state, &number, (wp_s32)sizeof( number ) );
    }
    if( wp_script_match_keyword( state, "false" ) )
    {
        number = 0.0;
        return wp_script_emit( state, OP_PUSH_NUM ) &&
               wp_script_emit_bytes( state, &number, (wp_s32)sizeof( number ) );
    }
    if( wp_script_match_keyword( state, "new" ) )
    {
        name = wp_script_identifier( state );
        if( !name || !name[0] )
        {
            free( name );
            wp_script_parser_error( state, "expected class name after 'new'" );
            return 0;
        }
        wp_script_emit( state, OP_GET_GLOBAL );
        wp_script_emit_name( state, name );
        free( name );
        if( !wp_script_parse_arguments( state, &arg_count ) )
            return 0;
        return wp_script_emit( state, OP_NEW_OBJECT ) &&
               wp_script_emit( state, (wp_u8)arg_count );
    }

    name = wp_script_identifier( state );
    if( !name || !name[0] )
    {
        free( name );
        wp_script_parser_error( state, "expected script expression" );
        return 0;
    }
    if( strcmp( name, "self" ) == 0 )
    {
        free( name );
        return wp_script_emit( state, OP_GET_SELF );
    }
    parameter = wp_script_find_parameter( state, name );
    if( parameter >= 0 )
    {
        free( name );
        return wp_script_emit( state, OP_GET_ARG ) &&
               wp_script_emit( state, (wp_u8)parameter );
    }

    wp_script_emit( state, OP_GET_GLOBAL );
    wp_script_emit_name( state, name );
    free( name );
    return !state->had_error;
}

static wp_s32 wp_script_parse_expression( wp_script_state *state )
{
    wp_c8 *method;
    wp_s32 arg_count;

    if( !wp_script_parse_primary( state ) )
        return 0;

    while( wp_script_match( state, ":" ) || wp_script_match( state, "." ) )
    {
        method = wp_script_identifier( state );
        if( !method || !method[0] )
        {
            free( method );
            wp_script_parser_error( state, "expected method name" );
            return 0;
        }
        if( !wp_script_parse_arguments( state, &arg_count ) )
        {
            free( method );
            return 0;
        }
        wp_script_emit( state, OP_CALL_METHOD );
        wp_script_emit_name( state, method );
        wp_script_emit( state, (wp_u8)arg_count );
        free( method );
    }
    return !state->had_error;
}

WP_SCRIPT_LIB void wp_script_parse_class( wp_script_state *state )
{
    wp_c8 *name;
    wp_c8 *parent_name;
    wp_script_class *klass;
    wp_c8 quote;
    wp_s32 start;
    wp_s32 parenthesized;

    wp_script_skip( state );
    name = NULL;
    if( state->src[state->pos] == '\'' || state->src[state->pos] == '"' )
    {
        quote = state->src[state->pos++];
        start = state->pos;
        while( state->src[state->pos] && state->src[state->pos] != quote )
            state->pos++;
        if( state->src[state->pos] != quote )
        {
            wp_script_parser_error( state, "unterminated class name" );
            return;
        }
        name = wp_script_parser_strndup( &state->src[start], state->pos - start );
        state->pos++;
    }
    else
    {
        name = wp_script_identifier( state );
    }
    if( !name || !name[0] )
    {
        free( name );
        wp_script_parser_error( state, "expected class name" );
        return;
    }

    klass = wp_script_new_class( state, name );
    free( name );
    if( !klass )
        return;

    parenthesized = wp_script_match( state, "(" );
    if( parenthesized || wp_script_match( state, ":" ) )
    {
        parent_name = wp_script_identifier( state );
        if( !parent_name || !parent_name[0] )
        {
            free( parent_name );
            wp_script_parser_error( state, "expected parent class name" );
            return;
        }
        if( !wp_script_class_set_parent( klass, parent_name ) )
        {
            free( parent_name );
            return;
        }
        free( parent_name );
        if( parenthesized && !wp_script_match( state, ")" ) )
        {
            wp_script_parser_error( state, "expected ')' after parent class" );
            return;
        }
    }
}

static wp_s32 wp_script_parse_super_call( wp_script_state *state )
{
    wp_c8 *method;
    wp_s32 arg_count;

    if( !wp_script_match( state, ":" ) && !wp_script_match( state, "." ) )
    {
        wp_script_parser_error( state, "expected ':' after 'super'" );
        return 0;
    }
    method = wp_script_identifier( state );
    if( !method || !method[0] )
    {
        free( method );
        wp_script_parser_error( state, "expected super method name" );
        return 0;
    }
    if( !wp_script_parse_arguments( state, &arg_count ) )
    {
        free( method );
        return 0;
    }
    wp_script_emit( state, OP_CALL_SUPER );
    wp_script_emit_name( state, method );
    wp_script_emit( state, (wp_u8)arg_count );
    free( method );
    return !state->had_error;
}

static wp_s32 wp_script_parse_statement( wp_script_state *state )
{
    wp_s32 saved_pos;
    wp_s32 cursor;
    wp_c8 *name;

    wp_script_skip( state );
    if( wp_script_match_keyword( state, "return" ) )
    {
        cursor = state->pos;
        while( state->src[cursor] == ' ' || state->src[cursor] == '\t' ||
               state->src[cursor] == '\v' || state->src[cursor] == '\f' )
            cursor++;
        if( !state->src[cursor] || state->src[cursor] == '\r' ||
            state->src[cursor] == '\n' || state->src[cursor] == ';' ||
            state->src[cursor] == '#' ||
            ( state->src[cursor] == '-' && state->src[cursor + 1] == '-' ) )
        {
            state->pos = cursor;
            wp_script_emit( state, OP_PUSH_NIL );
        }
        else
        {
            if( !wp_script_parse_expression( state ) )
                return 0;
            wp_script_skip( state );
        }
        if( state->src[state->pos] == ';' )
            state->pos++;
        return wp_script_emit( state, OP_RETURN );
    }
    if( wp_script_match_keyword( state, "super" ) )
    {
        if( !wp_script_parse_super_call( state ) )
            return 0;
        wp_script_emit( state, OP_POP );
        wp_script_match( state, ";" );
        return !state->had_error;
    }

    saved_pos = state->pos;
    name = wp_script_identifier( state );
    if( name && name[0] && wp_script_match( state, "=" ) )
    {
        if( !wp_script_parse_expression( state ) )
        {
            free( name );
            return 0;
        }
        wp_script_emit( state, OP_SET_GLOBAL );
        wp_script_emit_name( state, name );
        free( name );
        wp_script_match( state, ";" );
        return !state->had_error;
    }
    free( name );
    state->pos = saved_pos;
    if( !wp_script_parse_expression( state ) )
        return 0;
    wp_script_emit( state, OP_POP );
    wp_script_match( state, ";" );
    return !state->had_error;
}

WP_SCRIPT_LIB void wp_script_parse_function( wp_script_state *state )
{
    wp_c8 *class_name;
    wp_c8 *method_name;
    wp_c8 *parameter;
    wp_script_class *klass;
    wp_s32 jump_patch;
    wp_s32 body_offset;
    wp_s32 i;

    class_name = wp_script_identifier( state );
    if( !class_name || !class_name[0] ||
        ( !wp_script_match( state, ":" ) && !wp_script_match( state, "." ) ) )
    {
        free( class_name );
        wp_script_parser_error( state, "expected Class:method after 'function'" );
        return;
    }
    method_name = wp_script_identifier( state );
    if( !method_name || !method_name[0] )
    {
        free( class_name );
        free( method_name );
        wp_script_parser_error( state, "expected script method name" );
        return;
    }

    state->compile_param_count = 0;
    if( wp_script_match( state, "(" ) )
    {
        if( !wp_script_match( state, ")" ) )
        {
            while( !state->had_error )
            {
                parameter = wp_script_identifier( state );
                if( !parameter || !parameter[0] )
                {
                    free( parameter );
                    wp_script_parser_error( state, "expected parameter name" );
                    break;
                }
                if( state->compile_param_count >= WP_SCRIPT_MAX_ARGS ||
                    strlen( parameter ) >= sizeof( state->compile_params[0] ) )
                {
                    free( parameter );
                    wp_script_parser_error( state, "too many or overly long parameters" );
                    break;
                }
                strcpy( state->compile_params[state->compile_param_count], parameter );
                state->compile_param_count++;
                free( parameter );
                if( wp_script_match( state, ")" ) )
                    break;
                if( !wp_script_match( state, "," ) )
                {
                    wp_script_parser_error( state, "expected ',' or ')' in parameter list" );
                    break;
                }
            }
        }
    }
    if( state->had_error )
    {
        free( class_name );
        free( method_name );
        return;
    }

    klass = wp_script_find_class( state, class_name );
    if( !klass )
    {
        free( class_name );
        free( method_name );
        wp_script_parser_error( state, "method refers to an unknown class" );
        return;
    }

    wp_script_emit( state, OP_JUMP );
    jump_patch = state->chunk.count;
    wp_script_emit_int( state, 0 );
    body_offset = state->chunk.count;
    while( !state->had_error )
    {
        wp_script_skip( state );
        if( !state->src[state->pos] )
        {
            wp_script_parser_error( state, "unterminated script function" );
            break;
        }
        if( wp_script_match_keyword( state, "end" ) )
            break;
        if( !wp_script_parse_statement( state ) )
            break;
    }

    if( !state->had_error )
    {
        wp_script_emit( state, OP_PUSH_NIL );
        wp_script_emit( state, OP_RETURN );
        wp_script_patch_int( state, jump_patch, state->chunk.count );
        if( !wp_script_table_set( &klass->methods, method_name,
                                  wp_script_make_script_method( body_offset ) ) )
            wp_script_parser_error( state, "script method table is full" );
    }

    for( i = 0; i < state->compile_param_count; ++i )
        state->compile_params[i][0] = '\0';
    state->compile_param_count = 0;
    free( class_name );
    free( method_name );
}

WP_SCRIPT_LIB void wp_script_parse_new( wp_script_state *state )
{
    wp_c8 *name;
    wp_s32 arg_count;

    name = wp_script_identifier( state );
    if( !name || !name[0] )
    {
        free( name );
        wp_script_parser_error( state, "expected class name after 'new'" );
        return;
    }
    wp_script_emit( state, OP_GET_GLOBAL );
    wp_script_emit_name( state, name );
    free( name );
    if( wp_script_parse_arguments( state, &arg_count ) )
    {
        wp_script_emit( state, OP_NEW_OBJECT );
        wp_script_emit( state, (wp_u8)arg_count );
    }
}

WP_SCRIPT_LIB void wp_script_parse_call( wp_script_state *state, const wp_c8 *object_name )
{
    wp_c8 *method;
    wp_s32 arg_count;

    if( !state || !object_name )
        return;
    method = wp_script_identifier( state );
    wp_script_emit( state, OP_GET_GLOBAL );
    wp_script_emit_name( state, object_name );
    if( method && method[0] && wp_script_parse_arguments( state, &arg_count ) )
    {
        wp_script_emit( state, OP_CALL_METHOD );
        wp_script_emit_name( state, method );
        wp_script_emit( state, (wp_u8)arg_count );
    }
    else
    {
        wp_script_parser_error( state, "expected method call" );
    }
    free( method );
}

WP_SCRIPT_LIB void wp_script_parse_assign( wp_script_state *state, const wp_c8 *name )
{
    if( !state || !name )
        return;
    if( !wp_script_parse_expression( state ) )
        return;
    wp_script_emit( state, OP_SET_GLOBAL );
    wp_script_emit_name( state, name );
}

WP_SCRIPT_LIB wp_s32 wp_script_get_compile_offset( wp_script_state *state )
{
    return state ? state->compile_offset : 0;
}

WP_SCRIPT_LIB wp_s32 wp_script_compile_ex( wp_script_state *state, const wp_c8 *code )
{
    if( !state || !code )
        return 0;
    wp_script_clear_error( state );
    state->src = code;
    state->pos = 0;
    state->compile_param_count = 0;
    return wp_script_lua_compile( state, code, &state->compile_offset );
}

WP_SCRIPT_LIB void wp_script_compile( wp_script_state *state, const wp_c8 *code )
{
    wp_script_compile_ex( state, code );
}
