#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Vehicle/IGearBox.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, IGearBox, IVehicleComponent );

        IGearBox::~IGearBox() = default;

    }  // namespace vehicle
}  // namespace workphone
