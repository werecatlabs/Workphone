#ifndef UiUtilHelper_h__
#define UiUtilHelper_h__

#include <Workphone/Workphone.hpp>
#include <lua.hpp>
#include <luabind/object.hpp>

namespace workphone
{
    class UiUtilHelper
    {
    public:
        static String getHorizontalAlignmentString( lua_Integer alignment );
        static lua_Integer getHorizontalAlignment( const String &str );
        static String getVerticalAlignmentString( lua_Integer alignment );
        static lua_Integer getVerticalAlignment( const String &str );
        static void setAlignment( SmartPtr<scene::IGameActor> actor, lua_Integer horizontal,
                                  lua_Integer vertical );
        static SmartPtr<scene::IGameActor> createDropdownWithOptions( const String &label,
                                                                       luabind::object options,
                                                                       bool addToScene );
        static luabind::object stringTable( lua_State *L, const Array<String> &strings );
    };
}  // namespace workphone

#endif  // UiUtilHelper_h__
