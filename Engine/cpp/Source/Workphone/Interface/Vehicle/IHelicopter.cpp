#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IHelicopter.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IHelicopter, IAircraft );

    IHelicopter::~IHelicopter() = default;

}  // namespace workphone::vehicle
