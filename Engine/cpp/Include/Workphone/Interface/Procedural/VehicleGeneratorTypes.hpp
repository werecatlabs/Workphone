#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/VehicleAppearanceTypes.hpp>
#include <Workphone/Interface/Procedural/VehicleGeometryTypes.hpp>
#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        struct WPCore_API VehicleGenerationConfig
        {
            std::uint32_t seed = 0x4C494F4Eu;
            VehiclePhysicsPreset physicsPreset = VehiclePhysicsPreset::GrandPrix;
            VehicleGeometryConfig geometry;
            VehicleAppearanceConfig appearance;

            /// Preset dimensions are authoritative when true. When false, geometry
            /// wheelbase/tracks/body dimensions are propagated into the physics data.
            bool synchronizeFromPhysics = true;
        };

        struct WPCore_API VehicleGenerationIssue
        {
            bool error = true;
            std::string component;
            std::string message;
        };

        struct WPCore_API GeneratedVehicle
        {
            VehicleGeometry geometry;
            VehicleAppearanceBundle appearance;
            VehiclePhysicsConfig physics;
            std::vector<VehicleGenerationIssue> issues;
            std::uint64_t contentHash = 0;

            bool isValid() const noexcept;
        };

    }  // namespace procedural
}  // namespace workphone
