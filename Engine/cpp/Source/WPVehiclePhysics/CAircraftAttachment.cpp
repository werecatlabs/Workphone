#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>
#include <WPVehiclePhysics/CAircraftBody.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <Workphone/Interface/Vehicle/IAerodymanicsWind.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/Interface/Vehicle/IAircraftControlSurface.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPowerUnit.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropeller.hpp>
#include <Workphone/Interface/Vehicle/IAircraftWing.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropWash.hpp>
#include <Workphone/Interface/Vehicle/IBatteryPack.hpp>
#include <Workphone/Interface/Vehicle/IESController.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone::vehicle
{
    template class CAircraftAttachment<IAerodymanicsWind>;
    template class CAircraftAttachment<IAircraftPropellerUnit>;
    template class CAircraftAttachment<IBatteryPack>;
    template class CAircraftAttachment<IESController>;

    template class CAircraftAttachment<IAircraftControlSurface>;
    template class CAircraftAttachment<IAircraftPowerUnit>;
    template class CAircraftAttachment<IAircraftPropeller>;
    template class CAircraftAttachment<IAircraftWing>;

    template class CAircraftAttachment<IAircraftBody>;
    template class CAircraftAttachment<IAircraftPropWash>;

    template class CAircraftAttachment<IVehicleComponent>;
} // namespace workphone::vehicle
