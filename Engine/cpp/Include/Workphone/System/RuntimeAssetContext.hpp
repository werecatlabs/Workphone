#pragma once
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>

namespace workphone::resource
{
    using RuntimeResourceResolver = std::function<std::shared_ptr<const RuntimeResource>(const String &,String &)>;
    /** A consumer's immutable pinned closure. No catalog or source fallback.
     * Resolver captures must not retain this context (the private VM may retain
     * the resolver); capture detached UUID/resource maps instead. */
    struct RuntimeAssetContext
    {
        String target="pc";
        bool packagedBuild=true;
        RuntimeResourceResolver resolve;
        Properties::ObjectResolver objects;
        SmartPtr<IScriptManager> scripts;
        bool requiresScripts=false;
    };
    WPCore_API bool prepareRuntimeAssetScripts(const std::shared_ptr<RuntimeAssetContext> &,String &error);
}
