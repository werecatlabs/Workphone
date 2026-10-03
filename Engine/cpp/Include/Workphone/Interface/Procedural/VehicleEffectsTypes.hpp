#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/VehicleDamageTypes.hpp>
#include <Workphone/Interface/Procedural/VehicleDynamicsTypes.hpp>
#include <Workphone/Interface/Procedural/VehiclePresentationTypes.hpp>
#include <array>
#include <cstdint>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        enum class VehicleEffectType
        {
            SkidmarkSegment,
            TireSmoke,
            WaterSpray,
            BrakeSpark,
            ExhaustFlame,
            ExhaustHeatHaze,
            DamageSmoke
        };

        struct WPCore_API VehicleEffectEnvironment
        {
            double wetness = 0.0;
            double ambientTemperatureC = 20.0;
        };

        struct WPCore_API VehicleEffectConfig
        {
            double minimumSkidSpeedMps = 4.0;
            double skidSlipThreshold = 0.22;
            double smokeEventsPerSecond = 18.0;
            double sprayEventsPerSecond = 22.0;
            double exhaustEventsPerSecond = 12.0;
            double damageSmokeEventsPerSecond = 8.0;
            double skidWidthScale = 0.82;
            std::size_t maximumEventsPerStep = 64;
        };

        struct WPCore_API VehicleEffectEvent
        {
            VehicleEffectType type = VehicleEffectType::TireSmoke;
            WheelCorner wheel = WheelCorner::FrontLeft;
            VehiclePhysicsVector3 position;
            VehiclePhysicsVector3 endPosition;
            VehiclePhysicsVector3 velocity;
            VehiclePhysicsVector3 linearColor = { 1.0, 1.0, 1.0 };
            double intensity = 0.0;
            double sizeM = 0.0;
            double lifetimeSeconds = 0.0;
        };

        struct WPCore_API VehicleEffectsState
        {
            std::uint64_t randomState = 0;
            std::array<VehiclePhysicsVector3, 4> previousContact;
            std::array<bool, 4> hasPreviousContact = { false, false, false, false };
            std::array<double, 4> smokeAccumulator = {};
            std::array<double, 4> sprayAccumulator = {};
            double exhaustAccumulator = 0.0;
            double damageSmokeAccumulator = 0.0;
        };

    }  // namespace procedural
}  // namespace workphone
