#ifndef WPLuaScriptError_h__
#define WPLuaScriptError_h__

#include "Workphone/WorkphoneTypes.hpp"

namespace fb
{
    class WPPythonScriptError
    {
    public:
        static void checkCode( s32 returnCode );
    };
}  // namespace fb

#ifndef _FINAL_
#    define PYTHON_SCRIPT_OBJ_CODE( x ) fb::WPPythonScriptError::checkCode( x )
#else
#    define PYTHON_SCRIPT_OBJ_CODE( x )
#endif

#endif  // WPLuaScriptError_h__
