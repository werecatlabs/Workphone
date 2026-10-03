#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IAnimationTextureControl.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationTextureControl, ISharedObject );

    IAnimationTextureControl::~IAnimationTextureControl() = default;

}  // namespace workphone::render
