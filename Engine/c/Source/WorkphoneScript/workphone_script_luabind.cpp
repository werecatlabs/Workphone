extern "C"
{
#include "workphone_script_luabind.h"
#include "workphone_script_method.h"
#include "workphone_script_object.h"
#include "workphone_script_table.h"
#include "workphone_script_value.h"
}

/* workphone_types.h supplies the C89 form of NULL.  Do not leak that
 * void-pointer macro into the C++ standard library or Luabind headers. */
#ifdef NULL
#undef NULL
#endif

#include <lua.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/luabind.hpp>

#include <cstdlib>
#include <cstring>
#include <exception>

namespace
{
const char *c_text( const wp_c8 *text )
{
    return reinterpret_cast<const char *>( text );
}

const wp_c8 *wp_text( const char *text )
{
    return reinterpret_cast<const wp_c8 *>( text );
}

void script_error( wp_script_state *state, const char *message )
{
    wp_script_set_error( state, wp_text( message ) );
}

lua_State *lua_state( const wp_script_state *state )
{
    return state ? static_cast<lua_State *>( state->lua_state ) : 0;
}

void set_lua_error( wp_script_state *state, lua_State *L, const char *fallback )
{
    const char *message = lua_isstring( L, -1 ) ? lua_tostring( L, -1 ) : fallback;
    script_error( state, message ? message : "unknown Lua error" );
    lua_pop( L, 1 );
}

luabind::detail::class_rep *class_at( lua_State *L, int index )
{
    index = lua_absindex( L, index );
    if( !luabind::detail::is_class_rep( L, index ) )
        return 0;
    return static_cast<luabind::detail::class_rep *>( lua_touserdata( L, index ) );
}

luabind::detail::object_rep *instance_at( lua_State *L, int index )
{
    index = lua_absindex( L, index );
    return luabind::detail::get_instance( L, index );
}

wp_script_class *sync_class( wp_script_state *state,
                             luabind::detail::class_rep *lua_class )
{
    wp_script_class *klass;
    wp_script_class *parent;

    if( !state || !lua_class )
        return 0;

    klass = wp_script_find_class( state, wp_text( lua_class->name() ) );
    parent = 0;
    if( !lua_class->bases().empty() )
        parent = sync_class( state, lua_class->bases()[0].base );

    if( !klass )
    {
        klass = wp_script_new_class( state, wp_text( lua_class->name() ) );
        if( !klass )
            return 0;
    }
    if( parent && !klass->parent )
        wp_script_class_set_parent( klass, parent->name );
    return klass;
}

wp_script_class *native_ancestor( wp_script_class *klass )
{
    wp_s32 depth = 0;
    while( klass && !klass->native_create && depth < TABLE_SIZE )
    {
        klass = klass->parent;
        ++depth;
    }
    return depth < TABLE_SIZE ? klass : 0;
}

wp_script_object *allocate_object( wp_script_state *state, wp_script_class *klass,
                                   void *identity, lua_State *L, int index )
{
    wp_script_object *object;
    wp_script_class *native_class;

    object = static_cast<wp_script_object *>( std::calloc( 1, sizeof( wp_script_object ) ) );
    if( !object )
    {
        script_error( state, "unable to allocate Lua object bridge" );
        return 0;
    }

    object->klass = klass;
    object->lua_identity = identity;
    object->lua_ref = LUA_NOREF;
    object->next = state->objects;
    state->objects = object;

    if( L && index != 0 )
    {
        lua_pushvalue( L, lua_absindex( L, index ) );
        object->lua_ref = luaL_ref( L, LUA_REGISTRYINDEX );
    }

    native_class = native_ancestor( klass );
    if( native_class )
    {
        object->native_destroy = native_class->native_destroy;
        object->native_user_data = native_class->native_user_data;
        object->native_object = native_class->native_create(
            state, object, native_class->native_user_data );
    }
    return object;
}

wp_script_object *ensure_object( wp_script_state *state, lua_State *L, int index )
{
    luabind::detail::object_rep *lua_object;
    wp_script_object *object;
    wp_script_class *klass;

    lua_object = instance_at( L, index );
    if( !lua_object )
    {
        script_error( state, "Lua method receiver is not a Luabind object" );
        return 0;
    }

    object = state->objects;
    while( object )
    {
        if( object->lua_identity == lua_object )
            return object;
        object = object->next;
    }

    klass = sync_class( state, lua_object->crep() );
    if( !klass )
        return 0;
    return allocate_object( state, klass, lua_object, L, index );
}

wp_script_value value_from_lua( wp_script_state *state, lua_State *L, int index )
{
    luabind::detail::class_rep *lua_class;
    luabind::detail::object_rep *lua_object;
    wp_script_object *object;

    switch( lua_type( L, index ) )
    {
    case LUA_TNUMBER:
        return wp_script_make_number( static_cast<wp_f64>( lua_tonumber( L, index ) ) );
    case LUA_TSTRING:
        return wp_script_make_string( wp_text( lua_tostring( L, index ) ) );
    case LUA_TBOOLEAN:
        return wp_script_make_number( lua_toboolean( L, index ) ? 1.0 : 0.0 );
    case LUA_TLIGHTUSERDATA:
    {
        wp_script_value value = wp_script_make_nil();
        value.type = VAL_NATIVE;
        value.as.pointer = lua_touserdata( L, index );
        return value;
    }
    case LUA_TUSERDATA:
        lua_class = class_at( L, index );
        if( lua_class )
            return wp_script_make_class( sync_class( state, lua_class ) );
        lua_object = instance_at( L, index );
        if( lua_object )
        {
            object = ensure_object( state, L, index );
            return object ? wp_script_make_object( object ) : wp_script_make_nil();
        }
        break;
    default:
        break;
    }
    return wp_script_make_nil();
}

void push_value( lua_State *L, const wp_script_value &value )
{
    switch( value.type )
    {
    case VAL_NUMBER:
        lua_pushnumber( L, static_cast<lua_Number>( value.as.number ) );
        break;
    case VAL_STRING:
        lua_pushstring( L, value.as.string ? c_text( value.as.string ) : "" );
        break;
    case VAL_OBJECT:
        if( value.as.object && value.as.object->lua_ref != LUA_NOREF )
            lua_rawgeti( L, LUA_REGISTRYINDEX, value.as.object->lua_ref );
        else
            lua_pushnil( L );
        break;
    case VAL_CLASS:
        if( value.as.klass && value.as.klass->name )
            lua_getglobal( L, c_text( value.as.klass->name ) );
        else
            lua_pushnil( L );
        break;
    case VAL_NATIVE:
        lua_pushlightuserdata( L, value.as.pointer );
        break;
    default:
        lua_pushnil( L );
        break;
    }
}

void sync_globals( wp_script_state *state )
{
    lua_State *L = lua_state( state );
    const char *name;
    wp_script_value value;

    lua_pushglobaltable( L );
    lua_pushnil( L );
    while( lua_next( L, -2 ) != 0 )
    {
        if( lua_type( L, -2 ) == LUA_TSTRING && lua_type( L, -1 ) == LUA_TUSERDATA )
        {
            name = lua_tostring( L, -2 );
            if( class_at( L, -1 ) || instance_at( L, -1 ) )
            {
                value = value_from_lua( state, L, -1 );
                if( value.type != VAL_NIL )
                    wp_script_table_set( &state->globals, wp_text( name ), value );
            }
        }
        lua_pop( L, 1 );
    }
    lua_pop( L, 1 );
}

int native_constructor( lua_State *L )
{
    wp_script_state *state = static_cast<wp_script_state *>(
        lua_touserdata( L, lua_upvalueindex( 1 ) ) );

    wp_script_clear_error( state );
    if( !ensure_object( state, L, 1 ) || state->had_error )
    {
        lua_pushstring( L, c_text( wp_script_get_last_error( state ) ) );
        return lua_error( L );
    }
    return 0;
}

int native_method( lua_State *L )
{
    wp_script_state *state = static_cast<wp_script_state *>(
        lua_touserdata( L, lua_upvalueindex( 1 ) ) );
    wp_script_class *owner = static_cast<wp_script_class *>(
        lua_touserdata( L, lua_upvalueindex( 2 ) ) );
    const char *name = lua_tostring( L, lua_upvalueindex( 3 ) );
    wp_script_value method;
    wp_script_value args[WP_SCRIPT_MAX_ARGS];
    wp_script_value result;
    wp_script_object *object;
    wp_script_object *saved_object;
    wp_script_class *saved_class;
    const wp_script_value *saved_args;
    wp_s32 saved_arg_count;
    wp_s32 arg_count;
    wp_s32 i;
    wp_s32 ok;

    wp_script_clear_error( state );
    arg_count = static_cast<wp_s32>( lua_gettop( L ) - 1 );
    if( arg_count < 0 || arg_count > WP_SCRIPT_MAX_ARGS )
    {
        lua_pushstring( L, "too many arguments for C89 script callback" );
        return lua_error( L );
    }
    if( state->call_depth >= WP_SCRIPT_MAX_CALL_DEPTH )
    {
        lua_pushstring( L, "maximum C89 script callback depth exceeded" );
        return lua_error( L );
    }
    object = ensure_object( state, L, 1 );
    method = wp_script_table_get( &owner->methods, wp_text( name ) );
    if( !object || method.type != VAL_NATIVE_FUNC || !method.native_method )
    {
        lua_pushstring( L, state->had_error ? c_text( wp_script_get_last_error( state ) )
                                            : "invalid C89 script method binding" );
        return lua_error( L );
    }

    for( i = 0; i < arg_count; ++i )
        args[i] = value_from_lua( state, L, i + 2 );
    result = wp_script_make_nil();
    saved_object = state->current_object;
    saved_class = state->current_class;
    saved_args = state->current_args;
    saved_arg_count = state->current_arg_count;
    state->current_object = object;
    state->current_class = owner;
    state->current_args = arg_count ? args : 0;
    state->current_arg_count = arg_count;
    ++state->call_depth;
    ok = method.native_method( state, object, args, arg_count, &result,
                               method.native_user_data );
    --state->call_depth;
    state->current_object = saved_object;
    state->current_class = saved_class;
    state->current_args = saved_args;
    state->current_arg_count = saved_arg_count;

    if( !ok || state->had_error )
    {
        lua_pushstring( L, state->had_error ? c_text( wp_script_get_last_error( state ) )
                                            : "C89 script method failed" );
        return lua_error( L );
    }
    push_value( L, result );
    return 1;
}

bool create_lua_class( wp_script_class *klass )
{
    wp_script_state *state = klass->state;
    lua_State *L = lua_state( state );
    const char *parent_name = c_text( klass->parent_name );

    lua_getglobal( L, c_text( klass->name ) );
    if( class_at( L, -1 ) )
    {
        lua_pop( L, 1 );
        return true;
    }
    lua_pop( L, 1 );

    lua_getglobal( L, "class" );
    lua_pushstring( L, c_text( klass->name ) );
    if( lua_pcall( L, 1, 1, 0 ) != LUA_OK )
    {
        set_lua_error( state, L, "unable to create Luabind class" );
        return false;
    }

    if( parent_name && parent_name[0] )
    {
        lua_getglobal( L, parent_name );
        if( !class_at( L, -1 ) )
        {
            lua_pop( L, 2 );
            script_error( state, "Luabind parent class is not registered" );
            return false;
        }
        if( lua_pcall( L, 1, 0, 0 ) != LUA_OK )
        {
            set_lua_error( state, L, "unable to derive Luabind class" );
            return false;
        }
    }
    else
    {
        lua_pop( L, 1 );
    }
    return true;
}
} // namespace

