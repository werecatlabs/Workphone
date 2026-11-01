#pragma once
#include <Workphone/System/ResourceLeafProvider.hpp>

namespace workphone::resource
{
    /** Combines installed native family loaders with recipe-declared high LOD
     * mesh metadata. No source/catalog/compiler access is permitted. */
    WPCore_API std::shared_ptr<ResourceLeafProviderFactory> createRuntimeSceneLeafFactory(
        std::shared_ptr<ResourcePayloadMount>,String &error);
}
