#ifndef GearTrain_h__
#define GearTrain_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone::vehicle
{
    class GearTrain
    {
    public:
        GearTrain() = default;

        physics_Num getEngineMainRatio() const;
        void setEngineMainRatio(physics_Num engineMainRatio);

        physics_Num m_mainTailRatio = 0.0;
        physics_Num m_engineTailRatio = 0.0;
        physics_Num m_tailDriveLoss = 0.0;
        physics_Num m_headFriction = 0.0;
        bool m_drivenTail = false;

    private:
        physics_Num m_engineMainRatio = 0.0;
    };

    inline physics_Num GearTrain::getEngineMainRatio() const
    {
        return m_engineMainRatio;
    }

    inline void GearTrain::setEngineMainRatio(physics_Num engineMainRatio)
    {
        m_engineMainRatio = engineMainRatio;
    }
}

#endif // GearTrain_h__
