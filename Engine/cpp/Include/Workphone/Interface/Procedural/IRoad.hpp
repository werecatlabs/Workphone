#ifndef IRoad_h__
#define IRoad_h__

#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoadHitPoint.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for procedural road objects.
         *
         * This interface defines the contract for road objects in the procedural generation system.
         * It provides methods for managing road nodes, sections, types, and intersections with other
         * roads.
         */
        class WPCore_API IRoad : public IProceduralObject
        {
        public:
            /**
             * @brief Types of roads supported by the procedural system.
             */
            enum class RoadType
            {
                None,        /**< No road type specified. */
                Residential, /**< Residential road. */
                Trunk,       /**< Trunk road (major road). */
                Footway,     /**< Footway (pedestrian path). */
                Steps,       /**< Steps or stairways. */

                Count /**< Number of road types. */
            };

            /**
             * @brief Types of lane configurations for a road.
             */
            enum class LaneType
            {
                OneLane = 1, /**< Single lane road. */
                TwoLane,     /**< Two lane road. */
                ThreeLane,   /**< Three lane road. */
                FourLane,    /**< Four lane road. */
                FiveLane,    /**< Five lane road. */
                SixLane      /**< Six lane road. */
            };

            /**
             * @brief Virtual destructor.
             */
            ~IRoad() override;

            /**
             * @brief Computes intersection points with another road.
             * @param road The other road to check for intersections.
             * @return Array of intersection points (IRoadHitPoint).
             */
            virtual Array<SmartPtr<IRoadHitPoint>> intersects( SmartPtr<IRoad> road ) = 0;

            /**
             * @brief Gets all nodes that define the road geometry.
             * @return Array of road nodes (IRoadNode).
             */
            virtual Array<SmartPtr<IRoadNode>> getRoadNodes() const = 0;

            /**
             * @brief Gets the road node at the specified index.
             * @param index Index of the node.
             * @return Smart pointer to the road node.
             */
            virtual SmartPtr<IRoadNode> getNode( size_t index ) const = 0;

            /**
             * @brief Gets the first node of the road.
             * @return Smart pointer to the first road node.
             */
            virtual SmartPtr<IRoadNode> getFirstNode() const = 0;

            /**
             * @brief Gets the last node of the road.
             * @return Smart pointer to the last road node.
             */
            virtual SmartPtr<IRoadNode> getLastNode() const = 0;

            /**
             * @brief Gets the road type as a string.
             * @return The road type string.
             */
            virtual String getRoadType() const = 0;

            /**
             * @brief Sets the road type.
             * @param roadType The road type string to set.
             */
            virtual void setRoadType( const String &roadType ) = 0;

            /**
             * @brief Gets the marker index from a given transform.
             * @param transform The transform to query.
             * @return The marker index corresponding to the transform.
             */
            virtual s32 getMarkerFromTransform( const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Adds a road section to this road.
             * @param section The road section to add.
             */
            virtual void addRoadSection( SmartPtr<IRoadSection> section ) = 0;

            /**
             * @brief Removes a road section from this road.
             * @param section The road section to remove.
             */
            virtual void removeRoadSection( SmartPtr<IRoadSection> section ) = 0;

            /**
             * @brief Gets all road sections that make up this road.
             * @return Array of road sections (IRoadSection).
             */
            virtual Array<SmartPtr<IRoadSection>> getRoadSections() const = 0;

            /**
             * @brief Sets the road sections for this road.
             * @param roadSections Array of road sections to set.
             */
            virtual void setRoadSections( Array<SmartPtr<IRoadSection>> roadSections ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoad_h__
