#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IMassData2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IMassData2, ISharedObject );

    IMassData2::~IMassData2() = default;

}  // namespace workphone::physics
