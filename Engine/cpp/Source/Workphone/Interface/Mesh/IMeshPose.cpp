#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IMeshPose.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IMeshPose, ISharedObject );

    IMeshPose::~IMeshPose() = default;
}  // namespace workphone
