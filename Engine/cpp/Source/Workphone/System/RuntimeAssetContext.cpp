#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>

namespace workphone::resource
{
    bool prepareRuntimeAssetScripts(const std::shared_ptr<RuntimeAssetContext> &context,String &error)
    {
        error.clear();
        if(!context||!context->resolve){error="Runtime assets require a pinned closure";return false;}
        if(!context->requiresScripts)return true;
        if(context->scripts)return context->scripts->isLoaded();
        auto app=core::IApplicationManager::instancePtr();
        auto prototype=app?app->getScriptManager():nullptr;
        if(!prototype){error="Runtime Script components require an installed script backend";return false;}
        auto candidate=prototype->createRuntimeInstance(context->resolve,error);
        if(!candidate||!candidate->isLoaded())return false;
        context->scripts=std::move(candidate);return true;
    }
}
