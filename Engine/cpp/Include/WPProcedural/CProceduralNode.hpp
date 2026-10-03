#ifndef CProceduralNode_h__
#define CProceduralNode_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoadNode.hpp>
#include <Workphone/Interface/Procedural/IProceduralNode.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/Set.hpp>

namespace workphone
{
    namespace procedural
    {
        /// Concrete implementation of the IProceduralNode interface.
        template <class T>
        class CProceduralNode : public CProceduralObject<T>
        {
        public:
            CProceduralNode()
            {
            }

            ~CProceduralNode() override
            {
                unload( nullptr );
            }

            void unload( SmartPtr<ISharedObject> data ) override
            {
                m_connectedNodes.clear();
                m_mergedNodes.clear();

                m_parentNode = nullptr;
                m_mergedNode = nullptr;
                m_networkNode = nullptr;
            }

            SmartPtr<IProceduralNode> getParentNode() const
            {
                return m_parentNode;
            }

            void setParentNode( SmartPtr<IProceduralNode> value )
            {
                m_parentNode = value;
            }

            void addConnectedObject( SmartPtr<IProceduralNode> obj )
            {
                m_connectedObjects.push_back( obj );
            }

            void removeConnectedObject( SmartPtr<IProceduralNode> obj )
            {
                auto it = std::find( m_connectedObjects.begin(), m_connectedObjects.end(), obj );
                if( it != m_connectedObjects.end() )
                {
                    m_connectedObjects.erase( it );
                }
            }

            bool isConnected( SmartPtr<IProceduralNode> node )
            {
                auto it = std::find( m_connectedNodes.begin(), m_connectedNodes.end(), node );
                if( it != m_connectedNodes.end() )
                {
                    return true;
                }

                return false;
            }

            void connect( SmartPtr<IProceduralNode> node )
            {
                WP_ASSERT( node );
                // WP_ASSERT(node != getSharedFromThis<IProceduralNode>());

                Set<SmartPtr<IProceduralNode>> set( m_connectedNodes.begin(), m_connectedNodes.end() );
                set.insert( node );

                m_connectedNodes = Array<SmartPtr<IProceduralNode>>( set.begin(), set.end() );
            }

            void disconnect( SmartPtr<IProceduralNode> node )
            {
                auto it = std::find( m_connectedNodes.begin(), m_connectedNodes.end(), node );
                if( it != m_connectedNodes.end() )
                {
                    m_connectedNodes.erase( it );
                }
            }

            void disconnectAll()
            {
                auto pThisNode = nullptr;  // getSharedFromThis<IProceduralNode>();

                if( m_mergedNode )
                {
                    if( m_mergedNode->hasMergedNode( pThisNode ) )
                    {
                        m_mergedNode->removeMergedNode( pThisNode );
                    }
                }

                for( auto n : m_connectedNodes )
                {
                    if( n )
                    {
                        n->disconnect( pThisNode );
                    }
                }

                m_connectedNodes.clear();
            }

            Array<SmartPtr<IProceduralNode>> getConnectedNodes() const
            {
                return m_connectedNodes;
            }

            size_t getNumConnections() const
            {
                return m_connectedNodes.size();
            }

            void clearConnections()
            {
                m_connectedNodes.clear();
            }

            bool hasMergedNode( SmartPtr<IProceduralNode> n )
            {
                auto it = std::find( m_mergedNodes.begin(), m_mergedNodes.end(), n );
                if( it != m_mergedNodes.end() )
                {
                    return true;
                }

                return false;
            }

            void addMergedNode( SmartPtr<IProceduralNode> n )
            {
                m_mergedNodes.push_back( n );
            }

            void removeMergedNode( SmartPtr<IProceduralNode> n )
            {
                auto it = std::find( m_mergedNodes.begin(), m_mergedNodes.end(), n );
                if( it != m_mergedNodes.end() )
                {
                    m_mergedNodes.erase( it );
                }
            }

            Array<SmartPtr<IProceduralNode>> getMergedNodes() const
            {
                return m_mergedNodes;
            }

            void setMergedNodes( Array<SmartPtr<IProceduralNode>> value )
            {
                m_mergedNodes = value;
            }

            SmartPtr<IProceduralNode> getMergedNode() const override
            {
                return m_mergedNode;
            }

            void setMergedNode( SmartPtr<IProceduralNode> node ) override
            {
                m_mergedNode = node;
            }

            void setIsConnection( bool bIsConnection ) override
            {
                m_isConnection = bIsConnection;
            }

            bool isConnection() const override
            {
                return m_isConnection;
            }

            String getUniqueId() const
            {
                return m_uniqueId;
            }

            void setUniqueId( const String &value )
            {
                m_uniqueId = value;
            }

            s32 getGraphId() const
            {
                return m_graphId;
            }

            void setGraphId( s32 id )
            {
                m_graphId = id;
            }

            s32 getRoadId() const
            {
                return m_roadNodeId;
            }

            void setRoadId( s32 id )
            {
                m_roadNodeId = id;
            }

            SmartPtr<IProceduralNode> getNetworkNode() const
            {
                return m_networkNode;
            }

            void setNetworkNode( SmartPtr<IProceduralNode> value )
            {
                m_networkNode = value;
            }

            CProceduralNode &operator=( CProceduralNode &other )
            {
                CProceduralObject<T>::operator=( other );

                m_connectedNodes = other.m_connectedNodes;
                m_mergedNodes = other.m_mergedNodes;
                m_graphId = other.m_graphId;
                return *this;
            }

        protected:
            /// The parent of this node
            SmartPtr<IProceduralNode> m_parentNode;

            /// The merged node that this node is a part of.
            SmartPtr<IProceduralNode> m_mergedNode;

            /// The road network node represent this node.
            SmartPtr<IProceduralNode> m_networkNode;

            /// The objects connected to this node.
            Array<SmartPtr<IProceduralObject>> m_connectedObjects;

            /// The nodes connected to this node.
            Array<SmartPtr<IProceduralNode>> m_connectedNodes;

            /// The merged node.
            Array<SmartPtr<IProceduralNode>> m_mergedNodes;

            /// An id to make this node unique.
            String m_uniqueId = "";

            /// An id to identify this node in the graph.
            s32 m_graphId = -1;

            /// An id to identify this node in the graph.
            s32 m_nodeId = -1;

            /// An id to identify this node in the graph.
            s32 m_roadNodeId = -1;

            /// Used to know if this node is a connection e.g a junction.
            bool m_isConnection = false;

            /// Used to generate an id
            static u32 m_nextId;
        };

        template <class T>
        u32 CProceduralNode<T>::m_nextId = 0;
    }  // namespace procedural
}  // namespace workphone

#endif  // CProceduralNode_h__
