#ifndef WP_LUA_COMPAT_HPP
#define WP_LUA_COMPAT_HPP
#include <cstddef>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <cmath>
#include <limits>

// Use distinct names: the bundled Luabind supplies and then undefines several
// compatibility macros in its public headers.
inline void wpLuaPushGlobalTable( lua_State *state )
{
#if LUA_VERSION_NUM < 502
    lua_pushvalue( state, LUA_GLOBALSINDEX );
#else
    lua_pushglobaltable( state );
#endif
}
inline void wpLuaGetUserValue( lua_State *state, int index )
{
#if LUA_VERSION_NUM < 502
    lua_getfenv( state, index );
#else
    lua_getuservalue( state, index );
#endif
}
inline int wpLuaAbsIndex( lua_State *state, int index )
{
    return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop( state ) + index + 1;
}
inline void wpLuaRequire( lua_State *state, const char *name, lua_CFunction open )
{
#if LUA_VERSION_NUM < 502
    lua_getfield( state, LUA_REGISTRYINDEX, "_LOADED" );
    lua_getfield( state, -1, name );
    if( !lua_toboolean( state, -1 ) )
    {
        lua_pop( state, 1 );
        lua_pushcfunction( state, open );
        lua_pushstring( state, name );
        lua_call( state, 1, 1 );
        lua_pushvalue( state, -1 );
        lua_setfield( state, -3, name );
    }
    lua_remove( state, -2 );
    lua_pushvalue( state, -1 );
    lua_setglobal( state, name );
#else
    luaL_requiref( state, name, open, 1 );
#endif
}
#ifdef _MSC_VER
#pragma float_control(precise, on, push)
#endif
inline lua_Integer wpLuaToIntegerExact( lua_State *state, int index, int *success )
{
#if LUA_VERSION_NUM < 503
    const auto number = lua_tonumber( state, index );
    // LuaJIT's lua_tointegerx truncates fractions, unlike modern Lua. Test the
    // floating value before conversion, including the exclusive upper bound.
    const auto upper = std::ldexp( lua_Number( 1 ), std::numeric_limits<lua_Integer>::digits );
    *success = lua_isnumber( state, index ) && std::isfinite( number ) &&
               std::floor( number ) == number && number >= -upper && number < upper;
    return *success ? static_cast<lua_Integer>( number ) : 0;
#else
    return lua_tointegerx( state, index, success );
#endif
}
#ifdef _MSC_VER
#pragma float_control(pop)
#endif
#endif
