#ifndef _CPacket_H
#define _CPacket_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>
#include <BitStream.h>
#include <RakNetTypes.h>

namespace workphone
{

    /**
     * @brief RakNet @c BitStream-backed implementation of IPacket.
     *
     * CPacket wraps a @c RakNet::BitStream and provides typed read/write
     * access to network payload.  It has two distinct life-cycle modes:
     *
     * - **Outgoing** – created via @c INetworkManager::createPacket() and
     *   initialised with @c initialise().  The @c BitStream is heap-allocated
     *   and owned by this object; it is destroyed in the destructor.
     * - **Incoming** – initialised with @c initialise(RakNet::Packet*).  The
     *   @c BitStream is wrapped around the packet's raw buffer
     *   (@c copyData = @c false), so it @e must not outlive the @c RakNet::Packet.
     *   The packet itself is @b not owned here; RakNet reclaims it after the
     *   update loop calls @c DeallocatePacket.
     *
     * ### Outgoing example
     * @code
     * auto packet = networkManager->createPacket();
     * packet->write( static_cast<u8>( MyMsgId::PlayerMove ) );
     * packet->write( position );
     * networkManager->sendPacket( packet );
     * @endcode
     *
     * ### Incoming example (inside INetworkListener::handlePacket)
     * @code
     * void MyListener::handlePacket( SmartPtr<IPacket> p )
     * {
     *     p->ignoreMessageId();   // skip the RakNet message-type byte
     *     Vector3F pos;
     *     p->read( pos );
     * }
     * @endcode
     *
     * @note Values must be read in exactly the same order they were written.
     *
     * @see IPacket, INetworkManager, CSystemAddress
     */
    class CPacket : public IPacket
    {
    public:
        CPacket();
        ~CPacket() override;

        // ------------------------------------------------------------------
        // Initialisation
        // ------------------------------------------------------------------

        /**
         * @brief Initialises an outgoing packet with a fresh, empty @c BitStream.
         *
         * Call once after construction when creating a packet to send.  The
         * @c BitStream is heap-allocated and owned by this object.
         */
        void initialise();

        /**
         * @brief Initialises an incoming packet by wrapping an existing RakNet packet.
         *
         * The @c BitStream is created with @c copyData = @c false so it references
         * the packet's internal buffer directly.  The packet's system address is
         * copied into a new @c CSystemAddress.
         *
         * @param packet  Raw RakNet packet received from @c RakPeerInterface::Receive().
         *                Must remain valid for the lifetime of this @c CPacket.
         *
         * @warning Do not call @c DeallocatePacket on @p packet while this object
         *          is still alive.
         */
        void initialise( RakNet::Packet *packet );

        // ------------------------------------------------------------------
        // IPacket — read overloads
        // ------------------------------------------------------------------

        void read( s8 &value ) override;
        void read( u8 &value ) override;
        void read( u16 &value ) override;
        void read( s16 &value ) override;
        void read( u32 &value ) override;
        void read( s32 &value ) override;
        void read( f32 &value ) override;

        void read( Vector2I &value ) override;
        void read( Vector2F &value ) override;
        void read( Vector3I &value ) override;
        void read( Vector3F &value ) override;

        void read( String &value ) override;
        void read( bool &value ) override;

        // ------------------------------------------------------------------
        // IPacket — write overloads
        // ------------------------------------------------------------------

        void write( s8 value ) override;
        void write( u8 value ) override;
        void write( u16 value ) override;
        void write( s16 value ) override;
        void write( u32 value ) override;
        void write( s32 value ) override;
        void write( f32 value ) override;

        void write( const Vector2I &value ) override;
        void write( const Vector2F &value ) override;
        void write( const Vector3I &value ) override;
        void write( const Vector3F &value ) override;

        void write( const String &value ) override;
        void write( const bool &value ) override;

        // ------------------------------------------------------------------
        // IPacket — buffer management
        // ------------------------------------------------------------------

        /// @copydoc IPacket::ignoreMessageId
        void ignoreMessageId() override;

        /// @copydoc IPacket::getDataLength
        [[nodiscard]] u32 getDataLength() const override;

        /// @copydoc IPacket::resetReadPointer
        void resetReadPointer() override;

        // ------------------------------------------------------------------
        // IPacket — addressing
        // ------------------------------------------------------------------

        /// @copydoc IPacket::getSystemAddress
        [[nodiscard]] SmartPtr<ISystemAddress> getSystemAddress() const override;

        // ------------------------------------------------------------------
        // RakNet-specific accessors
        // ------------------------------------------------------------------

        /**
         * @brief Returns the raw RakNet packet (incoming path only).
         *
         * @return Pointer to the @c RakNet::Packet passed to @c initialise(Packet*),
         *         or @c nullptr for outgoing packets created with @c initialise().
         */
        [[nodiscard]] RakNet::Packet *getPacket() const;
        void setPacket( RakNet::Packet *packet );

        /**
         * @brief Returns the underlying @c BitStream for direct RakNet operations.
         *
         * Exposed so that @c CNetworkManager can pass the stream directly to
         * @c RakPeerInterface::Send without an extra copy.
         */
        [[nodiscard]] RakNet::BitStream *getBitStream() const;
        void setBitStream( RakNet::BitStream *bitStream );

        void setSystemAddress( SmartPtr<ISystemAddress> systemAddress );

    protected:
        /// Raw RakNet packet (incoming only); not owned — do not delete.
        RakNet::Packet *m_packet = nullptr;

        /// Bit-stream for read/write operations; owned when created by initialise().
        RakNet::BitStream *m_bitStream = nullptr;

        /// Network address of the packet's sender, populated during initialise().
        SmartPtr<ISystemAddress> m_systemAddress;
    };

}  // namespace workphone

#endif
