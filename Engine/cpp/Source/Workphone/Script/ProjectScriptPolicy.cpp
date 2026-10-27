#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ProjectScriptPolicy.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <filesystem>
#include <fstream>
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

    bool applyProjectScriptSettings( SmartPtr<Properties> settings, const String &directory, String &error )
    {
        try
        {
            auto app = core::IApplicationManager::instancePtr();
            if( !settings || !app ) { error = "Project scripting requires a profile and application"; return false; }
            SmartPtr<IScriptManager> previous;
            { std::lock_guard<std::mutex> lock( projectVmMutex ); previous = projectVm; }
            const bool external = settings->getPropertyAsInt( "scripting.backend", 0 ) == 3;
            if( external != static_cast<bool>( previous ) )
            {
                SmartPtr<IScriptManager> next;
                if( external )
                {
                    auto factory = app->getFactoryManager();
                    auto provider = factory ? factory->getFactoryByName( "ExternalProjectScriptManager" ) : nullptr;
                    if( !provider || !provider->isObjectDerivedFromByInfo( IScriptManager::typeInfo() ) )
                    { error = "ExternalProjectScriptManager must implement IScriptManager"; return false; }
                    next = factory ? factory->createObjectFromType<IScriptManager>( "ExternalProjectScriptManager" ) : nullptr;
                    if( !next ) { error = "Install ExternalProjectScriptManager provider plugin"; return false; }
                    next->load( nullptr );
                    if( !next->isLoaded() ) { error = "External script provider failed to load"; return false; }
                }
                if( auto scenes = app->getGameManager() )
                    for( auto actor : scenes->getActors() )
                        for( auto script : actor->getComponentsByType<scene::Script>() ) script->resetProjectScriptInstance();
                setProjectScriptManager( next );
                if( previous ) previous->unload( nullptr );
            }
            if( settings->getPropertyAsInt( "scripting.reflection", 1 ) >= 2 )
            {
                auto registry = make_ptr<Properties>();
                if( auto scripts = getProjectScriptManager() ) registry->setProperty( "classNames", scripts->getRuntimeClassNames() );
                const auto path = std::filesystem::u8path( directory.c_str() ) / ".workphone/generated/ScriptReflection.settings";
                std::filesystem::create_directories( path.parent_path() );
                std::ofstream stream( path, std::ios::binary | std::ios::trunc );
                stream.exceptions( std::ios::badbit | std::ios::failbit );
                stream << DataUtil::toString( registry.get(), true ); stream.close();
            }
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void releaseProjectScriptManager()
    {
        SmartPtr<IScriptManager> previous;
        { std::lock_guard<std::mutex> lock( projectVmMutex ); previous = projectVm; projectVm = nullptr; }
        if( previous ) previous->unload( nullptr );
    }
}
