#ifndef IRoadNetwork_h__
#define IRoadNetwork_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for a procedural road network.
         *
         * This interface represents a network of roads, nodes, and connections that can be used for
         * procedural generation of road systems. It provides methods to manage nodes, roads, and their
         * connections, as well as to set up the underlying graph structure for the network.
         */
        class WPCore_API IRoadNetwork : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IRoadNetwork() override;

            /**
             * @brief Retrieves a road node by its graph ID.
             * @param id The graph ID of the node.
             * @return Smart pointer to the road node, or nullptr if not found.
             */
            virtual SmartPtr<IRoadNode> getNodeByGraphId( s32 id ) const = 0;

            /**
             * @brief Gets all nodes in the road network.
             * @return Array of smart pointers to road nodes.
             */
            virtual Array<SmartPtr<IRoadNode>> getNodes() const = 0;

            /**
             * @brief Sets the nodes in the road network.
             * @param nodes Array of smart pointers to road nodes.
             */
            virtual void setNodes( const Array<SmartPtr<IRoadNode>> &nodes ) = 0;

            /**
             * @brief Gets the merged nodes in the road network.
             *        Merged nodes may represent combined or simplified nodes for optimization.
             * @return Array of smart pointers to merged road nodes.
             */
            virtual Array<SmartPtr<IRoadNode>> getMergedNodes() const = 0;

            /**
             * @brief Sets the merged nodes in the road network.
             * @param mergedNodes Array of smart pointers to merged road nodes.
             */
            virtual void setMergedNodes( const Array<SmartPtr<IRoadNode>> &mergedNodes ) = 0;

            /**
             * @brief Gets all roads in the network (const version).
             * @return Const reference to an array of smart pointers to roads.
             */
            virtual const Array<SmartPtr<IRoad>> &getRoads() const = 0;

            /**
             * @brief Gets all roads in the network (mutable version).
             * @return Reference to an array of smart pointers to roads.
             */
            virtual Array<SmartPtr<IRoad>> &getRoads() = 0;

            /**
             * @brief Removes a road from the network.
             * @param road Smart pointer to the road to remove.
             */
            virtual void removeRoad( SmartPtr<IRoad> road ) = 0;

            /**
             * @brief Adds a road to the network.
             * @param road Smart pointer to the road to add.
             */
            virtual void addRoad( SmartPtr<IRoad> road ) = 0;

            /**
             * @brief Adds multiple roads to the network.
             * @param roads Array of smart pointers to roads to add.
             */
            virtual void addRoads( const Array<SmartPtr<IRoad>> &roads ) = 0;

            /**
             * @brief Removes multiple roads from the network.
             * @param roads Array of smart pointers to roads to remove.
             */
            virtual void removeRoads( const Array<SmartPtr<IRoad>> &roads ) = 0;

            /**
             * @brief Gets all road connections in the network.
             * @return Array of smart pointers to road connections.
             */
            virtual Array<SmartPtr<IRoadConnection>> getRoadConnections() const = 0;

            /**
             * @brief Sets the road connections in the network.
             * @param roadConnections Array of smart pointers to road connections.
             */
            virtual void setRoadConnections(
                const Array<SmartPtr<IRoadConnection>> &roadConnections ) = 0;

            /**
             * @brief Removes a road connection from the network.
             * @param roadConnection Smart pointer to the road connection to remove.
             */
            virtual void removeRoadConnection( SmartPtr<IRoadConnection> roadConnection ) = 0;

            /**
             * @brief Adds a road connection to the network.
             * @param roadConnection Smart pointer to the road connection to add.
             */
            virtual void addRoadConnection( SmartPtr<IRoadConnection> roadConnection ) = 0;

            /**
             * @brief Sets up the underlying graph structure for the road network.
             *        This should be called after modifying nodes, roads, or connections to update the
             * internal representation.
             */
            virtual void setupGraph() = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadNetwork_h__
