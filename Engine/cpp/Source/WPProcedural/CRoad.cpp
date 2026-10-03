#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadHitPoint.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoad, CProceduralObject<IRoad> );

        // Property key constants
        static const char *const kPropRoadType = "roadType";
        static const char *const kPropRoadLength = "roadLength";
        static const char *const kPropNodeCount = "nodeCount";
        static const char *const kPropSectionCount = "sectionCount";
        static const char *const kPropSegmentCount = "segmentCount";
        static const char *const kPropName = "name";

        CRoad::CRoad()
        {
        }

        CRoad::~CRoad()
        {
            unload( nullptr );
        }

        void CRoad::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                for( auto &roadNode : m_roadNodes )
                {
                    if( roadNode )
                    {
                        roadNode->unload( nullptr );
                    }
                }

                for( auto &roadSegment : m_roadSegments )
                {
                    if( roadSegment )
                    {
                        roadSegment->unload( nullptr );
                    }
                }

                for( auto &roadSection : m_roadSections )
                {
                    if( roadSection )
                    {
                        roadSection->unload( nullptr );
                    }
                }

                m_roadNodes.clear();
                m_roadSegments.clear();
                m_roadSections.clear();

                CProceduralObject<IRoad>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoad::build()
        {
            try
            {
                updateBounds();

                auto sections = getRoadSections();
                for( auto &section : sections )
                {
                    if( !section )
                    {
                        WP_LOG_ERROR( "CRoad::build - null section encountered, skipping." );
                        continue;
                    }

                    try
                    {
                        section->build();
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        f32 CRoad::getRoadLength() const
        {
            auto roadLength = static_cast<real_Num>( 0.0 );

            auto roadNodes = getRoadNodes();
            if( roadNodes.size() < 2 )
            {
                return roadLength;
            }

            for( size_t nodeIdx = 0; nodeIdx < roadNodes.size() - 1; ++nodeIdx )
            {
                auto roadNode0 = getNode( nodeIdx );
                auto roadNode1 = getNode( nodeIdx + 1 );

                if( roadNode0 && roadNode1 )
                {
                    auto roadNodePos0 = roadNode0->getWorldTransform().getPosition();
                    auto roadNodePos1 = roadNode1->getWorldTransform().getPosition();
                    roadLength += ( roadNodePos1 - roadNodePos0 ).length();
                }
            }

            return roadLength;
        }

        void CRoad::setRoadType( const String &value )
        {
            m_roadType = value;
        }

        String CRoad::getRoadType() const
        {
            return m_roadType;
        }

        void CRoad::setRoadSegments( Array<SmartPtr<IRoadElement>> value )
        {
            m_roadSegments = value;
        }

        const Array<SmartPtr<IRoadElement>> &CRoad::getRoadSegments() const
        {
            return m_roadSegments;
        }

        Array<SmartPtr<IRoadElement>> &CRoad::getRoadSegments()
        {
            return m_roadSegments;
        }

        const Array<SmartPtr<IRoadNode>> &CRoad::getRoadNodeObjects() const
        {
            return m_roadNodes;
        }

        void CRoad::updateBounds()
        {
            try
            {
                auto bounds = AABB3<real_Num>();
                bool hasMerged = false;

                auto roadNodes = getRoadNodes();
                for( auto &node : roadNodes )
                {
                    if( !node )
                    {
                        continue;
                    }

                    bounds.merge( node->getPosition() );
                    hasMerged = true;
                }

                if( hasMerged )
                {
                    m_Bounds = bounds;
                    auto center = m_Bounds.getCenter();
                    auto halfExtent = m_Bounds.getExtent().length();
                    m_BoundingSphere = Sphere3<real_Num>( center, halfExtent );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool CRoad::isPartOfRoad( const SmartPtr<IRoadNode> &node ) const
        {
            if( !node )
            {
                return false;
            }

            return std::find( m_roadNodes.begin(), m_roadNodes.end(), node ) != m_roadNodes.end();
        }

        bool CRoad::intersects( const SmartPtr<IRoadNode> node, Array<SmartPtr<IRoadNode>> &nodes,
                                bool checkStart ) const
        {
            if( !node )
            {
                return false;
            }

            try
            {
                auto roadNodes = getRoadNodes();
                if( roadNodes.size() < 2 )
                {
                    return false;
                }

                auto nodeSphere = Sphere3<real_Num>( node->getPosition(), static_cast<real_Num>( 0.5 ) );

                size_t startIdx = checkStart ? 0u : 1u;
                for( size_t i = startIdx; i < roadNodes.size() - 1; ++i )
                {
                    auto &n0 = roadNodes[i];
                    auto &n1 = roadNodes[i + 1];
                    if( !n0 || !n1 )
                    {
                        continue;
                    }

                    if( nodeSphere.intersects( n0->getPosition() ) ||
                        nodeSphere.intersects( n1->getPosition() ) )
                    {
                        nodes.push_back( n0 );
                        nodes.push_back( n1 );
                        return true;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool CRoad::intersects( const SmartPtr<IRoad> road, Vector3<real_Num> &intersectionPoint,
                                Array<SmartPtr<IRoadNode>> &nodes, bool checkStart ) const
        {
            if( !road )
            {
                return false;
            }

            try
            {
                auto roadNodesA = getRoadNodes();
                auto roadNodesB = road->getRoadNodes();

                if( roadNodesA.size() < 2 || roadNodesB.size() < 2 )
                {
                    return false;
                }

                size_t startA = checkStart ? 0u : 1u;

                for( size_t i = startA; i < roadNodesA.size() - 1; ++i )
                {
                    auto &a0 = roadNodesA[i];
                    auto &a1 = roadNodesA[i + 1];
                    if( !a0 || !a1 )
                    {
                        continue;
                    }

                    auto posA0 = a0->getPosition();
                    auto posA1 = a1->getPosition();
                    Line2<real_Num> segA( posA0.x, posA0.z, posA1.x, posA1.z );

                    for( size_t j = 0; j < roadNodesB.size() - 1; ++j )
                    {
                        auto &b0 = roadNodesB[j];
                        auto &b1 = roadNodesB[j + 1];
                        if( !b0 || !b1 )
                        {
                            continue;
                        }

                        auto posB0 = b0->getPosition();
                        auto posB1 = b1->getPosition();
                        Line2<real_Num> segB( posB0.x, posB0.z, posB1.x, posB1.z );

                        Vector2<real_Num> hit2d;
                        if( segA.intersectWith( segB, hit2d ) )
                        {
                            intersectionPoint.x = hit2d.x;
                            intersectionPoint.y = ( posA0.y + posB0.y ) * static_cast<real_Num>( 0.5 );
                            intersectionPoint.z = hit2d.y;

                            nodes.push_back( a0 );
                            nodes.push_back( a1 );
                            nodes.push_back( b0 );
                            nodes.push_back( b1 );
                            return true;
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool CRoad::intersects( const SmartPtr<IRoad> road, Vector3<real_Num> &intersectionPoint,
                                bool checkStart ) const
        {
            if( !road )
            {
                return false;
            }

            Array<SmartPtr<IRoadNode>> nodes;
            return intersects( road, intersectionPoint, nodes, checkStart );
        }

        bool CRoad::intersects( const Line2<real_Num> &line, Vector2<real_Num> &intersectionPoint,
                                bool checkStart )
        {
            try
            {
                auto roadNodes = getRoadNodes();
                if( roadNodes.size() < 2 )
                {
                    return false;
                }

                size_t startIdx = checkStart ? 0u : 1u;
                for( size_t i = startIdx; i < roadNodes.size() - 1; ++i )
                {
                    auto &n0 = roadNodes[i];
                    auto &n1 = roadNodes[i + 1];
                    if( !n0 || !n1 )
                    {
                        continue;
                    }

                    auto pos0 = n0->getPosition();
                    auto pos1 = n1->getPosition();
                    Line2<real_Num> seg( pos0.x, pos0.z, pos1.x, pos1.z );

                    if( seg.intersectWith( line, intersectionPoint ) )
                    {
                        return true;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool CRoad::intersects( const Polygon2<real_Num> &polygon )
        {
            try
            {
                auto roadNodes = getRoadNodes();
                if( roadNodes.size() < 2 )
                {
                    return false;
                }

                auto polyPoints = polygon.getPoints();
                if( polyPoints.empty() )
                {
                    return false;
                }

                for( size_t i = 0; i < roadNodes.size() - 1; ++i )
                {
                    auto &n0 = roadNodes[i];
                    auto &n1 = roadNodes[i + 1];
                    if( !n0 || !n1 )
                    {
                        continue;
                    }

                    auto pos0 = n0->getPosition();
                    auto pos1 = n1->getPosition();
                    Line2<real_Num> seg( pos0.x, pos0.z, pos1.x, pos1.z );

                    for( size_t j = 0; j < polyPoints.size(); ++j )
                    {
                        const auto &p0 = polyPoints[j];
                        const auto &p1 = polyPoints[( j + 1 ) % polyPoints.size()];
                        Line2<real_Num> edge( p0.x, p0.y, p1.x, p1.y );

                        Vector2<real_Num> hit;
                        if( seg.intersectWith( edge, hit ) )
                        {
                            return true;
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool CRoad::intersects( const AABB3<real_Num> &box )
        {
            try
            {
                auto roadNodes = getRoadNodes();
                for( auto &node : roadNodes )
                {
                    if( node && box.isPointInside( node->getPosition() ) )
                    {
                        return true;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool CRoad::intersects( const Sphere3<real_Num> &sphere,
                                SmartPtr<IRoadNode> &collidingNode ) const
        {
            try
            {
                auto roadNodes = getRoadNodes();
                for( auto &node : roadNodes )
                {
                    if( node && sphere.intersects( node->getPosition() ) )
                    {
                        collidingNode = node;
                        return true;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool CRoad::intersects( const Sphere3<real_Num> &sphere )
        {
            SmartPtr<IRoadNode> dummy;
            return intersects( sphere, dummy );
        }

        Array<SmartPtr<IRoadHitPoint>> CRoad::intersects( SmartPtr<IRoad> road )
        {
            Array<SmartPtr<IRoadHitPoint>> hitPoints;

            if( !road )
            {
                WP_LOG_ERROR( "CRoad::intersects - null road argument." );
                return hitPoints;
            }

            try
            {
                auto roadNodesA = getRoadNodes();
                auto roadNodesB = road->getRoadNodes();

                if( roadNodesA.size() < 2 || roadNodesB.size() < 2 )
                {
                    return hitPoints;
                }

                for( size_t i = 0; i < roadNodesA.size() - 1; ++i )
                {
                    auto &a0 = roadNodesA[i];
                    auto &a1 = roadNodesA[i + 1];
                    if( !a0 || !a1 )
                    {
                        continue;
                    }

                    auto posA0 = a0->getPosition();
                    auto posA1 = a1->getPosition();
                    Line2<real_Num> segA( posA0.x, posA0.z, posA1.x, posA1.z );

                    for( size_t j = 0; j < roadNodesB.size() - 1; ++j )
                    {
                        auto &b0 = roadNodesB[j];
                        auto &b1 = roadNodesB[j + 1];
                        if( !b0 || !b1 )
                        {
                            continue;
                        }

                        auto posB0 = b0->getPosition();
                        auto posB1 = b1->getPosition();
                        Line2<real_Num> segB( posB0.x, posB0.z, posB1.x, posB1.z );

                        Vector2<real_Num> hit2d;
                        if( segA.intersectWith( segB, hit2d ) )
                        {
                            auto hitPoint = workphone::make_ptr<CRoadHitPoint>();
                            if( hitPoint )
                            {
                                Vector3<real_Num> hitPos(
                                    hit2d.x, ( posA0.y + posB0.y ) * static_cast<real_Num>( 0.5 ),
                                    hit2d.y );

                                hitPoint->setPosition( hitPos );
                                hitPoint->setRoad( this );
                                hitPoint->setOtherRoad( road );
                                hitPoint->setDistance( ( hitPos - posA0 ).length() );
                                hitPoints.push_back( hitPoint );
                            }
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return hitPoints;
        }

        void CRoad::insertNode( const Array<SmartPtr<IRoadNode>> &referenceNodes,
                                SmartPtr<IRoadNode> newNode )
        {
            if( !newNode )
            {
                WP_LOG_ERROR( "CRoad::insertNode - null node provided." );
                return;
            }

            try
            {
                if( referenceNodes.empty() )
                {
                    m_roadNodes.push_back( newNode );
                    return;
                }

                auto lastIt = m_roadNodes.end();
                for( const auto &refNode : referenceNodes )
                {
                    auto it = std::find( m_roadNodes.begin(), m_roadNodes.end(), refNode );
                    if( it != m_roadNodes.end() )
                    {
                        lastIt = it;
                    }
                }

                if( lastIt != m_roadNodes.end() )
                {
                    m_roadNodes.insert( std::next( lastIt ), newNode );
                }
                else
                {
                    m_roadNodes.push_back( newNode );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoad::addNode( SmartPtr<IRoadNode> node )
        {
            if( !node )
            {
                WP_LOG_ERROR( "CRoad::addNode - null node provided." );
                return;
            }

            m_roadNodes.push_back( node );
        }

        void CRoad::removeNode( SmartPtr<IRoadNode> node )
        {
            if( !node )
            {
                return;
            }

            auto it = std::find( m_roadNodes.begin(), m_roadNodes.end(), node );
            if( it != m_roadNodes.end() )
            {
                m_roadNodes.erase( it );
            }
        }

        void CRoad::removeNodes()
        {
            try
            {
                for( auto &node : m_roadNodes )
                {
                    if( node )
                    {
                        node->unload( nullptr );
                    }
                }

                m_roadNodes.clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Array<SmartPtr<IRoadNode>> CRoad::getRoadNodes() const
        {
            auto nodes = getNodes();

            auto roadNodes = Array<SmartPtr<IRoadNode>>();
            roadNodes.reserve( nodes.size() );

            for( auto &node : nodes )
            {
                if( !node )
                {
                    continue;
                }

                auto roadNode = workphone::static_pointer_cast<IRoadNode>( node );
                if( roadNode )
                {
                    roadNodes.push_back( roadNode );
                }
            }

            return roadNodes;
        }

        SmartPtr<IRoadNode> CRoad::getNode( size_t index ) const
        {
            if( index < m_roadNodes.size() )
            {
                return m_roadNodes[index];
            }

            return nullptr;
        }

        SmartPtr<IRoadNode> CRoad::getFirstNode() const
        {
            if( m_roadNodes.empty() == false )
            {
                return m_roadNodes.front();
            }

            return nullptr;
        }

        SmartPtr<IRoadNode> CRoad::getLastNode() const
        {
            if( m_roadNodes.empty() == false )
            {
                return m_roadNodes.back();
            }

            return nullptr;
        }

        Vector3<real_Num> CRoad::getPointOnRoad( real_Num distance,
                                                 Array<SmartPtr<IRoadNode>> &pointNodes )
        {
            auto point = Vector3<real_Num>();

            try
            {
                auto roadNodes = getRoadNodes();
                if( roadNodes.size() < 2 )
                {
                    return point;
                }

                auto roadLength = static_cast<real_Num>( 0.0 );

                for( size_t nodeIdx = 0; nodeIdx < roadNodes.size() - 1; ++nodeIdx )
                {
                    auto &roadNode0 = roadNodes[nodeIdx];
                    auto &roadNode1 = roadNodes[nodeIdx + 1];

                    if( !roadNode0 || !roadNode1 )
                    {
                        continue;
                    }

                    auto roadNodePos0 = roadNode0->getPosition();
                    auto roadNodePos1 = roadNode1->getPosition();

                    auto segmentLength = ( roadNodePos1 - roadNodePos0 ).length();
                    if( segmentLength <= static_cast<real_Num>( 0.0 ) )
                    {
                        continue;
                    }

                    roadLength += segmentLength;

                    if( roadLength > distance )
                    {
                        auto overshoot = roadLength - distance;
                        auto t = static_cast<real_Num>( 1.0 ) - ( overshoot / segmentLength );
                        point = roadNodePos0 + ( roadNodePos1 - roadNodePos0 ) * t;

                        pointNodes.push_back( roadNode0 );
                        pointNodes.push_back( roadNode1 );

                        return point;
                    }
                }

                if( roadNodes.back() )
                {
                    point = roadNodes.back()->getPosition();
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return point;
        }

        Vector3<real_Num> CRoad::getPointOnRoad( real_Num distance )
        {
            Array<SmartPtr<IRoadNode>> unused;
            return getPointOnRoad( distance, unused );
        }

        SmartPtr<IRoad> CRoad::subDivide( real_Num maxNodeDistance )
        {
            if( maxNodeDistance <= static_cast<real_Num>( 0.0 ) )
            {
                WP_LOG_ERROR( "CRoad::subDivide - maxNodeDistance must be greater than zero." );
                return nullptr;
            }

            // Subdivision requires a factory to create new nodes and roads.
            // Deferred to a higher-level system that has factory access.
            return nullptr;
        }

        s32 CRoad::getMarkerFromTransform( const Transform3<real_Num> &transform )
        {
            try
            {
                auto queryPos = transform.getPosition();
                auto roadNodes = getRoadNodes();

                s32 closestIdx = -1;
                auto closestDist = std::numeric_limits<real_Num>::max();

                for( size_t i = 0; i < roadNodes.size(); ++i )
                {
                    auto &node = roadNodes[i];
                    if( !node )
                    {
                        continue;
                    }

                    auto dist = ( node->getPosition() - queryPos ).length();
                    if( dist < closestDist )
                    {
                        closestDist = dist;
                        closestIdx = static_cast<s32>( i );
                    }
                }

                return closestIdx;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return -1;
        }

        void CRoad::addRoadSection( SmartPtr<IRoadSection> section )
        {
            if( !section )
            {
                WP_LOG_ERROR( "CRoad::addRoadSection - null section provided." );
                return;
            }

            m_roadSections.push_back( section );
        }

        void CRoad::removeRoadSection( SmartPtr<IRoadSection> section )
        {
            if( !section )
            {
                return;
            }

            auto it = std::find( m_roadSections.begin(), m_roadSections.end(), section );
            if( it != m_roadSections.end() )
            {
                m_roadSections.erase( it );
            }
        }

        Array<SmartPtr<IRoadSection>> CRoad::getRoadSections() const
        {
            return m_roadSections;
        }

        void CRoad::setRoadSections( Array<SmartPtr<IRoadSection>> value )
        {
            m_roadSections = value;
        }

        Array<SmartPtr<IProceduralNode>> CRoad::getNodes() const
        {
            auto roadNodes = Array<SmartPtr<IProceduralNode>>();

            try
            {
                auto sections = getRoadSections();
                for( auto &section : sections )
                {
                    if( !section )
                    {
                        WP_LOG_ERROR( "CRoad::getNodes - null section encountered, skipping." );
                        continue;
                    }

                    auto nodes = section->getNodes();
                    roadNodes.insert( roadNodes.end(), nodes.begin(), nodes.end() );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return roadNodes;
        }

        SmartPtr<Properties> CRoad::getProperties() const
        {
            try
            {
                auto properties = CProceduralObject<IRoad>::getProperties();
                if( !properties )
                {
                    WP_LOG_ERROR( "CRoad::getProperties - base properties is null." );
                    return nullptr;
                }

                properties->setProperty( kPropName, m_name );
                properties->setProperty( kPropRoadType, m_roadType );
                properties->setProperty( kPropRoadLength, getRoadLength() );
                properties->setProperty( kPropNodeCount, static_cast<s32>( m_roadNodes.size() ) );
                properties->setProperty( kPropSectionCount, static_cast<s32>( m_roadSections.size() ) );
                properties->setProperty( kPropSegmentCount, static_cast<s32>( m_roadSegments.size() ) );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoad::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoad::setProperties - null properties provided." );
                return;
            }

            try
            {
                CProceduralObject<IRoad>::setProperties( properties );

                auto name = m_name;
                auto roadType = m_roadType;

                properties->getPropertyValue( kPropName, name );
                properties->getPropertyValue( kPropRoadType, roadType );

                m_name = name;
                m_roadType = roadType;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // namespace procedural
}  // namespace workphone
