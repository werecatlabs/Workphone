// ============================================================================
// CRoadGenerator.cpp - AAA Basic Road Network Generator
// ============================================================================
// REWRITTEN to use the new WPRoadSystem for AAA-quality output:
//   - Cambered road surfaces with wheel ruts
//   - Sidewalks with slab joints and broken corners
//   - Kerbs with worn edges
//   - Road markings (center dash, edge lines, crosswalks)
//   - Road-side dressing (sand drifts, gutter pebbles)
//   - 4-way intersections with crosswalk stripes
//   - T-junctions
//   - Roundabouts (configurable)
//
// This generator creates radial highway patterns using the WPRoadSystem for
// AAA quality visuals.
// ============================================================================

#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CRoadGenerator.hpp"
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
#include <WPProcedural/WPRoadSystem.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Workphone.hpp>

#include <algorithm>

namespace workphone
{
    namespace procedural
    {

        namespace
        {
            constexpr int s_minSeed = 0;
            constexpr u32 s_minMajorRoads = 0;
            constexpr s32 s_minMinorRoads = 0;
            constexpr f32 s_minRoadOffset = 0.0f;
            constexpr f32 s_minRoadDistance = 1.0f;
            constexpr f32 s_minRoadLength = 1.0f;
            constexpr f32 s_minSearchRadius = 0.0f;
            constexpr double s_randomSeedMax = 9999.0;
        }  // namespace

        CRoadGenerator::CRoadGenerator() = default;

        CRoadGenerator::~CRoadGenerator() = default;

        void CRoadGenerator::load( SmartPtr<ISharedObject> data )
        {
            // The original OSM loading path is currently disabled. When OSM data is
            // reintroduced this is the hook to translate a data object into roads.
            WP_UNUSED( data );
        }

