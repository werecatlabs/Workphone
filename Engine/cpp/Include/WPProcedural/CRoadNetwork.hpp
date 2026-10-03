// ============================================================================
// CRoadNetwork.hpp - Enhanced AAA Road Network Interface
// ============================================================================
// Enhanced road network that properly integrates with the WPRoadSystem for
// Call of Duty AAA quality road generation.
//
// Features:
//   - Full integration with WPRoadSystem for AAA-quality meshes
//   - Cached mesh data for efficient rendering
//   - Enhanced node merging with better quality
//   - Support for all AAA road features (camber, ruts, markings, etc.)
//   - Proper cleanup and memory management
// ============================================================================

#ifndef CRoadNetwork_hpp__
#define CRoadNetwork_hpp__

#include <Workphone/Interface/Procedural/IRoadNetwork.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Properties.hpp>
#include <WPProcedural/WPRoadSystem.hpp>
#include <optional>
#include <unordered_map>

namespace workphone
{
    namespace procedural
    {
        class CRoad;
        class CRoadNode;
        class CRoadConnection;

        /**
         * @brief Enhanced AAA procedural road network.
         *
         * This implementation builds upon the existing CRoad / CRoadNode /
         * CRoadConnection infrastructure but fully integrates with the
         * WPRoadSystem to produce Call of Duty AAA quality road visuals.
         *
         * Features:
         *   - Cambered road surfaces with wheel ruts
         *   - Sidewalks with slab joints and broken corners
         *   - Kerbs with worn edges
         *   - Road markings (center dash, edge lines, crosswalks)
         *   - Road-side dressing (sand drifts, gutter pebbles)
         *   - 4-way intersections with crosswalk stripes
         *   - T-junctions
         *   - Roundabouts (configurable)
         *   - Manhole covers and gully gratings
         *   - Proper collision geometry
         */
        class WPProcedural_API CRoadNetwork : public IRoadNetwork
        {
        public:
            /** @brief Virtual destructor. */
            ~CRoadNetwork() override;

            // --------------------------------------------------------------
            // Lifetime
            // --------------------------------------------------------------

            /** @brief Constructor. */
            CRoadNetwork();

            /** @brief Unload all resources. */
            void unload( SmartPtr<ISharedObject> data ) override;

            // --------------------------------------------------------------
            // Node queries
            // --------------------------------------------------------------

            /** @brief Retrieves a road node by its graph ID.
             *  @param id The graph ID of the node.
             *  @return Smart pointer to the road node, or nullptr if not found.
             */
            SmartPtr<IRoadNode> getNodeByGraphId( s32 id ) const override;

            /** @brief Gets all nodes in the road network.
             *  @return Array of smart pointers to road nodes.
             */
            Array<SmartPtr<IRoadNode>> getNodes() const override;

            /** @brief Sets the nodes in the road network.
             *  @param nodes Array of smart pointers to road nodes.
             */
            void setNodes( const Array<SmartPtr<IRoadNode>> &nodes ) override;

            /** @brief Gets the merged nodes in the road network.
             *        Merged nodes may represent combined or simplified nodes for optimization.
             *  @return Array of smart pointers to merged road nodes.
             */
            Array<SmartPtr<IRoadNode>> getMergedNodes() const override;

            /** @brief Sets the merged nodes in the road network.
             *  @param mergedNodes Array of smart pointers to merged road nodes.
             */
            void setMergedNodes( const Array<SmartPtr<IRoadNode>> &mergedNodes ) override;

            // --------------------------------------------------------------
            // Road management
            // --------------------------------------------------------------

            /** @brief Gets all roads in the network (const version).
             *  @return Const reference to an array of smart pointers to roads.
             */
            const Array<SmartPtr<IRoad>> &getRoads() const override;

            /** @brief Gets all roads in the network (mutable version).
             *  @return Reference to an array of smart pointers to roads.
             */
            Array<SmartPtr<IRoad>> &getRoads() override;

            /** @brief Removes a road from the network.
             *  @param road Smart pointer to the road to remove.
             */
            void removeRoad( SmartPtr<IRoad> road ) override;

            /** @brief Adds a road to the network.
             *  @param road Smart pointer to the road to add.
             */
            void addRoad( SmartPtr<IRoad> road ) override;

