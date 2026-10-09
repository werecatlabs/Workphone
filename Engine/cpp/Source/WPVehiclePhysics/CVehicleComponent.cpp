#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Vehicle/IAerodymanicsWind.hpp>
#include <Workphone/Interface/Vehicle/IAircraftBody.hpp>
#include <Workphone/Interface/Vehicle/IAircraftControlSurface.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPowerUnit.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropeller.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropWash.hpp>
#include <Workphone/Interface/Vehicle/IAircraftTurbineUnit.hpp>
#include <Workphone/Interface/Vehicle/IAircraftWing.hpp>
#include <Workphone/Interface/Vehicle/IBatteryPack.hpp>
#include <Workphone/Interface/Vehicle/IDifferential.hpp>
#include <Workphone/Interface/Vehicle/IDriveTrain.hpp>
#include <Workphone/Interface/Vehicle/IElectricMotor.hpp>
#include <Workphone/Interface/Vehicle/IESController.hpp>
#include <Workphone/Interface/Vehicle/IGearBox.hpp>
#include <Workphone/Interface/Vehicle/IGroundEffect.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Vehicle/IVehiclePowerUnit.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>

namespace workphone
{
    template class CVehicleComponent<vehicle::IVehicleComponent>;
    template class CVehicleComponent<vehicle::IVehicleBody>;
    template class CVehicleComponent<vehicle::IVehiclePowerUnit>;
    template class CVehicleComponent<vehicle::IWheelComponent>;
    template class CVehicleComponent<vehicle::IDifferential>;
    template class CVehicleComponent<vehicle::IDriveTrain>;
    template class CVehicleComponent<vehicle::IGearBox>;
    template class CVehicleComponent<vehicle::IBatteryPack>;
    template class CVehicleComponent<vehicle::IElectricMotor>;
    template class CVehicleComponent<vehicle::IESController>;
    template class CVehicleComponent<vehicle::IGroundEffect>;
    template class CVehicleComponent<vehicle::IAerodymanicsWind>;
    template class CVehicleComponent<vehicle::IAircraftBody>;
    template class CVehicleComponent<vehicle::IAircraftControlSurface>;
    template class CVehicleComponent<vehicle::IAircraftPowerUnit>;
    template class CVehicleComponent<vehicle::IAircraftPropeller>;
    template class CVehicleComponent<vehicle::IAircraftPropellerUnit>;
    template class CVehicleComponent<vehicle::IAircraftPropWash>;
    template class CVehicleComponent<vehicle::IAircraftTurbineUnit>;
    template class CVehicleComponent<vehicle::IAircraftWing>;
}