        void CRoadGenerator::unload( SmartPtr<ISharedObject> data )
        {
            WP_UNUSED( data );

            try
            {
                clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ===================================================================
        // Missing virtual / accessor implementations
        // ===================================================================
        void CRoadGenerator::validate()
        {
            if( m_options.numMajorRoads < 1 )
                m_options.numMajorRoads = 1;
            if( m_options.roadLength <= 0.0f )
                m_options.roadLength = 200.0f;
            if( m_options.roadDistance <= 0.0f )
                m_options.roadDistance = 100.0f;
            if( m_options.roadOffset < 0.0f )
                m_options.roadOffset = 0.0f;
            if( m_options.connectionSearchRadius < 0.0f )
                m_options.connectionSearchRadius = 20.0f;
        }

        void CRoadGenerator::clear()
        {
            m_city = nullptr;
            m_roadNetwork = nullptr;
            m_cityGenerator = nullptr;
            m_cities.clear();
            m_selectedLayer = nullptr;
            m_finished = false;
            m_isGenerating = false;
        }

        bool CRoadGenerator::isFinished() const
        {
            return m_finished;
        }

        String CRoadGenerator::getPatternData() const
        {
            return m_options.patternName;
        }

        void CRoadGenerator::setPatternData( const String &value )
        {
            m_options.patternName = value;
            onOptionChanged();
        }

        SmartPtr<IProceduralCity> CRoadGenerator::getCity() const
        {
            return m_city;
        }

        void CRoadGenerator::setCity( SmartPtr<IProceduralCity> value )
        {
            m_city = value;
        }

        const SRoadGeneratorOptions &CRoadGenerator::getOptions() const
        {
            return m_options;
        }

        void CRoadGenerator::setOptions( const SRoadGeneratorOptions &options )
        {
            m_options = options;
            onOptionChanged();
        }

        void CRoadGenerator::resetToDefaults()
        {
            m_options = SRoadGeneratorOptions{};
            onOptionChanged();
        }

        void CRoadGenerator::onOptionChanged()
        {
            // Hook for subclasses to react to option changes.
        }

        void CRoadGenerator::loadOptions( SmartPtr<Properties> /*properties*/ )
        {
            // Data-driven configuration loading hook (currently a stub).
        }

        void CRoadGenerator::saveOptions( SmartPtr<Properties> /*properties*/ ) const
        {
            // Data-driven configuration saving hook (currently a stub).
        }

        // ===================================================================
        // Per-option accessors
        // ===================================================================
        int CRoadGenerator::getSeed() const
        {
            return m_options.seed;
        }
        void CRoadGenerator::setSeed( int seed )
        {
            m_options.seed = seed;
            onOptionChanged();
        }

        u32 CRoadGenerator::getNumMajorRoads() const
        {
            return m_options.numMajorRoads;
        }
        void CRoadGenerator::setNumMajorRoads( u32 count )
        {
            m_options.numMajorRoads = count;
            onOptionChanged();
        }

        s32 CRoadGenerator::getNumMinorRoadsPerMajor() const
        {
            return m_options.numMinorRoadsPerMajor;
        }
        void CRoadGenerator::setNumMinorRoadsPerMajor( s32 count )
        {
            m_options.numMinorRoadsPerMajor = count;
            onOptionChanged();
        }

        float CRoadGenerator::getRoadOffset() const
        {
            return m_options.roadOffset;
        }
        void CRoadGenerator::setRoadOffset( float offset )
        {
            m_options.roadOffset = offset;
            onOptionChanged();
        }

        float CRoadGenerator::getRoadDistance() const
        {
            return m_options.roadDistance;
        }
        void CRoadGenerator::setRoadDistance( float distance )
        {
            m_options.roadDistance = distance;
            onOptionChanged();
        }

        float CRoadGenerator::getRoadLength() const
        {
            return m_options.roadLength;
        }
        void CRoadGenerator::setRoadLength( float length )
        {
            m_options.roadLength = length;
            onOptionChanged();
        }

        float CRoadGenerator::getRotationVariation() const
        {
            return m_options.rotationVariation;
        }
        void CRoadGenerator::setRotationVariation( float variation )
        {
            m_options.rotationVariation = variation;
            onOptionChanged();
        }

        float CRoadGenerator::getConnectionSearchRadius() const
        {
            return m_options.connectionSearchRadius;
        }
        void CRoadGenerator::setConnectionSearchRadius( float radius )
        {
            m_options.connectionSearchRadius = radius;
            onOptionChanged();
        }

        bool CRoadGenerator::getGenerateMajorRoads() const
        {
            return m_options.generateMajorRoads;
        }
        void CRoadGenerator::setGenerateMajorRoads( bool generate )
        {
            m_options.generateMajorRoads = generate;
            onOptionChanged();
        }

        bool CRoadGenerator::getGenerateMinorRoads() const
        {
            return m_options.generateMinorRoads;
        }
        void CRoadGenerator::setGenerateMinorRoads( bool generate )
        {
            m_options.generateMinorRoads = generate;
            onOptionChanged();
        }

        float CRoadGenerator::getNodeBiasHeight() const
        {
            return m_options.nodeBiasHeight;
        }
        void CRoadGenerator::setNodeBiasHeight( float height )
        {
            m_options.nodeBiasHeight = height;
            onOptionChanged();
        }

        bool CRoadGenerator::getGenerateIntersections() const
        {
            return m_options.generateIntersections;
        }
        void CRoadGenerator::setGenerateIntersections( bool generate )
        {
            m_options.generateIntersections = generate;
            onOptionChanged();
        }

        bool CRoadGenerator::getRandomizeSeed() const
        {
            return m_options.randomizeSeed;
        }
        void CRoadGenerator::setRandomizeSeed( bool randomize )
        {
            m_options.randomizeSeed = randomize;
            onOptionChanged();
        }

        bool CRoadGenerator::getAutoUpdate() const
        {
            return m_options.autoUpdate;
        }
        void CRoadGenerator::setAutoUpdate( bool autoUpdate )
        {
            m_options.autoUpdate = autoUpdate;
            onOptionChanged();
        }

        void CRoadGenerator::generate()
        {
            m_finished = false;

            validate();

            if( m_options.randomizeSeed )
            {
                m_options.seed =
                    static_cast<int>( Math<real_Num>::RangedRandom( 0.0, s_randomSeedMax ) );
            }

            m_isGenerating = true;
            try
            {
                generateRoadsAAA();  // Use AAA quality generation

                if( m_options.generateIntersections )
                {
                    createIntersectionsAAA();  // Use AAA quality intersections
                }
            }
            catch( ... )
            {
                m_isGenerating = false;
                throw;
            }
            m_isGenerating = false;
            m_finished = true;
        }

        // ===================================================================
        // AAA QUALITY ROAD GENERATION
        // ===================================================================

        void CRoadGenerator::generateRoadsAAA()
        {
            auto city = getCity();
            if( !city )
            {
                return;
            }

            auto roadNetwork = city->getRoadNetwork();
            if( !roadNetwork )
            {
                return;
            }

            // Create the AAA road system
            WPRoadSystem roadSystem( static_cast<u32>( m_options.seed ) );

            Array<SmartPtr<IRoad>> majorRoads;
            Array<SmartPtr<IRoad>> minorRoads;

            // Generate major roads (radial highways)
            for( s32 majorIdx = 0; majorIdx < m_options.numMajorRoads; ++majorIdx )
            {
                // Calculate angle for this major road
                real_Num angle =
                    ( real_Num( majorIdx ) / m_options.numMajorRoads ) * 2.0f * Math<real_Num>::pi();
                Vector3<real_Num> startDir( std::cos( angle ), 0, std::sin( angle ) );

                // Start position at city center offset
                Vector3<real_Num> startPos = m_options.roadOffset * startDir;

                // End position at road length
                Vector3<real_Num> endPos = startPos + startDir * m_options.roadLength;

                // Create road segment spec
                RoadSegmentSpec spec;
                spec.start = startPos;
                spec.end = endPos;
                spec.roadClass = RoadClass::Arterial;  // Major roads are arterial
                spec.surface = RoadSurface::Asphalt;
                spec.seed = m_options.seed + majorIdx * 1000;
                spec.generateMarkings = true;
                spec.generateSidewalks = true;
                spec.generateKerbs = true;
                spec.generateDressing = true;

                // Generate the AAA road segment
                RoadSegmentResult segResult = roadSystem.generateSegment( spec );

                // Create the IRoad object
                SmartPtr<CRoad> majorRd = workphone::make_ptr<CRoad>();
                if( majorRd )
                {
                    majorRd->setName( "MajorRoad_" + StringUtil::toString( majorIdx ) );
                    majorRd->setRoadType( "MajorRoad" );

                    // Add start and end nodes
                    SmartPtr<CRoadNode> startNode = workphone::make_ptr<CRoadNode>();
                    SmartPtr<CRoadNode> endNode = workphone::make_ptr<CRoadNode>();
                    if( startNode )
                        startNode->setPosition( startPos );
                    if( endNode )
                        endNode->setPosition( endPos );
                    if( startNode && endNode )
                    {
                        startNode->connect( endNode );
                        endNode->connect( startNode );
                        majorRd->addNode( startNode );
                        majorRd->addNode( endNode );
                    }

                    // Store the AAA mesh result
                    majorRd->setUserData( StringUtil::getHash( "WPRoadSegmentResult" ),
                                          new RoadSegmentResult( segResult ) );

                    majorRoads.push_back( majorRd );
                }
            }

            // Generate minor roads (connecting the major roads at intervals)
            for( s32 majorIdx = 0; majorIdx < majorRoads.size(); ++majorIdx )
            {
                auto majorRoadCr = majorRoads[majorIdx];
                if( !majorRoadCr )
                {
                    continue;
                }

                const s32 numMinor = std::max( static_cast<s32>( 1 ), m_options.numMinorRoadsPerMajor );
                const auto clampedMinor = std::min( numMinor, m_options.numMinorRoadsPerMajor );

                for( s32 minorIdx = 0; minorIdx < clampedMinor; ++minorIdx )
                {
                    Array<SmartPtr<IRoadNode>> pointNodes;
                    const auto distance =
                        m_options.roadOffset + m_options.roadDistance * static_cast<f32>( minorIdx );
                    const auto startPos = static_cast<CRoad *>( majorRoadCr.get() )
                                              ->getPointOnRoad( distance, pointNodes );

                    if( pointNodes.size() < 2 )
                    {
                        continue;
                    }

                    const auto roadNodePos0 = pointNodes[0]->getPosition();
                    const auto roadNodePos1 = pointNodes[1]->getPosition();
                    const auto startDir =
                        ( roadNodePos1 - roadNodePos0 ).crossProduct( Vector3F::up() ).normaliseCopy();

                    // Create minor road spec
                    RoadSegmentSpec minorSpec;
                    minorSpec.start = startPos;
                    minorSpec.end =
                        startPos + startDir * m_options.roadLength * 0.5f;  // Shorter minor roads
                    minorSpec.roadClass = RoadClass::Residential;
                    minorSpec.surface = RoadSurface::Asphalt;
                    minorSpec.seed = m_options.seed + majorIdx * 2000 + minorIdx * 100;
                    minorSpec.generateMarkings = true;
                    minorSpec.generateSidewalks = true;
                    minorSpec.generateKerbs = true;
                    minorSpec.generateDressing = true;

                    // Generate the AAA minor road segment
                    RoadSegmentResult minorSegResult = roadSystem.generateSegment( minorSpec );

                    auto minorRd = workphone::make_ptr<CRoad>();
                    if( minorRd )
                    {
                        minorRd->setName( "MinorRoad_" + StringUtil::toString( majorIdx ) + "_" +
                                          StringUtil::toString( minorIdx ) );
                        minorRd->setRoadType( "MinorRoad" );

                        // Add start and end nodes
                        SmartPtr<CRoadNode> startNode = workphone::make_ptr<CRoadNode>();
                        SmartPtr<CRoadNode> endNode = workphone::make_ptr<CRoadNode>();
                        if( startNode )
                            startNode->setPosition( startPos );
                        if( endNode )
                            endNode->setPosition( startPos + startDir * m_options.roadLength * 0.5f );
                        if( startNode && endNode )
                        {
                            startNode->connect( endNode );
                            endNode->connect( startNode );
                            minorRd->addNode( startNode );
                            minorRd->addNode( endNode );
                        }

                        // Store the AAA mesh result
                        minorRd->setUserData( StringUtil::getHash( "WPRoadSegmentResult" ),
                                              new RoadSegmentResult( minorSegResult ) );

                        minorRoads.push_back( minorRd );
                    }
                }
            }

            roadNetwork->addRoads( majorRoads );
            roadNetwork->addRoads( minorRoads );
        }

        // ===================================================================
        // AAA QUALITY INTERSECTION GENERATION
        // ===================================================================

        void CRoadGenerator::createIntersectionsAAA()
        {
            auto city = getCity();
            if( !city )
            {
                return;
            }

            auto roadNetwork = city->getRoadNetwork();
            if( !roadNetwork )
            {
                return;
            }

            size_t connectionCount = 0;

            WPRoadSystem roadSystem( static_cast<u32>( m_options.seed ) );
            auto nodes = roadNetwork->getMergedNodes();
            for( auto node : nodes )
            {
                if( node && node->isConnection() )
                {
                    auto transform = node->getWorldTransform();
                    Vector3<real_Num> centerPos = transform.getPosition();

                    // Determine what types of roads connect here to choose intersection type
                    auto connectedRoads = node->getRoads();
                    size_t roadCount = 0;
                    for( auto connectedRoad : connectedRoads )
                    {
                        if( connectedRoad )
                        {
                            ++roadCount;
                        }
                    }

                    IntersectionType type = IntersectionType::X_Crossing;  // Default
                    RoadClass roadClass = RoadClass::Residential;

                    if( roadCount >= 3 )
                    {
                        // Check if any are major roads
                        bool hasMajor = false;
                        for( auto connectedRoad : connectedRoads )
                        {
                            if( connectedRoad && ( connectedRoad->getRoadType() == "MajorRoad" ||
                                                   connectedRoad->getRoadType() == "Arterial" ) )
                            {
                                hasMajor = true;
                                break;
                            }
                        }

                        if( hasMajor && roadCount == 4 )
                        {
                            type = IntersectionType::X_Crossing;
                            roadClass = RoadClass::Arterial;
                        }
                        else if( hasMajor && roadCount == 3 )
                        {
                            type = IntersectionType::T_Junction;
                            roadClass = RoadClass::Arterial;
                        }
                        else if( !hasMajor && roadCount == 4 )
                        {
                            type = IntersectionType::X_Crossing;
                            roadClass = RoadClass::Residential;
                        }
                        else
                        {
                            type = IntersectionType::X_Crossing;  // Fallback
                            roadClass = RoadClass::Residential;
                        }
                    }
                    else if( roadCount == 2 )
                    {
                        // Could be a bend or straight continuation
                        type = IntersectionType::L_Corner;
                        roadClass = RoadClass::Residential;
                    }

                    IntersectionSpec ispec;
                    ispec.center = centerPos;
                    ispec.type = type;
                    ispec.roadClass = roadClass;
                    ispec.surface = WPRoadSystem::getDefaultSurface( roadClass );
                    ispec.seed = m_options.seed + connectionCount * 100 + 50;

                    // For now, use default approaching roads - in a full implementation
                    // these would be calculated based on actual road directions
                    ispec.approaches = {
                        Vector3<real_Num>( -1, 0, 0 ),  // West
                        Vector3<real_Num>( 1, 0, 0 ),   // East
                        Vector3<real_Num>( 0, 0, -1 ),  // South
                        Vector3<real_Num>( 0, 0, 1 )    // North
                    };

                    // Limit approaches based on actual road count
                    if( roadCount < 4 )
                    {
                        ispec.approaches.resize( roadCount );
                    }

                    IntersectionResult intResult = roadSystem.generateIntersection( ispec );

                    // Create a road connection at this intersection
                    auto roadConnection = workphone::make_ptr<CRoadConnection>();
                    roadConnection->setName( "RoadConnection_" + StringUtil::toString( static_cast<u32>(
                                                                     connectionCount ) ) );
                    roadConnection->setPosition( centerPos );

                    // Set connection type based on intersection type
                    switch( type )
                    {
                    case IntersectionType::T_Junction:
                        roadConnection->setType( IRoadConnection::EType::T_Crossing );
                        break;
                    case IntersectionType::X_Crossing:
                        roadConnection->setType( IRoadConnection::EType::X_Crossing );
                        break;
                    case IntersectionType::Roundabout:
                        roadConnection->setType( IRoadConnection::EType::RoundingAbout );
                        break;
                    case IntersectionType::L_Corner:
                        roadConnection->setType( IRoadConnection::EType::L_Connection );
                        break;
                    default:
                        roadConnection->setType( IRoadConnection::EType::X_Crossing );
                        break;
                    }

                    roadConnection->setConnectionType(
                        type == IntersectionType::T_Junction   ? "T junction"
                        : type == IntersectionType::X_Crossing ? "4 way"
                        : type == IntersectionType::Roundabout ? "Roundabout"
                                                               : "L corner" );

                    // Store the AAA intersection result
                    roadConnection->setUserData( StringUtil::getHash( "WPIntersectionResult" ),
                                                 new IntersectionResult( intResult ) );

                    roadNetwork->addRoadConnection( roadConnection );
                }

                connectionCount++;
            }
        }

        void CRoadGenerator::generateRoads8()
        {
            // Reserved for the 8-way radial highway pattern. Re-enable by routing
            // generate() here when the city-center and highway helpers are fully wired.
        }

        void CRoadGenerator::createIntersections()
        {
            auto city = getCity();
            if( !city )
            {
                return;
            }

            auto roadNetwork = city->getRoadNetwork();
            if( !roadNetwork )
            {
                return;
            }

            size_t connectionCount = 0;

            WPRoadSystem roadSystem( static_cast<u32>( m_options.seed ) );
            auto nodes = roadNetwork->getMergedNodes();
            for( auto node : nodes )
            {
                if( node && node->isConnection() )
                {
                    auto transform = node->getWorldTransform();

                    auto roadConnection = workphone::make_ptr<CRoadConnection>();
                    roadConnection->setName( "RoadConnection_" + StringUtil::toString( static_cast<u32>(
                                                                     connectionCount ) ) );
                    roadConnection->setPosition( transform.getPosition() );

                    s32 marker = 0;
                    s32 connection = 0;
                    WP_UNUSED( marker );
                    WP_UNUSED( connection );

                    auto connectedRoads = node->getRoads();
                    for( auto connectedRoad : connectedRoads )
                    {
                        auto roadConnectionData = workphone::make_ptr<CRoadConnectionData>();
                        roadConnectionData->setRoad( connectedRoad );
                        roadConnectionData->setConnection( connection );
                        roadConnectionData->setMarker( marker );
                        roadConnection->addConnection( roadConnectionData );
                    }

                    roadNetwork->addRoadConnection( roadConnection );
                }

                connectionCount++;
            }
        }

        SmartPtr<IRoadConnection> CRoadGenerator::createConnection( const String &assetPath,
                                                                    Transform3<real_Num> t )
        {
            WP_UNUSED( assetPath );
            WP_UNUSED( t );

            // Asset-backed connection instantiation is not yet implemented.
            return nullptr;
        }

        SmartPtr<IRoad> CRoadGenerator::createRingRoad( SmartPtr<IProceduralCityCenter> targetCityCenter,
                                                        f32 distanceAlongRoad )
        {
            WP_UNUSED( distanceAlongRoad );

            if( !targetCityCenter )
            {
                return nullptr;
            }

            return RoadGeneratorUtil::createRingRoad( targetCityCenter, distanceAlongRoad );
        }

        bool CRoadGenerator::isRoad( SmartPtr<ISharedObject> data, const String &id )
        {
            WP_UNUSED( data );
            WP_UNUSED( id );
            return false;
        }

        String CRoadGenerator::getTagValue( SmartPtr<ISharedObject> data, const String &id,
                                            const String &tag )
        {
            WP_UNUSED( data );
            WP_UNUSED( id );
            WP_UNUSED( tag );
            return "";
        }
    }  // namespace procedural
}  // namespace workphone