extern "C" wp_s32 wp_script_lua_initialize( wp_script_state *state )
{
    lua_State *L;
    wp_script_class *object_class;

    if( !state )
        return 0;
    L = luaL_newstate();
    if( !L )
    {
        script_error( state, "unable to create Lua state" );
        return 0;
    }
    state->lua_state = L;
    luaL_openlibs( L );
    try
    {
        luabind::open( L );
    }
    catch( const std::exception &error )
    {
        script_error( state, error.what() );
        lua_close( L );
        state->lua_state = 0;
        return 0;
    }

    object_class = wp_script_new_class( state, wp_text( "Object" ) );
    if( !object_class || !wp_script_lua_bind_class( object_class ) )
    {
        lua_close( L );
        state->lua_state = 0;
        return 0;
    }
    return 1;
}

extern "C" void wp_script_lua_shutdown( wp_script_state *state )
{
    lua_State *L = lua_state( state );
    if( L )
        lua_close( L );
    if( state )
        state->lua_state = 0;
}

extern "C" wp_s32 wp_script_lua_compile( wp_script_state *state, const wp_c8 *code,
                                          wp_s32 *chunk_ref )
{
    lua_State *L = lua_state( state );
    int reference;

    if( !L || !code || !chunk_ref )
        return 0;
    if( luaL_loadbuffer( L, c_text( code ), std::strlen( c_text( code ) ),
                         "WorkphoneScript" ) != LUA_OK )
    {
        set_lua_error( state, L, "unable to compile Lua source" );
        return 0;
    }
    reference = luaL_ref( L, LUA_REGISTRYINDEX );
    if( reference == LUA_NOREF || reference == LUA_REFNIL )
    {
        script_error( state, "unable to retain compiled Lua chunk" );
        return 0;
    }
    *chunk_ref = static_cast<wp_s32>( reference );
    return 1;
}

