#ifndef __CRoadNode_h__
#define __CRoadNode_h__

#include "WPProcedural/CProceduralNode.hpp"
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoadNode.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CRoadNode
         * @brief Concrete implementation of the IRoadNode interface.
         *
         * Represents a single control point on a road spine. Each node may
         * belong to one or more roads, track a connection type (plain node vs.
         * junction), and hold a back-reference to the IRoadElement that owns
         * it. All state is accessible through the property system for
         * game-editor inspection and modification.
         */
        class WPProcedural_API CRoadNode : public CProceduralNode<IRoadNode>
        {
        public:
            CRoadNode();
            ~CRoadNode() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            // ---------------------------------------------------------------
            // IRoadNode interface
            // ---------------------------------------------------------------

            void addRoad( const SmartPtr<IRoad> &road ) override;
            void removeRoad( const SmartPtr<IRoad> &road ) override;
            Array<SmartPtr<IRoad>> getRoads() const override;

            s32 getRoadId() const override;
            void setRoadId( s32 id ) override;

            SmartPtr<IRoadNode> getRoadNodeFromMerged( SmartPtr<IRoad> road ) const override;

            SmartPtr<ISharedObject> clone() override;

            CRoadNode &operator=( const CRoadNode &other );

            String getConnectionType() const override;

            // ---------------------------------------------------------------
            // Road element back-reference
            // ---------------------------------------------------------------

            SmartPtr<IRoadElement> getRoadElement() const;
            void setRoadElement( SmartPtr<IRoadElement> value );

            // ---------------------------------------------------------------
            // Property system – exposes all members to the game editor
            // ---------------------------------------------------------------

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<IRoad>> m_roads;        ///< Roads that pass through this node.
            SmartPtr<IRoadElement> m_roadElement;  ///< Road element that owns this node (may be null).
            s32 m_roadId = -1;  ///< ID of the primary road this node belongs to (-1 = unset).
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // RoadNode_h__
