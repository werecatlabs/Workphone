#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/WPRoadSystem.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>
#include <limits>
#include <unordered_map>

namespace workphone
{
    namespace procedural
    {

        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadNetwork, IRoadNetwork );

        // ---------------------------------------------------------------
        // Property key constants
        // ---------------------------------------------------------------

        static const char *const kPropName = "name";
        static const char *const kPropRoadCount = "roadCount";
        static const char *const kPropNodeCount = "nodeCount";
        static const char *const kPropMergedNodeCount = "mergedNodeCount";
        static const char *const kPropConnectionCount = "connectionCount";
        static const char *const kPropMajorRoadCount = "majorRoadCount";
        static const char *const kPropMinorRoadCount = "minorRoadCount";

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadNetwork::CRoadNetwork()
        {
            // Intentionally empty � member containers are default-constructed.
        }

        CRoadNetwork::~CRoadNetwork()
        {
            unload( nullptr );
        }

        void CRoadNetwork::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                for( auto &node : m_nodes )
                {
                    if( node )
                    {
                        node->unload( data );
                    }
                }
                m_nodes.clear();

                for( auto &node : m_mergedNodes )
                {
                    if( node )
                    {
                        node->unload( data );
                    }
                }
                m_mergedNodes.clear();

                m_majorRoads.clear();
                m_minorRoads.clear();

                for( auto &node : m_mergedNetworkNodes )
                {
                    if( node )
                    {
                        node->unload( data );
                    }
                }
                m_mergedNetworkNodes.clear();

                for( auto &road : m_roads )
                {
                    if( road )
                    {
                        road->unload( data );
                    }
                }
                m_roads.clear();

                for( auto &roadConnection : m_roadConnections )
                {
                    if( roadConnection )
                    {
                        roadConnection->unload( data );
                    }
                }
                m_roadConnections.clear();

                // Clear cached mesh data
                m_cachedSegmentResults.clear();
                m_cachedIntersectionResults.clear();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Node queries
        // ---------------------------------------------------------------

