#ifndef SceneBind_h__
#define SceneBind_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"

namespace workphone
{
    void bindScene( lua_State *L );
    void bindSceneManager( lua_State *L );
} // namespace workphone

#endif // SceneBind_h__
