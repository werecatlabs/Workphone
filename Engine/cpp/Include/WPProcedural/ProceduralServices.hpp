#pragma once
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <WPProcedural/WPRoadSystem.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <WPProcedural/WPSkyAtmosphere.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <WPProcedural/WPTextureForge.hpp>
#include <Workphone/Interface/Procedural/IVehicleAppearance.hpp>
#include <WPProcedural/WPVehicleAppearance.hpp>
#include <Workphone/Interface/Procedural/IVehicleDamage.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>
#include <Workphone/Interface/Procedural/IVehicleDynamics.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>
#include <Workphone/Interface/Procedural/IVehicleEffects.hpp>
#include <WPProcedural/WPVehicleEffects.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <WPProcedural/WPVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/IVehicleGeometry.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <Workphone/Interface/Procedural/IVehiclePhysics.hpp>
#include <WPProcedural/WPVehiclePhysics.hpp>
#include <Workphone/Interface/Procedural/IVehiclePresentation.hpp>
#include <WPProcedural/WPVehiclePresentation.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CRoadSystem : public IRoadSystem
    {
    public:
        void setSeed( u32 seed ) override
        {
            mImplementation.setSeed( seed );
        }
        real_Num getRoadWidth( RoadClass cls ) override
        {
            return WPRoadSystem::getRoadWidth( cls );
        }
        real_Num getSidewalkWidth( RoadClass cls ) override
        {
            return WPRoadSystem::getSidewalkWidth( cls );
        }
        u32 getLaneCount( RoadClass cls ) override
        {
            return WPRoadSystem::getLaneCount( cls );
        }
        RoadSurface getDefaultSurface( RoadClass cls ) override
        {
            return WPRoadSystem::getDefaultSurface( cls );
        }
        RoadSegmentResult generateSegment( const RoadSegmentSpec &spec ) override
        {
            return mImplementation.generateSegment( spec );
        }
        IntersectionResult generateIntersection( const IntersectionSpec &spec ) override
        {
            return mImplementation.generateIntersection( spec );
        }
        IntersectionResult buildXCrossing( const IntersectionSpec &spec ) override
        {
            return mImplementation.buildXCrossing( spec );
        }
        IntersectionResult buildRoundabout( const IntersectionSpec &spec ) override
        {
            return mImplementation.buildRoundabout( spec );
        }
        IntersectionResult buildTJunction( const IntersectionSpec &spec ) override
        {
            return mImplementation.buildTJunction( spec );
        }
        IntersectionResult buildLCorner( const IntersectionSpec &spec ) override
        {
            return mImplementation.buildLCorner( spec );
        }
        RoadMesh buildOldTarmacPatches( const RoadSegmentSpec &spec, real_Num width,
                                        real_Num length ) override
        {
            return mImplementation.buildOldTarmacPatches( spec, width, length );
        }
        RoadMesh buildPotholes( const RoadSegmentSpec &spec, real_Num width, real_Num length ) override
        {
            return mImplementation.buildPotholes( spec, width, length );
        }
        RoadMesh buildManholes( const RoadSegmentSpec &spec, real_Num width, real_Num length ) override
        {
            return mImplementation.buildManholes( spec, width, length );
        }
        RoadMesh buildGullyGrates( const RoadSegmentSpec &spec, real_Num width,
                                   real_Num length ) override
        {
            return mImplementation.buildGullyGrates( spec, width, length );
        }
        RoadMesh buildArrows( const RoadSegmentSpec &spec, real_Num width, real_Num length ) override
        {
            return mImplementation.buildArrows( spec, width, length );
        }
        RoadMesh buildPedestrianCrossing( const RoadSegmentSpec &spec, real_Num width,
                                          real_Num length ) override
        {
            return mImplementation.buildPedestrianCrossing( spec, width, length );
        }
        IntersectionResult buildIntersectionApproach( const IntersectionSpec &spec ) override
        {
            return mImplementation.buildIntersectionApproach( spec );
        }
        WP_CLASS_REGISTER_DECL;

    private:
        WPRoadSystem mImplementation;
    };
    class WPProcedural_API CSkyAtmosphere : public ISkyAtmosphere
    {
    public:
        void setState( const SkyState &state ) override
        {
            mImplementation.setState( state );
        }
        const SkyState &getState() const override
        {
            return mImplementation.getState();
        }
        void update() override
        {
            mImplementation.update();
        }
        const SkyResults &getResults() const override
        {
            return mImplementation.getResults();
        }
        void computeCelestialPositions( real_Num timeOfDay, real_Num dayOfYear, real_Num latitude,
                                        Vector3<real_Num> &outSunDir,
                                        Vector3<real_Num> &outMoonDir ) override
        {
            WPSkyAtmosphere::computeCelestialPositions( timeOfDay, dayOfYear, latitude, outSunDir,
                                                        outMoonDir );
        }
        Colour computeSkyColour( const Vector3<real_Num> &viewDir, const Vector3<real_Num> &sunDir,
                                 real_Num turbidity, real_Num sunAltitude ) override
        {
            return WPSkyAtmosphere::computeSkyColour( viewDir, sunDir, turbidity, sunAltitude );
        }
        Colour computeSunColour( real_Num sunAltitude, real_Num intensity ) override
        {
            return WPSkyAtmosphere::computeSunColour( sunAltitude, intensity );
        }
        Colour computeFogColour( real_Num sunAltitude ) override
        {
            return WPSkyAtmosphere::computeFogColour( sunAltitude );
        }
        WP_CLASS_REGISTER_DECL;

    private:
        WPSkyAtmosphere mImplementation;
    };
    class WPProcedural_API CTextureForge : public ITextureForge
    {
    public:
        void setSeed( u32 seed ) override
        {
            mImplementation = WPTextureForge( seed );
        }
        SurfaceBakeResult bakeSurface( SurfaceTag tag, const SurfaceBakeParams &params ) override
        {
            return mImplementation.bakeSurface( tag, params );
        }
        void bakeConcrete( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeConcrete( out, params );
        }
        void bakePlaster( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakePlaster( out, params );
        }
        void bakeBrick( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeBrick( out, params );
        }
        void bakeWood( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeWood( out, params );
        }
        void bakeMetal( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeMetal( out, params );
        }
        void bakeAsphalt( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeAsphalt( out, params );
        }
        void bakeSand( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeSand( out, params );
        }
        void bakeFabric( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeFabric( out, params );
        }
        void bakeFoliage( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeFoliage( out, params );
        }
        void bakeGlass( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeGlass( out, params );
        }
        void bakePaint( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakePaint( out, params );
        }
        void bakeRubber( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeRubber( out, params );
        }
        void bakeDirt( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeDirt( out, params );
        }
        void bakeStone( SurfaceBakeResult &out, const SurfaceBakeParams &params ) override
        {
            mImplementation.bakeStone( out, params );
        }
        void heightToNormal( const HeightBuffer &height, TextureBuffer &normalOut,
                             real_Num strength ) override
        {
            WPTextureForge::heightToNormal( height, normalOut, strength );
        }
        void computeORM( const HeightBuffer &height, TextureBuffer &ormOut, SurfaceTag tag ) override
        {
            WPTextureForge::computeORM( height, ormOut, tag );
        }
        void encodeSRGB( TextureBuffer &albedo ) override
        {
            WPTextureForge::encodeSRGB( albedo );
        }
        u32 getSeed() const override
        {
            return mImplementation.getSeed();
        }
        WP_CLASS_REGISTER_DECL;

    private:
        WPTextureForge mImplementation;
    };
    class WPProcedural_API CVehicleAppearance : public IVehicleAppearance
    {
    public:
        VehicleAppearanceBundle build( const VehicleAppearanceConfig &config ) const override
        {
            return mImplementation.build( config );
        }
        VehicleAppearanceQualityProfile profileFor( VehicleAppearanceQuality quality ) override
        {
            return WPVehicleAppearance::profileFor( quality );
        }
        VehicleMaterialSlot slotForGeometryMaterial( VehicleMaterial material ) override
        {
            return WPVehicleAppearance::slotForGeometryMaterial( material );
        }
        VehicleAppearanceValidation validate( const VehicleAppearanceConfig &config ) override
        {
            return WPVehicleAppearance::validate( config );
        }
        VehicleAppearanceValidation validate( const VehicleAppearanceBundle &bundle ) override
        {
            return WPVehicleAppearance::validate( bundle );
        }
        WP_CLASS_REGISTER_DECL;

    private:
        WPVehicleAppearance mImplementation;
    };
    class WPProcedural_API CVehicleDamage : public IVehicleDamage
    {
    public:
        VehicleDamageState reset( std::uint64_t seed ) override
        {
            return WPVehicleDamage::reset( seed );
        }
        VehicleDamageEvent registerImpact( const VehicleDamageConfig &config,
                                           const VehiclePhysicsConfig &physics,
                                           const VehicleDamageImpact &impact,
                                           VehicleDamageState &state ) override
        {
            return WPVehicleDamage::registerImpact( config, physics, impact, state );
        }
        bool update( const VehicleDamageConfig &config, double fixedDeltaSeconds,
                     VehicleDamageState &state ) override
        {
            return WPVehicleDamage::update( config, fixedDeltaSeconds, state );
        }
        VehicleDamageTelemetry telemetry( const VehicleDamageConfig &config,
                                          const VehicleDamageState &state ) override
        {
            return WPVehicleDamage::telemetry( config, state );
        }
        VehicleDynamicsModifiers dynamicsModifiers( const VehicleDamageTelemetry &telemetry ) override
        {
            return WPVehicleDamage::dynamicsModifiers( telemetry );
        }
        bool applyImpactResponse( const VehicleDamageEvent &event,
                                  VehicleDynamicsState &dynamics ) override
        {
            return WPVehicleDamage::applyImpactResponse( event, dynamics );
        }
        VehicleDamageValidation validate( const VehicleDamageConfig &config ) override
        {
            return WPVehicleDamage::validate( config );
        }
        VehicleDamageValidation validateState( const VehicleDamageState &state ) override
        {
            return WPVehicleDamage::validateState( state );
        }
        WP_CLASS_REGISTER_DECL;
    };
    class WPProcedural_API CVehicleDynamics : public IVehicleDynamics
    {
    public:
        VehicleDynamicsState reset( const VehiclePhysicsConfig &physics,
                                    const VehiclePhysicsVector3 &position, double yawRad ) override
        {
            return WPVehicleDynamics::reset( physics, position, yawRad );
        }
        bool stepFixed( const VehiclePhysicsConfig &physics, const VehicleDynamicsTuning &tuning,
                        const VehicleControlInput &input,
                        const std::array<VehicleSurfaceSample, 4> &surfaces, double fixedDeltaSeconds,
                        VehicleDynamicsState &state, VehicleDynamicsTelemetry &telemetry ) override
        {
            return WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, fixedDeltaSeconds,
                                                 state, telemetry );
        }
        bool stepFixed( const VehiclePhysicsConfig &physics, const VehicleDynamicsTuning &tuning,
                        const VehicleControlInput &input,
                        const std::array<VehicleSurfaceSample, 4> &surfaces,
                        const VehicleDynamicsModifiers &modifiers, double fixedDeltaSeconds,
                        VehicleDynamicsState &state, VehicleDynamicsTelemetry &telemetry ) override
        {
            return WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, modifiers,
                                                 fixedDeltaSeconds, state, telemetry );
        }
        VehicleDynamicsValidation validate( const VehiclePhysicsConfig &physics,
                                            const VehicleDynamicsTuning &tuning ) override
        {
            return WPVehicleDynamics::validate( physics, tuning );
        }
        VehicleDynamicsValidation validateState( const VehiclePhysicsConfig &physics,
                                                 const VehicleDynamicsState &state ) override
        {
            return WPVehicleDynamics::validateState( physics, state );
        }
        WP_CLASS_REGISTER_DECL;
    };
    class WPProcedural_API CVehicleEffects : public IVehicleEffects
    {
    public:
        VehicleEffectsState reset( std::uint64_t seed ) override
        {
            return WPVehicleEffects::reset( seed );
        }
        bool emit( const VehicleEffectConfig &config, const VehiclePhysicsConfig &physics,
                   const VehicleDynamicsState &dynamics, const VehicleDynamicsTelemetry &telemetry,
                   const VehiclePresentationState &presentation, const VehicleDamageTelemetry &damage,
                   const VehicleEffectEnvironment &environment, double fixedDeltaSeconds,
                   VehicleEffectsState &state, std::vector<VehicleEffectEvent> &events ) override
        {
            return WPVehicleEffects::emit( config, physics, dynamics, telemetry, presentation, damage,
                                           environment, fixedDeltaSeconds, state, events );
        }
        WP_CLASS_REGISTER_DECL;
    };
    class WPProcedural_API CVehicleGenerator : public IVehicleGenerator
    {
    public:
        GeneratedVehicle generate( const VehicleGenerationConfig &config ) override
        {
            return WPVehicleGenerator::generate( config );
        }
        std::vector<VehicleGenerationIssue> validate( const GeneratedVehicle &vehicle ) override
        {
            return WPVehicleGenerator::validate( vehicle );
        }
        VehicleMaterialSlot materialSlotFor( VehicleMaterial material ) override
        {
            return WPVehicleGenerator::materialSlotFor( material );
        }
        WP_CLASS_REGISTER_DECL;
    };
    class WPProcedural_API CVehicleGeometry : public IVehicleGeometry
    {
    public:
        VehicleGeometry generate( const VehicleGeometryConfig &config ) override
        {
            return WPVehicleGeometry::generate( config );
        }
        WP_CLASS_REGISTER_DECL;
    };
    class WPProcedural_API CVehiclePhysics : public IVehiclePhysics
    {
    public:
        VehiclePhysicsConfig generate( VehiclePhysicsPreset preset, std::uint64_t seed ) override
        {
            return WPVehiclePhysics::generate( preset, seed );
        }
        void recomputeDerivedProperties( VehiclePhysicsConfig &config ) override
        {
            WPVehiclePhysics::recomputeDerivedProperties( config );
        }
        VehiclePhysicsValidation validate( const VehiclePhysicsConfig &config ) override
        {
            return WPVehiclePhysics::validate( config );
        }
        double staticWheelLoadN( const VehiclePhysicsConfig &config, WheelCorner corner ) override
        {
            return WPVehiclePhysics::staticWheelLoadN( config, corner );
        }
        double aerodynamicDragN( const VehiclePhysicsConfig &config, double speedMps ) override
        {
            return WPVehiclePhysics::aerodynamicDragN( config, speedMps );
        }
        double aerodynamicDownforceN( const VehiclePhysicsConfig &config, double speedMps,
                                      double rideHeightM ) override
        {
            return WPVehiclePhysics::aerodynamicDownforceN( config, speedMps, rideHeightM );
        }
        WP_CLASS_REGISTER_DECL;
    };
    class WPProcedural_API CVehiclePresentation : public IVehiclePresentation
    {
    public:
        VehiclePresentationState reset( const VehiclePhysicsConfig &physics ) override
        {
            return WPVehiclePresentation::reset( physics );
        }
        bool update( const VehiclePresentationConfig &config, const VehiclePhysicsConfig &physics,
                     const VehicleDynamicsState &dynamics, const VehicleDynamicsTelemetry &telemetry,
                     const VehicleDamageTelemetry &damage, const VehiclePresentationInput &input,
                     double deltaSeconds, VehiclePresentationState &state ) override
        {
            return WPVehiclePresentation::update( config, physics, dynamics, telemetry, damage, input,
                                                  deltaSeconds, state );
        }
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
