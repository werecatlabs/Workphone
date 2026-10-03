#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Helpers/ScriptObjectHelper.hpp>

namespace fb
{

    void ScriptObjectHelper::setObject( SmartPtr<ISharedObject> obj, const String &objectName,
                                        SmartPtr<ISharedObject> scriptObj )
    {
        u32 id = StringUtil::getHash( objectName );
        setObjectFromHash( obj, id, scriptObj );
    }

    void ScriptObjectHelper::setObjectFromHash( SmartPtr<ISharedObject> obj, python_Integer hash,
                                                SmartPtr<ISharedObject> scriptObj )
    {
        //obj->setObject( hash, scriptObj );
    }

}  // namespace fb
