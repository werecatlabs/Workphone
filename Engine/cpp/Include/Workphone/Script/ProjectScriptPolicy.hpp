#pragma once
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    WPCore_API void setProjectScriptManager( SmartPtr<IScriptManager> manager );
    WPCore_API SmartPtr<IScriptManager> getProjectScriptManager( ISharedObject *owner = nullptr );
    WPCore_API bool applyProjectScriptSettings( SmartPtr<Properties> settings,
                                               const String &projectDirectory, String &error );
    WPCore_API void releaseProjectScriptManager();
    /** The Editor's own Lua windows remain active when project gameplay scripts are disabled. */
    inline bool projectScriptExecutionEnabled()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto settings = app ? app->getPlayerSettings() : nullptr;
        if( !settings ) return true;
        const auto backend = settings->getPropertyAsInt( "scripting.backend", 0 );
        return backend == 3 || ( backend != 1 && settings->getPropertyAsBool( "scripting.lua", true ) );
    }
}
