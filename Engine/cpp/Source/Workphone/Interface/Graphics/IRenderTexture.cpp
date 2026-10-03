#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IRenderTexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IRenderTexture, IRenderTarget );

    IRenderTexture::~IRenderTexture() = default;

}  // namespace workphone::render
