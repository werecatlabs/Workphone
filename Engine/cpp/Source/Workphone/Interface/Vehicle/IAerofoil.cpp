#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IAerofoil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IAerofoil, ISharedObject );

        IAerofoil::~IAerofoil() = default;

    }  // namespace vehicle
}  // namespace workphone
