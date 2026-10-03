#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ITransform, ISharedObject );

    const u8 ITransform::localDirtyFlag = 1 << 1;
    const u8 ITransform::dirtyFlag = 1 << 2;
    const u8 ITransform::enabledFlag = 1 << 3;
    const u8 ITransform::smoothMotionFlag = 1 << 4;

    ITransform::~ITransform() = default;

}  // namespace workphone::scene
