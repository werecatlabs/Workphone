#ifndef CRoadSection_h__
#define CRoadSection_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoadSection.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Line3.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Core/Handle.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CRoadSection
         * @brief Concrete implementation of IRoadSection.
         *
         * A road section owns an ordered list of IRoadElement segments that make
         * up one continuous stretch of road between two junctions.  It is
         * responsible for splitting the raw node sequence at connection points
         * to produce individual driveable segments, computing geometry bounds,
         * and exposing all metadata to the game editor through the property
         * system.
         */
        class WPProcedural_API CRoadSection : public CProceduralObject<IRoadSection>
        {
        public:
            CRoadSection();
            ~CRoadSection() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void build() override;

            void updateBounds() override;

            // ---------------------------------------------------------------
            // IRoadSection interface
            // ---------------------------------------------------------------

            void addRoadElement( SmartPtr<IRoadElement> roadElement ) override;
            void removeRoadElement( SmartPtr<IRoadElement> roadElement ) override;
            void clearRoadElements() override;

            Array<SmartPtr<IRoadElement>> getElements() const override;
            void setElements( const Array<SmartPtr<IRoadElement>> &elements ) override;

            // ---------------------------------------------------------------
            // Local helpers
            // ---------------------------------------------------------------

            Array<SmartPtr<IRoadNode>> getRoadNodes() const;

            // ---------------------------------------------------------------
            // Property system – exposes all members to the game editor
            // ---------------------------------------------------------------

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            /// Build one IRoadElement per sub-segment separated by connection nodes.
            void createChildRoads();

            /// Count the number of intermediate connection nodes (excludes endpoints).
            s32 getNumConnections() const;

            /// Split the raw node list into sub-lists at each connection node.
            Array<SmartPtr<IRoadElement>> splitRoadByConnection();

            /// Return the node list partitioned at each connection node.
            Array<Array<SmartPtr<IRoadNode>>> getRoadNodesByConnections() const;

            SmartPtr<IRoad>
                m_parentRoad;  ///< Parent road that owns this section (may be null during construction).
            Array<SmartPtr<ISidewalk>> m_sidewalks;  ///< Sidewalk objects flanking this section.
            Array<Array<SmartPtr<IRoadElement>>>
                m_roadElements;  ///< 2-D grouping used during road splitting (columns = connection spans).
            Array<SmartPtr<IRoadElement>> m_elements;  ///< Final flat list of road element segments.
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // RoadSegment_h__
