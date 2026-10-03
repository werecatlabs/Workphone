#ifndef _IPacket_H
#define _IPacket_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Engine-agnostic network packet interface.
     *
     * IPacket wraps an underlying transport-layer bit-stream (e.g. RakNet's
     * @c BitStream) and exposes typed read/write overloads for all primitive
     * types used by the engine's networking layer.
     *
     * The packet carries a sender address (@c getSystemAddress) and supports
     * the common RakNet idiom of skipping the leading message-type byte via
     * @c ignoreMessageId before reading application-level payload.
     *
     * ### Typical write path (sender)
     * @code
     * auto packet = networkManager->createPacket();
     * packet->write( static_cast<u8>( MessageId::StateSync ) );
     * packet->write( viewId );
     * packet->write( position );
     * networkManager->sendPacket( packet );
     * @endcode
     *
     * ### Typical read path (receiver — inside INetworkListener::handlePacket)
     * @code
     * void MyListener::handlePacket( SmartPtr<IPacket> packet )
     * {
     *     packet->ignoreMessageId();   // skip the one-byte message ID
     *     s32 viewId;
     *     Vector3F position;
     *     packet->read( viewId );
     *     packet->read( position );
     * }
     * @endcode
     *
     * @note Values must be read in exactly the same order they were written.
     *
     * @see INetworkManager, INetworkListener, INetworkStream, ISystemAddress
     */
    class WPCore_API IPacket : public ISharedObject
    {
    public:
        ~IPacket() override;

        // ------------------------------------------------------------------
        // Read overloads
        // ------------------------------------------------------------------

        /// @brief Reads a signed 8-bit integer from the stream.
        virtual void read( s8 &value ) = 0;

        /// @brief Reads an unsigned 8-bit integer from the stream.
        virtual void read( u8 &value ) = 0;

        /// @brief Reads an unsigned 16-bit integer from the stream.
        virtual void read( u16 &value ) = 0;

        /// @brief Reads a signed 16-bit integer from the stream.
        virtual void read( s16 &value ) = 0;

        /// @brief Reads an unsigned 32-bit integer from the stream.
        virtual void read( u32 &value ) = 0;

        /// @brief Reads a signed 32-bit integer from the stream.
        virtual void read( s32 &value ) = 0;

        /// @brief Reads a 32-bit floating-point value from the stream.
        virtual void read( f32 &value ) = 0;

        /// @brief Reads a 2D integer vector from the stream.
        virtual void read( Vector2I &value ) = 0;

        /// @brief Reads a 2D floating-point vector from the stream.
        virtual void read( Vector2<real_Num> &value ) = 0;

        /// @brief Reads a 3D integer vector from the stream.
        virtual void read( Vector3I &value ) = 0;

        /// @brief Reads a 3D floating-point vector from the stream.
        virtual void read( Vector3<real_Num> &value ) = 0;

        /// @brief Reads a length-prefixed string from the stream.
        virtual void read( String &value ) = 0;

        /// @brief Reads a boolean value from the stream.
        virtual void read( bool &value ) = 0;

        // ------------------------------------------------------------------
        // Write overloads
        // ------------------------------------------------------------------

        /// @brief Writes a signed 8-bit integer to the stream.
        virtual void write( s8 value ) = 0;

        /// @brief Writes an unsigned 8-bit integer to the stream.
        virtual void write( u8 value ) = 0;

        /// @brief Writes an unsigned 16-bit integer to the stream.
        virtual void write( u16 value ) = 0;

        /// @brief Writes a signed 16-bit integer to the stream.
        virtual void write( s16 value ) = 0;

        /// @brief Writes an unsigned 32-bit integer to the stream.
        virtual void write( u32 value ) = 0;

        /// @brief Writes a signed 32-bit integer to the stream.
        virtual void write( s32 value ) = 0;

        /// @brief Writes a 32-bit floating-point value to the stream.
        virtual void write( f32 value ) = 0;

        /// @brief Writes a 2D integer vector to the stream.
        virtual void write( const Vector2I &value ) = 0;

        /// @brief Writes a 2D floating-point vector to the stream.
        virtual void write( const Vector2<real_Num> &value ) = 0;

        /// @brief Writes a 3D integer vector to the stream.
        virtual void write( const Vector3I &value ) = 0;

        /// @brief Writes a 3D floating-point vector to the stream.
        virtual void write( const Vector3<real_Num> &value ) = 0;

        /// @brief Writes a length-prefixed string to the stream.
        virtual void write( const String &value ) = 0;

        /// @brief Writes a boolean value to the stream.
        virtual void write( const bool &value ) = 0;

        // ------------------------------------------------------------------
        // Buffer management
        // ------------------------------------------------------------------

        /**
         * @brief Skips the leading one-byte message-type identifier.
         *
         * RakNet prepends every packet with an @c unsigned @c char message ID.
         * Call this once at the start of @c INetworkListener::handlePacket so
         * that subsequent @c read calls begin at the application-level payload.
         */
        virtual void ignoreMessageId() = 0;

        /**
         * @brief Returns the number of bytes of payload written into the stream.
         *
         * Useful for logging and for deciding whether to send a packet at all
         * (e.g. skip a send if @c getDataLength() == 0 after writes).
         *
         * @return Byte count of data currently in the stream.
         */
        virtual u32 getDataLength() const = 0;

        /**
         * @brief Resets the read cursor to the beginning of the stream.
         *
         * Allows the same packet to be re-parsed without allocating a new
         * object.  The write cursor and payload data are unaffected.
         */
        virtual void resetReadPointer() = 0;

        // ------------------------------------------------------------------
        // Addressing
        // ------------------------------------------------------------------

        /**
         * @brief Returns the network address of the sender of this packet.
         *
         * On outgoing packets created via @c INetworkManager::createPacket the
         * address is unset; it is populated by the transport layer upon receipt.
         *
         * @return Shared pointer to the sender's system address; may be null
         *         for locally-created packets that have not been sent yet.
         */
        virtual SmartPtr<ISystemAddress> getSystemAddress() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
