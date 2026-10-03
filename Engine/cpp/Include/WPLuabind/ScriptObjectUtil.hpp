#ifndef ScriptObjectUtil_h__
#define ScriptObjectUtil_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

struct lua_State;

namespace workphone
{
    class ScriptObjectUtil
    {
    public:
        static bool toLua( lua_State *L, ISharedObject *scriptObj );
    };

    template <class T>
    class CastUtil
    {
    public:
        static void down_cast( lua_State *L, const SmartPtr<T> &ptr );
    };
} // namespace workphone

#endif // ScriptObjectUtil_h__
