#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/RoadGeneratorUtil.hpp"
#include "WPProcedural/CRoad.hpp"
#include "WPProcedural/CRoadElement.hpp"
#include "WPProcedural/CRoadNode.hpp"
#include "WPProcedural/CRoadSection.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr f32 kMinWidth = 0.01f;
            constexpr f32 kMaxWidth = 1000.0f;
            constexpr f32 kDefaultRoadWidth = 7.0f;
            constexpr f32 kDefaultRoadDistance = 50.0f;
            constexpr f32 kMinRoadDistance = 0.1f;
            constexpr f32 kMaxRoadDistance = 50000.0f;
            constexpr f32 kMinRadius = 0.1f;
            constexpr f32 kMaxRadius = 10000.0f;
            constexpr f32 kMinSphereRadius = 0.001f;
            constexpr f32 kEpsilon = 1.0e-6f;
            constexpr f32 kPi = 3.14159265358979323846f;
            constexpr size_t kMaxGeneratedNodes = 10000u;

            bool isFinite( f32 value )
            {
                return std::isfinite( value );
            }

            bool isPositionValid( const Vector3F &position )
            {
                return isFinite( position.X() ) && isFinite( position.Y() ) && isFinite( position.Z() );
            }

            bool isDirectionValid( const Vector3F &direction )
            {
                return isPositionValid( direction ) && direction.lengthSquared() > kEpsilon;
            }

            bool isSphereValid( const Sphere3F &sphere )
            {
                return isPositionValid( sphere.getCenter() ) && isFinite( sphere.getRadius() ) &&
                       sphere.getRadius() >= kMinSphereRadius;
            }

            f32 sanitiseWidth( f32 width, const char *context )
            {
                if( !isFinite( width ) )
                {
                    WP_LOG_ERROR( String( context ) + " - non-finite width; using default width." );
                    return kDefaultRoadWidth;
                }

                if( width < kMinWidth || width > kMaxWidth )
                {
                    WP_LOG_WARNING( String( context ) + " - width outside supported range; clamping." );
                    return std::clamp( width, kMinWidth, kMaxWidth );
                }

                return width;
            }

            f32 sanitiseDistance( f32 distance, f32 fallback, const char *context )
            {
                if( !isFinite( distance ) || distance < kMinRoadDistance )
                {
                    WP_LOG_WARNING( String( context ) + " - invalid distance; using a safe fallback." );
                    return fallback;
                }

                if( distance > kMaxRoadDistance )
                {
                    WP_LOG_WARNING( String( context ) +
                                    " - distance exceeds supported range; clamping." );
                    return kMaxRoadDistance;
                }

                return distance;
            }

            Vector3F normalisedOr( const Vector3F &value, const Vector3F &fallback )
            {
                if( isDirectionValid( value ) )
                {
                    return value.normaliseCopy();
                }

                if( isDirectionValid( fallback ) )
                {
                    return fallback.normaliseCopy();
                }

                return Vector3F( 1.0f, 0.0f, 0.0f );
            }

            Vector3F roadPerpendicular( const Vector3F &direction )
            {
                auto horizontal = Vector3F( direction.X(), 0.0f, direction.Z() );
                if( !isDirectionValid( horizontal ) )
                {
                    horizontal = direction;
                }

                const auto normalised = normalisedOr( horizontal, Vector3F( 1.0f, 0.0f, 0.0f ) );
                return normalisedOr( Vector3F( -normalised.Z(), 0.0f, normalised.X() ),
                                     Vector3F( 0.0f, 0.0f, 1.0f ) );
            }

            bool positionsDiffer( const Vector3F &a, const Vector3F &b )
            {
                return ( b - a ).lengthSquared() > kEpsilon;
            }

            Array<Vector3F> sanitisePath( const Array<Vector3F> &points, const char *context )
            {
                Array<Vector3F> result;
                result.reserve( std::min( points.size(), kMaxGeneratedNodes ) );

                for( size_t i = 0; i < points.size(); ++i )
                {
                    if( result.size() >= kMaxGeneratedNodes )
                    {
                        WP_LOG_WARNING( String( context ) +
                                        " - node limit reached; remaining points were skipped." );
                        break;
                    }

                    const auto &point = points[i];
                    if( !isPositionValid( point ) )
                    {
                        WP_LOG_ERROR( String( context ) + " - invalid point at index " +
                                      std::to_string( i ) + "; point was skipped." );
                        continue;
                    }

                    if( !result.empty() && !positionsDiffer( result.back(), point ) )
                    {
                        WP_LOG_WARNING( String( context ) + " - duplicate point at index " +
                                        std::to_string( i ) + "; point was skipped." );
                        continue;
                    }

                    result.push_back( point );
                }

                return result;
            }

            SmartPtr<IRoadElement> createElementFromNodes( const SmartPtr<IRoadNode> &nodeA,
                                                           const SmartPtr<IRoadNode> &nodeB, f32 width,
                                                           SmartPtr<IRoadSection> section,
                                                           IRoad::RoadType roadType )
            {
                if( !nodeA || !nodeB )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil - cannot create an element from null nodes." );
                    return nullptr;
                }

                auto element = workphone::make_ptr<CRoadElement>();
                if( !element )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil - failed to allocate a road element." );
                    return nullptr;
                }

                element->setRoadWidth( width );
                element->setRoadType( roadType );
                element->setLaneType( width >= 10.0f ? IRoad::LaneType::FourLane
                                                     : IRoad::LaneType::TwoLane );
                element->setParentSection( section );
                element->addNode( nodeA );
                element->addNode( nodeB );
                return element;
            }

            SmartPtr<IRoad> createRoadFromPath( const Array<Vector3F> &sourcePoints,
                                                const String &roadType, f32 width,
                                                SmartPtr<IRoad> connectedRoad = nullptr )
            {
                auto points = sanitisePath( sourcePoints, "RoadGeneratorUtil::createRoadFromPath" );
                if( points.size() < 2u )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createRoadFromPath - at least two distinct valid points are "
                        "required." );
                    return nullptr;
                }

                width = sanitiseWidth( width, "RoadGeneratorUtil::createRoadFromPath" );

                auto road = workphone::make_ptr<CRoad>();
                auto section = workphone::make_ptr<CRoadSection>();
                if( !road || !section )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createRoadFromPath - failed to allocate road objects." );
                    return nullptr;
                }

                road->setRoadType( roadType );
                road->addRoadSection( section );

                Array<SmartPtr<IRoadNode>> nodes;
                nodes.reserve( points.size() );

                for( size_t i = 0; i < points.size(); ++i )
                {
                    try
                    {
                        auto node = workphone::make_ptr<CRoadNode>();
                        if( !node )
                        {
                            WP_LOG_ERROR(
                                "RoadGeneratorUtil::createRoadFromPath - failed to allocate node " +
                                std::to_string( i ) + "." );
                            continue;
                        }

                        node->setPosition( points[i] );
                        node->addRoad( road );
                        section->addNode( node );
                        road->addNode( node );

                        if( !nodes.empty() )
                        {
                            nodes.back()->connect( node );
                            node->connect( nodes.back() );
                        }

                        nodes.push_back( node );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                    catch( ... )
                    {
                        WP_LOG_ERROR(
                            "RoadGeneratorUtil::createRoadFromPath - unknown exception while creating a "
                            "node; continuing." );
                    }
                }

                if( nodes.size() < 2u )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createRoadFromPath - fewer than two nodes could be "
                        "created." );
                    return nullptr;
                }

                if( connectedRoad )
                {
                    nodes.front()->addRoad( connectedRoad );
                    nodes.front()->setIsConnection( true );
                }

                const auto elementRoadType =
                    roadType == "MajorRoad" ? IRoad::RoadType::Trunk : IRoad::RoadType::Residential;
                Array<SmartPtr<IRoadElement>> elements;
                elements.reserve( nodes.size() - 1u );

                for( size_t i = 0; i + 1u < nodes.size(); ++i )
                {
                    try
                    {
                        auto element = createElementFromNodes( nodes[i], nodes[i + 1u], width, section,
                                                               elementRoadType );
                        if( element )
                        {
                            elements.push_back( element );
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                    catch( ... )
                    {
                        WP_LOG_ERROR(
                            "RoadGeneratorUtil::createRoadFromPath - unknown exception while creating "
                            "an element; continuing." );
                    }
                }

                section->setElements( elements );
                road->setRoadSegments( elements );
                road->updateBounds();
                return road;
            }

            Array<Vector3F> makeStraightPath( const Vector3F &start, const Vector3F &direction,
                                              f32 distance )
            {
                Array<Vector3F> points;
                points.reserve( 2u );
                points.push_back( start );
                points.push_back( start +
                                  normalisedOr( direction, Vector3F( 1.0f, 0.0f, 0.0f ) ) * distance );
                return points;
            }

            Array<Vector3F> makeHermitePath( const Vector3F &start, const Vector3F &target,
                                             const Vector3F &initialDirection )
            {
                Array<Vector3F> points;
                const auto delta = target - start;
                const auto distance = delta.length();
                if( !isFinite( distance ) || distance <= kMinRoadDistance )
                {
                    return points;
                }

                const auto segmentCount = std::clamp<size_t>(
                    static_cast<size_t>( std::ceil( distance / kDefaultRoadDistance ) ), 1u,
                    kMaxGeneratedNodes - 1u );
                points.reserve( segmentCount + 1u );

                const auto targetDirection = normalisedOr( delta, Vector3F( 1.0f, 0.0f, 0.0f ) );
                const auto startDirection = normalisedOr( initialDirection, targetDirection );
                const auto tangentA = startDirection * distance;
                const auto tangentB = targetDirection * distance;

                for( size_t i = 0; i <= segmentCount; ++i )
                {
                    const auto t = static_cast<f32>( i ) / static_cast<f32>( segmentCount );
                    const auto t2 = t * t;
                    const auto t3 = t2 * t;
                    const auto h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
                    const auto h10 = t3 - 2.0f * t2 + t;
                    const auto h01 = -2.0f * t3 + 3.0f * t2;
                    const auto h11 = t3 - t2;
                    points.push_back( start * h00 + tangentA * h10 + target * h01 + tangentB * h11 );
                }

                return points;
            }

            Array<Vector3F> makeCircularPath( const Vector3F &center, f32 radius, size_t segmentCount )
            {
                Array<Vector3F> points;
                segmentCount = std::clamp<size_t>( segmentCount, 8u, 256u );
                points.reserve( segmentCount + 1u );

                for( size_t i = 0; i <= segmentCount; ++i )
                {
                    const auto angle =
                        2.0f * kPi * static_cast<f32>( i ) / static_cast<f32>( segmentCount );
                    points.push_back( center + Vector3F( std::cos( angle ) * radius, 0.0f,
                                                         std::sin( angle ) * radius ) );
                }

                return points;
            }
        }  // namespace

        SmartPtr<IProceduralCity> StraightRoadData::getCity() const
        {
            return m_city;
        }

        void StraightRoadData::setCity( SmartPtr<IProceduralCity> value )
        {
            m_city = value;
        }

        Vector3F StraightRoadData::getStartPosition() const
        {
            return m_startPosition;
        }

        void StraightRoadData::setStartPosition( const Vector3F &value )
        {
            m_startPosition = value;
        }

        Vector3F StraightRoadData::getStartDirection() const
        {
            return m_startDirection;
        }

        void StraightRoadData::setStartDirection( const Vector3F &value )
        {
            m_startDirection = value;
        }

        SmartPtr<IRoadElement> RoadGeneratorUtil::createRoadSegment( const Vector3F &posA,
                                                                     const Vector3F &posB,
                                                                     const Vector3F &tangentA,
                                                                     const Vector3F &tangentB,
                                                                     f32 widthA, f32 widthB )
        {
            try
            {
                if( !isPositionValid( posA ) || !isPositionValid( posB ) )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRoadSegment - endpoints must be finite." );
                    return nullptr;
                }

                if( !positionsDiffer( posA, posB ) )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRoadSegment - endpoints must be distinct." );
                    return nullptr;
                }

                if( !isDirectionValid( tangentA ) || !isDirectionValid( tangentB ) )
                {
                    WP_LOG_WARNING(
                        "RoadGeneratorUtil::createRoadSegment - invalid tangent; geometry consumers "
                        "will derive one from the endpoints." );
                }

                widthA = sanitiseWidth( widthA, "RoadGeneratorUtil::createRoadSegment" );
                widthB = sanitiseWidth( widthB, "RoadGeneratorUtil::createRoadSegment" );

                auto nodeA = workphone::make_ptr<CRoadNode>();
                auto nodeB = workphone::make_ptr<CRoadNode>();
                if( !nodeA || !nodeB )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createRoadSegment - failed to allocate endpoint nodes." );
                    return nullptr;
                }

                nodeA->setPosition( posA );
                nodeB->setPosition( posB );
                nodeA->connect( nodeB );
                nodeB->connect( nodeA );

                auto element = createElementFromNodes( nodeA, nodeB, ( widthA + widthB ) * 0.5f, nullptr,
                                                       IRoad::RoadType::Residential );
                if( element )
                {
                    element->build();
                }
                return element;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR(
                    "RoadGeneratorUtil::createRoadSegment - unknown exception; segment was not "
                    "created." );
            }

            return nullptr;
        }

        SmartPtr<IMesh> RoadGeneratorUtil::buildMesh( const Array<SmartPtr<IRoadElement>> &roadSegments )
        {
            try
            {
                if( roadSegments.empty() )
                {
                    WP_LOG_WARNING( "RoadGeneratorUtil::buildMesh - no road segments supplied." );
                    return nullptr;
                }

                Array<Vector3F> positions;
                Array<Vector2F> uvs;

                for( size_t segmentIndex = 0; segmentIndex < roadSegments.size(); ++segmentIndex )
                {
                    try
                    {
                        const auto &segment = roadSegments[segmentIndex];
                        if( !segment )
                        {
                            WP_LOG_ERROR( "RoadGeneratorUtil::buildMesh - null segment at index " +
                                          std::to_string( segmentIndex ) + "; skipped." );
                            continue;
                        }

                        auto nodes = segment->getRoadNodes();
                        if( nodes.size() < 2u )
                        {
                            WP_LOG_ERROR( "RoadGeneratorUtil::buildMesh - segment at index " +
                                          std::to_string( segmentIndex ) +
                                          " has fewer than two nodes; skipped." );
                            continue;
                        }

                        f32 width = kDefaultRoadWidth;
                        if( auto concrete = workphone::dynamic_pointer_cast<CRoadElement>( segment ) )
                        {
                            width = concrete->getRoadWidth();
                        }
                        width = sanitiseWidth( width, "RoadGeneratorUtil::buildMesh" ) * 0.5f;

                        for( size_t nodeIndex = 0; nodeIndex + 1u < nodes.size(); ++nodeIndex )
                        {
                            if( !nodes[nodeIndex] || !nodes[nodeIndex + 1u] )
                            {
                                WP_LOG_ERROR( "RoadGeneratorUtil::buildMesh - null node in segment " +
                                              std::to_string( segmentIndex ) + "; pair skipped." );
                                continue;
                            }

                            const auto posA = nodes[nodeIndex]->getPosition();
                            const auto posB = nodes[nodeIndex + 1u]->getPosition();
                            const auto tangent = roadPerpendicular( posB - posA );
                            createMeshSegment( posA, posB, tangent, tangent, width, width, positions,
                                               uvs );
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                    catch( ... )
                    {
                        WP_LOG_ERROR(
                            "RoadGeneratorUtil::buildMesh - unknown segment error; continuing." );
                    }
                }

                if( positions.empty() || positions.size() != uvs.size() )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::buildMesh - no consistent mesh geometry could be "
                        "generated." );
                    return nullptr;
                }

                if( positions.size() > std::numeric_limits<u32>::max() )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::buildMesh - generated mesh exceeds 32-bit index limits." );
                    return nullptr;
                }

                Array<Vector3<real_Num>> meshPositions( positions.begin(), positions.end() );
                Array<Vector3<real_Num>> normals;
                Array<Vector2<real_Num>> meshUvs( uvs.begin(), uvs.end() );
                Array<u32> indices;
                normals.assign( meshPositions.size(), Vector3<real_Num>( 0.0, 1.0, 0.0 ) );
                indices.reserve( meshPositions.size() );
                for( u32 i = 0; i < static_cast<u32>( meshPositions.size() ); ++i )
                {
                    indices.push_back( i );
                }

                auto mesh = MeshUtil::createMesh( meshPositions, normals, meshUvs, indices );
                if( !mesh )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::buildMesh - mesh creation failed." );
                    return nullptr;
                }

                if( auto subMesh = mesh->getSubMesh( 0u ) )
                {
                    subMesh->setMaterialName( "RoadDefault" );
                }
                return mesh;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR(
                    "RoadGeneratorUtil::buildMesh - unknown exception; mesh was not created." );
            }

            return nullptr;
        }

        void RoadGeneratorUtil::createMeshSegment( const Vector3F &posA, const Vector3F &posB,
                                                   const Vector3F &tangentA, const Vector3F &tangentB,
                                                   f32 widthA, f32 widthB,
                                                   Array<Vector3F> &vertexPositions,
                                                   Array<Vector2F> &uvs )
        {
            try
            {
                if( !isPositionValid( posA ) || !isPositionValid( posB ) ||
                    !positionsDiffer( posA, posB ) )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createMeshSegment - endpoints are invalid or coincident." );
                    return;
                }

                widthA = sanitiseWidth( widthA, "RoadGeneratorUtil::createMeshSegment" );
                widthB = sanitiseWidth( widthB, "RoadGeneratorUtil::createMeshSegment" );

                const auto fallback = roadPerpendicular( posB - posA );
                const auto sideA = normalisedOr( tangentA, fallback );
                const auto sideB = normalisedOr( tangentB, fallback );

                const auto p0 = posA - sideA * widthA;
                const auto p1 = posA + sideA * widthA;
                const auto p2 = posB + sideB * widthB;
                const auto p3 = posB - sideB * widthB;

                const Vector3F localPositions[] = { p0, p1, p2, p0, p2, p3 };
                const Vector2F localUvs[] = { Vector2F( 0.0f, 0.0f ), Vector2F( 0.0f, 1.0f ),
                                              Vector2F( 1.0f, 1.0f ), Vector2F( 0.0f, 0.0f ),
                                              Vector2F( 1.0f, 1.0f ), Vector2F( 1.0f, 0.0f ) };
                vertexPositions.insert( vertexPositions.end(), std::begin( localPositions ),
                                        std::end( localPositions ) );
                uvs.insert( uvs.end(), std::begin( localUvs ), std::end( localUvs ) );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR(
                    "RoadGeneratorUtil::createMeshSegment - unknown exception; segment was skipped." );
            }
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createHighway( SmartPtr<HighwayInfo> highwayInfo )
        {
            try
            {
                if( !highwayInfo )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createHighway - null configuration." );
                    return nullptr;
                }
                if( !isPositionValid( highwayInfo->StartPosition ) ||
                    !isPositionValid( highwayInfo->TargetPosition ) )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createHighway - positions must be finite." );
                    return nullptr;
                }
                if( !positionsDiffer( highwayInfo->StartPosition, highwayInfo->TargetPosition ) )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createHighway - start and target coincide." );
                    return nullptr;
                }

                const auto path =
                    makeHermitePath( highwayInfo->StartPosition, highwayInfo->TargetPosition,
                                     highwayInfo->InitialDirection );
                return createRoadFromPath( path, "MajorRoad", 14.0f, highwayInfo->TargetRoad );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createHighway - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createHighway( SmartPtr<ICityCenter> targetCityCenter,
                                                          const Vector3F &startPos,
                                                          const Vector3F &targetPosition,
                                                          const Vector3F &startDir,
                                                          SmartPtr<IRoad> targetRoad )
        {
            if( !targetCityCenter )
            {
                WP_LOG_WARNING(
                    "RoadGeneratorUtil::createHighway - no city center supplied; generating from "
                    "explicit positions." );
            }

            auto info = workphone::make_ptr<HighwayInfo>();
            if( !info )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createHighway - failed to allocate configuration." );
                return nullptr;
            }
            info->CityCenter = targetCityCenter;
            info->StartPosition = startPos;
            info->TargetPosition = targetPosition;
            info->InitialDirection = startDir;
            info->TargetRoad = targetRoad;
            return createHighway( info );
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createStraightRoad(
            SmartPtr<StraightRoadInfo> straightRoadInfo )
        {
            try
            {
                if( !straightRoadInfo )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createStraightRoad - null configuration." );
                    return nullptr;
                }
                if( !isPositionValid( straightRoadInfo->StartPos ) ||
                    !isDirectionValid( straightRoadInfo->StartDir ) )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createStraightRoad - start position or direction is "
                        "invalid." );
                    return nullptr;
                }

                const auto distance =
                    sanitiseDistance( straightRoadInfo->MinRoadDistance, kDefaultRoadDistance,
                                      "RoadGeneratorUtil::createStraightRoad" );
                return createRoadFromPath(
                    makeStraightPath( straightRoadInfo->StartPos, straightRoadInfo->StartDir, distance ),
                    "Residential", kDefaultRoadWidth, straightRoadInfo->ConnentedRoad );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createStraightRoad - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createStraightRoad(
            SmartPtr<StraightRoadData> straightRoadData )
        {
            try
            {
                if( !straightRoadData )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createStraightRoad - null road data." );
                    return nullptr;
                }

                auto distance = kDefaultRoadDistance;
                if( auto city = straightRoadData->getCity() )
                {
                    const auto size = city->getSize();
                    const auto cityExtent = static_cast<f32>( std::max( size.X(), size.Y() ) );
                    if( isFinite( cityExtent ) && cityExtent > kMinRoadDistance )
                    {
                        distance = std::clamp( cityExtent, kDefaultRoadDistance, kMaxRoadDistance );
                    }
                }

                const auto start = straightRoadData->getStartPosition();
                const auto direction = straightRoadData->getStartDirection();
                if( !isPositionValid( start ) || !isDirectionValid( direction ) )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createStraightRoad - start position or direction is "
                        "invalid." );
                    return nullptr;
                }

                return createRoadFromPath( makeStraightPath( start, direction, distance ), "Residential",
                                           kDefaultRoadWidth );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createStraightRoad - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createRadialPatternRoad(
            SmartPtr<RadialPatternInfo> radialPatternInfo )
        {
            try
            {
                if( !radialPatternInfo )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRadialPatternRoad - null configuration." );
                    return nullptr;
                }
                if( !isPositionValid( radialPatternInfo->StartPos ) ||
                    !isDirectionValid( radialPatternInfo->StartDir ) )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createRadialPatternRoad - start position or direction is "
                        "invalid." );
                    return nullptr;
                }

                auto distance = kDefaultRoadDistance;
                if( radialPatternInfo->CityCenter )
                {
                    const auto radius = radialPatternInfo->CityCenter->getRadius();
                    distance = sanitiseDistance( radius, kDefaultRoadDistance,
                                                 "RoadGeneratorUtil::createRadialPatternRoad" );
                }

                return createRoadFromPath( makeStraightPath( radialPatternInfo->StartPos,
                                                             radialPatternInfo->StartDir, distance ),
                                           "Residential", kDefaultRoadWidth,
                                           radialPatternInfo->ConnentedRoad );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createRadialPatternRoad - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createRoad( SmartPtr<IRoad> connentedRoad, Vector3F startPos,
                                                       Vector3F startDir )
        {
            try
            {
                if( !isPositionValid( startPos ) || !isDirectionValid( startDir ) )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createRoad - start position or direction is invalid." );
                    return nullptr;
                }
                return createRoadFromPath( makeStraightPath( startPos, startDir, kDefaultRoadDistance ),
                                           "Residential", kDefaultRoadWidth, connentedRoad );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createRoad - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createRingRoad( SmartPtr<ICityCenter> targetCityCenter,
                                                           f32 distanceAlongRoad )
        {
            try
            {
                if( !targetCityCenter )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRingRoad - null city center." );
                    return nullptr;
                }
                const auto center = targetCityCenter->getPosition();
                if( !isPositionValid( center ) )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRingRoad - invalid city center position." );
                    return nullptr;
                }

                auto radius = distanceAlongRoad;
                if( !isFinite( radius ) || radius < kMinRadius )
                {
                    radius = targetCityCenter->getRadius();
                }
                if( !isFinite( radius ) || radius < kMinRadius )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRingRoad - no valid radius available." );
                    return nullptr;
                }
                radius = std::clamp( radius, kMinRadius, kMaxRadius );

                const auto segmentCount = std::clamp<size_t>(
                    static_cast<size_t>( std::ceil( 2.0f * kPi * radius / kDefaultRoadDistance ) ), 12u,
                    128u );
                return createRoadFromPath( makeCircularPath( center, radius, segmentCount ), "RingRoad",
                                           kDefaultRoadWidth );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createRingRoad - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createRoundAbout( const Vector3F &centerPosition, f32 radius )
        {
            try
            {
                if( !isPositionValid( centerPosition ) || !isFinite( radius ) || radius < kMinRadius )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createRoundAbout - center or radius is invalid." );
                    return nullptr;
                }
                radius = std::clamp( radius, kMinRadius, kMaxRadius );
                const auto segmentCount = std::clamp<size_t>(
                    static_cast<size_t>( std::ceil( 2.0f * kPi * radius / 5.0f ) ), 12u, 64u );
                return createRoadFromPath( makeCircularPath( centerPosition, radius, segmentCount ),
                                           "RoundAbout", kDefaultRoadWidth );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createRoundAbout - unknown exception." );
            }
            return nullptr;
        }

        SmartPtr<IRoad> RoadGeneratorUtil::createBranch( SmartPtr<IRoad> connentedRoad,
                                                         Vector3F startPos, Vector3F startDir )
        {
            if( !connentedRoad )
            {
                WP_LOG_WARNING(
                    "RoadGeneratorUtil::createBranch - connected road is null; creating a standalone "
                    "road." );
            }
            return createRoad( connentedRoad, startPos, startDir );
        }

        void RoadGeneratorUtil::createCheckerPattern( SmartPtr<ICityBlock> block,
                                                      Array<SmartPtr<IRoad>> &roads )
        {
            try
            {
                if( !block )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::createCheckerPattern - null block." );
                    return;
                }

                auto points = block->getPoints();
                if( points.size() < 3u )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createCheckerPattern - block needs at least three points." );
                    return;
                }

                f32 minX = std::numeric_limits<f32>::max();
                f32 minZ = std::numeric_limits<f32>::max();
                f32 maxX = std::numeric_limits<f32>::lowest();
                f32 maxZ = std::numeric_limits<f32>::lowest();
                f32 y = 0.0f;
                size_t validPointCount = 0u;
                for( size_t i = 0; i < points.size(); ++i )
                {
                    const Vector3F point( static_cast<f32>( points[i].X() ),
                                          static_cast<f32>( points[i].Y() ),
                                          static_cast<f32>( points[i].Z() ) );
                    if( !isPositionValid( point ) )
                    {
                        WP_LOG_ERROR( "RoadGeneratorUtil::createCheckerPattern - invalid block point " +
                                      std::to_string( i ) + "; skipped." );
                        continue;
                    }
                    minX = std::min( minX, point.X() );
                    minZ = std::min( minZ, point.Z() );
                    maxX = std::max( maxX, point.X() );
                    maxZ = std::max( maxZ, point.Z() );
                    y += point.Y();
                    ++validPointCount;
                }

                if( validPointCount < 3u || maxX - minX <= kMinRoadDistance ||
                    maxZ - minZ <= kMinRoadDistance )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::createCheckerPattern - block bounds are degenerate." );
                    return;
                }
                y /= static_cast<f32>( validPointCount );

                const auto spacing = kDefaultRoadDistance;
                const auto xLineCount = std::min<size_t>(
                    static_cast<size_t>( std::floor( ( maxX - minX ) / spacing ) ) + 1u, 128u );
                const auto zLineCount = std::min<size_t>(
                    static_cast<size_t>( std::floor( ( maxZ - minZ ) / spacing ) ) + 1u, 128u );

                for( size_t i = 0; i < xLineCount; ++i )
                {
                    const auto x = std::min( minX + static_cast<f32>( i ) * spacing, maxX );
                    auto road = createRoadFromPath( { Vector3F( x, y, minZ ), Vector3F( x, y, maxZ ) },
                                                    "Residential", kDefaultRoadWidth );
                    if( road )
                    {
                        roads.push_back( road );
                    }
                }

                for( size_t i = 0; i < zLineCount; ++i )
                {
                    const auto z = std::min( minZ + static_cast<f32>( i ) * spacing, maxZ );
                    auto road = createRoadFromPath( { Vector3F( minX, y, z ), Vector3F( maxX, y, z ) },
                                                    "Residential", kDefaultRoadWidth );
                    if( road )
                    {
                        roads.push_back( road );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::createCheckerPattern - unknown exception." );
            }
        }

        bool RoadGeneratorUtil::isRoadFinished( SmartPtr<RoadFinishedInfo> info )
        {
            try
            {
                if( !info )
                {
                    WP_LOG_ERROR(
                        "RoadGeneratorUtil::isRoadFinished - null state; stopping defensively." );
                    return true;
                }
                if( !info->IsInCityBoundaries )
                {
                    return true;
                }
                if( !info->CollidesWithTerrain )
                {
                    return true;
                }
                if( info->CollidingRoad )
                {
                    const auto roadType = info->CollidingRoad->getRoadType();
                    return roadType == "RoundAbout" || roadType == "MajorRoad";
                }
                return false;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::isRoadFinished - unknown exception." );
            }
            return true;
        }

        bool RoadGeneratorUtil::isNearExistingRoadNode( const Array<SmartPtr<IRoad>> &roads,
                                                        SmartPtr<IRoad> currentRoad, Sphere3F nodeSphere,
                                                        SmartPtr<IRoadNode> &roadNode,
                                                        SmartPtr<IRoad> &collidingRoad )
        {
            roadNode = nullptr;
            collidingRoad = nullptr;

            try
            {
                if( !isSphereValid( nodeSphere ) )
                {
                    WP_LOG_ERROR( "RoadGeneratorUtil::isNearExistingRoadNode - invalid query sphere." );
                    return false;
                }

                const auto queryCenter = nodeSphere.getCenter();
                const auto radiusSquared = nodeSphere.getRadius() * nodeSphere.getRadius();
                auto closestDistanceSquared = std::numeric_limits<f32>::max();

                for( size_t roadIndex = 0; roadIndex < roads.size(); ++roadIndex )
                {
                    try
                    {
                        const auto &candidateRoad = roads[roadIndex];
                        if( !candidateRoad )
                        {
                            WP_LOG_ERROR(
                                "RoadGeneratorUtil::isNearExistingRoadNode - null road at index " +
                                std::to_string( roadIndex ) + "; skipped." );
                            continue;
                        }
                        if( candidateRoad == currentRoad )
                        {
                            continue;
                        }

                        auto candidateNodes = candidateRoad->getRoadNodes();
                        if( candidateNodes.empty() )
                        {
                            if( auto concreteRoad =
                                    workphone::dynamic_pointer_cast<CRoad>( candidateRoad ) )
                            {
                                candidateNodes = concreteRoad->getRoadNodeObjects();
                            }
                        }
                        for( const auto &candidateNode : candidateNodes )
                        {
                            if( !candidateNode )
                            {
                                WP_LOG_ERROR(
                                    "RoadGeneratorUtil::isNearExistingRoadNode - null node skipped." );
                                continue;
                            }

                            const auto position = candidateNode->getPosition();
                            if( !isPositionValid( position ) )
                            {
                                WP_LOG_ERROR(
                                    "RoadGeneratorUtil::isNearExistingRoadNode - invalid node position "
                                    "skipped." );
                                continue;
                            }

                            const auto distanceSquared =
                                static_cast<f32>( ( queryCenter - position ).lengthSquared() );
                            if( distanceSquared <= radiusSquared &&
                                distanceSquared < closestDistanceSquared )
                            {
                                closestDistanceSquared = distanceSquared;
                                roadNode = candidateNode;
                                collidingRoad = candidateRoad;
                            }
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                    catch( ... )
                    {
                        WP_LOG_ERROR(
                            "RoadGeneratorUtil::isNearExistingRoadNode - unknown road error; "
                            "continuing." );
                    }
                }

                return roadNode != nullptr;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "RoadGeneratorUtil::isNearExistingRoadNode - unknown exception." );
            }
            return false;
        }
    }  // namespace procedural
}  // namespace workphone
