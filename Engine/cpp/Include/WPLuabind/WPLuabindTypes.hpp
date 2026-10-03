#ifndef WPLuabindTypes_h__
#define WPLuabindTypes_h__

#include <Workphone/WorkphoneTypes.hpp>

// Some helpful macros for defining constants  (sort of) in Lua. Similar to this code:
// object g = globals(L);
// object table = g["class"];
// table["constant"] = class::constant;
#define LUA_CONST_START( class ) \
    {                            \
        object g = globals( L ); \
        object table = g[#class];
#define LUA_CONST( class, name ) table[#name] = class ::name
#define LUA_CONST_END }

#endif // WPLuabindTypes_h__
