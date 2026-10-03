#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ITextureManager, IResourceManager );

    ITextureManager::~ITextureManager() = default;

}  // namespace workphone::render
