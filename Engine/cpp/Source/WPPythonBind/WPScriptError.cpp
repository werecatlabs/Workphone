#include "WPPythonBind/WPPythonBindPCH.hpp"
#include "WPPythonBind/WPScriptError.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{

    //---------------------------------------------------------------------------------------------------
    String getDebugStr()
    {
        String debugStr;

        //IApplicationManager *engine = Engine::getSingletonPtr();
        //ScriptManagerPtr scriptMgr = engine->getScriptManager();

        //if( scriptMgr )
        //    debugStr += scriptMgr->getDebugInfo();

        return debugStr;
    }

}  // end namespace fb
