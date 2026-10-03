#ifndef SmartPtrConverter_h__
#define SmartPtrConverter_h__

#include <WPLuaBind/ScriptObjectUtil.hpp>
#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/detail/convert_to_lua.hpp>
#include <typeinfo>

namespace luabind
{
    template <typename B>
    struct default_converter<workphone::SmartPtr<B>> : native_converter_base<workphone::SmartPtr<B>>
    {
        static int compute_score( lua_State *L, int index );

        workphone::SmartPtr<B> from( lua_State *L, int index );

        void to( lua_State *L, const workphone::SmartPtr<B> &ptr );
    };

    template <typename B>
    int luabind::default_converter<workphone::SmartPtr<B>>::compute_score( lua_State *L, int index )
    {
        auto type = lua_type( L, index );
        return type == LUA_TUSERDATA ? 0 : -1;
    }

    template <typename B>
    workphone::SmartPtr<B> luabind::default_converter<workphone::SmartPtr<B>>::from( lua_State *L,
                                                                                     int        index )
    {
        if( lua_isnil( L, index ) )
        {
            return nullptr;
        }

        auto obj = detail::get_instance( L, index );
        if( !obj )
        {
            return nullptr;
        }

        auto s = obj->get_instance( detail::registered_class<B>::id );
        if( s.first )
        {
            return static_cast<B *>( s.first );
        }

        auto p = obj->get_instance_by_type<B>();
        if( p )
        {
            return p;
        }

        return nullptr;
    }

    template <typename B>
    void luabind::default_converter<workphone::SmartPtr<B>>::to( lua_State                    *L,
                                                                 const workphone::SmartPtr<B> &ptr )
    {
        if( !ptr )
        {
            lua_pushnil( L );
            return;
        }

        auto scriptObj = static_cast<workphone::ISharedObject *>( ptr.get() );
        bool hasCast = workphone::ScriptObjectUtil::toLua( L, scriptObj );
        if( !hasCast )
        {
            detail::convert_to_lua( L, static_cast<B *>( scriptObj ) );
        }
    }

    template <typename B>
    struct default_converter<const workphone::SmartPtr<B> &> : default_converter<workphone::SmartPtr<B>>
    {
    };
} // namespace luabind

#endif // SmartPtrConverter_h__
