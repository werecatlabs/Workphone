#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/VehicleDynamicsTypes.hpp>
#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        struct WPCore_API VehicleDamageConfig
        {
            double minimumSeverity = 0.04;
            double hitCooldownSeconds = 0.10;
            double wreckSeverity = 0.68;
            double damagePerSeverity = 0.58;
            double permanentFraction = 0.12;
            double maximumPermanentDamage = 0.62;
            double repairDelaySeconds = 5.0;
            double repairRatePerSecond = 0.025;
            double engineCutSeconds = 1.15;
            double wreckControlLockSeconds = 1.65;
            double torquePenaltyAtFullDamage = 0.34;
            double gripPenaltyAtFullDamage = 0.18;
            double steeringPullAtFullDamage = 0.10;
        };

        struct WPCore_API VehicleDamageImpact
        {
            double closingSpeedMps = 0.0;
            double impulseNs = 0.0;
            /// Optional normalized severity. A negative value requests automatic derivation.
            double severity = -1.0;
            /// Impact normal expressed in vehicle-local coordinates (+X right, -Z forward).
            VehiclePhysicsVector3 localNormal;
        };

        struct WPCore_API VehicleDamageState
        {
            double damage = 0.0;
            double permanentDamage = 0.0;
            double hitCooldownRemaining = 0.0;
            double secondsSinceImpact = 1.0e9;
            double engineCutRemaining = 0.0;
            double controlLockRemaining = 0.0;
            double trauma = 0.0;
            double smokeBurst = 0.0;
            double steeringPullSign = 1.0;
            double lastSeverity = 0.0;
            std::uint32_t hitCount = 0;
            std::uint32_t wreckCount = 0;
            std::uint64_t randomState = 0;
        };

        struct WPCore_API VehicleDamageEvent
        {
            bool accepted = false;
            bool wrecked = false;
            double severity = 0.0;
            double damageAdded = 0.0;
            double recommendedSpeedScale = 1.0;
            double recommendedYawImpulseRadPerSec = 0.0;
        };

        struct WPCore_API VehicleDamageTelemetry
        {
            double damage = 0.0;
            double permanentDamage = 0.0;
            double torqueScale = 1.0;
            double gripScale = 1.0;
            double steeringBias = 0.0;
            double trauma = 0.0;
            double smoke = 0.0;
            bool engineCut = false;
            bool controlsLocked = false;
            bool wrecked = false;
        };

        struct WPCore_API VehicleDamageValidation
        {
            std::vector<std::string> errors;
            bool isValid() const noexcept
            {
                return errors.empty();
            }
        };

    }  // namespace procedural
}  // namespace workphone
