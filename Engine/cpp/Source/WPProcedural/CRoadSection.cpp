#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CRoadSection.hpp"
#include "WPProcedural/CRoadElement.hpp"
#include "WPProcedural/CRoadNode.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadSection,
                                   CProceduralObject<IRoadSection> );

        // ---------------------------------------------------------------
        // Property key constants
        // ---------------------------------------------------------------

        static const char *const kPropName = "name";
        static const char *const kPropElementCount = "elementCount";
        static const char *const kPropNodeCount = "nodeCount";
        static const char *const kPropSidewalkCount = "sidewalkCount";

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadSection::CRoadSection()
        {
        }

        CRoadSection::~CRoadSection()
        {
            unload( nullptr );
        }

        void CRoadSection::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                m_parentRoad = nullptr;

                for( auto &sidewalk : m_sidewalks )
                {
                    if( sidewalk )
                    {
                        sidewalk->unload( nullptr );
                    }
                }
                m_sidewalks.clear();

                for( auto &elementRow : m_roadElements )
                {
                    for( auto &roadElement : elementRow )
                    {
                        if( roadElement )
                        {
                            roadElement->unload( nullptr );
                        }
                    }
                }
                m_roadElements.clear();

                for( auto &roadElement : m_elements )
                {
                    if( roadElement )
                    {
                        roadElement->unload( nullptr );
                    }
                }
                m_elements.clear();

                CProceduralObject<IRoadSection>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Build / bounds
        // ---------------------------------------------------------------

        void CRoadSection::build()
        {
            try
            {
                updateBounds();
                createChildRoads();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadSection::updateBounds()
        {
            try
            {
                auto bounds = AABB3<real_Num>();
                bool hasMerge = false;

                auto roadNodes = getRoadNodes();
                for( auto &node : roadNodes )
                {
                    if( !node )
                    {
                        continue;
                    }

                    bounds.merge( node->getPosition() );
                    hasMerge = true;
                }

                if( hasMerge )
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

        // ---------------------------------------------------------------
        // IRoadSection – element management
        // ---------------------------------------------------------------

        void CRoadSection::addRoadElement( SmartPtr<IRoadElement> roadElement )
        {
            if( !roadElement )
            {
                WP_LOG_ERROR( "CRoadSection::addRoadElement - null element provided." );
                return;
            }

            m_elements.push_back( roadElement );
        }

        void CRoadSection::removeRoadElement( SmartPtr<IRoadElement> roadElement )
        {
            if( !roadElement )
            {
                return;
            }

            auto it = std::find( m_elements.begin(), m_elements.end(), roadElement );
            if( it != m_elements.end() )
            {
                m_elements.erase( it );
            }
        }

        void CRoadSection::clearRoadElements()
        {
            // Unload and clear both the 2-D grouping array and the flat element list
            // so the two containers stay consistent with each other.
            for( auto &elementRow : m_roadElements )
            {
                for( auto &roadElement : elementRow )
                {
                    if( roadElement )
                    {
                        roadElement->unload( nullptr );
                    }
                }
            }
            m_roadElements.clear();

            for( auto &roadElement : m_elements )
            {
                if( roadElement )
                {
                    roadElement->unload( nullptr );
                }
            }
            m_elements.clear();
        }

        Array<SmartPtr<IRoadElement>> CRoadSection::getElements() const
        {
            return m_elements;
        }

        void CRoadSection::setElements( const Array<SmartPtr<IRoadElement>> &elements )
        {
            m_elements.clear();
            m_elements.reserve( elements.size() );

            for( auto &elem : elements )
            {
                if( !elem )
                {
                    WP_LOG_ERROR( "CRoadSection::setElements - null element skipped." );
                    continue;
                }

                m_elements.push_back( elem );
            }
        }

        // ---------------------------------------------------------------
        // Node access
        // ---------------------------------------------------------------

        Array<SmartPtr<IRoadNode>> CRoadSection::getRoadNodes() const
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

        // ---------------------------------------------------------------
        // Child road / splitting logic
        // ---------------------------------------------------------------

        void CRoadSection::createChildRoads()
        {
            try
            {
                // Rebuild from scratch – discard any previously built elements.
                clearRoadElements();

                auto roadElements = splitRoadByConnection();
                if( roadElements.empty() )
                {
                    WP_LOG_ERROR(
                        "CRoadSection::createChildRoads - splitRoadByConnection returned no elements." );
                    return;
                }

                auto pThisSection = getSharedFromThis<IRoadSection>();
                if( !pThisSection )
                {
                    WP_LOG_ERROR(
                        "CRoadSection::createChildRoads - could not obtain shared_from_this." );
                    return;
                }

                for( auto &roadElement : roadElements )
                {
                    if( !roadElement )
                    {
                        continue;
                    }

                    roadElement->setParentSection( pThisSection );
                    addRoadElement( roadElement );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        s32 CRoadSection::getNumConnections() const
        {
            auto nodes = getRoadNodes();
            if( nodes.size() < 2 )
            {
                return 0;
            }

            auto numConnections = s32{ 0 };
            auto lastIndex = static_cast<s32>( nodes.size() ) - 1;

            for( s32 i = 1; i < lastIndex; ++i )
            {
                auto &node = nodes[static_cast<size_t>( i )];
                if( !node )
                {
                    continue;
                }

                auto networkNode = node->getNetworkNode();
                if( !networkNode )
                {
                    continue;
                }

                if( networkNode->isConnection() )
                {
                    ++numConnections;
                    continue;
                }

                auto mergedNode = networkNode->getMergedNode();
                if( mergedNode && mergedNode->isConnection() )
                {
                    ++numConnections;
                }
            }

            return numConnections;
        }

        Array<SmartPtr<IRoadElement>> CRoadSection::splitRoadByConnection()
        {
            auto result = Array<SmartPtr<IRoadElement>>();

            try
            {
                auto nodeGroups = getRoadNodesByConnections();
                if( nodeGroups.empty() )
                {
                    WP_LOG_ERROR( "CRoadSection::splitRoadByConnection - no node groups produced." );
                    return result;
                }

                auto pRoadSection = getSharedFromThis<IRoadSection>();

                for( auto &nodes : nodeGroups )
                {
                    if( nodes.empty() )
                    {
                        WP_LOG_ERROR(
                            "CRoadSection::splitRoadByConnection - empty node group skipped." );
                        continue;
                    }

                    auto roadElement = workphone::make_ptr<CRoadElement>();
                    if( !roadElement )
                    {
                        WP_LOG_ERROR(
                            "CRoadSection::splitRoadByConnection - failed to allocate CRoadElement." );
                        continue;
                    }

                    if( pRoadSection )
                    {
                        roadElement->setParentSection( pRoadSection );
                    }

                    roadElement->setScene( getScene() );

                    // Clone each source node so this element owns its own copies.
                    for( auto &sourceNode : nodes )
                    {
                        if( !sourceNode )
                        {
                            WP_LOG_ERROR(
                                "CRoadSection::splitRoadByConnection - null source node skipped." );
                            continue;
                        }

                        auto clonedNode = workphone::make_ptr<CRoadNode>();
                        if( !clonedNode )
                        {
                            WP_LOG_ERROR(
                                "CRoadSection::splitRoadByConnection - failed to allocate CRoadNode." );
                            continue;
                        }

                        clonedNode->setParentNode( sourceNode );
                        clonedNode->setRoadElement( roadElement );
                        roadElement->addNode( clonedNode );
                    }

                    // Rebuild the linear connection chain on the cloned nodes.
                    auto clonedNodes = roadElement->getNodes();

                    for( auto &clonedNode : clonedNodes )
                    {
                        if( clonedNode )
                        {
                            clonedNode->disconnectAll();
                        }
                    }

                    auto numPairs = static_cast<s32>( clonedNodes.size() ) - 1;
                    for( s32 idx = 0; idx < numPairs; ++idx )
                    {
                        auto &nodeA = clonedNodes[static_cast<size_t>( idx )];
                        auto &nodeB = clonedNodes[static_cast<size_t>( idx ) + 1];

                        if( nodeA && nodeB )
                        {
                            nodeA->connect( nodeB );
                            nodeB->connect( nodeA );
                        }
                    }

                    try
                    {
                        roadElement->build();
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }

                    result.push_back( roadElement );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return result;
        }

        Array<Array<SmartPtr<IRoadNode>>> CRoadSection::getRoadNodesByConnections() const
        {
            auto nodesByConnections = Array<Array<SmartPtr<IRoadNode>>>();

            try
            {
                auto nodes = getRoadNodes();
                if( nodes.size() < 2 )
                {
                    // Nothing to split – return the whole node list as a single group.
                    if( !nodes.empty() )
                    {
                        nodesByConnections.push_back( nodes );
                    }
                    return nodesByConnections;
                }

                // Walk the node list once, emitting a new group whenever we reach a
                // connection node (that is neither the first nor the last node).
                // The connection node is included as both the last node of the
                // current group and the first node of the next group so that
                // adjacent segments share their junction point.

                auto currentGroup = Array<SmartPtr<IRoadNode>>();
                auto lastIndex = static_cast<s32>( nodes.size() ) - 1;

                for( s32 i = 0; i <= lastIndex; ++i )
                {
                    auto &node = nodes[static_cast<size_t>( i )];
                    if( !node )
                    {
                        WP_LOG_ERROR( "CRoadSection::getRoadNodesByConnections - null node at index " +
                                      std::to_string( i ) + ", skipping." );
                        continue;
                    }

                    currentGroup.push_back( node );

                    // Check whether this is an intermediate connection node.
                    bool isConnectionNode = false;
                    if( i > 0 && i < lastIndex )
                    {
                        auto networkNode = node->getNetworkNode();
                        if( networkNode )
                        {
                            if( networkNode->isConnection() )
                            {
                                isConnectionNode = true;
                            }
                            else
                            {
                                auto mergedNode = networkNode->getMergedNode();
                                if( mergedNode && mergedNode->isConnection() )
                                {
                                    isConnectionNode = true;
                                }
                            }
                        }
                    }

                    if( isConnectionNode )
                    {
                        // Seal this group and start a new one that begins with the
                        // shared junction node.
                        nodesByConnections.push_back( currentGroup );
                        currentGroup.clear();
                        currentGroup.push_back( node );
                    }
                }

                // Push the final group even if no connection node was found (simple
                // road with no splits).
                if( !currentGroup.empty() )
                {
                    nodesByConnections.push_back( currentGroup );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nodesByConnections;
        }

        // ---------------------------------------------------------------
        // Property system
        // ---------------------------------------------------------------

        SmartPtr<Properties> CRoadSection::getProperties() const
        {
            try
            {
                auto properties = CProceduralObject<IRoadSection>::getProperties();
                if( !properties )
                {
                    WP_LOG_ERROR( "CRoadSection::getProperties - base properties is null." );
                    return nullptr;
                }

                properties->setProperty( kPropName, m_name );
                properties->setProperty( kPropElementCount, static_cast<s32>( m_elements.size() ) );
                properties->setProperty( kPropNodeCount, static_cast<s32>( getRoadNodes().size() ) );
                properties->setProperty( kPropSidewalkCount, static_cast<s32>( m_sidewalks.size() ) );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoadSection::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoadSection::setProperties - null properties provided." );
                return;
            }

            try
            {
                CProceduralObject<IRoadSection>::setProperties( properties );

                auto name = m_name;
                properties->getPropertyValue( kPropName, name );
                m_name = name;

                // kPropElementCount, kPropNodeCount, kPropSidewalkCount are read-only
                // diagnostic fields and are intentionally not written back here.
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // end namespace procedural
}  // namespace workphone
