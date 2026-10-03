#ifndef NetworkMessage_h__
#define NetworkMessage_h__

#include <Workphone/Interface/Net/IPacket.hpp>

namespace workphone
{

    /**
     * @brief Engine-reserved message identifiers used by actor replication.
     *
     * Application messages should use values at or above @c UserMessageBegin so
     * they cannot be mistaken for engine-owned replication traffic.
     */
    enum class NetworkMessageType : u8
    {
        Invalid = 0,
        ActorState = 1,
        OwnershipTransfer = 2,
        OwnershipRequest = 3,
        RemoteProcedureCall = 4,
        UserMessageBegin = 64
    };

    /**
     * @brief Versioned envelope placed in front of engine network messages.
     *
     * This keeps packet routing independent from the transport and lets future
     * protocol versions coexist without interpreting arbitrary payload bytes as
     * a view ID. The layout is written field-by-field and is therefore not
     * affected by compiler structure padding.
     */
    struct NetworkMessageHeader
    {
        static constexpr u32 Magic = 0x574E4554u;  // "WNET"
        static constexpr u8 CurrentVersion = 1u;
        static constexpr u32 SerializedSize =
            sizeof( u32 ) + sizeof( u8 ) + sizeof( u8 ) + sizeof( s32 );

        NetworkMessageType type = NetworkMessageType::Invalid;
        s32 objectId = -1;
        u8 version = CurrentVersion;

        void write( SmartPtr<IPacket> packet ) const
        {
            if( !packet )
            {
                return;
            }

            packet->write( Magic );
            packet->write( version );
            packet->write( static_cast<u8>( type ) );
            packet->write( objectId );
        }

        static bool read( SmartPtr<IPacket> packet, NetworkMessageHeader &header )
        {
            if( !packet || packet->getDataLength() < SerializedSize )
            {
                return false;
            }

            try
            {
                u32 magic = 0;
                u8 messageType = 0;
                packet->read( magic );
                packet->read( header.version );
                packet->read( messageType );
                packet->read( header.objectId );

                if( magic != Magic || header.version != CurrentVersion )
                {
                    return false;
                }

                header.type = static_cast<NetworkMessageType>( messageType );
                return header.type != NetworkMessageType::Invalid;
            }
            catch( ... )
            {
                return false;
            }
        }
    };

}  // namespace workphone

#endif  // NetworkMessage_h__
