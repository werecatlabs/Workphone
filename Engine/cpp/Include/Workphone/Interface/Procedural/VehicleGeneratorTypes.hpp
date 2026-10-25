#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/Procedural/VehicleAppearanceTypes.hpp>
#include <Workphone/Interface/Procedural/VehicleGeometryTypes.hpp>
#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>
#include <Workphone/System/SourceBudgetStdString.hpp>
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

        struct WPCore_API VehicleGenerationIssue : SourceBudgetObject
        {
            bool error = true;
            SourceBudgetStdString component;
            SourceBudgetStdString message;

            VehicleGenerationIssue() = default;
            VehicleGenerationIssue( bool isError, SourceBudgetStdString componentValue,
                                    SourceBudgetStdString messageValue ) noexcept :
                error( isError ), component( std::move( componentValue ) ),
                message( std::move( messageValue ) ) {}
            VehicleGenerationIssue( const VehicleGenerationIssue & ) = default;
            VehicleGenerationIssue( VehicleGenerationIssue && ) noexcept = default;
            VehicleGenerationIssue &operator=( VehicleGenerationIssue && ) noexcept = default;
            VehicleGenerationIssue &operator=( const VehicleGenerationIssue &other )
            {
                if( this != &other ) { VehicleGenerationIssue candidate( other ); swap( candidate ); }
                return *this;
            }
            void swap( VehicleGenerationIssue &other ) noexcept { std::swap( *this, other ); }
        };

        struct WPCore_API GeneratedVehicle : SourceBudgetObject
        {
            VehicleGeometry geometry;
            VehicleAppearanceBundle appearance;
            VehiclePhysicsConfig physics;
            Array<VehicleGenerationIssue> issues{ Allocator<VehicleGenerationIssue>( sourceBudget ) };
            std::uint64_t contentHash = 0;

            GeneratedVehicle() = default;
            GeneratedVehicle( const GeneratedVehicle &other ) :
                geometry( other.geometry ), appearance( other.appearance ), physics( other.physics ),
                issues( makeSourceBudgetSnapshot( other.issues ) ), contentHash( other.contentHash ) {}
            GeneratedVehicle( GeneratedVehicle && ) noexcept = default;
            GeneratedVehicle &operator=( GeneratedVehicle && ) noexcept = default;
            GeneratedVehicle &operator=( const GeneratedVehicle &other )
            {
                if( this != &other ) { GeneratedVehicle candidate( other ); swap( candidate ); }
                return *this;
            }
            void swap( GeneratedVehicle &other ) noexcept { std::swap( *this, other ); }

            bool isValid() const noexcept;
        };

    }  // namespace procedural
}  // namespace workphone
