#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IVideo, ISharedObject );

    IVideo::~IVideo() = default;

}  // namespace workphone::render
