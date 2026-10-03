#ifndef ref_h__
#define ref_h__

#include <luabind/detail/ref.hpp>
#include <luabind/detail/stack_utils.hpp>
#include "luabind/detail/object.hpp"

namespace luabind
{
    namespace detail
    {

        int ref( lua_State *L, const object &obj )
        {
            obj.push( L );
            return luaL_ref( L, LUA_REGISTRYINDEX );
        }

        void unref( lua_State *L, int ref )
        {
            luaL_unref( L, LUA_REGISTRYINDEX, ref );
        }

    }  // namespace detail
}  // namespace luabind

#endif  // ref_h__
