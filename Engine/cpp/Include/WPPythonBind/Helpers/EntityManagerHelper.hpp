#ifndef EntityManagerHelper_h__
#define EntityManagerHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    class EntityManagerHelper
    {
    public:
        static void addEntity( SmartPtr<scene::ISceneManager> entityMgr,
                               SmartPtr<ISharedObject> entity );
    };

}  // end namespace fb

#endif  // EntityManagerHelper_h__
