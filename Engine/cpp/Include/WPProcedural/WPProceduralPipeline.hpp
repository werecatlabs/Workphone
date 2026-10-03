// ============================================================================
// WPProceduralPipeline.hpp - AAA Procedural Pipeline (top-level facade)
// ============================================================================
// Ties together every procedural system: noise, textures, geometry, buildings,
// ground, sky, atmosphere. The pipeline owns instances of each subsystem and
// orchestrates generation, baking, and updating.
//
// Usage:
//   WPProceduralPipeline pipeline( 0xc0de );
//   pipeline.initialize();
//
//   BuildingSpec spec;
//   spec.floors = 5;
//   spec.width = 12.0f;
//   spec.depth = 10.0f;
//   BuildingResult b = pipeline.buildings().generate( spec, Vector3(...) );
//
//   pipeline.shutdown();
// ============================================================================

#ifndef WPProceduralPipeline_h__
#define WPProceduralPipeline_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPProcedural/WPNoise.hpp"
#include "WPProcedural/WPTextureForge.hpp"
#include "WPProcedural/WPGeometryKit.hpp"
#include "WPProcedural/WPBuildingGenerator.hpp"
#include "WPProcedural/WPSkyAtmosphere.hpp"

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Aggregates all procedural systems and orchestrates them.
         */
        class WPProcedural_API WPProceduralPipeline
        {
        public:
            explicit WPProceduralPipeline( u32 seed = 0xc0de );
            ~WPProceduralPipeline();

            /// Bring all subsystems online.
            void initialize();

            /// Bring all subsystems down.
            void shutdown();

            /// Per-frame update (sky, particles, etc.)
            void update( real_Num dt );

            // --------------------------------------------------------------
            // Subsystem accessors
            // --------------------------------------------------------------
            WPNoise &noise();
            WPTextureForge &textures();
            WPBuildingGenerator &buildings();
            WPSkyAtmosphere &sky();

            const WPNoise &noise() const;
            const WPTextureForge &textures() const;
            const WPBuildingGenerator &buildings() const;
            const WPSkyAtmosphere &sky() const;

            u32 getSeed() const;
            void setSeed( u32 seed );

        private:
            u32 mSeed;
            WPNoise mNoise;
            WPTextureForge mTextures;
            WPBuildingGenerator mBuildings;
            WPSkyAtmosphere mSky;
            bool mInitialized;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPProceduralPipeline_h__
