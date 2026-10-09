#ifndef CAircraftPropellerUnit_h__
#define CAircraftPropellerUnit_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include <array>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CAircraftPropellerUnit
        : public CAircraftAttachment<IAircraftPropellerUnit>
    {
    public:
        CAircraftPropellerUnit();
        ~CAircraftPropellerUnit() override;

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

        real_Num getFlowSettlingTime() const;
        void setFlowSettlingTime(real_Num flowSettlingTime);

        Vector3<real_Num> getThrust() const override;
        void setThrust(const Vector3<real_Num> &thrust) override;

        Vector3<real_Num> getPropwash() const override;
        void setPropwash(const Vector3<real_Num> &propwash) override;

    protected:
        void ePropellerSimple(const double &t, const double &dt);
        void ePropellerOld(const double &t, const double &dt);
        void ePropellerNew(const double &t, const double &dt);
        void propellerNitro(const double &dt);

        void escCutoutControl(SmartPtr<IESController> pESC, float dt);

        float lookupSlope(float angle);
        float lookupCdSlope(float angle);

        SmartPtr<IBatteryPack> m_batteryPack;
        SmartPtr<IESController> m_esc;
        SmartPtr<IAircraftPowerUnit> m_pu;
        SmartPtr<IAircraftPropeller> m_propeller;

        Vector3<real_Num> m_thrust;
        Vector3<real_Num> m_propwash;

        Vector3<real_Num> m_inflowVec;
        Vector3<real_Num> m_windTunnel;

        Quaternion<real_Num> m_inflowQuat;

        real_Num m_inFlowSettlingTime;

        real_Num m_downWashCoef;
        real_Num m_torqueMultiplier;
        real_Num m_totalCurrent;
        real_Num m_inFlowTC;
        real_Num m_vh;
        real_Num m_vc;
        real_Num m_thisPW;
        real_Num m_inflowFollowTC;
        real_Num m_inflowDotProduct;
        real_Num m_lastTime;
        real_Num m_vibration;
        real_Num m_propPowerCoef;
        real_Num m_negativeLift;
        real_Num m_vrsModifier;
        real_Num m_fixedVRSModifier;
        real_Num m_thrustAccelerationModifier;
        real_Num m_temp;
        real_Num m_propGA;
        real_Num m_propWash;
        real_Num m_propDiskFlowX;
        real_Num m_pind;
        real_Num m_pdrag;
        real_Num m_lastPropThrust;
        real_Num m_stallPoint;
        real_Num m_stallThrust;
        real_Num m_thrustSlope;
        real_Num m_vortex;
        real_Num m_translation;
        real_Num m_gfValue;

        s32 m_frameCount;
        s32 m_flag;

        bool m_packDead = false;
        bool m_propDamageVibration = false;
        bool m_modelIsElectric = true;
        bool m_motorReversed = false;
        bool m_vrs = false;

        Array<float> m_clSlopes;
        Array<float> m_cdSlopes;

        s32 m_id;

        /// Used to generate a unique id.
        static u32 m_idExt;
    };
}

#endif // CAircraftPropellerUnit_h__
