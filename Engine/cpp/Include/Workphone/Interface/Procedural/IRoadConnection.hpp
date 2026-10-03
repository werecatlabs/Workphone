#ifndef IRoadConnection_h__
#define IRoadConnection_h__

#include <Workphone/Interface/Procedural/IProceduralObject.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IRoadConnection : public IProceduralObject
        {
        public:
            enum class EType
            {
                None,
                T_Crossing,
                X_Crossing,
                L_Connection,
                RoundingAbout,

                Count
            };

            ~IRoadConnection() override;

            virtual String getResourceName() const = 0;
            virtual void setResourceName( const String &name ) = 0;

            virtual String getConnectionType() const = 0;
            virtual void setConnectionType( const String &connectionType ) = 0;

            virtual void addConnection( SmartPtr<IRoadConnectionData> connection ) = 0;
            virtual void removeConnection( SmartPtr<IRoadConnectionData> connection ) = 0;

            virtual Array<SmartPtr<IRoadConnectionData>> getConnectionData() const = 0;
            virtual void setConnectionData( Array<SmartPtr<IRoadConnectionData>> connectData ) = 0;

            virtual SmartPtr<IRoadNode> getNode() const = 0;
            virtual Array<SmartPtr<IRoadNode>> getRoadNodes() const = 0;
            virtual void setRoadNodes( const Array<SmartPtr<IRoadNode>> &roadNodes ) = 0;

            virtual EType getType() const = 0;
            virtual void setType( EType type ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadConnection_h__
