#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IGroundEffect.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IGroundEffect, IVehicleComponent );

        IGroundEffect::~IGroundEffect() = default;

    }  // namespace vehicle
}  // namespace workphone
