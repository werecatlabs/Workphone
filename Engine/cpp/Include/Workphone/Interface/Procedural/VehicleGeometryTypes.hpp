#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/System/SourceBudgetStdString.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
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
            Suspension,
            SecondaryPaint  ///< Uses the configurable secondary paint appearance slot.
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
        struct WPCore_API VehicleMeshSection : SourceBudgetObject
        {
            SourceBudgetStdString name;
            VehicleMaterial material = VehicleMaterial::Paint;
            SourceBudgetVector<VehicleVertex> vertices;
            SourceBudgetVector<std::uint32_t> indices;

            VehicleMeshSection() = default;
            VehicleMeshSection( const VehicleMeshSection & ) = default;
            VehicleMeshSection( VehicleMeshSection && ) noexcept = default;
            VehicleMeshSection &operator=( VehicleMeshSection && ) noexcept = default;
            VehicleMeshSection &operator=( const VehicleMeshSection &other )
            {
                if( this != &other ) { VehicleMeshSection candidate( other ); swap( candidate ); }
                return *this;
            }
            void swap( VehicleMeshSection &other ) noexcept
            {
                name.swap( other.name );
                std::swap( material, other.material );
                vertices.swap( other.vertices );
                indices.swap( other.indices );
            }

            bool hasGeometry() const;
        };

        struct WPCore_API VehicleLOD : SourceBudgetObject
        {
            std::uint32_t level = 0;
            float suggestedScreenCoverage = 1.0f;
            SourceBudgetVector<VehicleMeshSection> sections;

            VehicleLOD() = default;
            VehicleLOD( const VehicleLOD & ) = default;
            VehicleLOD( VehicleLOD && ) noexcept = default;
            VehicleLOD &operator=( VehicleLOD && ) noexcept = default;
            VehicleLOD &operator=( const VehicleLOD &other )
            {
                if( this != &other ) { VehicleLOD candidate( other ); swap( candidate ); }
                return *this;
            }
            void swap( VehicleLOD &other ) noexcept
            {
                std::swap( level, other.level );
                std::swap( suggestedScreenCoverage, other.suggestedScreenCoverage );
                sections.swap( other.sections );
            }
        };

        struct WPCore_API VehicleGeometry : SourceBudgetObject
        {
            SourceBudgetVector<VehicleLOD> lods;
            Vector3F boundsMin{};
            Vector3F boundsMax{};
            Vector3F centreOfMass{};
            Vector3F frontAxle{};
            Vector3F rearAxle{};
            Vector3F cockpitEye{};

            VehicleGeometry() = default;
            VehicleGeometry( const VehicleGeometry & ) = default;
            VehicleGeometry( VehicleGeometry && ) noexcept = default;
            VehicleGeometry &operator=( VehicleGeometry && ) noexcept = default;
            VehicleGeometry &operator=( const VehicleGeometry &other )
            {
                if( this != &other ) { VehicleGeometry candidate( other ); swap( candidate ); }
                return *this;
            }
            void swap( VehicleGeometry &other ) noexcept
            {
                using std::swap;
                lods.swap( other.lods );
                swap( boundsMin, other.boundsMin );
                swap( boundsMax, other.boundsMax );
                swap( centreOfMass, other.centreOfMass );
                swap( frontAxle, other.frontAxle );
                swap( rearAxle, other.rearAxle );
                swap( cockpitEye, other.cockpitEye );
            }

            bool hasGeometry() const;
        };

        enum class VehicleBodyStyle : std::uint8_t { GrandPrix, GT, RoadSport, HelicopterLittleBird, Warbird };

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
            VehicleBodyStyle bodyStyle = VehicleBodyStyle::GrandPrix;
            // Helicopter silhouette controls. Other body styles ignore these values.
            float mainRotorRadius = 4.15f;
            float tailRotorRadius = .68f;
            std::uint32_t mainRotorBlades = 5;
            std::uint32_t tailRotorBlades = 2;
            float wingSpan = 11.2f;
            float propellerRadius = 1.55f;
            std::uint32_t propellerBlades = 4;
            bool landingGearDown = true;
        };

    }  // namespace procedural
}  // namespace workphone
