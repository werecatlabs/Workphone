#ifndef CRoadElement_h__
#define CRoadElement_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoad.hpp>
#include <Workphone/Interface/Procedural/IRoadElement.hpp>
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
         * @class CRoadElement
         * @brief Concrete implementation of IRoadElement.
         *
         * Represents a single logical segment of a road inside a road section.
         * Stores geometry-driving metadata (road type, lane count, width, speed
         * limit, lighting, etc.) and exposes all fields through the property
         * system so the game editor can inspect and modify them at runtime.
         */
        class WPProcedural_API CRoadElement : public CProceduralObject<IRoadElement>
        {
        public:
            CRoadElement();
            ~CRoadElement() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void build() override;

            void updateBounds() override;

            // ---------------------------------------------------------------
            // IRoadElement interface
            // ---------------------------------------------------------------

            SmartPtr<IRoadSection> getParentSection() const override;
            void setParentSection( SmartPtr<IRoadSection> value ) override;

            Array<SmartPtr<IRoadNode>> getRoadNodes() const override;

            Array<SmartPtr<IRoadNode>> getSidewalks() const override;
            void setSidewalks( Array<SmartPtr<IRoadNode>> value ) override;

            IRoad::RoadType getRoadType() const override;
            void setRoadType( IRoad::RoadType value ) override;

            IRoad::LaneType getLaneType() const override;
            void setLaneType( IRoad::LaneType value ) override;

            bool isLit() const override;
            void setIsLit( bool value ) override;

            // ---------------------------------------------------------------
            // Extended metadata (not in IRoadElement interface)
            // ---------------------------------------------------------------

            bool isOneWay() const;
            void setOneWay( bool value );

            bool isBicycle() const;
            void setIsBicycle( bool value );

            bool isFootway() const;
            void setIsFootway( bool value );

            f32 getRoadWidth() const;
            void setRoadWidth( f32 value );

            s32 getSpeedLimit() const;
            void setSpeedLimit( s32 value );

            String getReference() const;
            void setReference( const String &value );

            s32 getNumConnections() const;
            void setNumConnections( s32 value );

            // ---------------------------------------------------------------
            // Property system – exposes all members to the game editor
            // ---------------------------------------------------------------

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IRoadSection> m_parentSection;  ///< Owning road section.
            Array<SmartPtr<IRoadNode>> m_sidewalks;  ///< Sidewalk nodes attached to this element.

            IRoad::RoadType m_roadType = IRoad::RoadType::None;     ///< Functional road classification.
            IRoad::LaneType m_laneType = IRoad::LaneType::TwoLane;  ///< Number of lanes.

            f32 m_RoadWidth = 10.0f;  ///< Physical carriageway width in world units.
            s32 m_SpeedLimit =
                30;  ///< Posted speed limit (km/h or mph depending on project convention).
            s32 m_NumConnections = 0;  ///< Number of other road elements connected at either end.
            String m_Reference;        ///< Optional OSM-style reference tag (e.g. "A1", "M25").

            bool m_IsLit = true;       ///< Whether the road has street lighting.
            bool m_OneWay = false;     ///< Whether traffic flows in one direction only.
            bool m_IsBicycle = false;  ///< Whether the element is a dedicated cycle lane.
            bool m_IsFootway = false;  ///< Whether the element is a pedestrian footway.
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // RoadSegment_h__
