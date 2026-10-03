#ifndef CRoadConnection_h__
#define CRoadConnection_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/IRoadConnection.hpp>
#include <WPProcedural/CProceduralObject.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Math/Line2.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CRoadConnection
         * @brief Concrete implementation of IRoadConnection.
         *
         * Represents a connection point between two or more road segments in the
         * procedural road network (e.g. T-crossing, X-crossing, roundabout).
         * Exposes all state through the property system so the game editor can
         * inspect and modify connections at runtime.
         */
        class WPProcedural_API CRoadConnection : public CProceduralObject<IRoadConnection>
        {
        public:
            CRoadConnection();
            ~CRoadConnection() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            // ---------------------------------------------------------------
            // IRoadConnection interface
            // ---------------------------------------------------------------

            String getResourceName() const override;
            void setResourceName( const String &name ) override;

            String getConnectionType() const override;
            void setConnectionType( const String &connectionType ) override;

            void addConnection( SmartPtr<IRoadConnectionData> connection ) override;
            void removeConnection( SmartPtr<IRoadConnectionData> connection ) override;

            Array<SmartPtr<IRoadConnectionData>> getConnectionData() const override;
            void setConnectionData( Array<SmartPtr<IRoadConnectionData>> connectionData ) override;

            SmartPtr<IRoadNode> getNode() const override;
            Array<SmartPtr<IRoadNode>> getRoadNodes() const override;
            void setRoadNodes( const Array<SmartPtr<IRoadNode>> &roadNodes ) override;

            EType getType() const override;
            void setType( EType type ) override;

            // ---------------------------------------------------------------
            // Property system – exposes members to the game editor
            // ---------------------------------------------------------------

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            EType m_type = EType::None;  ///< Crossing / connection geometry type.
            String m_resourceName;       ///< Asset resource name for this connection mesh.
            String m_connectionType;     ///< Free-form type tag (e.g. "intersection", "merge").
            SmartPtr<IRoadNode> m_node;  ///< Primary road node at this connection.
            Array<SmartPtr<IRoadNode>>
                m_roadNodes;  ///< All road nodes that participate in this connection.
            Array<SmartPtr<IRoadConnectionData>>
                m_connectionData;  ///< Per-road connection descriptor records.
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CRoadConnection_h__
