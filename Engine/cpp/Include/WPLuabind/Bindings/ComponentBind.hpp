#ifndef ComponentBind_h__
#define ComponentBind_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"

namespace workphone
{
    void bindComponent( lua_State *L );
    void bindComponentUI( lua_State *L );

} // namespace workphone

#endif // ComponentBind_h__
