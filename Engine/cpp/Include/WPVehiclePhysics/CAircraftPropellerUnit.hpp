#ifndef CAircraftPropellerUnit_h__
#define CAircraftPropellerUnit_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include <array>

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CAircraftPropellerUnit
            : public CAircraftAttachment<IAircraftPropellerUnit>
        {
        public:
            CAircraftPropellerUnit();
            ~CAircraftPropellerUnit() override;

            bool isValid() const override;

            void update( const double &t, const double &dt ) override;

            SmartPtr<IBatteryPack>       &getBatteryPack() override;
            const SmartPtr<IBatteryPack> &getBatteryPack() const override;
            void                          setBatteryPack( SmartPtr<IBatteryPack> batteryPack ) override;

            SmartPtr<IESController>       &getESC() override;
            const SmartPtr<IESController> &getESC() const override;
            void                           setESC( SmartPtr<IESController> esc ) override;

            SmartPtr<IAircraftPowerUnit>       &getPowerUnit() override;
            const SmartPtr<IAircraftPowerUnit> &getPowerUnit() const override;
            void setPowerUnit( SmartPtr<IAircraftPowerUnit> powerUnit ) override;

            SmartPtr<IAircraftPropeller>       &getPropeller() override;
            const SmartPtr<IAircraftPropeller> &getPropeller() const override;
            void setPropeller( SmartPtr<IAircraftPropeller> propeller ) override;

            real_Num getFlowSettlingTime() const;
            void     setFlowSettlingTime( real_Num flowSettlingTime );

            Vector3<real_Num> getThrust() const override;
            void              setThrust( const Vector3<real_Num> &thrust ) override;

            Vector3<real_Num> getPropwash() const override;
            void              setPropwash( const Vector3<real_Num> &propwash ) override;

        protected:
            void EPropellerSimple( const double &t, const double &dt );
            void EPropellerOld( const double &t, const double &dt );
            void EPropellerNew( const double &t, const double &dt );
            void PropellerNitro( const double &dt );

            void ESCCutoutControl( SmartPtr<IESController> pESC, float dt );

            float lookupSlope( float angle );
            float lookupCdSlope( float angle );

            SmartPtr<IBatteryPack>       m_batteryPack;
            SmartPtr<IESController>      m_esc;
            SmartPtr<IAircraftPowerUnit> m_pu;
            SmartPtr<IAircraftPropeller> m_propeller;

            Vector3<real_Num> m_thrust;
            Vector3<real_Num> m_propwash;

            Vector3<real_Num> inflowVec;
            Vector3<real_Num> windTunnel;

            Quaternion<real_Num> inflowQuat;

            real_Num m_inFlowSettlingTime;

            real_Num downWashCoef;
            real_Num torqueMultiplier;
            real_Num TotalCurrent;
            real_Num inFlowTC;
            real_Num vh;
            real_Num vc;
            real_Num ThisPW;
            real_Num inflowFollowTC;
            real_Num inflowDotProduct;
            real_Num lastTime;
            real_Num vibration;
            real_Num propPowerCoef;
            real_Num negativeLift;
            real_Num vrsModifier;
            real_Num fixedVRSModifier;
            real_Num thrustAccelerationModifier;
            real_Num Temp;
            real_Num PropGA;
            real_Num propWash;
            real_Num propDiskFlowX;
            real_Num Pind;
            real_Num Pdrag;
            real_Num lastPropThrust;
            real_Num stallPoint;
            real_Num stallThrust;
            real_Num thrustSlope;
            real_Num vortex;
            real_Num translation;
            real_Num gfValue;

            s32 frameCount;
            s32 flag;

            bool packDead = false;
            bool propDamageVibration = false;
            bool ModelIsElectric = true;
            bool motorReversed = false;
            bool vrs = false;

            Array<float> clSlopes;
            Array<float> cdSlopes;

            s32 m_id;

            /// Used to generate a unique id.
            static u32 m_idExt;
        };
    } // namespace vehicle
} // namespace workphone

#endif // CAircraftPropellerUnit_h__
