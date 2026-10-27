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
#if defined(_WIN32)
#include <windows.h>
#endif

namespace workphone
{
    namespace
    {
        std::mutex projectVmMutex; SmartPtr<IScriptManager> projectVm; String projectVmSignature; SmartPtr<Properties> projectScriptProfile;
        SmartPtr<Properties> copyProfile(const Properties &source)
        {
            auto result=make_ptr<Properties>();result->setName(source.getName());
            for(const auto &property:source.getPropertiesAsArray())result->addProperty(property);
            for(auto child:source.getChildren())if(child)result->addChild(copyProfile(*child));
            return result;
        }
    }
    void setProjectScriptManager( SmartPtr<IScriptManager> manager )
    {
        SmartPtr<IScriptManager> retired;
        {std::lock_guard<std::mutex> lock(projectVmMutex);retired=projectVm;projectVm=manager;projectVmSignature.clear();projectScriptProfile=nullptr;}
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
        SmartPtr<IScriptManager> candidate;bool candidateCommitted=false;
        std::filesystem::path temporary;
        struct Cleanup
        {
            SmartPtr<IScriptManager> &manager;bool &committed;std::filesystem::path &path;
            ~Cleanup()
            {
                if(manager && !committed)try{manager->unload(nullptr);}catch(...){}
                std::error_code ignored;if(!path.empty())std::filesystem::remove(path,ignored);
            }
        } cleanup{candidate,candidateCommitted,temporary};
        try
        {
            auto app = core::IApplicationManager::instancePtr();
            if(!settings){error="Project scripting requires a profile";return false;}
            bool requested=false;
            for(const char *key:{"scripting.backend","scripting.lua","scripting.hotReload","scripting.reflection"})
                requested=requested || settings->hasProperty(key);
            if(!requested){error.clear();return true;}
            if(!app){error="Project scripting requires an application";return false;}
            SmartPtr<IScriptManager> previous;String previousSignature;auto profile=copyProfile(*settings);
            { std::lock_guard<std::mutex> lock( projectVmMutex ); previous = projectVm;previousSignature=projectVmSignature;
              if(projectScriptProfile)for(const auto &property:projectScriptProfile->getPropertiesAsArray())
                  if(!profile->hasProperty(property.getName()))profile->addProperty(property); }
            if(!profile->hasProperty("scripting.backend"))profile->setProperty("scripting.backend",s32(previous?3:0));
            String signature;
            for(const char *key:{"scripting.backend","scripting.lua","scripting.hotReload","scripting.reflection"})signature+=String(key)+"="+profile->getProperty(key)+";";
            const bool external = profile->getPropertyAsInt( "scripting.backend", 0 ) == 3;
            const bool switching=external!=static_cast<bool>(previous) || (external && signature!=previousSignature);
            if( external && switching )
            {
                auto factory = app->getFactoryManager();
                auto provider = factory ? factory->getFactoryByName( "ExternalProjectScriptManager" ) : nullptr;
                if( !provider || !provider->isObjectDerivedFromByInfo( IScriptManager::typeInfo() ) )
                { error = "ExternalProjectScriptManager must implement IScriptManager"; return false; }
                candidate=factory->createObjectFromType<IScriptManager>("ExternalProjectScriptManager");
                if(!candidate){error="Install ExternalProjectScriptManager provider plugin";return false;}
                candidate->setProperties(profile);candidate->load(nullptr);
                if(!candidate->isLoaded()){error="External script provider failed to load";return false;}
            }
            if( profile->getPropertyAsInt( "scripting.reflection", 1 ) >= 2 )
            {
                auto registry = make_ptr<Properties>();
                auto scripts=external?(candidate?candidate:previous):app->getScriptManager();
                if(scripts)registry->setProperty("classNames",scripts->getRuntimeClassNames());
                const auto path = std::filesystem::u8path( directory.c_str() ) / ".workphone/generated/ScriptReflection.settings";
                std::filesystem::create_directories( path.parent_path() );
                temporary=path;temporary+=std::string(("."+StringUtil::getUUID()+".tmp").c_str());
                std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                stream.exceptions( std::ios::badbit | std::ios::failbit );
                stream << DataUtil::toString( registry.get(), true ); stream.close();
#if defined(_WIN32)
                if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
                    throw std::runtime_error("Unable to publish generated script reflection registry");
#else
                std::filesystem::rename(temporary,path);
#endif
            }
            if(switching)
            {
                if(auto scenes=app->getGameManager())for(auto actor:scenes->getActors())
                    for(auto script:actor->getComponentsByType<scene::Script>())script->resetProjectScriptInstance();
                {std::lock_guard<std::mutex> lock(projectVmMutex);projectVm=candidate;projectVmSignature=signature;}
                candidateCommitted=true;
                if(previous)try{previous->unload(nullptr);}catch(const std::exception &e){WP_LOG_ERROR(String("Previous project VM unload failed: ")+e.what());}
            }
            {std::lock_guard<std::mutex> lock(projectVmMutex);projectScriptProfile=profile;}
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void releaseProjectScriptManager()
    {
        SmartPtr<IScriptManager> previous;
        { std::lock_guard<std::mutex> lock( projectVmMutex ); previous = projectVm; projectVm = nullptr;projectVmSignature.clear();projectScriptProfile=nullptr; }
        if( previous ) previous->unload( nullptr );
    }
}
