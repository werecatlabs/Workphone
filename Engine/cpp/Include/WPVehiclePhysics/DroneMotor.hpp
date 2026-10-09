#ifndef MultiRotorMotor_h__
#define MultiRotorMotor_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "Workphone/Interface/Memory/ISharedObject.hpp"
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <deque>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API DroneMotor : public ISharedObject
    {
    public:
        DroneMotor();
        ~DroneMotor() override;

        void initialise(const std::string &objectID);

        void init();

        void input(real_Num inputValue, const double &dt);

        void enterFlightState();
        void enterWorkbenchState();

        void clearAverageSoundRPM();

        void updateSound();

        real_Num getMaxTorque() const;
        void setMaxTorque(real_Num maxTorque);

        Vector3<real_Num> getLocalPos() const;
        void setLocalPos(const Vector3<real_Num> &localPos);

        f32 getAverageMotorRPM() const;
        void setAverageMotorRPM(f32 averageMotorRPM);

        int getDebugId() const;
        void setDebugId(int debugId);

        Quaternion<real_Num> getRotation() const;
        void setRotation(const Quaternion<real_Num> &rotation);

        s32 getIndex() const;
        void setIndex(s32 index);

        std::string getPropReference() const;
        void setPropReference(const std::string &propReference);

        SmartPtr<DroneProp> getProp() const;
        void setProp(SmartPtr<DroneProp> prop);

    protected:
        void setupUserData();

        SmartPtr<DroneProp> m_prop;
        Quaternion<real_Num> m_rotation;
        Vector3<real_Num> m_localPos;

        real_Num m_maxTorque;
        real_Num m_lastInput;
        real_Num m_slewRateUp;
        real_Num m_slewRateDown;
        real_Num m_slewedInput;
        bool m_powerOn;
        int m_debugId;
        f32 m_averageMotorRPM;
        s32 m_index;

        Deque<f32> m_averageSoundRPM;

        std::string m_propReference;
    };
}

#endif // MultiRotorMotor_h__