extern "C" wp_s32 wp_script_lua_run( wp_script_state *state, wp_s32 chunk_ref,
                                      wp_script_value *result )
{
    lua_State *L = lua_state( state );

    if( result )
        *result = wp_script_make_nil();
    if( !L || chunk_ref < 0 )
        return 0;
    wp_script_clear_error( state );
    lua_rawgeti( L, LUA_REGISTRYINDEX, chunk_ref );
    if( !lua_isfunction( L, -1 ) )
    {
        lua_pop( L, 1 );
        script_error( state, "invalid compiled Lua chunk" );
        return 0;
    }
    if( lua_pcall( L, 0, 1, 0 ) != LUA_OK )
    {
        set_lua_error( state, L, "unable to execute Lua chunk" );
        return 0;
    }
    if( result )
        *result = value_from_lua( state, L, -1 );
    lua_pop( L, 1 );
    if( state->had_error )
        return 0;
    sync_globals( state );
    return !state->had_error;
}

extern "C" wp_s32 wp_script_lua_bind_class( wp_script_class *klass )
{
    lua_State *L;

    if( !klass || !klass->state )
        return 0;
    L = lua_state( klass->state );
    if( !L || !create_lua_class( klass ) )
        return 0;

    lua_getglobal( L, c_text( klass->name ) );
    lua_pushlightuserdata( L, klass->state );
    lua_pushcclosure( L, native_constructor, 1 );
    lua_setfield( L, -2, "__init" );
    lua_pop( L, 1 );
    return 1;
}

