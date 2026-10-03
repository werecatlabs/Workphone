#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralTexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProceduralTexture, ISharedObject );

    IProceduralTexture::~IProceduralTexture() = default;
}  // namespace workphone::procedural
