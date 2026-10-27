#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ProjectScriptPolicy.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <mutex>

namespace workphone
{
    namespace { std::mutex projectVmMutex; SmartPtr<IScriptManager> projectVm; }
    void setProjectScriptManager( SmartPtr<IScriptManager> manager )
    {
        std::lock_guard<std::mutex> lock( projectVmMutex ); projectVm = manager;
    }
    SmartPtr<IScriptManager> getProjectScriptManager( ISharedObject *owner )
    {
        if( !owner || dynamic_cast<scene::Script *>( owner ) )
        {
            std::lock_guard<std::mutex> lock( projectVmMutex );
            if( projectVm ) return projectVm;
        }
        auto app = core::IApplicationManager::instancePtr();
        return app ? app->getScriptManager() : nullptr;
    }
}
