#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IShaderManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IShaderManager, ISharedObject );

    IShaderManager::~IShaderManager() = default;

}  // namespace workphone::render
