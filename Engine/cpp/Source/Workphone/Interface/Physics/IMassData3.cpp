#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IMassData3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IMassData3, ISharedObject );

    IMassData3::~IMassData3() = default;

}  // namespace workphone::physics
