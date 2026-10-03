#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        /// Stable material identifiers. Render backends may map these to any PBR material system.
        enum class VehicleMaterial : std::uint8_t
        {
            Paint,
            CarbonGloss,
            CarbonMatte,
            Glass,
            Headlight,
            TailLight,
            RainLight,
            Tyre,
            Rim,
            Brake,
            Caliper,
            BrakeDuct,
            Decal,
            Suspension
        };

        struct WPCore_API VehicleVertex
        {
            Vector3F position{};
            Vector3F normal{};
            Vector3F tangent{ 1.0f, 0.0f, 0.0f };
            Vector2F uv{};
            Vector3F mask{};  ///< wear, grime and ambient-occlusion weights
            float tangentSign = 1.0f;
        };

        /// One independently drawable, single-material indexed triangle mesh.
        struct WPCore_API VehicleMeshSection
        {
            std::string name;
            VehicleMaterial material = VehicleMaterial::Paint;
            std::vector<VehicleVertex> vertices;
            std::vector<std::uint32_t> indices;

            bool hasGeometry() const;
        };

        struct WPCore_API VehicleLOD
        {
            std::uint32_t level = 0;
            float suggestedScreenCoverage = 1.0f;
            std::vector<VehicleMeshSection> sections;
        };

        struct WPCore_API VehicleGeometry
        {
            std::vector<VehicleLOD> lods;
            Vector3F boundsMin{};
            Vector3F boundsMax{};
            Vector3F centreOfMass{};
            Vector3F frontAxle{};
            Vector3F rearAxle{};
            Vector3F cockpitEye{};

            bool hasGeometry() const;
        };

        struct WPCore_API VehicleGeometryConfig
        {
            std::uint32_t seed = 0x4C494F4Eu;
            float wheelbase = 3.60f;
            float frontTrack = 1.62f;
            float rearTrack = 1.56f;
            float tyreRadius = 0.360f;
            float frontTyreWidth = 0.305f;
            float rearTyreWidth = 0.405f;
            float bodyLength = 5.45f;
            float bodyWidth = 2.00f;
            bool generateThreeLODs = true;
            bool includeFineDetails = true;
        };

    }  // namespace procedural
}  // namespace workphone
