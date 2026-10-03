#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterialNodeAnimatedTexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterialNodeAnimatedTexture, IMaterialNode );

    IMaterialNodeAnimatedTexture::~IMaterialNodeAnimatedTexture() = default;

}  // namespace workphone::render
