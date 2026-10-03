#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehiclePhysics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr double Pi = 3.1415926535897932384626433832795;

            struct DeterministicRandom
            {
                explicit DeterministicRandom( std::uint64_t seed ) : state( seed )
                {
                }

                double signedUnit()
                {
                    // SplitMix64: platform-independent integer path, then exactly 53 bits.
                    state += 0x9e3779b97f4a7c15ULL;
                    std::uint64_t z = state;
                    z = ( z ^ ( z >> 30U ) ) * 0xbf58476d1ce4e5b9ULL;
                    z = ( z ^ ( z >> 27U ) ) * 0x94d049bb133111ebULL;
                    z ^= z >> 31U;
                    const double unit = static_cast<double>( z >> 11U ) * ( 1.0 / 9007199254740992.0 );
                    return 2.0 * unit - 1.0;
                }

                std::uint64_t state;
            };

            bool finite( double value )
            {
                return std::isfinite( value );
            }

            double clamp( double value, double lo, double hi )
            {
                return std::max( lo, std::min( value, hi ) );
            }

            double sqr( double value )
            {
                return value * value;
            }

            VehicleMassElement box( const char *name, double mass, double x, double y, double z,
                                    double width, double height, double length )
            {
                VehicleMassElement result;
                result.name = name;
                result.massKg = mass;
                result.centre = { x, y, z };
                result.dimensions = { width, height, length };
                return result;
            }

            VehicleTireConfig tire( double radius, double width, double mass, double mu,
                                    double corneringStiffness, double verticalStiffness )
            {
                VehicleTireConfig result;
                result.radiusM = radius;
                result.unloadedRadiusM = radius + 0.0065;
                result.widthM = width;
                result.wheelMassKg = mass;
                // Conservative hoop + solid-disc blend for tyre, rim, brake and upright.
                result.wheelInertiaKgM2 = 0.72 * mass * radius * radius;
                result.verticalStiffnessNPerM = verticalStiffness;
                result.longitudinalStiffnessNPerSlip = corneringStiffness * 1.08;
                result.corneringStiffnessNPerRad = corneringStiffness;
                result.peakLongitudinalFriction = mu;
                result.peakLateralFriction = mu * 1.03;
                result.rollingResistance = 0.012;
                result.optimalTemperatureC = 92.0;
                result.coldFrictionScale = 0.72;
                result.loadSensitivity = 0.12;
                return result;
            }

            VehicleSuspensionConfig suspension( double spring, double bumpDamper, double reboundDamper,
                                                double antiRoll, double bumpTravel, double reboundTravel,
                                                double motionRatio )
            {
                VehicleSuspensionConfig result;
                result.springRateNPerM = spring;
                result.damperCompressionNPerMps = bumpDamper;
                result.damperReboundNPerMps = reboundDamper;
                result.antiRollRateNmPerRad = antiRoll;
                result.bumpTravelM = bumpTravel;
                result.reboundTravelM = reboundTravel;
                result.motionRatio = motionRatio;
                result.staticCamberRad = -2.2 * Pi / 180.0;
                result.staticToeRad = 0.08 * Pi / 180.0;
                result.casterRad = 6.5 * Pi / 180.0;
                result.kingpinInclinationRad = 8.0 * Pi / 180.0;
                return result;
            }

            void placeWheels( VehiclePhysicsConfig &c, double frontAxleZ, double rearAxleZ,
                              double frontRadius, double rearRadius )
            {
                c.wheels[0].hubPosition = { -0.5 * c.frontTrackM, frontRadius, frontAxleZ };
                c.wheels[1].hubPosition = { 0.5 * c.frontTrackM, frontRadius, frontAxleZ };
                c.wheels[2].hubPosition = { -0.5 * c.rearTrackM, rearRadius, rearAxleZ };
                c.wheels[3].hubPosition = { 0.5 * c.rearTrackM, rearRadius, rearAxleZ };
                c.wheels[0].steerable = c.wheels[1].steerable = true;
            }

            void configureDrivenWheels( VehiclePhysicsConfig &c )
            {
                for( auto &wheel : c.wheels )
                    wheel.driven = false;
                if( c.drivetrain.drivenAxle != DrivenAxle::Rear )
                    c.wheels[0].driven = c.wheels[1].driven = true;
                if( c.drivetrain.drivenAxle != DrivenAxle::Front )
                    c.wheels[2].driven = c.wheels[3].driven = true;
            }

            void addError( VehiclePhysicsValidation &result, const std::string &text )
            {
                result.errors.push_back( text );
            }

            void requirePositive( VehiclePhysicsValidation &result, double value, const char *label )
            {
                if( !finite( value ) || value <= 0.0 )
                    addError( result, std::string( label ) + " must be finite and positive" );
            }
        }  // namespace

        VehiclePhysicsConfig WPVehiclePhysics::generate( VehiclePhysicsPreset preset,
                                                         std::uint64_t seed )
        {
            VehiclePhysicsConfig c;
            c.seed = seed;
            c.preset = preset;
            DeterministicRandom rng( seed ^
                                     ( 0x8d58ac26afe12e47ULL + static_cast<std::uint64_t>( preset ) ) );

            if( preset == VehiclePhysicsPreset::GrandPrix )
            {
                // Geometry is aligned with apex-gp/src/car: axles -1.62/+1.98 m,
                // 1.62/1.56 m tracks and 305/405-720R18 tyres.
                c.wheelbaseM = 3.60;
                c.frontTrackM = 1.62;
                c.rearTrackM = 1.56;
                c.bodyLengthM = 5.45;
                c.bodyWidthM = 2.00;
                c.massElements = {
                    box( "survival cell", 286.0, 0.0, 0.315, -0.18, 1.22, 0.48, 2.70 ),
                    box( "power unit", 151.0, 0.0, 0.310, 1.10, 0.64, 0.52, 0.88 ),
                    box( "gearbox and differential", 64.0, 0.0, 0.275, 1.72, 0.58, 0.42, 0.62 ),
                    box( "driver and seat", 82.0, 0.0, 0.490, -0.15, 0.46, 0.72, 0.92 ),
                    box( "fuel", 92.0, 0.0, 0.285, 0.52, 0.58, 0.42, 0.62 ),
                    box( "front aero and crash structure", 42.0, 0.0, 0.155, -2.23, 1.82, 0.18, 1.08 ),
                    box( "rear aero and crash structure", 35.0, 0.0, 0.590, 2.15, 1.04, 0.56, 0.54 ),
                    box( "front running gear", 22.0, 0.0, 0.300, -1.62, 1.45, 0.25, 0.34 ),
                    box( "rear running gear", 24.0, 0.0, 0.300, 1.98, 1.40, 0.25, 0.34 )
                };  // 798 kg including driver and nominal fuel.
                const auto frontTire = tire( 0.360, 0.305, 18.5, 1.78, 186000.0, 355000.0 );
                const auto rearTire = tire( 0.360, 0.405, 21.5, 1.82, 205000.0, 385000.0 );
                const auto frontSusp =
                    suspension( 195000.0, 6200.0, 9300.0, 62000.0, 0.032, 0.044, 0.82 );
                const auto rearSusp =
                    suspension( 175000.0, 5900.0, 8700.0, 54000.0, 0.038, 0.048, 0.85 );
                for( std::size_t i = 0; i < 4; ++i )
                {
                    c.wheels[i].tire = i < 2 ? frontTire : rearTire;
                    c.wheels[i].suspension = i < 2 ? frontSusp : rearSusp;
                    c.wheels[i].brakeTorqueNm = i < 2 ? 3150.0 : 2280.0;
                    c.wheels[i].maxSteerRad = i < 2 ? 0.38 : 0.0;
                }
                placeWheels( c, -1.62, 1.98, 0.3535, 0.3535 );
                c.aero = { 1.50, 0.91, 3.85, 0.455, 1.225, { 0.0, 0.12, 0.30 }, 0.72, 0.055, 0.42 };
                c.drivetrain = {
                    DrivenAxle::Rear, 745000.0, 760.0, 4200.0,
                    15000.0,          3.18,     0.965, { 3.20, 2.42, 1.91, 1.58, 1.35, 1.18, 1.05, 0.94 }
                };
            }
            else if( preset == VehiclePhysicsPreset::GT )
            {
                c.wheelbaseM = 2.78;
                c.frontTrackM = 1.66;
                c.rearTrackM = 1.64;
                c.bodyLengthM = 4.73;
                c.bodyWidthM = 2.01;
                c.massElements = {
                    box( "body shell and cage", 585.0, 0.0, 0.48, 0.02, 1.78, 0.82, 3.72 ),
                    box( "power unit", 214.0, 0.0, 0.43, -0.82, 0.74, 0.61, 0.84 ),
                    box( "transaxle", 96.0, 0.0, 0.36, 1.02, 0.62, 0.46, 0.62 ),
                    box( "driver and seat", 92.0, -0.22, 0.57, -0.12, 0.48, 0.76, 0.82 ),
                    box( "fuel", 94.0, 0.0, 0.37, 0.66, 0.62, 0.38, 0.58 ),
                    box( "aero and safety systems", 128.0, 0.0, 0.43, 0.12, 1.86, 0.42, 4.10 ),
                    box( "running gear", 116.0, 0.0, 0.40, 0.06, 1.61, 0.32, 2.78 )
                };
                const auto frontTire = tire( 0.335, 0.300, 23.0, 1.47, 142000.0, 285000.0 );
                const auto rearTire = tire( 0.340, 0.320, 25.0, 1.50, 151000.0, 300000.0 );
                const auto frontSusp =
                    suspension( 112000.0, 5100.0, 7500.0, 43000.0, 0.054, 0.066, 0.91 );
                const auto rearSusp =
                    suspension( 105000.0, 4900.0, 7200.0, 39000.0, 0.058, 0.070, 0.93 );
                for( std::size_t i = 0; i < 4; ++i )
                {
                    c.wheels[i].tire = i < 2 ? frontTire : rearTire;
                    c.wheels[i].suspension = i < 2 ? frontSusp : rearSusp;
                    c.wheels[i].brakeTorqueNm = i < 2 ? 5100.0 : 3900.0;
                    c.wheels[i].maxSteerRad = i < 2 ? 0.55 : 0.0;
                }
                placeWheels( c, -1.39, 1.39, 0.327, 0.332 );
                c.aero = { 2.05, 0.43, 1.18, 0.46, 1.225, { 0.0, 0.26, 0.10 }, 0.54, 0.085, 0.22 };
                c.drivetrain = {
                    DrivenAxle::Rear, 430000.0, 650.0, 1100.0,
                    8500.0,           3.73,     0.94,  { 2.92, 2.12, 1.67, 1.35, 1.13, 0.97 }
                };
            }
            else
            {
                c.wheelbaseM = 2.64;
                c.frontTrackM = 1.58;
                c.rearTrackM = 1.57;
                c.bodyLengthM = 4.46;
                c.bodyWidthM = 1.88;
                c.massElements = { box( "body in white", 720.0, 0.0, 0.55, 0.02, 1.72, 0.92, 3.72 ),
                                   box( "power unit", 205.0, 0.0, 0.47, -0.91, 0.72, 0.63, 0.78 ),
                                   box( "transmission", 92.0, 0.0, 0.42, -0.43, 0.58, 0.45, 0.60 ),
                                   box( "occupants", 150.0, 0.0, 0.73, -0.02, 1.02, 0.78, 0.76 ),
                                   box( "fuel and fluids", 72.0, 0.0, 0.42, 0.76, 0.58, 0.39, 0.51 ),
                                   box( "interior and glazing", 135.0, 0.0, 0.77, 0.10, 1.60, 0.64,
                                        2.25 ),
                                   box( "running gear", 106.0, 0.0, 0.43, 0.05, 1.52, 0.34, 2.64 ) };
                const auto commonTire = tire( 0.335, 0.265, 22.0, 1.18, 108000.0, 245000.0 );
                const auto frontSusp =
                    suspension( 52000.0, 3400.0, 5100.0, 26000.0, 0.074, 0.090, 0.94 );
                const auto rearSusp = suspension( 48000.0, 3200.0, 4800.0, 23000.0, 0.078, 0.094, 0.95 );
                for( std::size_t i = 0; i < 4; ++i )
                {
                    c.wheels[i].tire = commonTire;
                    c.wheels[i].suspension = i < 2 ? frontSusp : rearSusp;
                    c.wheels[i].brakeTorqueNm = i < 2 ? 4200.0 : 2900.0;
                    c.wheels[i].handbrakeTorqueNm = i >= 2 ? 3300.0 : 0.0;
                    c.wheels[i].maxSteerRad = i < 2 ? 0.62 : 0.0;
                }
                placeWheels( c, -1.32, 1.32, 0.327, 0.327 );
                c.aero = { 2.10, 0.34, 0.18, 0.49, 1.225, { 0.0, 0.50, -0.02 }, 0.34, 0.125, 0.08 };
                c.drivetrain = {
                    DrivenAxle::All, 331000.0, 560.0, 850.0,
                    7200.0,          3.91,     0.91,  { 3.13, 2.05, 1.48, 1.16, 0.94, 0.79 }
                };
            }

            // Seeded factory/setup variation is deliberately tiny and bilateral so it does
            // not create artificial left/right imbalance or alter homologated dimensions.
            const double springScale = 1.0 + 0.008 * rng.signedUnit();
            const double damperScale = 1.0 + 0.012 * rng.signedUnit();
            for( auto &wheel : c.wheels )
            {
                wheel.suspension.springRateNPerM *= springScale;
                wheel.suspension.damperCompressionNPerMps *= damperScale;
                wheel.suspension.damperReboundNPerMps *= damperScale;
            }
            recomputeDerivedProperties( c );
            return c;
        }

        void WPVehiclePhysics::recomputeDerivedProperties( VehiclePhysicsConfig &c )
        {
            VehicleMassProperties mass;
            for( const auto &element : c.massElements )
            {
                mass.massKg += element.massKg;
                mass.centreOfMass.x += element.massKg * element.centre.x;
                mass.centreOfMass.y += element.massKg * element.centre.y;
                mass.centreOfMass.z += element.massKg * element.centre.z;
            }
            if( mass.massKg > 0.0 )
            {
                mass.centreOfMass.x /= mass.massKg;
                mass.centreOfMass.y /= mass.massKg;
                mass.centreOfMass.z /= mass.massKg;
            }

            for( const auto &e : c.massElements )
            {
                const double dx = e.centre.x - mass.centreOfMass.x;
                const double dy = e.centre.y - mass.centreOfMass.y;
                const double dz = e.centre.z - mass.centreOfMass.z;
                const double localX =
                    e.massKg * ( sqr( e.dimensions.y ) + sqr( e.dimensions.z ) ) / 12.0;
                const double localY =
                    e.massKg * ( sqr( e.dimensions.x ) + sqr( e.dimensions.z ) ) / 12.0;
                const double localZ =
                    e.massKg * ( sqr( e.dimensions.x ) + sqr( e.dimensions.y ) ) / 12.0;
                mass.inertia.xx += localX + e.massKg * ( dy * dy + dz * dz );
                mass.inertia.yy += localY + e.massKg * ( dx * dx + dz * dz );
                mass.inertia.zz += localZ + e.massKg * ( dx * dx + dy * dy );
                mass.inertia.xy -= e.massKg * dx * dy;
                mass.inertia.xz -= e.massKg * dx * dz;
                mass.inertia.yz -= e.massKg * dy * dz;
            }

            const double frontZ = 0.5 * ( c.wheels[0].hubPosition.z + c.wheels[1].hubPosition.z );
            const double rearZ = 0.5 * ( c.wheels[2].hubPosition.z + c.wheels[3].hubPosition.z );
            const double axleSpan = rearZ - frontZ;
            mass.frontStaticWeightFraction =
                axleSpan > 0.0 ? clamp( ( rearZ - mass.centreOfMass.z ) / axleSpan, 0.0, 1.0 ) : 0.5;
            c.massProperties = mass;
            configureDrivenWheels( c );
        }

        VehiclePhysicsValidation WPVehiclePhysics::validate( const VehiclePhysicsConfig &c )
        {
            VehiclePhysicsValidation result;
            requirePositive( result, c.wheelbaseM, "wheelbase" );
            requirePositive( result, c.frontTrackM, "front track" );
            requirePositive( result, c.rearTrackM, "rear track" );
            requirePositive( result, c.massProperties.massKg, "mass" );
            requirePositive( result, c.gravityMps2, "gravity" );
            requirePositive( result, c.massProperties.inertia.xx, "pitch inertia" );
            requirePositive( result, c.massProperties.inertia.yy, "yaw inertia" );
            requirePositive( result, c.massProperties.inertia.zz, "roll inertia" );

            if( c.massProperties.inertia.xx + c.massProperties.inertia.yy <=
                    c.massProperties.inertia.zz ||
                c.massProperties.inertia.xx + c.massProperties.inertia.zz <=
                    c.massProperties.inertia.yy ||
                c.massProperties.inertia.yy + c.massProperties.inertia.zz <=
                    c.massProperties.inertia.xx )
                addError( result, "inertia principal-axis triangle inequality is violated" );

            const auto &tensor = c.massProperties.inertia;
            const double tensorDeterminant =
                tensor.xx * tensor.yy * tensor.zz - tensor.xx * tensor.yz * tensor.yz -
                tensor.yy * tensor.xz * tensor.xz - tensor.zz * tensor.xy * tensor.xy +
                2.0 * tensor.xy * tensor.xz * tensor.yz;
            if( !finite( tensorDeterminant ) || tensor.xx * tensor.yy - sqr( tensor.xy ) <= 0.0 ||
                tensorDeterminant <= 0.0 )
                addError( result, "inertia tensor must be symmetric positive definite" );

            for( const auto &element : c.massElements )
            {
                if( element.name.empty() )
                    result.warnings.push_back( "mass element has no diagnostic name" );
                if( !finite( element.massKg ) || element.massKg <= 0.0 || !finite( element.centre.x ) ||
                    !finite( element.centre.y ) || !finite( element.centre.z ) ||
                    element.dimensions.x <= 0.0 || element.dimensions.y <= 0.0 ||
                    element.dimensions.z <= 0.0 )
                    addError( result, "mass element '" + element.name +
                                          "' has invalid mass, position or dimensions" );
            }

            VehiclePhysicsConfig derived = c;
            recomputeDerivedProperties( derived );
            const double massTolerance = std::max( 0.001, derived.massProperties.massKg * 1.0e-6 );
            if( std::abs( derived.massProperties.massKg - c.massProperties.massKg ) > massTolerance ||
                std::abs( derived.massProperties.centreOfMass.x - c.massProperties.centreOfMass.x ) >
                    1.0e-6 ||
                std::abs( derived.massProperties.centreOfMass.y - c.massProperties.centreOfMass.y ) >
                    1.0e-6 ||
                std::abs( derived.massProperties.centreOfMass.z - c.massProperties.centreOfMass.z ) >
                    1.0e-6 )
                addError( result, "derived mass properties are stale; call recomputeDerivedProperties" );

            if( c.massProperties.frontStaticWeightFraction < 0.25 ||
                c.massProperties.frontStaticWeightFraction > 0.75 )
                addError( result, "centre of mass lies outside a credible axle load envelope" );
            if( c.massProperties.centreOfMass.y <= 0.0 || c.massProperties.centreOfMass.y > 1.2 )
                addError( result, "centre-of-mass height is outside the supported range" );

            const double frontZ = 0.5 * ( c.wheels[0].hubPosition.z + c.wheels[1].hubPosition.z );
            const double rearZ = 0.5 * ( c.wheels[2].hubPosition.z + c.wheels[3].hubPosition.z );
            if( std::abs( ( rearZ - frontZ ) - c.wheelbaseM ) > 0.002 )
                addError( result, "wheel hub positions do not match configured wheelbase" );
            if( std::abs( ( c.wheels[1].hubPosition.x - c.wheels[0].hubPosition.x ) - c.frontTrackM ) >
                    0.002 ||
                std::abs( ( c.wheels[3].hubPosition.x - c.wheels[2].hubPosition.x ) - c.rearTrackM ) >
                    0.002 )
                addError( result, "wheel hub positions do not match configured track widths" );

            for( std::size_t i = 0; i < c.wheels.size(); ++i )
            {
                const auto &w = c.wheels[i];
                const std::string prefix = "wheel " + std::to_string( i ) + ": ";
                if( !finite( w.hubPosition.x ) || !finite( w.hubPosition.y ) ||
                    !finite( w.hubPosition.z ) )
                    addError( result, prefix + "hub position must be finite" );
                if( w.tire.radiusM <= 0.1 || w.tire.widthM <= 0.05 ||
                    w.tire.unloadedRadiusM < w.tire.radiusM )
                    addError( result, prefix + "tire dimensions are invalid" );
                requirePositive( result, w.tire.wheelMassKg, "wheel mass" );
                requirePositive( result, w.tire.wheelInertiaKgM2, "wheel inertia" );
                requirePositive( result, w.tire.verticalStiffnessNPerM, "tire vertical stiffness" );
                requirePositive( result, w.suspension.springRateNPerM, "suspension spring rate" );
                requirePositive( result, w.suspension.damperCompressionNPerMps, "compression damping" );
                requirePositive( result, w.suspension.damperReboundNPerMps, "rebound damping" );
                if( w.suspension.motionRatio <= 0.0 || w.suspension.motionRatio > 1.5 )
                    addError( result, prefix + "motion ratio is outside (0, 1.5]" );
                if( w.tire.peakLongitudinalFriction < 0.5 || w.tire.peakLongitudinalFriction > 2.5 )
                    addError( result, prefix + "peak friction is outside supported range" );

                const double cornerMass =
                    staticWheelLoadN( c, static_cast<WheelCorner>( i ) ) / c.gravityMps2;
                const double wheelRate = w.suspension.springRateNPerM * sqr( w.suspension.motionRatio );
                const double naturalHz =
                    std::sqrt( wheelRate / std::max( 1.0, cornerMass ) ) / ( 2.0 * Pi );
                if( naturalHz < 1.0 || naturalHz > 6.5 )
                {
                    std::ostringstream message;
                    message << prefix << "ride frequency " << naturalHz
                            << " Hz is outside the 1.0-6.5 Hz supported envelope";
                    result.warnings.push_back( message.str() );
                }
            }

            requirePositive( result, c.aero.referenceAreaM2, "aero reference area" );
            if( !finite( c.aero.dragCoefficient ) || c.aero.dragCoefficient < 0.0 )
                addError( result, "drag coefficient must be finite and non-negative" );
            if( !finite( c.aero.downforceCoefficient ) || c.aero.downforceCoefficient < 0.0 )
                addError( result, "downforce coefficient must be finite and non-negative" );
            if( c.aero.frontDownforceFraction < 0.0 || c.aero.frontDownforceFraction > 1.0 )
                addError( result, "front aero balance must lie in [0, 1]" );
            requirePositive( result, c.aero.airDensityKgPerM3, "air density" );
            requirePositive( result, c.drivetrain.peakPowerW, "peak power" );
            requirePositive( result, c.drivetrain.peakTorqueNm, "peak torque" );
            requirePositive( result, c.drivetrain.redlineRpm, "redline" );
            if( c.drivetrain.idleRpm < 0.0 || c.drivetrain.idleRpm >= c.drivetrain.redlineRpm )
                addError( result, "idle speed must be non-negative and below redline" );
            if( c.drivetrain.transmissionEfficiency <= 0.0 || c.drivetrain.transmissionEfficiency > 1.0 )
                addError( result, "transmission efficiency must lie in (0, 1]" );
            if( c.drivetrain.forwardGearRatios.empty() ||
                !std::all_of( c.drivetrain.forwardGearRatios.begin(),
                              c.drivetrain.forwardGearRatios.end(),
                              []( double ratio ) { return finite( ratio ) && ratio > 0.0; } ) )
                addError( result, "forward gear ratios must be finite, positive and non-empty" );
            return result;
        }

        double WPVehiclePhysics::staticWheelLoadN( const VehiclePhysicsConfig &c, WheelCorner corner )
        {
            const std::size_t index = static_cast<std::size_t>( corner );
            if( index >= c.wheels.size() )
                return 0.0;
            const bool front = index < 2;
            const double axleFraction = front ? c.massProperties.frontStaticWeightFraction
                                              : 1.0 - c.massProperties.frontStaticWeightFraction;
            return 0.5 * c.massProperties.massKg * c.gravityMps2 * axleFraction;
        }

        double WPVehiclePhysics::aerodynamicDragN( const VehiclePhysicsConfig &c, double speedMps )
        {
            const double speed = std::max( 0.0, speedMps );
            return 0.5 * c.aero.airDensityKgPerM3 * c.aero.referenceAreaM2 * c.aero.dragCoefficient *
                   speed * speed;
        }

        double WPVehiclePhysics::aerodynamicDownforceN( const VehiclePhysicsConfig &c, double speedMps,
                                                        double rideHeightM )
        {
            const double speed = std::max( 0.0, speedMps );
            double groundMultiplier = 1.0;
            if( c.aero.groundEffectRideHeightM > 0.0 )
            {
                const double rideDelta =
                    ( c.aero.groundEffectRideHeightM - std::max( 0.0, rideHeightM ) ) /
                    c.aero.groundEffectRideHeightM;
                groundMultiplier += c.aero.groundEffectSensitivity * clamp( rideDelta, -1.0, 1.0 );
            }
            return 0.5 * c.aero.airDensityKgPerM3 * c.aero.referenceAreaM2 *
                   c.aero.downforceCoefficient * speed * speed * groundMultiplier;
        }
    }  // namespace procedural
}  // namespace workphone
