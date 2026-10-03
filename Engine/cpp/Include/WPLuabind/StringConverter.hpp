#ifndef StringConverter_h__
#define StringConverter_h__

#include <Workphone/Core/StringBase.hpp>
#include <luabind/luabind.hpp>

namespace luabind
{
    template <class Traits, class Allocator>
    struct default_converter<workphone::StringBase<char, Traits, Allocator>>
        : native_converter_base<workphone::StringBase<char, Traits, Allocator>>
    {
        using string_type = workphone::StringBase<char, Traits, Allocator>;

        static int compute_score( lua_State *L, int index )
        {
            return lua_type( L, index ) == LUA_TSTRING ? 0 : -1;
        }

        string_type from( lua_State *L, int index )
        {
            std::size_t length = 0;
            const auto *value = lua_tolstring( L, index, &length );
            return string_type( value, length );
        }

        void to( lua_State *L, const string_type &value )
        {
            lua_pushlstring( L, value.empty() ? "" : value.data(), value.size() );
        }
    };

    template <class Traits, class Allocator>
    struct default_converter<const workphone::StringBase<char, Traits, Allocator>>
        : default_converter<workphone::StringBase<char, Traits, Allocator>>
    {
    };

    template <class Traits, class Allocator>
    struct default_converter<const workphone::StringBase<char, Traits, Allocator> &>
        : default_converter<workphone::StringBase<char, Traits, Allocator>>
    {
    };
} // namespace luabind

#endif // StringConverter_h__
