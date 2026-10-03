// ============================================================================
// WPBuildingGenerator.hpp - AAA Procedural Building Generator
// ============================================================================
// Mirrors the Claude-of-Duty buildings.js facade assembly system:
//   - Floor count with per-floor height variation
//   - Setback architecture (Mediterranean/Levantine form)
//   - Window states: glazed/open/boarded/shuttered/lit/curtain/ajar
//   - Facade elements: shopfronts, doors, windows, balconies, parapets
//   - Wear: spalled render, bullet damage, water stains, efflorescence
//   - Roof clutter: AC units, water tanks, satellite dishes, vents
//   - Setback terraces with coping and parapets
//   - Interior partitions, stairs, room plans
//
// Every building is generated from a deterministic seed.
// ============================================================================

#ifndef WPBuildingGenerator_h__
#define WPBuildingGenerator_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPProcedural/WPGeometryKit.hpp"
#include "WPProcedural/WPNoise.hpp"
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        /// Window state enum (matches Claude-of-Duty kit.js)
        enum class WindowState : u8
        {
            Glazed = 0,
            Open = 1,
            Boarded = 2,
            Shuttered = 3,
            Lit = 4,
            Curtain = 5,
            Ajar = 6
        };

        /// Parameters describing one building to generate.
        struct WPProcedural_API BuildingSpec
        {
            u32 seed = 0;
            s32 floors = 4;
            real_Num width = 10.0f;
            real_Num depth = 10.0f;
            real_Num floorH = 3.2f;
            real_Num wallThick = 0.16f;

            /// Building type hint for material selection.
            enum class Type : u8
            {
                Residential,
                Commercial,
                Industrial,
                MixedUse
            } type = Type::Residential;

            /// Setback configuration (Levantine/Mediterranean roof terrace).
            struct WPProcedural_API Setback
            {
                s32 from = -1;          ///< Floor index at which setback starts
                s32 fromFloor = -1;     ///< Optional: explicit floor number
                real_Num depth = 1.5f;  ///< Pull-in distance
                s32 side = 0;           ///< 0=front, 1=right, 2=back, 3=left
            };
            std::vector<Setback> setbacks;

            /// Per-floor room plans (optional). If absent, just wall shell is generated.
            bool generateInterior = false;

            /// Add bullet damage cluster to walls (visual pock marks).
            bool generateDamage = false;

            /// Random tint family: cream, sand, blue, pink, white, plaster variants.
            std::string wallKey = "";
        };

        /// Resulting geometry of a generated building (all transforms applied).
        struct WPProcedural_API BuildingResult
        {
            ProceduralMesh body;
            std::vector<ProceduralMesh> windows;
            std::vector<ProceduralMesh> details;  // AC units, antennas, etc.
            ProceduralMesh roof;
            ProceduralMesh parapet;
            std::vector<ProceduralMesh> interiors;
            Vector3<real_Num> position;
            real_Num wallKeyVariant = 0.0f;  // [0,1] tint choice
        };

        /**
         * @brief Generates a complete procedural building assembly.
         */
        class WPProcedural_API WPBuildingGenerator
        {
        public:
            WPBuildingGenerator( u32 seed = 0xc0de );

            /// Generate a building from a spec.
            BuildingResult generate( const BuildingSpec &spec, const Vector3<real_Num> &origin );

            // -----------------------------------------------------------------
            // Building sub-assemblies
            // -----------------------------------------------------------------

            /// Main wall shell with chamfered corners and subtle warp.
            ProceduralMesh buildBody( const BuildingSpec &spec, u32 seed );

            /// Window opening with one of the seven states.
            ProceduralMesh buildWindow( WindowState state, u32 seed, real_Num width = 1.2f,
                                        real_Num height = 1.5f );

            /// Flat roof slab with coping and parapet.
            ProceduralMesh buildRoof( const BuildingSpec &spec, u32 seed );

            /// Roof access penthouse with door opening.
            ProceduralMesh buildRoofAccess( const BuildingSpec &spec, u32 seed );

            /// Interior partition walls (room plans).
            ProceduralMesh buildInterior( const BuildingSpec &spec, s32 floor, u32 seed );

            /// Stair run with risers, treads, and optional railings.
            ProceduralMesh buildStairs( const BuildingSpec &spec, s32 fromFloor, u32 seed );

            /// AC unit on the roof.
            ProceduralMesh buildACUnit( u32 seed );

            /// Setback terrace with slab + parapet + coping.
            ProceduralMesh buildTerrace( const BuildingSpec &spec, u32 seed );

            /// Drainpipe with offset bracket.
            ProceduralMesh buildDrainpipe( real_Num height, u32 seed );

            /// Window state picker (deterministic by floor + seed).
            static WindowState pickWindowState( u32 seed, s32 floor, real_Num damage = 0.2f );

        private:
            u32 mSeed;
            WPNoise mNoise;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPBuildingGenerator_h__
