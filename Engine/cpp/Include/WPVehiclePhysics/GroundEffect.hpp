#ifndef GroundEffect_h__
#define GroundEffect_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API GroundEffect : public ISharedObject
        {
        public:
            GroundEffect();
            ~GroundEffect() override;

            void getGroundEffectCoefficients( Vector3<real_Num> PointA, Vector3<real_Num> PointB,
                                              Vector3<real_Num> PointC, Vector3<real_Num> PointD,
                                              real_Num &clMultiplier, real_Num &cdMultiplier );

            Vector3<real_Num> m_rayCastAxis;
            real_Num          m_wingspan;
        };
    } // namespace vehicle
} // namespace workphone

#endif // GroundEffect_h__
