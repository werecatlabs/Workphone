#ifndef ArrayFunctions_h__
#define ArrayFunctions_h__

#include "WPLuabind/WPLuabindTypes.hpp"
#include "WPLuaScriptError.hpp"

namespace workphone
{
    template <class T>
    class ArrayFunctions
    {
    public:
        static T &get( Array<T> &params, lua_Integer index )
        {
            return params[index];
        }
    };
} // namespace workphone

#endif // ArrayFunctions_h__
