#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IVideoManager, ISharedObject );

    IVideoManager::~IVideoManager() = default;

}  // namespace workphone::render
