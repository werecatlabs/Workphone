#ifndef __WPSystemBind_h__
#define __WPSystemBind_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"

namespace workphone
{
    void bindIO( lua_State *L );
    void bindSystem( lua_State *L );
} // namespace workphone

#endif // __WPSystemBind_h__