        SmartPtr<IRoadNode> CRoadNetwork::getNodeByGraphId( s32 id ) const
        {
            try
            {
                for( auto &r : m_roads )
                {
                    if( !r )
                    {
                        continue;
                    }

                    auto nodes = r->getNodes();
                    for( auto &n : nodes )
                    {
                        if( !n )
                        {
                            continue;
                        }

                        if( n->getGraphId() == id )
                        {
                            return workphone::static_pointer_cast<IRoadNode>( n );
                        }
                    }
                }

                for( auto &rc : m_roadConnections )
                {
                    if( !rc )
                    {
                        continue;
                    }

                    auto node = rc->getNode();
                    if( node && node->getGraphId() == id )
                    {
                        return node;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        Array<SmartPtr<IRoadNode>> CRoadNetwork::getNodes() const
        {
            return m_nodes;
        }

        void CRoadNetwork::setNodes( const Array<SmartPtr<IRoadNode>> &value )
        {
            m_nodes = value;
        }

        Array<SmartPtr<IRoadNode>> CRoadNetwork::getMergedNodes() const
        {
            return m_mergedNodes;
        }

        void CRoadNetwork::setMergedNodes( const Array<SmartPtr<IRoadNode>> &mergedNodes )
        {
            m_mergedNodes = mergedNodes;
        }

        const Array<SmartPtr<IRoad>> &CRoadNetwork::getRoads() const
        {
            return m_roads;
        }

        Array<SmartPtr<IRoad>> &CRoadNetwork::getRoads()
        {
            return m_roads;
        }

        void CRoadNetwork::removeRoad( SmartPtr<IRoad> road )
        {
            try
            {
                if( !road )
                {
                    return;
                }

                auto it = std::find( m_roads.begin(), m_roads.end(), road );
                if( it != m_roads.end() )
                {
                    m_roads.erase( it );

                    // Clean up cached data
                    m_cachedSegmentResults.erase( road.get() );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadNetwork::addRoad( SmartPtr<IRoad> road )
        {
            try
            {
                if( !road )
                {
                    WP_LOG_ERROR( "CRoadNetwork::addRoad - null road provided." );
                    return;
                }

                // Check if already present
                if( std::find( m_roads.begin(), m_roads.end(), road ) != m_roads.end() )
                {
                    return;
                }

                m_roads.push_back( road );

                // Initialize cached data for this road
                m_cachedSegmentResults[road.get()] = std::nullopt;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadNetwork::addRoads( const Array<SmartPtr<IRoad>> &roads )
        {
            try
            {
                for( auto &road : roads )
                {
                    addRoad( road );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadNetwork::removeRoads( const Array<SmartPtr<IRoad>> &roads )
        {
            try
            {
                for( auto &road : roads )
                {
                    removeRoad( road );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Array<SmartPtr<IRoadConnection>> CRoadNetwork::getRoadConnections() const
        {
            return m_roadConnections;
        }

        void CRoadNetwork::setRoadConnections( const Array<SmartPtr<IRoadConnection>> &roadConnections )
        {
            m_roadConnections = roadConnections;
        }

        void CRoadNetwork::removeRoadConnection( SmartPtr<IRoadConnection> roadConnection )
        {
            try
            {
                if( !roadConnection )
                {
                    return;
                }

                auto it =
                    std::find( m_roadConnections.begin(), m_roadConnections.end(), roadConnection );
                if( it != m_roadConnections.end() )
                {
                    m_roadConnections.erase( it );

                    // Clean up cached data
                    m_cachedIntersectionResults.erase( roadConnection.get() );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadNetwork::addRoadConnection( SmartPtr<IRoadConnection> roadConnection )
        {
            try
            {
                if( !roadConnection )
                {
                    WP_LOG_ERROR( "CRoadNetwork::addRoadConnection - null road connection provided." );
                    return;
                }

                // Check if already present
                if( std::find( m_roadConnections.begin(), m_roadConnections.end(), roadConnection ) !=
                    m_roadConnections.end() )
                {
                    return;
                }

                m_roadConnections.push_back( roadConnection );

                // Initialize cached data for this connection
                m_cachedIntersectionResults[roadConnection.get()] = std::nullopt;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadNetwork::setupGraph()
        {
            try
            {
                // Clear and rebuild connection lists for all nodes
                for( auto &node : m_nodes )
                {
                    if( node )
                    {
                        node->clearConnections();
                    }
                }

                for( auto &node : m_mergedNodes )
                {
                    if( node )
                    {
                        node->clearConnections();
                    }
                }

                // Rebuild connections from roads
                for( auto &road : m_roads )
                {
                    if( !road )
                    {
                        continue;
                    }

                    auto nodes = road->getNodes();
                    for( size_t i = 0; i < nodes.size(); ++i )
                    {
                        auto currentNode = nodes[i];
                        if( !currentNode )
                        {
                            continue;
                        }

                        // Connect to previous node
                        if( i > 0 )
                        {
                            auto prevNode = nodes[i - 1];
                            if( prevNode )
                            {
                                currentNode->connect( prevNode );
                                prevNode->connect( currentNode );
                            }
                        }

                        // Connect to next node
                        if( i + 1 < nodes.size() )
                        {
                            auto nextNode = nodes[i + 1];
                            if( nextNode )
                            {
                                currentNode->connect( nextNode );
                                nextNode->connect( currentNode );
                            }
                        }
                    }
                }

                // Rebuild connections from road connections (intersections)
                for( auto &connection : m_roadConnections )
                {
                    if( !connection )
                    {
                        continue;
                    }

                    auto node = connection->getNode();
                    if( node )
                    {
                        // Connect the intersection node to all connected roads
                        // This is handled by the connection itself in most cases
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Node merging - Enhanced for AAA quality
        // ---------------------------------------------------------------

        SmartPtr<IRoadNode> CRoadNetwork::merge( const Array<SmartPtr<IRoadNode>> &nodes )
        {
            try
            {
                auto mergedNode = workphone::make_ptr<CRoadNode>();
                if( !mergedNode )
                {
                    WP_LOG_ERROR( "CRoadNetwork::merge - failed to allocate CRoadNode." );
                    return nullptr;
                }

                mergedNode->setIsConnection( true );

                // Build a name from all contributing nodes.
                auto mergeNodeName = String( "RoadNode_Merged_" );
                auto posAccum = Vector3<real_Num>::zero();
                auto validCount = s32{ 0 };

                for( auto &n : nodes )
                {
                    if( !n )
                    {
                        WP_LOG_ERROR( "CRoadNetwork::merge - null node in merge group, skipping." );
                        continue;
                    }

                    mergeNodeName += n->getName();
                    posAccum += n->getPosition();
                    ++validCount;
                }

                if( validCount == 0 )
                {
                    WP_LOG_ERROR( "CRoadNetwork::merge - all nodes in merge group were null." );
                    return nullptr;
                }

                mergedNode->setName( mergeNodeName );

                // Position is the centroid of all valid contributing nodes.
                auto mergedPosition = posAccum / static_cast<real_Num>( validCount );
                mergedNode->setPosition( mergedPosition );

                // Cross-link each source node with the merged node.
                for( auto n : nodes )
                {
                    if( !n )
                    {
                        continue;
                    }

                    n->setMergedNode( mergedNode );
                    mergedNode->addMergedNode( n );
                }

                // Inherit all connections from source nodes.
                for( auto &n : nodes )
                {
                    if( !n )
                    {
                        continue;
                    }

                    auto connectedNodes = n->getConnectedNodes();
                    for( auto &c : connectedNodes )
                    {
                        if( !c )
                        {
                            continue;
                        }

                        mergedNode->connect( c );
                        c->connect( mergedNode );
                    }
                }

                return mergedNode;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        // ---------------------------------------------------------------
        // Property system
        // ---------------------------------------------------------------

        SmartPtr<Properties> CRoadNetwork::getProperties() const
        {
            try
            {
                auto properties = workphone::make_ptr<Properties>();
                if( !properties )
                {
                    WP_LOG_ERROR( "CRoadNetwork::getProperties - failed to allocate Properties." );
                    return nullptr;
                }

                properties->setProperty( kPropRoadCount, static_cast<s32>( m_roads.size() ) );
                properties->setProperty( kPropNodeCount, static_cast<s32>( m_nodes.size() ) );
                properties->setProperty( kPropMergedNodeCount,
                                         static_cast<s32>( m_mergedNodes.size() ) );
                properties->setProperty( kPropConnectionCount,
                                         static_cast<s32>( m_roadConnections.size() ) );
                properties->setProperty( kPropMajorRoadCount, static_cast<s32>( m_majorRoads.size() ) );
                properties->setProperty( kPropMinorRoadCount, static_cast<s32>( m_minorRoads.size() ) );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoadNetwork::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoadNetwork::setProperties - null properties provided." );
                return;
            }

            // All network-topology fields (road count, node count, etc.) are structural
            // state that must be modified through the dedicated add/remove methods;
            // they are intentionally read-only from the property panel.
        }

        // ---------------------------------------------------------------
        // AAA Quality Enhancements - Get mesh data for rendering
        // ---------------------------------------------------------------

        /**
         * @brief Get the AAA road segment result for a road (if available)
         * @param road The road to query
         * @return Pointer to the WPRoadSystem segment result, or nullptr if not available
         */
        const RoadSegmentResult *CRoadNetwork::getRoadSegmentResult( SmartPtr<IRoad> road ) const
        {
            try
            {
                if( !road )
                {
                    return nullptr;
                }

                auto it = m_cachedSegmentResults.find( road.get() );
                if( it != m_cachedSegmentResults.end() && it->second.has_value() )
                {
                    return &it->second.value();
                }

                // Try to get from user data
                auto userData = road->getUserData( StringUtil::getHash( "WPRoadSegmentResult" ) );
                if( userData )
                {
                    // Cache it for future use
                    m_cachedSegmentResults[road.get()] = *static_cast<RoadSegmentResult *>( userData );
                    return &m_cachedSegmentResults[road.get()].value();
                }

                return nullptr;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                return nullptr;
            }
        }

        /**
         * @brief Get the AAA intersection result for a road connection (if available)
         * @param connection The road connection to query
         * @return Pointer to the WPRoadSystem intersection result, or nullptr if not available
         */
        const IntersectionResult *CRoadNetwork::getIntersectionResult(
            SmartPtr<IRoadConnection> connection ) const
        {
            try
            {
                if( !connection )
                {
                    return nullptr;
                }

                auto it = m_cachedIntersectionResults.find( connection.get() );
                if( it != m_cachedIntersectionResults.end() && it->second.has_value() )
                {
                    return &it->second.value();
                }

                // Try to get from user data
                auto userData = connection->getUserData( StringUtil::getHash( "WPIntersectionResult" ) );
                if( userData )
                {
                    // Cache it for future use
                    m_cachedIntersectionResults[connection.get()] =
                        *static_cast<IntersectionResult *>( userData );
                    return &m_cachedIntersectionResults[connection.get()].value();
                }

                return nullptr;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                return nullptr;
            }
        }

        /**
         * @brief Set AAA road segment result on a road (for generators to use)
         * @param road The road to set data on
         * @param result The AAA road segment result
         */
        void CRoadNetwork::setRoadSegmentResult( SmartPtr<IRoad> road, const RoadSegmentResult &result )
        {
            try
            {
                if( !road )
                {
                    WP_LOG_ERROR( "CRoadNetwork::setRoadSegmentResult - null road provided." );
                    return;
                }

                // Store in user data for the road
                road->setUserData( StringUtil::getHash( "WPRoadSegmentResult" ),
                                   new RoadSegmentResult( result ) );

                // Also cache it
                m_cachedSegmentResults[road.get()] = result;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        /**
         * @brief Set AAA intersection result on a road connection (for generators to use)
         * @param connection The road connection to set data on
         * @param result The AAA intersection result
         */
        void CRoadNetwork::setIntersectionResult( SmartPtr<IRoadConnection> connection,
                                                  const IntersectionResult &result )
        {
            try
            {
                if( !connection )
                {
                    WP_LOG_ERROR(
                        "CRoadNetwork::setIntersectionResult - null road connection provided." );
                    return;
                }

                // Store in user data for the connection
                connection->setUserData( StringUtil::getHash( "WPIntersectionResult" ),
                                         new IntersectionResult( result ) );

                // Also cache it
                m_cachedIntersectionResults[connection.get()] = result;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }  // end namespace procedural
}  // namespace workphone
