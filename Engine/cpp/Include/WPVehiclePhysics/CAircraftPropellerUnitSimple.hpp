#ifndef CAircraftPropellerUnitSimple_h__
#define CAircraftPropellerUnitSimple_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include <array>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CAircraftPropellerUnitSimple
        : public CAircraftAttachment<IAircraftPropellerUnit>
    {
    public:
        CAircraftPropellerUnitSimple();
        ~CAircraftPropellerUnitSimple() override;

        bool isValid() const override;

        void update(const double &t, const double &dt) override;

        SmartPtr<IBatteryPack> &getBatteryPack() override;
        const SmartPtr<IBatteryPack> &getBatteryPack() const override;
        void setBatteryPack(SmartPtr<IBatteryPack> batteryPack) override;

        SmartPtr<IESController> &getESC() override;
        const SmartPtr<IESController> &getESC() const override;
        void setESC(SmartPtr<IESController> esc) override;

        SmartPtr<IAircraftPowerUnit> &getPowerUnit() override;
        const SmartPtr<IAircraftPowerUnit> &getPowerUnit() const override;
        void setPowerUnit(SmartPtr<IAircraftPowerUnit> powerUnit) override;

        SmartPtr<IAircraftPropeller> &getPropeller() override;
        const SmartPtr<IAircraftPropeller> &getPropeller() const override;
        void setPropeller(SmartPtr<IAircraftPropeller> propeller) override;

        Vector3<real_Num> getThrust() const override;
        void setThrust(const Vector3<real_Num> &thrust) override;

        Vector3<real_Num> getPropwash() const override;
        void setPropwash(const Vector3<real_Num> &propwash) override;

    protected:
        SmartPtr<IBatteryPack> m_batteryPack;
        SmartPtr<IESController> m_esc;
        SmartPtr<IAircraftPowerUnit> m_powerUnit;
        SmartPtr<IAircraftPropeller> m_propeller;

        Vector3<real_Num> m_thrust;

        s32 m_id;

        /// Used to generate a unique id.
        static u32 m_idExt;
    };
}

#endif // CAircraftPropellerUnitSimple_h__
