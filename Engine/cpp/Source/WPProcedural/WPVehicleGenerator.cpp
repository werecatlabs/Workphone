#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            bool finite( float value )
            {
                return std::isfinite( static_cast<double>( value ) );
            }

            float dot( const Vector3F &a, const Vector3F &b )
            {
                return a.x * b.x + a.y * b.y + a.z * b.z;
            }

            float lengthSquared( const Vector3F &v )
            {
                return dot( v, v );
            }

            void addIssue( std::vector<VehicleGenerationIssue> &issues, bool error,
                           const char *component, const std::string &message )
            {
                issues.push_back( { error, component, message } );
            }

            std::uint64_t fnvBytes( std::uint64_t hash, const void *data, std::size_t size )
            {
                const auto *bytes = static_cast<const unsigned char *>( data );
                for( std::size_t i = 0; i < size; ++i )
                {
                    hash ^= bytes[i];
                    hash *= 1099511628211ull;
                }
                return hash;
            }

            std::uint64_t fnvU64( std::uint64_t hash, std::uint64_t value )
            {
                for( unsigned shift = 0; shift < 64; shift += 8 )
                {
                    const unsigned char byte = static_cast<unsigned char>( value >> shift );
                    hash = fnvBytes( hash, &byte, 1u );
                }
                return hash;
            }

            std::uint64_t fnvU32( std::uint64_t hash, std::uint32_t value )
            {
                for( unsigned shift = 0; shift < 32; shift += 8 )
                {
                    const unsigned char byte = static_cast<unsigned char>( value >> shift );
                    hash = fnvBytes( hash, &byte, 1u );
                }
                return hash;
            }

            std::uint64_t fnvFloat( std::uint64_t hash, float value )
            {
                if( value == 0.0f )
                    value = 0.0f;
                std::uint32_t bits = 0;
                std::memcpy( &bits, &value, sizeof( bits ) );
                if( ( bits & 0x7f800000u ) == 0x7f800000u && ( bits & 0x007fffffu ) != 0u )
                    bits = 0x7fc00000u;
                return fnvU32( hash, bits );
            }

            std::uint64_t fnvDouble( std::uint64_t hash, double value )
            {
                if( value == 0.0 )
                    value = 0.0;
                std::uint64_t bits = 0;
                std::memcpy( &bits, &value, sizeof( bits ) );
                if( ( bits & 0x7ff0000000000000ull ) == 0x7ff0000000000000ull &&
                    ( bits & 0x000fffffffffffffull ) != 0ull )
                    bits = 0x7ff8000000000000ull;
                return fnvU64( hash, bits );
            }

            std::uint64_t fnvVector3( std::uint64_t hash, const Vector3F &value )
            {
                hash = fnvFloat( hash, value.x );
                hash = fnvFloat( hash, value.y );
                return fnvFloat( hash, value.z );
            }

            std::uint64_t fnvPhysicsVector( std::uint64_t hash, const VehiclePhysicsVector3 &value )
            {
                hash = fnvDouble( hash, value.x );
                hash = fnvDouble( hash, value.y );
                return fnvDouble( hash, value.z );
            }

            void applyGeometryDimensions( VehiclePhysicsConfig &physics,
                                          const VehicleGeometryConfig &geometry )
            {
                physics.wheelbaseM = geometry.wheelbase;
                physics.frontTrackM = geometry.frontTrack;
                physics.rearTrackM = geometry.rearTrack;
                physics.bodyLengthM = geometry.bodyLength;
                physics.bodyWidthM = geometry.bodyWidth;

                // WPVehicleGeometry deliberately places the CG origin 45% of the
                // wheelbase behind the front axle (the reference car is -1.62/+1.98).
                const double frontZ = -0.45 * physics.wheelbaseM;
                const double rearZ = 0.55 * physics.wheelbaseM;
                for( std::size_t i = 0; i < physics.wheels.size(); ++i )
                {
                    const bool front = i < 2;
                    const bool left = ( i & 1u ) == 0u;
                    const double track = front ? physics.frontTrackM : physics.rearTrackM;
                    auto &wheel = physics.wheels[i];
                    wheel.hubPosition.x = ( left ? -0.5 : 0.5 ) * track;
                    wheel.hubPosition.z = front ? frontZ : rearZ;
                    wheel.tire.radiusM = geometry.tyreRadius;
                    wheel.tire.unloadedRadiusM = geometry.tyreRadius + 0.0065;
                    wheel.tire.widthM = front ? geometry.frontTyreWidth : geometry.rearTyreWidth;
                    wheel.tire.wheelInertiaKgM2 =
                        0.72 * wheel.tire.wheelMassKg * wheel.tire.radiusM * wheel.tire.radiusM;
                    wheel.hubPosition.y = geometry.tyreRadius;
                }
                WPVehiclePhysics::recomputeDerivedProperties( physics );
            }

            void applyPhysicsDimensions( VehicleGeometryConfig &geometry,
                                         const VehiclePhysicsConfig &physics )
            {
                geometry.wheelbase = static_cast<float>( physics.wheelbaseM );
                geometry.frontTrack = static_cast<float>( physics.frontTrackM );
                geometry.rearTrack = static_cast<float>( physics.rearTrackM );
                geometry.bodyLength = static_cast<float>( physics.bodyLengthM );
                geometry.bodyWidth = static_cast<float>( physics.bodyWidthM );
                geometry.tyreRadius = static_cast<float>( physics.wheels[0].tire.radiusM );
                geometry.frontTyreWidth = static_cast<float>( physics.wheels[0].tire.widthM );
                geometry.rearTyreWidth = static_cast<float>( physics.wheels[2].tire.widthM );
            }

            void applyAppearanceUVContract( VehicleGeometry &geometry )
            {
                // Geometry lofts are naturally authored as (circumference,
                // nose-to-tail). Appearance publishes the inverse contract:
                // u=nose-to-tail, v=right/top/left/floor circumference.
                for( auto &lod : geometry.lods )
                    for( auto &section : lod.sections )
                        if( section.material == VehicleMaterial::Paint )
                            for( auto &vertex : section.vertices )
                            {
                                std::swap( vertex.uv.x, vertex.uv.y );
                                // Swapping UV axes also swaps the tangent-frame axes.
                                // Reconstruct the old bitangent before changing handedness.
                                const Vector3F oldBitangent =
                                    vertex.normal.crossProduct( vertex.tangent ) * vertex.tangentSign;
                                vertex.tangent = oldBitangent;
                                vertex.tangentSign = -vertex.tangentSign;
                            }
            }

            void translateGeometryZ( VehicleGeometry &geometry, float offset )
            {
                if( offset == 0.0f )
                    return;
                for( auto &lod : geometry.lods )
                    for( auto &section : lod.sections )
                        for( auto &vertex : section.vertices )
                            vertex.position.z += offset;
                geometry.boundsMin.z += offset;
                geometry.boundsMax.z += offset;
                geometry.centreOfMass.z += offset;
                geometry.frontAxle.z += offset;
                geometry.rearAxle.z += offset;
                geometry.cockpitEye.z += offset;
            }

            std::uint64_t hashVehicleVertex( std::uint64_t hash, const VehicleVertex &vertex )
            {
                // Hash semantic fields rather than the object representation: padding
                // bytes are not stable across builds or necessarily initialized.
                hash = fnvVector3( hash, vertex.position );
                hash = fnvVector3( hash, vertex.normal );
                hash = fnvVector3( hash, vertex.tangent );
                hash = fnvFloat( hash, vertex.uv.x );
                hash = fnvFloat( hash, vertex.uv.y );
                hash = fnvVector3( hash, vertex.mask );
                return fnvFloat( hash, vertex.tangentSign );
            }

            std::uint64_t hashPhysicsConfig( std::uint64_t hash, const VehiclePhysicsConfig &physics )
            {
                hash = fnvU64( hash, physics.seed );
                hash = fnvU32( hash, static_cast<std::uint32_t>( physics.preset ) );
                for( double value :
                     { physics.wheelbaseM, physics.frontTrackM, physics.rearTrackM, physics.bodyLengthM,
                       physics.bodyWidthM, physics.gravityMps2, physics.massProperties.massKg,
                       physics.massProperties.frontStaticWeightFraction } )
                    hash = fnvDouble( hash, value );
                hash = fnvPhysicsVector( hash, physics.massProperties.centreOfMass );
                const auto &inertia = physics.massProperties.inertia;
                for( double value :
                     { inertia.xx, inertia.yy, inertia.zz, inertia.xy, inertia.xz, inertia.yz } )
                    hash = fnvDouble( hash, value );
                hash = fnvU64( hash, static_cast<std::uint64_t>( physics.massElements.size() ) );
                for( const auto &element : physics.massElements )
                {
                    hash = fnvU64( hash, static_cast<std::uint64_t>( element.name.size() ) );
                    hash = fnvBytes( hash, element.name.data(), element.name.size() );
                    hash = fnvDouble( hash, element.massKg );
                    hash = fnvPhysicsVector( hash, element.centre );
                    hash = fnvPhysicsVector( hash, element.dimensions );
                }
                for( const auto &wheel : physics.wheels )
                {
                    hash = fnvPhysicsVector( hash, wheel.hubPosition );
                    const auto &s = wheel.suspension;
                    for( double value :
                         { s.springRateNPerM, s.damperCompressionNPerMps, s.damperReboundNPerMps,
                           s.antiRollRateNmPerRad, s.bumpTravelM, s.reboundTravelM, s.motionRatio,
                           s.staticCamberRad, s.staticToeRad, s.casterRad, s.kingpinInclinationRad } )
                        hash = fnvDouble( hash, value );
                    const auto &tire = wheel.tire;
                    for( double value :
                         { tire.radiusM, tire.widthM, tire.wheelMassKg, tire.wheelInertiaKgM2,
                           tire.unloadedRadiusM, tire.verticalStiffnessNPerM,
                           tire.longitudinalStiffnessNPerSlip, tire.corneringStiffnessNPerRad,
                           tire.peakLongitudinalFriction, tire.peakLateralFriction,
                           tire.rollingResistance, tire.optimalTemperatureC, tire.coldFrictionScale,
                           tire.loadSensitivity, wheel.maxSteerRad, wheel.brakeTorqueNm,
                           wheel.handbrakeTorqueNm } )
                        hash = fnvDouble( hash, value );
                    hash = fnvU32( hash, wheel.steerable ? 1u : 0u );
                    hash = fnvU32( hash, wheel.driven ? 1u : 0u );
                }
                const auto &aero = physics.aero;
                for( double value :
                     { aero.referenceAreaM2, aero.dragCoefficient, aero.downforceCoefficient,
                       aero.frontDownforceFraction, aero.airDensityKgPerM3, aero.yawStabilityCoefficient,
                       aero.groundEffectRideHeightM, aero.groundEffectSensitivity } )
                    hash = fnvDouble( hash, value );
                hash = fnvPhysicsVector( hash, aero.centreOfPressure );
                const auto &drive = physics.drivetrain;
                hash = fnvU32( hash, static_cast<std::uint32_t>( drive.drivenAxle ) );
                for( double value :
                     { drive.peakPowerW, drive.peakTorqueNm, drive.idleRpm, drive.redlineRpm,
                       drive.finalDriveRatio, drive.transmissionEfficiency } )
                    hash = fnvDouble( hash, value );
                hash = fnvU64( hash, static_cast<std::uint64_t>( drive.forwardGearRatios.size() ) );
                for( double ratio : drive.forwardGearRatios )
                    hash = fnvDouble( hash, ratio );
                return hash;
            }
        }  // namespace



        VehicleMaterialSlot WPVehicleGenerator::materialSlotFor( VehicleMaterial material )
        {
            // Keep one canonical mapping. In particular, rims and suspension had
            // drifted to different slots in the facade and appearance APIs.
            return WPVehicleAppearance::slotForGeometryMaterial( material );
        }

        GeneratedVehicle WPVehicleGenerator::generate( const VehicleGenerationConfig &config )
        {
            GeneratedVehicle result;
            result.physics = WPVehiclePhysics::generate( config.physicsPreset, config.seed );

            VehicleGeometryConfig geometryConfig = config.geometry;
            geometryConfig.seed = config.seed;
            if( config.synchronizeFromPhysics )
                applyPhysicsDimensions( geometryConfig, result.physics );
            else
                applyGeometryDimensions( result.physics, geometryConfig );
            result.geometry = WPVehicleGeometry::generate( geometryConfig );
            const float physicsFrontAxleZ =
                static_cast<float>( 0.5 * ( result.physics.wheels[0].hubPosition.z +
                                            result.physics.wheels[1].hubPosition.z ) );
            translateGeometryZ( result.geometry, physicsFrontAxleZ - result.geometry.frontAxle.z );
            result.geometry.centreOfMass =
                Vector3F( static_cast<float>( result.physics.massProperties.centreOfMass.x ),
                          static_cast<float>( result.physics.massProperties.centreOfMass.y ),
                          static_cast<float>( result.physics.massProperties.centreOfMass.z ) );
            applyAppearanceUVContract( result.geometry );

            VehicleAppearanceConfig appearanceConfig = config.appearance;
            appearanceConfig.seed = config.seed;
            // The appearance builder combines its constructor seed and config seed.
            // Supplying config.seed to both XORed the seed with itself, making every
            // integrated vehicle share seed zero.
            result.appearance = WPVehicleAppearance( 0u ).build( appearanceConfig );
            result.issues = validate( result );
            if( !config.synchronizeFromPhysics )
                addIssue( result.issues, true, "integration",
                          "synchronizeFromPhysics=false changes dimensions without consistently "
                          "rescaling mass elements, suspension or aerodynamic data; generate "
                          "custom standalone geometry instead" );
            const VehicleAppearanceValidation requestedAppearance =
                WPVehicleAppearance::validate( config.appearance );
            for( const auto &issue : requestedAppearance.issues )
                addIssue( result.issues, issue.severity == VehicleAppearanceIssueSeverity::Error,
                          "appearance-config", issue.code + ": " + issue.message );

            std::uint64_t hash = 14695981039346656037ull;
            hash = fnvU64( hash, result.appearance.contentHash );
            hash = hashPhysicsConfig( hash, result.physics );
            hash = fnvVector3( hash, result.geometry.boundsMin );
            hash = fnvVector3( hash, result.geometry.boundsMax );
            hash = fnvVector3( hash, result.geometry.centreOfMass );
            hash = fnvVector3( hash, result.geometry.frontAxle );
            hash = fnvVector3( hash, result.geometry.rearAxle );
            for( const auto &lod : result.geometry.lods )
            {
                hash = fnvU32( hash, lod.level );
                hash = fnvFloat( hash, lod.suggestedScreenCoverage );
                for( const auto &section : lod.sections )
                {
                    hash = fnvU64( hash, static_cast<std::uint64_t>( section.name.size() ) );
                    hash = fnvBytes( hash, section.name.data(), section.name.size() );
                    hash = fnvU32( hash, static_cast<std::uint32_t>( section.material ) );
                    const auto vertexCount = section.vertices.size();
                    const auto indexCount = section.indices.size();
                    hash = fnvU64( hash, static_cast<std::uint64_t>( vertexCount ) );
                    hash = fnvU64( hash, static_cast<std::uint64_t>( indexCount ) );
                    for( const auto &vertex : section.vertices )
                        hash = hashVehicleVertex( hash, vertex );
                    for( const std::uint32_t index : section.indices )
                        hash = fnvU32( hash, index );
                }
            }
            result.contentHash = hash;
            return result;
        }

        std::vector<VehicleGenerationIssue> WPVehicleGenerator::validate(
            const GeneratedVehicle &vehicle )
        {
            std::vector<VehicleGenerationIssue> issues;
            const auto appearance = WPVehicleAppearance::validate( vehicle.appearance );
            for( const auto &issue : appearance.issues )
                addIssue( issues, issue.severity == VehicleAppearanceIssueSeverity::Error, "appearance",
                          issue.code + ": " + issue.message );

            const auto physics = WPVehiclePhysics::validate( vehicle.physics );
            for( const auto &message : physics.errors )
                addIssue( issues, true, "physics", message );
            for( const auto &message : physics.warnings )
                addIssue( issues, false, "physics", message );

            if( vehicle.geometry.lods.empty() || !vehicle.geometry.hasGeometry() )
                addIssue( issues, true, "geometry", "no renderable LOD geometry was produced" );

            std::uint32_t expectedLevel = 0;
            float priorCoverage = std::numeric_limits<float>::infinity();
            for( const auto &lod : vehicle.geometry.lods )
            {
                if( lod.level != expectedLevel++ )
                    addIssue( issues, true, "geometry", "LOD levels must be contiguous" );
                if( lod.suggestedScreenCoverage >= priorCoverage )
                    addIssue( issues, true, "geometry", "LOD screen coverage must decrease" );
                priorCoverage = lod.suggestedScreenCoverage;

                for( const auto &section : lod.sections )
                {
                    if( static_cast<std::uint32_t>( section.material ) >
                        static_cast<std::uint32_t>( VehicleMaterial::Suspension ) )
                        addIssue( issues, true, "geometry",
                                  section.name + " uses an unknown material identifier" );
                    if( !section.hasGeometry() )
                        addIssue( issues, true, "geometry",
                                  section.name + " is an empty or non-renderable material section" );
                    if( section.indices.size() % 3u != 0u )
                        addIssue( issues, true, "geometry",
                                  section.name + " has a non-triangular index tail" );
                    for( const auto index : section.indices )
                    {
                        if( index >= section.vertices.size() )
                        {
                            addIssue( issues, true, "geometry",
                                      section.name + " contains an out-of-range index" );
                            break;
                        }
                    }
                    for( const auto &vertex : section.vertices )
                    {
                        const bool allFinite =
                            finite( vertex.position.x ) && finite( vertex.position.y ) &&
                            finite( vertex.position.z ) && finite( vertex.normal.x ) &&
                            finite( vertex.normal.y ) && finite( vertex.normal.z ) &&
                            finite( vertex.tangent.x ) && finite( vertex.tangent.y ) &&
                            finite( vertex.tangent.z ) && finite( vertex.uv.x ) &&
                            finite( vertex.uv.y ) && finite( vertex.mask.x ) &&
                            finite( vertex.mask.y ) && finite( vertex.mask.z ) &&
                            finite( vertex.tangentSign );
                        const float normalLength = lengthSquared( vertex.normal );
                        const float tangentLength = lengthSquared( vertex.tangent );
                        if( !allFinite || normalLength < 0.98f || normalLength > 1.02f ||
                            tangentLength < 0.98f || tangentLength > 1.02f ||
                            std::abs( dot( vertex.normal, vertex.tangent ) ) > 0.02f ||
                            std::abs( std::abs( vertex.tangentSign ) - 1.0f ) > 0.001f )
                        {
                            addIssue( issues, true, "geometry",
                                      section.name + " has a non-finite or invalid tangent frame" );
                            break;
                        }
                    }
                    bool degenerateTriangle = false;
                    for( std::size_t i = 0; i + 2u < section.indices.size(); i += 3u )
                    {
                        const std::uint32_t ia = section.indices[i];
                        const std::uint32_t ib = section.indices[i + 1u];
                        const std::uint32_t ic = section.indices[i + 2u];
                        if( ia >= section.vertices.size() || ib >= section.vertices.size() ||
                            ic >= section.vertices.size() )
                            continue;
                        const Vector3F edgeA =
                            section.vertices[ib].position - section.vertices[ia].position;
                        const Vector3F edgeB =
                            section.vertices[ic].position - section.vertices[ia].position;
                        if( lengthSquared( edgeA.crossProduct( edgeB ) ) < 1.0e-14f )
                        {
                            degenerateTriangle = true;
                            break;
                        }
                    }
                    if( degenerateTriangle )
                        addIssue( issues, true, "geometry",
                                  section.name + " contains a zero-area triangle" );
                }
            }

            const Vector3F &boundsMin = vehicle.geometry.boundsMin;
            const Vector3F &boundsMax = vehicle.geometry.boundsMax;
            if( !finite( boundsMin.x ) || !finite( boundsMin.y ) || !finite( boundsMin.z ) ||
                !finite( boundsMax.x ) || !finite( boundsMax.y ) || !finite( boundsMax.z ) ||
                boundsMin.x > boundsMax.x || boundsMin.y > boundsMax.y || boundsMin.z > boundsMax.z )
                addIssue( issues, true, "geometry", "declared bounds are invalid" );
            else
            {
                constexpr float boundsTolerance = 0.002f;
                bool escapedBounds = false;
                for( const auto &lod : vehicle.geometry.lods )
                    for( const auto &section : lod.sections )
                        for( const auto &vertex : section.vertices )
                            if( vertex.position.x < boundsMin.x - boundsTolerance ||
                                vertex.position.y < boundsMin.y - boundsTolerance ||
                                vertex.position.z < boundsMin.z - boundsTolerance ||
                                vertex.position.x > boundsMax.x + boundsTolerance ||
                                vertex.position.y > boundsMax.y + boundsTolerance ||
                                vertex.position.z > boundsMax.z + boundsTolerance )
                            {
                                escapedBounds = true;
                                break;
                            }
                if( escapedBounds )
                    addIssue( issues, true, "geometry",
                              "one or more vertices lie outside declared bounds" );
            }

            const double meshWheelbase = std::abs( static_cast<double>( vehicle.geometry.rearAxle.z ) -
                                                   vehicle.geometry.frontAxle.z );
            if( std::abs( meshWheelbase - vehicle.physics.wheelbaseM ) > 0.01 )
                addIssue( issues, true, "integration",
                          "render and physics wheelbases differ by more than 10 mm" );
            if( vehicle.physics.preset != VehiclePhysicsPreset::GrandPrix )
                addIssue( issues, true, "integration",
                          "the current geometry generator authors an open-wheel Grand Prix car; "
                          "GT and RoadSport physics presets require a different body archetype" );
            const double hubTolerance = 0.01;
            if( std::abs( static_cast<double>( vehicle.geometry.frontAxle.z ) -
                          vehicle.physics.wheels[0].hubPosition.z ) > hubTolerance ||
                std::abs( static_cast<double>( vehicle.geometry.rearAxle.z ) -
                          vehicle.physics.wheels[2].hubPosition.z ) > hubTolerance )
                addIssue( issues, true, "integration",
                          "render and physics axle centres differ by more than 10 mm" );
            if( std::abs( static_cast<double>( vehicle.geometry.frontAxle.y ) -
                          vehicle.physics.wheels[0].tire.radiusM ) > hubTolerance ||
                std::abs( static_cast<double>( vehicle.geometry.rearAxle.y ) -
                          vehicle.physics.wheels[2].tire.radiusM ) > hubTolerance )
                addIssue( issues, true, "integration",
                          "render wheel radius and physics tyre radius differ by more than 10 mm" );
            const double expectedHalfWidth =
                0.5 * std::max( vehicle.physics.frontTrackM + vehicle.physics.wheels[0].tire.widthM,
                                vehicle.physics.rearTrackM + vehicle.physics.wheels[2].tire.widthM );
            if( std::abs( static_cast<double>( vehicle.geometry.boundsMax.x ) - expectedHalfWidth ) >
                    hubTolerance ||
                std::abs( static_cast<double>( vehicle.geometry.boundsMin.x ) + expectedHalfWidth ) >
                    hubTolerance )
                addIssue( issues, true, "integration",
                          "render and physics track/tyre envelope differ by more than 10 mm" );
            const auto &physicalCom = vehicle.physics.massProperties.centreOfMass;
            if( std::abs( static_cast<double>( vehicle.geometry.centreOfMass.x ) - physicalCom.x ) >
                    0.001 ||
                std::abs( static_cast<double>( vehicle.geometry.centreOfMass.y ) - physicalCom.y ) >
                    0.001 ||
                std::abs( static_cast<double>( vehicle.geometry.centreOfMass.z ) - physicalCom.z ) >
                    0.001 )
                addIssue( issues, true, "integration",
                          "render and physics centre-of-mass markers differ by more than 1 mm" );
            return issues;
        }
    }  // namespace procedural
}  // namespace workphone