extern "C" wp_s32 wp_script_lua_bind_method( wp_script_class *klass,
                                               const wp_c8 *name )
{
    lua_State *L;

    if( !klass || !klass->state || !name )
        return 0;
    L = lua_state( klass->state );
    lua_getglobal( L, c_text( klass->name ) );
    if( !class_at( L, -1 ) )
    {
        lua_pop( L, 1 );
        script_error( klass->state, "Luabind class is not registered" );
        return 0;
    }
    lua_pushlightuserdata( L, klass->state );
    lua_pushlightuserdata( L, klass );
    lua_pushstring( L, c_text( name ) );
    lua_pushcclosure( L, native_method, 3 );
    lua_setfield( L, -2, c_text( name ) );
    lua_pop( L, 1 );
    return 1;
}

extern "C" wp_script_object *wp_script_lua_new_object( wp_script_state *state,
                                                        wp_script_class *klass )
{
    lua_State *L = lua_state( state );
    wp_script_object *object;

    if( !L || !klass || !klass->name )
        return 0;
    wp_script_clear_error( state );
    lua_getglobal( L, c_text( klass->name ) );
    if( !class_at( L, -1 ) )
    {
        lua_pop( L, 1 );
        script_error( state, "cannot instantiate an unregistered Luabind class" );
        return 0;
    }
    if( lua_pcall( L, 0, 1, 0 ) != LUA_OK )
    {
        set_lua_error( state, L, "unable to construct Luabind object" );
        return 0;
    }
    object = ensure_object( state, L, -1 );
    lua_pop( L, 1 );
    return state->had_error ? 0 : object;
}

extern "C" wp_s32 wp_script_lua_call_method( wp_script_object *object,
                                              const wp_c8 *name,
                                              const wp_script_value *args,
                                              wp_s32 arg_count,
                                              wp_script_value *result )
{
    wp_script_state *state;
    lua_State *L;
    wp_s32 i;

    if( result )
        *result = wp_script_make_nil();
    if( !object || !object->klass || !name || arg_count < 0 ||
        arg_count > WP_SCRIPT_MAX_ARGS || ( arg_count && !args ) )
        return 0;
    state = object->klass->state;
    L = lua_state( state );
    if( !L || object->lua_ref == LUA_NOREF )
        return 0;
    wp_script_clear_error( state );

    lua_rawgeti( L, LUA_REGISTRYINDEX, object->lua_ref );
    lua_getfield( L, -1, c_text( name ) );
    if( !lua_isfunction( L, -1 ) )
    {
        lua_pop( L, 2 );
        return 0;
    }
    lua_insert( L, -2 );
    for( i = 0; i < arg_count; ++i )
        push_value( L, args[i] );
    if( lua_pcall( L, arg_count + 1, 1, 0 ) != LUA_OK )
    {
        set_lua_error( state, L, "Lua method call failed" );
        return 0;
    }
    if( result )
        *result = value_from_lua( state, L, -1 );
    lua_pop( L, 1 );
    return !state->had_error;
}

extern "C" wp_s32 wp_script_lua_has_method( const wp_script_object *object,
                                             const wp_c8 *name )
{
    lua_State *L;
    wp_s32 found;

    if( !object || !object->klass || !name || object->lua_ref == LUA_NOREF )
        return 0;
    L = lua_state( object->klass->state );
    if( !L )
        return 0;
    lua_rawgeti( L, LUA_REGISTRYINDEX, object->lua_ref );
    lua_getfield( L, -1, c_text( name ) );
    found = lua_isfunction( L, -1 ) ? 1 : 0;
    lua_pop( L, 2 );
    return found;
}
