#ifndef IProceduralNode_h__
#define IProceduralNode_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for a procedural node in a procedural generation system.
         *
         * A procedural node represents a point or element in a procedural graph, such as a node in a
         * road network or terrain system. This interface provides methods for managing connections to
         * other nodes, merging nodes, and associating nodes with graphs or roads.
         */
        class WPCore_API IProceduralNode : public IProceduralObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IProceduralNode() override;

            /**
             * @brief Connects this node to another node.
             * @param node The node to connect to.
             */
            virtual void connect( SmartPtr<IProceduralNode> node ) = 0;

            /**
             * @brief Disconnects this node from another node.
             * @param node The node to disconnect from.
             */
            virtual void disconnect( SmartPtr<IProceduralNode> node ) = 0;

            /**
             * @brief Disconnects this node from all connected nodes.
             */
            virtual void disconnectAll() = 0;

            /**
             * @brief Gets all nodes connected to this node.
             * @return An array of connected nodes.
             */
            virtual Array<SmartPtr<IProceduralNode>> getConnectedNodes() const = 0;

            /**
             * @brief Gets the number of connections this node has.
             * @return The number of connected nodes.
             */
            virtual size_t getNumConnections() const = 0;

            /**
             * @brief Removes all connections from this node.
             */
            virtual void clearConnections() = 0;

            /**
             * @brief Checks if a node is merged with this node.
             * @param n The node to check.
             * @return True if the node is merged, false otherwise.
             */
            virtual bool hasMergedNode( SmartPtr<IProceduralNode> n ) = 0;

            /**
             * @brief Adds a node to the list of merged nodes.
             * @param n The node to merge.
             */
            virtual void addMergedNode( SmartPtr<IProceduralNode> n ) = 0;

            /**
             * @brief Removes a node from the list of merged nodes.
             * @param n The node to remove from merged nodes.
             */
            virtual void removeMergedNode( SmartPtr<IProceduralNode> n ) = 0;

            /**
             * @brief Gets all nodes merged with this node.
             * @return An array of merged nodes.
             */
            virtual Array<SmartPtr<IProceduralNode>> getMergedNodes() const = 0;

            /**
             * @brief Sets the list of merged nodes.
             * @param mergedNodes The array of nodes to set as merged.
             */
            virtual void setMergedNodes( Array<SmartPtr<IProceduralNode>> mergedNodes ) = 0;

            /**
             * @brief Gets the graph ID this node belongs to.
             * @return The graph ID.
             */
            virtual s32 getGraphId() const = 0;

            /**
             * @brief Sets the graph ID for this node.
             * @param id The graph ID to set.
             */
            virtual void setGraphId( s32 id ) = 0;

            /**
             * @brief Gets the road ID this node is associated with.
             * @return The road ID.
             */
            virtual s32 getRoadId() const = 0;

            /**
             * @brief Sets the road ID for this node.
             * @param id The road ID to set.
             */
            virtual void setRoadId( s32 id ) = 0;

            /**
             * @brief Gets the primary merged node, if any.
             * @return The merged node.
             */
            virtual SmartPtr<IProceduralNode> getMergedNode() const = 0;

            /**
             * @brief Sets the primary merged node.
             * @param node The node to set as merged.
             */
            virtual void setMergedNode( SmartPtr<IProceduralNode> node ) = 0;

            /**
             * @brief Sets whether this node is a connection node.
             * @param bIsConnection True if this node is a connection, false otherwise.
             */
            virtual void setIsConnection( bool bIsConnection ) = 0;

            /**
             * @brief Checks if this node is a connection node.
             * @return True if this node is a connection, false otherwise.
             */
            virtual bool isConnection() const = 0;

            /**
             * @brief Gets the type of connection for this node.
             * @return The connection type as a string.
             */
            virtual String getConnectionType() const = 0;

            /**
             * @brief Gets the network node associated with this node.
             * @return The network node.
             */
            virtual SmartPtr<IProceduralNode> getNetworkNode() const = 0;

            /**
             * @brief Sets the network node associated with this node.
             * @param networkNode The network node to associate.
             */
            virtual void setNetworkNode( SmartPtr<IProceduralNode> networkNode ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralNode_h__
