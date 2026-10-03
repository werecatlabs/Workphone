#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/VehicleDamageTypes.hpp>
#include <Workphone/Interface/Procedural/VehicleDynamicsTypes.hpp>
#include <array>

namespace workphone
{
    namespace procedural
    {
        struct WPCore_API VehiclePresentationConfig
        {
            double pitchRadiansPerMps2 = 0.0065;
            double rollRadiansPerMps2 = 0.0085;
            double maximumPitchRad = 0.065;
            double maximumRollRad = 0.095;
            double bodyResponsePerSecond = 11.0;
            double lightResponsePerSecond = 18.0;
            double wheelBlurStartRadPerSec = 38.0;
            double wheelBlurFullRadPerSec = 115.0;
        };

        struct WPCore_API VehiclePresentationInput
        {
            bool headlightsEnabled = true;
            bool rainLightEnabled = false;
            double throttle = 0.0;
            double backfire = 0.0;
        };

        struct WPCore_API VehicleWheelPose
        {
            VehiclePhysicsVector3 hubPosition;
            double steerAngleRad = 0.0;
            double rotationAngleRad = 0.0;
            double suspensionOffsetM = 0.0;
            double rotationalBlur = 0.0;
        };

        struct WPCore_API VehiclePresentationState
        {
            double bodyPitchRad = 0.0;
            double bodyRollRad = 0.0;
            double bodyHeaveM = 0.0;
            double brakeLightIntensity = 0.0;
            double headlightIntensity = 0.0;
            double rainLightIntensity = 0.0;
            double exhaustIntensity = 0.0;
            double damageSmokeIntensity = 0.0;
            std::array<VehicleWheelPose, 4> wheels;
        };

    }  // namespace procedural
}  // namespace workphone