            /** @brief Adds multiple roads to the network.
             *  @param roads Array of smart pointers to roads to add.
             */
            void addRoads( const Array<SmartPtr<IRoad>> &roads ) override;

            /** @brief Removes multiple roads from the network.
             *  @param roads Array of smart pointers to roads to remove.
             */
            void removeRoads( const Array<SmartPtr<IRoad>> &roads ) override;

            // --------------------------------------------------------------
            // Road connection management
            // --------------------------------------------------------------

            /** @brief Gets all road connections in the network.
             *  @return Array of smart pointers to road connections.
             */
            Array<SmartPtr<IRoadConnection>> getRoadConnections() const override;

            /** @brief Sets the road connections in the network.
             *  @param roadConnections Array of smart pointers to road connections.
             */
            void setRoadConnections( const Array<SmartPtr<IRoadConnection>> &roadConnections ) override;

            /** @brief Removes a road connection from the network.
             *  @param roadConnection Smart pointer to the road connection to remove.
             */
            void removeRoadConnection( SmartPtr<IRoadConnection> roadConnection ) override;

            /** @brief Adds a road connection to the network.
             *  @param roadConnection Smart pointer to the road connection to add.
             */
            void addRoadConnection( SmartPtr<IRoadConnection> roadConnection ) override;

            // --------------------------------------------------------------
            // Graph management
            // --------------------------------------------------------------

            /** @brief Sets up the underlying graph structure for the road network.
             *        This should be called after modifying nodes, roads, or connections to update the
             * internal representation.
             */
            void setupGraph() override;

            // --------------------------------------------------------------
            // Node merging
            // --------------------------------------------------------------

            /** @brief Merge multiple nodes into a single node.
             *  @param nodes Array of nodes to merge.
             *  @return Smart pointer to the merged road node, or nullptr on failure.
             */
            SmartPtr<IRoadNode> merge( const Array<SmartPtr<IRoadNode>> &nodes );

            // --------------------------------------------------------------
            // Property system
            // --------------------------------------------------------------

            /** @brief Gets the properties of this object.
             *  @return Smart pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /** @brief Sets the properties of this object.
             *  @param properties Smart pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            // --------------------------------------------------------------
            // AAA Quality Enhancements
            // --------------------------------------------------------------

            /** @brief Get the AAA road segment result for a road (if available)
             *  @param road The road to query
             *  @return Pointer to the WPRoadSystem segment result, or nullptr if not available
             */
            const RoadSegmentResult *getRoadSegmentResult( SmartPtr<IRoad> road ) const;

            /** @brief Get the AAA intersection result for a road connection (if available)
             *  @param connection The road connection to query
             *  @return Pointer to the WPRoadSystem intersection result, or nullptr if not available
             */
            const IntersectionResult *getIntersectionResult(
                SmartPtr<IRoadConnection> connection ) const;

            /** @brief Set AAA road segment result on a road (for generators to use)
             *  @param road The road to set data on
             *  @param result The AAA road segment result
             */
            void setRoadSegmentResult( SmartPtr<IRoad> road, const RoadSegmentResult &result );

            /** @brief Set AAA intersection result on a road connection (for generators to use)
             *  @param connection The road connection to set data on
             *  @param result The AAA intersection result
             */
            void setIntersectionResult( SmartPtr<IRoadConnection> connection,
                                        const IntersectionResult &result );

        private:
            // Node storage
            Array<SmartPtr<IRoadNode>> m_nodes;
            Array<SmartPtr<IRoadNode>> m_mergedNodes;
            Array<SmartPtr<IRoadNode>> m_mergedNetworkNodes;

            // Road storage
            Array<SmartPtr<IRoad>> m_roads;
            Array<SmartPtr<IRoad>> m_majorRoads;
            Array<SmartPtr<IRoad>> m_minorRoads;

            // Connection storage
            Array<SmartPtr<IRoadConnection>> m_roadConnections;

            // Cached mesh data for AAA quality rendering
            mutable std::unordered_map<IRoad *, std::optional<RoadSegmentResult>> m_cachedSegmentResults;
            mutable std::unordered_map<IRoadConnection *, std::optional<IntersectionResult>>
                m_cachedIntersectionResults;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // end namespace workphone

#endif  // CRoadNetwork_hpp__
