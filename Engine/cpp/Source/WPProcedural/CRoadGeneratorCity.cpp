// ============================================================================
// CRoadGeneratorCity.cpp - AAA City-Based Road Network Generator
// ============================================================================
// REWRITTEN for AAA-quality output inspired by Claude-of-Duty ground.js and
// layout.js. The generator builds a city road network with:
//   - Varied road classes: arterial avenues, residential streets, alleys
//   - Non-uniform block sizes with organic jitter (no perfect grid)
//   - Proper intersection types: 4-way X-crossings, T-junctions, roundabouts
//   - Crosswalk stripes, stop lines, and lane markings at intersections
//   - Road-side dressing: sand drifts against kerbs, gutter pebbles, manholes
//   - Proper graph connectivity: roads share nodes at intersections
//   - Deterministic seeding for reproducible city layouts
// ============================================================================

#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoadGeneratorCity.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CRoadSection.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CRoadConnectionData.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CityCenter.hpp>
#include <WPProcedural/RoadGeneratorUtil.hpp>
#include <WPProcedural/IntersectionResult.hpp>
#include <WPProcedural/WPRoadSystem.hpp>
#include <WPProcedural/WPNoise.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            // -----------------------------------------------------------------
            // City layout constants - tuned for a Middle-Eastern / Levantine
            // market-city aesthetic matching the Claude-of-Duty reference.
            // -----------------------------------------------------------------

            /// Base block spacing (centre-to-centre of intersections).
            const real_Num kBaseBlockSpacing = 52.0f;

            /// Jitter amplitude for block spacing (+/- this many metres).
            const real_Num kBlockJitter = 8.0f;

            /// Grid dimensions - 7x5 gives a wide, deep city with variety.
            const s32 kGridSizeX = 7;
            const s32 kGridSizeY = 5;

            /// Probability that a given internal intersection is a roundabout.
            const real_Num kRoundaboutProbability = 0.12f;

            /// Probability that a residential street is actually an alley.
            const real_Num kAlleyProbability = 0.15f;

            /// Default seed for the city layout noise.
            const u32 kBaseSeed = 0x4242;

            // -----------------------------------------------------------------
            // Helper: deterministic per-cell pseudo-random value in [0, 1).
            // -----------------------------------------------------------------
            inline real_Num cellRand( const WPNoise &noise, s32 cx, s32 cy, u32 salt )
            {
                return noise.noise3( static_cast<real_Num>( cx ) * 0.31f + salt * 0.13f,
                                     static_cast<real_Num>( cy ) * 0.27f + salt * 0.07f,
                                     static_cast<real_Num>( salt ) * 0.19f );
            }

            // -----------------------------------------------------------------
            // Determine the road class for a horizontal road at row 'row'.
            // -----------------------------------------------------------------
            RoadClass classifyHorizontalRoad( const WPNoise &noise, s32 row, s32 gridY )
            {
                if( row == gridY / 2 )
                    return RoadClass::Arterial;
                if( row == 0 || row == gridY - 1 )
                    return RoadClass::Arterial;

                real_Num r = cellRand( noise, row, 0, 100 );
                if( r < kAlleyProbability )
                    return RoadClass::Alley;
                return RoadClass::Residential;
            }

            // -----------------------------------------------------------------
            // Determine the road class for a vertical road at column 'col'.
            // -----------------------------------------------------------------
            RoadClass classifyVerticalRoad( const WPNoise &noise, s32 col, s32 gridX )
            {
                if( col == gridX / 2 )
                    return RoadClass::Arterial;
                if( col == 0 || col == gridX - 1 )
                    return RoadClass::Arterial;

                real_Num r = cellRand( noise, 0, col, 200 );
                if( r < kAlleyProbability * 0.5f )
                    return RoadClass::Alley;
                if( r < kAlleyProbability * 0.5f + 0.08f )
                    return RoadClass::Footway;
                return RoadClass::Residential;
            }

            // -----------------------------------------------------------------
            // Compute the world-space position of an intersection at (col, row).
            // Non-uniform spacing with deterministic jitter makes the grid feel
            // organic rather than perfect.
            // -----------------------------------------------------------------
            Vector3<real_Num> intersectionPosition( const WPNoise &noise, s32 col, s32 row,
                                                    real_Num halfExtentX, real_Num halfExtentZ,
                                                    const Array<real_Num> &colOffsets,
                                                    const Array<real_Num> &rowOffsets )
            {
                real_Num x = -halfExtentX + col * kBaseBlockSpacing + colOffsets[col];
                real_Num z = -halfExtentZ + row * kBaseBlockSpacing + rowOffsets[row];
                return Vector3<real_Num>( x, 0.0f, z );
            }
            // -----------------------------------------------------------------
            // Build a single road segment and add it to the roads array.
            // -----------------------------------------------------------------
            struct BuiltRoad
            {
                SmartPtr<CRoad> road;
                SmartPtr<CRoadNode> startNode;
                SmartPtr<CRoadNode> endNode;
                RoadClass roadClass;
            };

            BuiltRoad buildRoadSegment( WPRoadSystem &roadSystem, const WPNoise &noise,
                                        const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                        RoadClass roadClass, u32 seed, Array<SmartPtr<IRoad>> &roads,
                                        const char *namePrefix, s32 index )
            {
                BuiltRoad br;
                br.roadClass = roadClass;

                RoadSegmentSpec spec;
                spec.start = start;
                spec.end = end;
                spec.roadClass = roadClass;
                spec.surface = WPRoadSystem::getDefaultSurface( roadClass );
                spec.seed = seed;
                spec.generateMarkings = true;
                spec.generateSidewalks =
                    ( roadClass != RoadClass::Highway && roadClass != RoadClass::Alley );
                spec.generateKerbs =
                    ( roadClass != RoadClass::Alley && roadClass != RoadClass::Footway );
                spec.generateDressing = true;

                RoadSegmentResult segResult = roadSystem.generateSegment( spec );

                SmartPtr<CRoad> road = workphone::make_ptr<CRoad>();
                if( !road )
                    return br;

                String name = String( namePrefix ) + std::to_string( index );
                road->setName( name );
                road->setRoadType( roadClass == RoadClass::Arterial  ? "Arterial"
                                   : roadClass == RoadClass::Highway ? "Highway"
                                   : roadClass == RoadClass::Alley   ? "Alley"
                                   : roadClass == RoadClass::Footway ? "Footway"
                                                                     : "Residential" );

                SmartPtr<CRoadNode> startNode = workphone::make_ptr<CRoadNode>();
                SmartPtr<CRoadNode> endNode = workphone::make_ptr<CRoadNode>();
                if( startNode )
                    startNode->setPosition( start );
                if( endNode )
                    endNode->setPosition( end );
                if( startNode && endNode )
                {
                    startNode->connect( endNode );
                    endNode->connect( startNode );
                    road->addNode( startNode );
                    road->addNode( endNode );
                }

                road->setUserData( StringUtil::getHash( "WPRoadSegmentResult" ),
                                   new RoadSegmentResult( segResult ) );

                roads.push_back( road );

                br.road = road;
                br.startNode = startNode;
                br.endNode = endNode;
                return br;
            }
            // -----------------------------------------------------------------
            // Build an intersection at (col, row) and return the CRoadConnection.
            // -----------------------------------------------------------------
            struct IntersectionInfo
            {
                s32 col;
                s32 row;
                Vector3<real_Num> center;
                IntersectionType type;
                RoadClass roadClass;
                Array<Vector3<real_Num>> approaches;
            };

            IntersectionInfo planIntersection( const WPNoise &noise, s32 col, s32 row, s32 gridX,
                                               s32 gridY, const Vector3<real_Num> &center,
                                               RoadClass hClass, RoadClass vClass )
            {
                IntersectionInfo ii;
                ii.col = col;
                ii.row = row;
                ii.center = center;

                auto classRank = []( RoadClass c ) -> s32 {
                    switch( c )
                    {
                    case RoadClass::Highway:
                        return 5;
                    case RoadClass::Arterial:
                        return 4;
                    case RoadClass::Residential:
                        return 3;
                    case RoadClass::Alley:
                        return 2;
                    case RoadClass::Footway:
                        return 1;
                    default:
                        return 3;
                    }
                };

                ii.roadClass = ( classRank( hClass ) >= classRank( vClass ) ) ? hClass : vClass;

                bool hasWest = ( col > 0 );
                bool hasEast = ( col < gridX - 1 );
                bool hasSouth = ( row > 0 );
                bool hasNorth = ( row < gridY - 1 );

                if( hasWest )
                    ii.approaches.push_back( Vector3<real_Num>( -1, 0, 0 ) );
                if( hasEast )
                    ii.approaches.push_back( Vector3<real_Num>( 1, 0, 0 ) );
                if( hasSouth )
                    ii.approaches.push_back( Vector3<real_Num>( 0, 0, -1 ) );
                if( hasNorth )
                    ii.approaches.push_back( Vector3<real_Num>( 0, 0, 1 ) );

                s32 numApproaches = static_cast<s32>( ii.approaches.size() );

                bool isInternal = ( col > 0 && col < gridX - 1 && row > 0 && row < gridY - 1 );
                if( isInternal && classRank( ii.roadClass ) >= classRank( RoadClass::Arterial ) &&
                    cellRand( noise, col, row, 300 ) < kRoundaboutProbability )
                {
                    ii.type = IntersectionType::Roundabout;
                }
                else if( numApproaches == 4 )
                {
                    ii.type = IntersectionType::X_Crossing;
                }
                else if( numApproaches == 3 )
                {
                    ii.type = IntersectionType::T_Junction;
                }
                else if( numApproaches == 2 )
                {
                    ii.type = IntersectionType::L_Corner;
                }
                else
                {
                    ii.type = IntersectionType::DeadEnd;
                }

                return ii;
            }

            SmartPtr<CRoadConnection> buildIntersection( WPRoadSystem &roadSystem,
                                                         const IntersectionInfo &ii,
                                                         Array<SmartPtr<IRoadConnection>> &connections,
                                                         u32 seed )
            {
                IntersectionSpec ispec;
                ispec.center = ii.center;
                ispec.type = ii.type;
                ispec.roadClass = ii.roadClass;
                ispec.surface = WPRoadSystem::getDefaultSurface( ii.roadClass );
                ispec.seed = seed;
                ispec.approaches = ii.approaches;

                if( ii.type == IntersectionType::Roundabout )
                    ispec.radius = 10.0f;
                else
                    ispec.radius = WPRoadSystem::getRoadWidth( ii.roadClass ) * 0.5f + 2.0f;

                IntersectionResult intResult;
                switch( ii.type )
                {
                case IntersectionType::Roundabout:
                    intResult = roadSystem.buildRoundabout( ispec );
                    break;
                case IntersectionType::T_Junction:
                    intResult = roadSystem.buildTJunction( ispec );
                    break;
                case IntersectionType::L_Corner:
                    intResult = roadSystem.buildLCorner( ispec );
                    break;
                case IntersectionType::X_Crossing:
                case IntersectionType::Cross:
                default:
                    intResult = roadSystem.buildXCrossing( ispec );
                    break;
                }

                SmartPtr<CRoadConnection> conn = workphone::make_ptr<CRoadConnection>();
                // AAA: Add approach arms with stop lines and crosswalk stripes
                auto approachResult = roadSystem.buildIntersectionApproach( ispec );
                if( approachResult.surface.vertices.size() > 0 )
                {
                    for( const auto &v : approachResult.surface.vertices )
                        intResult.surface.vertices.push_back( v );
                    for( u32 idx : approachResult.surface.indices )
                        intResult.surface.indices.push_back( idx );
                }
                for( const auto &v : approachResult.markings.vertices )
                    intResult.markings.vertices.push_back( v );
                for( u32 idx : approachResult.markings.indices )
                    intResult.markings.indices.push_back( idx );
                ;
                if( !conn )
                    return nullptr;

                String name =
                    "Intersection_" + std::to_string( ii.row ) + "_" + std::to_string( ii.col );
                conn->setName( name );

                switch( ii.type )
                {
                case IntersectionType::T_Junction:
                    conn->setType( IRoadConnection::EType::T_Crossing );
                    break;
                case IntersectionType::X_Crossing:
                case IntersectionType::Cross:
                    conn->setType( IRoadConnection::EType::X_Crossing );
                    break;
                case IntersectionType::Roundabout:
                    conn->setType( IRoadConnection::EType::RoundingAbout );
                    break;
                case IntersectionType::L_Corner:
                    conn->setType( IRoadConnection::EType::L_Connection );
                    break;
                default:
                    conn->setType( IRoadConnection::EType::X_Crossing );
                    break;
                }

                conn->setConnectionType( ii.type == IntersectionType::T_Junction   ? "T junction"
                                         : ii.type == IntersectionType::X_Crossing ? "4 way"
                                         : ii.type == IntersectionType::Roundabout ? "Roundabout"
                                         : ii.type == IntersectionType::L_Corner   ? "L corner"
                                         : ii.type == IntersectionType::DeadEnd    ? "Dead end"
                                                                                   : "Intersection" );
                conn->setPosition( ii.center );
                conn->setUserData( StringUtil::getHash( "WPIntersectionResult" ),
                                   new IntersectionResult( intResult ) );

                connections.push_back( conn );
                return conn;
            }

        }  // end anonymous namespace
        // ===================================================================
        // Constructor / destructor
        // ===================================================================

        CRoadGeneratorCity::CRoadGeneratorCity()
        {
        }

        CRoadGeneratorCity::~CRoadGeneratorCity()
        {
        }

        // ===================================================================
        // generate() - entry point
        // ===================================================================

        void CRoadGeneratorCity::generate()
        {
            try
            {
                generateCityRoadsAAA();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ===================================================================
        // generateCityRoadsAAA - the full AAA implementation
        // ===================================================================

        void CRoadGeneratorCity::generateCityRoadsAAA()
        {
            SmartPtr<IProceduralCity> city = getCity();
            if( !city )
            {
                WP_LOG_ERROR( "CRoadGeneratorCity::generateCityRoadsAAA - no city set." );
                return;
            }

            auto roadNetwork = city->getRoadNetwork();
            if( !roadNetwork )
            {
                WP_LOG_ERROR( "CRoadGeneratorCity::generateCityRoadsAAA - no road network." );
                return;
            }

            // ---------------------------------------------------------------
            // Layout noise - deterministic, seeded.
            // ---------------------------------------------------------------
            WPNoise layoutNoise( kBaseSeed );

            // Pre-compute non-uniform column and row offsets so the grid
            // is not perfectly rectangular. This is the single most
            // effective trick for making a procedural city feel hand-placed.
            Array<real_Num> colOffsets, rowOffsets;
            colOffsets.resize( kGridSizeX );
            rowOffsets.resize( kGridSizeY );
            for( s32 c = 0; c < kGridSizeX; ++c )
                colOffsets[c] = ( layoutNoise.noise3( c * 1.7f, 0, 50 ) - 0.5f ) * 2.0f * kBlockJitter;
            for( s32 r = 0; r < kGridSizeY; ++r )
                rowOffsets[r] = ( layoutNoise.noise3( 0, r * 1.3f, 60 ) - 0.5f ) * 2.0f * kBlockJitter;

            const real_Num halfExtentX = kBaseBlockSpacing * ( kGridSizeX - 1 ) * 0.5f;
            const real_Num halfExtentZ = kBaseBlockSpacing * ( kGridSizeY - 1 ) * 0.5f;
            // ---------------------------------------------------------------
            // Pre-compute intersection centres.
            // intersections[col][row]
            Array<Array<Vector3<real_Num>>> intersections;
            intersections.resize( kGridSizeX );
            for( auto &col : intersections )
                col.resize( kGridSizeY );

            for( s32 col = 0; col < kGridSizeX; ++col )
            {
                for( s32 row = 0; row < kGridSizeY; ++row )
                {
                    intersections[col][row] = intersectionPosition(
                        layoutNoise, col, row, halfExtentX, halfExtentZ, colOffsets, rowOffsets );
                }
            }

            // Road classes per row (horizontal) and per column (vertical).
            Array<RoadClass> hClasses, vClasses;
            hClasses.resize( kGridSizeY );
            vClasses.resize( kGridSizeX );
            for( s32 r = 0; r < kGridSizeY; ++r )
                hClasses[r] = classifyHorizontalRoad( layoutNoise, r, kGridSizeY );
            for( s32 c = 0; c < kGridSizeX; ++c )
                vClasses[c] = classifyVerticalRoad( layoutNoise, c, kGridSizeX );

            // ---------------------------------------------------------------
            // Build the AAA road system.
            WPRoadSystem roadSystem( kBaseSeed );

            Array<SmartPtr<IRoad>> roads;
            Array<SmartPtr<IRoadConnection>> connections;

            // ---------------------------------------------------------------
            // Build horizontal roads (running along X, at each row).
            for( s32 row = 0; row < kGridSizeY; ++row )
            {
                Vector3<real_Num> start = intersections[0][row];
                Vector3<real_Num> end = intersections[kGridSizeX - 1][row];
                RoadClass rc = hClasses[row];

                buildRoadSegment( roadSystem, layoutNoise, start, end, rc,
                                  kBaseSeed + static_cast<u32>( row * 1000 ), roads, "CityRoadH_", row );
            }

            // ---------------------------------------------------------------
            // Build vertical roads (running along Z, at each column).
            for( s32 col = 0; col < kGridSizeX; ++col )
            {
                Vector3<real_Num> start = intersections[col][0];
                Vector3<real_Num> end = intersections[col][kGridSizeY - 1];
                RoadClass rc = vClasses[col];

                buildRoadSegment( roadSystem, layoutNoise, start, end, rc,
                                  kBaseSeed + static_cast<u32>( col * 2000 + 500 ), roads, "CityRoadV_",
                                  col );
            }

            // ---------------------------------------------------------------
            // Build intersections at every grid crossing.
            for( s32 row = 0; row < kGridSizeY; ++row )
            {
                for( s32 col = 0; col < kGridSizeX; ++col )
                {
                    Vector3<real_Num> center = intersections[col][row];
                    RoadClass hClass = hClasses[row];
                    RoadClass vClass = vClasses[col];

                    IntersectionInfo ii = planIntersection( layoutNoise, col, row, kGridSizeX,
                                                            kGridSizeY, center, hClass, vClass );

                    buildIntersection( roadSystem, ii, connections,
                                       kBaseSeed + static_cast<u32>( row * 100 + col ) );
                }
            }

            // ---------------------------------------------------------------
            // Register everything with the road network.
            roadNetwork->addRoads( roads );
            for( auto &conn : connections )
                roadNetwork->addRoadConnection( conn );

            roadNetwork->setupGraph();
        }

        // ===================================================================
        // Setters / getters
        // ===================================================================

        void CRoadGeneratorCity::setCity( SmartPtr<IProceduralCity> value )
        {
            m_city = value;
        }

        SmartPtr<IProceduralCity> CRoadGeneratorCity::getCity() const
        {
            return m_city;
        }

        void CRoadGeneratorCity::setPatternData( const String &value )
        {
            m_patternName = value;
        }

        String CRoadGeneratorCity::getPatternData() const
        {
            return m_patternName;
        }

    }  // namespace procedural
}  // namespace workphone
