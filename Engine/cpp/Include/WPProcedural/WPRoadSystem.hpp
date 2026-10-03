#ifndef WPRoadSystem_h__
#define WPRoadSystem_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/IntersectionResult.hpp>
#include <WPProcedural/RoadMesh.hpp>
#include <WPProcedural/RoadSegmentResult.hpp>
#include <WPProcedural/RoadSegmentSpec.hpp>
#include <WPProcedural/WPNoise.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>

#include <Workphone/Interface/Procedural/RoadSystemTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPRoadSystem
        {
        public:
            // --- Nested aliases so callers may write either
            //     WPRoadSystem::RoadSegmentResult or the unqualified
            //     RoadSegmentResult (they refer to the same type). ---
            using RoadClass = ::workphone::procedural::RoadClass;
            using RoadSurface = ::workphone::procedural::RoadSurface;
            using IntersectionType = ::workphone::procedural::IntersectionType;

            /// Construct with a seed value.  The seed initialises the
            /// internal noise generator so that all subsequent geometry
            /// is deterministic.
            explicit WPRoadSystem( u32 seed = 0 );

            /// Destructor releases the owned WPNoise instance.
            ~WPRoadSystem();

            /// Re-seed the noise generator.  All subsequent generation calls
            /// use the new seed; previously built meshes are unaffected.
            void setSeed( u32 seed );

            // -----------------------------------------------------------------
            // Dimension lookups
            // -----------------------------------------------------------------

            /// Roadway width in metres for the given functional class.
            static real_Num getRoadWidth( RoadClass cls );

            /// Pavement / sidewalk width in metres for the given class.
            static real_Num getSidewalkWidth( RoadClass cls );

            /// Number of traffic lanes for the given class.
            static u32 getLaneCount( RoadClass cls );

            /// Default carriageway surface type for the given class.
            static RoadSurface getDefaultSurface( RoadClass cls );

            // -----------------------------------------------------------------
            // Segment generation
            // -----------------------------------------------------------------

            /// Generate a single road segment with full cross-section detail.
            ///
            /// The result contains separate RoadMesh objects for each render
            /// layer.  Each mesh is pre-oriented so its Z axis aligns with
            /// spec.start -> spec.end.  Callers typically upload each RoadMesh
            /// to a separate GPU buffer / sub-mesh.
            RoadSegmentResult generateSegment( const RoadSegmentSpec &spec );

            // -----------------------------------------------------------------
            // Intersection / junction generation
            // -----------------------------------------------------------------

            /// Generate an intersection geometry from |spec|.
            ///
            /// The returned IntersectionResult is pre-positioned at
            /// spec.center and fully triangulated.  The |type| field selects
            /// the junction template (X crossing, T junction, roundabout,
            /// L corner).
            IntersectionResult generateIntersection( const IntersectionSpec &spec );

            // -----------------------------------------------------------------
            // Direct intersection builders (for advanced users)
            // -----------------------------------------------------------------
            IntersectionResult buildXCrossing( const IntersectionSpec &spec );
            IntersectionResult buildRoundabout( const IntersectionSpec &spec );
            IntersectionResult buildTJunction( const IntersectionSpec &spec );
            IntersectionResult buildLCorner( const IntersectionSpec &spec );

            RoadMesh buildOldTarmacPatches( const RoadSegmentSpec &spec, real_Num width,
                                            real_Num length );
            RoadMesh buildPotholes( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildManholes( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildGullyGrates( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildArrows( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildPedestrianCrossing( const RoadSegmentSpec &spec, real_Num width,
                                              real_Num length );
            IntersectionResult buildIntersectionApproach( const IntersectionSpec &spec );

            /*RoadMesh buildOldTarmacPatches( const RoadSegmentSpec &spec, real_Num width,
                                            real_Num length );
            RoadMesh buildPotholes( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildManholes( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildGullyGrates( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildArrows( const RoadSegmentSpec &spec, real_Num width, real_Num length );
            RoadMesh buildPedestrianCrossing( const RoadSegmentSpec &spec, real_Num width,
                                              real_Num length );
            IntersectionResult buildIntersectionApproach( const IntersectionSpec &spec );*/

            RoadMesh oldTarmac;
            RoadMesh potholes;
            RoadMesh manholes;
            RoadMesh gullyGrates;
            RoadMesh arrows;
            RoadMesh pedXing;

        private:
            RoadMesh buildRoadSurface( const RoadSegmentSpec &spec, real_Num width, real_Num length,
                                       u32 segments, u32 lengthSegs );
            RoadMesh buildSidewalk( const RoadSegmentSpec &spec, real_Num sidewalkW, real_Num length,
                                    bool isLeftSide );
            RoadMesh buildKerb( const RoadSegmentSpec &spec, real_Num kerbW, real_Num length,
                                bool isLeftSide );
            RoadMesh buildMarkings( const RoadSegmentSpec &spec, real_Num roadWidth, real_Num length );
            RoadMesh buildDressing( const RoadSegmentSpec &spec, real_Num roadWidth, real_Num length );
            RoadMesh buildCollision( const RoadSegmentSpec &spec, real_Num width, real_Num length );

            u32 mSeed;
            WPNoise *mNoise;  // Owned
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // WPRoadSystem_h__
