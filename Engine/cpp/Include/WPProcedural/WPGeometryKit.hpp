// ============================================================================
// WPGeometryKit.hpp - AAA Procedural Geometry Primitives
// ============================================================================
// Provides the geometry toolkit used by all AAA procedural generators:
//   - Chamfered boxes (different bevel radii for different surfaces)
//   - LP-ball sandbag silhouettes (NOT ellipsoids - reads as ravioli)
//   - Walled/faceted rocks from noise-deformed spheres
//   - Polyprisms with arbitrary cross-sections
//   - Tubes and cylinders with optional taper
//   - Cloth/sail geometry with catenary sag
//   - Stair runs with real rise/run geometry
//
// All builders return vertex/index buffers in a common intermediate
// representation that can be uploaded to any engine vertex format.
// ============================================================================

#ifndef WPGeometryKit_h__
#define WPGeometryKit_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPProcedural/ProceduralModel.hpp"
#include <vector>
#include <cstdint>

namespace workphone
{
    namespace procedural
    {

        /**
         * @brief Procedural geometry toolkit - the building blocks.
         *
         * All methods are static and stateless.
         */
        class WPCore_API WPGeometryKit
        {
        public:
            // -----------------------------------------------------------------
            // Boxes
            // -----------------------------------------------------------------

            /// Chamfered axis-aligned box of dimensions (w, h, d) with bevelled edges.
            /// Vertices on the chamfer strip are flagged with mask.r = 1 for wear.
            static ProceduralMesh chamferedBox( real_Num w, real_Num h, real_Num d,
                                                real_Num bevel = 0.012f, u32 subdivX = 1,
                                                u32 subdivY = 1, u32 subdivZ = 1 );

            /// Plain 12-triangle box for thin members (frame strips, kerbs).
            static ProceduralMesh plainBox( real_Num w, real_Num h, real_Num d );

            /// Single 4-vertex quad in XY plane, centered on origin.
            static ProceduralMesh quad( real_Num w = 1.0f, real_Num h = 1.0f );

            // -----------------------------------------------------------------
            // Sandbag - Lp-ball silhouette (not ellipsoid!)
            // -----------------------------------------------------------------

            /// Generate a filled sandbag with the correct rounded-box silhouette.
            /// Reference: a filled bag is a rounded BOX, not a lozenge.
            /// @param variant  0 = plump, 1 = slumped, 2 = half-empty
            /// @param w,h,d    Full dimensions of the bag in metres
            /// @param box      Lp-ball exponent (2=sphere, 4=nearly a brick). Default 3.1.
            /// @param lump     Lumpiness factor 0..2
            static ProceduralMesh sandbag( u32 seed, s32 variant = 0, real_Num w = 0.5f,
                                           real_Num h = 0.17f, real_Num d = 0.3f, real_Num box = 3.1f,
                                           real_Num lump = 1.0f );

            // -----------------------------------------------------------------
            // Rocks and rubble
            // -----------------------------------------------------------------

            /// Faceted noise-deformed rock of approximate radius
            static ProceduralMesh rock( u32 seed, real_Num radius, u32 lod = 0, real_Num lump = 0.7f );

            /// Walled prism from a polygon in the XY plane, extruded along +Z by depth.
            static ProceduralMesh polyPrism( const std::vector<Vector2<real_Num>> &polygon,
                                             real_Num depth );

            // -----------------------------------------------------------------
            // Cylinders and tubes
            // -----------------------------------------------------------------

            /// Cylinder along +Y, radius height.
            static ProceduralMesh cylinder( real_Num radius, real_Num height, u32 radialSegments = 12,
                                            u32 heightSegments = 1, bool open = false,
                                            real_Num taper = 1.0f );

            /// Straight tube along +Y for pipes, poles, rebar.
            static ProceduralMesh tube( real_Num radius, real_Num height, u32 radial = 8 );

            // -----------------------------------------------------------------
            // Cloth and fabric
            // -----------------------------------------------------------------

            /// Cloth rectangle with catenary droop between two anchor points.
            /// @param w,h        Width and height of the cloth panel
            /// @param from,to    Anchor points in 3D space
            /// @param sag        Maximum droop at center as a fraction of distance
            /// @param segs       Subdivisions (width, height)
            /// @param jitter     Per-vertex noise jitter amount
            static ProceduralMesh cloth( real_Num w, real_Num h, const Vector3<real_Num> &from,
                                         const Vector3<real_Num> &to, real_Num sag = 0.1f,
                                         u32 segsX = 16, u32 segsY = 16, real_Num jitter = 0.05f,
                                         u32 seed = 1 );

            // -----------------------------------------------------------------
            // Stairs
            // -----------------------------------------------------------------

            /// A stair run - real geometry with proper rise/run.
            /// @param width     Run width
            /// @param steps     Number of treads
            /// @param rise      Total vertical rise
            /// @param run       Total horizontal run
            /// @param sideRails Optional: "left" | "right" | "both" | "none"
            static ProceduralMesh stairRun( real_Num width, s32 steps, real_Num rise, real_Num run,
                                            const std::string &sideRails = "right" );

            // -----------------------------------------------------------------
            // Patch / footprint shapes
            // -----------------------------------------------------------------

            /// A planar polygonal patch for ground skirts, scorch marks, etc.
            static ProceduralMesh patch( u32 seed, real_Num radius, u32 lobes = 11,
                                         real_Num wobble = 0.5f );

            // -----------------------------------------------------------------
            // Helpers
            // -----------------------------------------------------------------

            /// Apply a subtle warp to every vertex position based on FBM noise.
            static void warpMesh( ProceduralMesh &mesh, real_Num amp = 0.018f, real_Num freq = 0.5f,
                                  u32 seed = 0 );
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPGeometryKit_h__
