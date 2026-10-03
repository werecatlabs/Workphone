#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CRoadNode.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadNode, CProceduralNode<IRoadNode> );

        // ---------------------------------------------------------------
        // Property key constants
        // ---------------------------------------------------------------

        static const char *const kPropName = "name";
        static const char *const kPropRoadId = "roadId";
        static const char *const kPropGraphId = "graphId";
        static const char *const kPropUniqueId = "uniqueId";
        static const char *const kPropIsConnection = "isConnection";
        static const char *const kPropConnectionType = "connectionType";
        static const char *const kPropRoadCount = "roadCount";
        static const char *const kPropConnectedCount = "connectedNodeCount";
        static const char *const kPropMergedCount = "mergedNodeCount";
        static const char *const kPropPositionX = "positionX";
        static const char *const kPropPositionY = "positionY";
        static const char *const kPropPositionZ = "positionZ";

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadNode::CRoadNode()
        {
            // Assign a unique default name using the inherited static counter.
            auto name = String( "node_" ) + StringUtil::toString( m_nextId++ );
            setName( name );
        }

        CRoadNode::~CRoadNode()
        {
            unload( nullptr );
        }

        void CRoadNode::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                m_roads.clear();
                m_roadElement = nullptr;

                // Delegate connected-node and merged-node teardown to the base
                // class so it can maintain its own invariants.
                CProceduralNode<IRoadNode>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Road membership
        // ---------------------------------------------------------------

        void CRoadNode::addRoad( const SmartPtr<IRoad> &road )
        {
            if( !road )
            {
                WP_LOG_ERROR( "CRoadNode::addRoad - null road provided." );
                return;
            }

            // Guard against duplicate entries.
            auto it = std::find( m_roads.begin(), m_roads.end(), road );
            if( it == m_roads.end() )
            {
                m_roads.push_back( road );
            }
        }

        void CRoadNode::removeRoad( const SmartPtr<IRoad> &road )
        {
            if( !road )
            {
                return;
            }

            auto it = std::find( m_roads.begin(), m_roads.end(), road );
            if( it != m_roads.end() )
            {
                m_roads.erase( it );
            }
        }

        Array<SmartPtr<IRoad>> CRoadNode::getRoads() const
        {
            return m_roads;
        }

        // ---------------------------------------------------------------
        // Road ID
        // ---------------------------------------------------------------

        s32 CRoadNode::getRoadId() const
        {
            return m_roadId;
        }

        void CRoadNode::setRoadId( s32 id )
        {
            if( id < -1 )
            {
                WP_LOG_ERROR( "CRoadNode::setRoadId - invalid id, clamping to -1." );
                m_roadId = -1;
                return;
            }

            m_roadId = id;
        }

        // ---------------------------------------------------------------
        // Merged-node query
        // ---------------------------------------------------------------

        SmartPtr<IRoadNode> CRoadNode::getRoadNodeFromMerged( SmartPtr<IRoad> road ) const
        {
            if( !road )
            {
                return nullptr;
            }

            try
            {
                // Search through the merged nodes looking for one that belongs to
                // the requested road.
                auto mergedNodes = getMergedNodes();
                for( auto &mergedNode : mergedNodes )
                {
                    if( !mergedNode )
                    {
                        continue;
                    }

                    auto roadNode = workphone::dynamic_pointer_cast<IRoadNode>( mergedNode );
                    if( !roadNode )
                    {
                        continue;
                    }

                    auto roads = roadNode->getRoads();
                    auto it = std::find( roads.begin(), roads.end(), road );
                    if( it != roads.end() )
                    {
                        return roadNode;
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        // ---------------------------------------------------------------
        // Clone
        // ---------------------------------------------------------------

        SmartPtr<ISharedObject> CRoadNode::clone()
        {
            try
            {
                auto node = workphone::make_ptr<CRoadNode>();
                if( !node )
                {
                    WP_LOG_ERROR( "CRoadNode::clone - failed to allocate CRoadNode." );
                    return nullptr;
                }

                *node = *this;
                return node;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        // ---------------------------------------------------------------
        // Assignment
        // ---------------------------------------------------------------

        CRoadNode &CRoadNode::operator=( const CRoadNode &other )
        {
            if( this == &other )
            {
                return *this;
            }

            CProceduralNode<IRoadNode>::operator=( const_cast<CRoadNode &>( other ) );

            m_roads = other.m_roads;
            m_roadId = other.m_roadId;
            m_roadElement = other.m_roadElement;

            return *this;
        }

        // ---------------------------------------------------------------
        // Connection type
        // ---------------------------------------------------------------

        String CRoadNode::getConnectionType() const
        {
            if( isConnection() )
            {
                // Classify the junction by number of connected arms.
                auto numConnections = getNumConnections();
                if( numConnections == 2 )
                {
                    return "L_Connection";
                }
                if( numConnections == 3 )
                {
                    return "T_Crossing";
                }
                if( numConnections >= 4 )
                {
                    return "X_Crossing";
                }

                return "Connection";
            }

            return "None";
        }

        // ---------------------------------------------------------------
        // Road element back-reference
        // ---------------------------------------------------------------

        SmartPtr<IRoadElement> CRoadNode::getRoadElement() const
        {
            return m_roadElement;
        }

        void CRoadNode::setRoadElement( SmartPtr<IRoadElement> value )
        {
            m_roadElement = value;
        }

        // ---------------------------------------------------------------
        // Property system
        // ---------------------------------------------------------------

        SmartPtr<Properties> CRoadNode::getProperties() const
        {
            try
            {
                auto properties = CProceduralNode<IRoadNode>::getProperties();
                if( !properties )
                {
                    WP_LOG_ERROR( "CRoadNode::getProperties - base properties is null." );
                    return nullptr;
                }

                auto pos = getPosition();

                properties->setProperty( kPropName, m_name );
                properties->setProperty( kPropRoadId, m_roadId );
                properties->setProperty( kPropGraphId, m_graphId );
                properties->setProperty( kPropUniqueId, m_uniqueId );
                properties->setProperty( kPropIsConnection, m_isConnection );
                properties->setProperty( kPropConnectionType, getConnectionType() );
                properties->setProperty( kPropRoadCount, static_cast<s32>( m_roads.size() ) );
                properties->setProperty( kPropConnectedCount, static_cast<s32>( getNumConnections() ) );
                properties->setProperty( kPropMergedCount, static_cast<s32>( getMergedNodes().size() ) );
                properties->setProperty( kPropPositionX, pos.x );
                properties->setProperty( kPropPositionY, pos.y );
                properties->setProperty( kPropPositionZ, pos.z );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CRoadNode::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CRoadNode::setProperties - null properties provided." );
                return;
            }

            try
            {
                CProceduralNode<IRoadNode>::setProperties( properties );

                auto name = m_name;
                auto roadId = m_roadId;
                auto graphId = m_graphId;
                auto uniqueId = m_uniqueId;
                auto isConnection = m_isConnection;
                auto pos = getPosition();

                properties->getPropertyValue( kPropName, name );
                properties->getPropertyValue( kPropRoadId, roadId );
                properties->getPropertyValue( kPropGraphId, graphId );
                properties->getPropertyValue( kPropUniqueId, uniqueId );
                properties->getPropertyValue( kPropIsConnection, isConnection );
                properties->getPropertyValue( kPropPositionX, pos.x );
                properties->getPropertyValue( kPropPositionY, pos.y );
                properties->getPropertyValue( kPropPositionZ, pos.z );

                m_name = name;
                setRoadId( roadId );  // routes through validated setter
                m_graphId = graphId;
                m_uniqueId = uniqueId;
                m_isConnection = isConnection;
                setPosition( pos );

                // kPropConnectionType, kPropRoadCount, kPropConnectedCount,
                // kPropMergedCount are read-only diagnostic fields.
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // end namespace procedural
}  // namespace workphone
