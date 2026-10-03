#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVideoStream.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IVideoStream, IVideo );

    IVideoStream::~IVideoStream() = default;

}  // namespace workphone::render
