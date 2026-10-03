#ifndef GearTrain_h__
#define GearTrain_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone
{
    namespace vehicle
    {
        class GearTrain
        {
        public:
            GearTrain() = default;

            physics_Num getEngineMainRatio() const;
            void        setEngineMainRatio( physics_Num engineMainRatio );

            physics_Num m_mainTailRatio = static_cast<physics_Num>( 0.0 );
            physics_Num m_engineTailRatio = static_cast<physics_Num>( 0.0 );
            physics_Num m_tailDriveLoss = static_cast<physics_Num>( 0.0 );
            physics_Num m_headFriction = static_cast<physics_Num>( 0.0 );
            bool        m_drivenTail = false;

        private:
            physics_Num m_engineMainRatio = 0.0;
        };

        inline physics_Num GearTrain::getEngineMainRatio() const
        {
            return m_engineMainRatio;
        }

        inline void GearTrain::setEngineMainRatio( physics_Num engineMainRatio )
        {
            m_engineMainRatio = engineMainRatio;
        }
    } // namespace vehicle
} // namespace workphone

#endif // GearTrain_h__
