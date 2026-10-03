#ifndef INetworkStream_h__
#define INetworkStream_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Typed, mode-aware serialization stream for network state synchronisation.
     *
     * INetworkStream mirrors Unity PUN's @c PhotonStream, which is passed to
     * @c OnPhotonSerializeView every network tick.  The stream is either in
     * @e writing mode (the local peer owns the object and pushes state) or
     * @e reading mode (a remote peer owns the object and the local client
     * consumes incoming state).
     *
     * ### Typical usage
     * @code
     * void MyComponent::onSerializeView( SmartPtr<INetworkStream> stream )
     * {
     *     if ( stream->isWriting() )
     *     {
     *         stream->write( m_position );
     *         stream->write( m_health );
     *     }
     *     else
     *     {
     *         stream->read( m_position );
     *         stream->read( m_health );
     *     }
     * }
     * @endcode
     *
     * @note Values must be written and read in exactly the same order.
     *
     * @see INetworkManager, NetworkView, IPacket
     */
    class WPCore_API INetworkStream : public ISharedObject
    {
    public:
        ~INetworkStream() override;

        // ------------------------------------------------------------------
        // Mode queries
        // ------------------------------------------------------------------

        /**
         * @brief Returns @c true when the stream is in write (send) mode.
         *
         * The local peer owns the networked object and is responsible for
         * serialising its state into the stream each network tick.
         *
         * @return @c true if writing, @c false if reading.
         */
        virtual bool isWriting() const = 0;

        /**
         * @brief Returns @c true when the stream is in read (receive) mode.
         *
         * The stream contains data sent by the authoritative peer; the local
         * client must deserialise values in the same order they were written.
         *
         * @return @c true if reading, @c false if writing.
         */
        virtual bool isReading() const = 0;

        // ------------------------------------------------------------------
        // Raw byte access
        // ------------------------------------------------------------------

        /**
         * @brief Reads raw bytes from the stream into @p buffer.
         *
         * @param buffer Destination buffer; must be at least @p size bytes.
         * @param size   Number of bytes to read.
         * @return       Actual number of bytes read (may be less at end-of-stream).
         */
        virtual size_t read( void *buffer, size_t size ) = 0;

        /**
         * @brief Writes raw bytes from @p buffer into the stream.
         *
         * @param buffer Source buffer.
         * @param size   Number of bytes to write.
         * @return       Actual number of bytes written.
         */
        virtual size_t write( const void *buffer, size_t size ) = 0;

        // ------------------------------------------------------------------
        // Typed read overloads
        // ------------------------------------------------------------------

        /** @brief Reads a signed 8-bit integer. */
        virtual void read( s8 &value ) = 0;
        /** @brief Reads an unsigned 8-bit integer. */
        virtual void read( u8 &value ) = 0;
        /** @brief Reads a signed 16-bit integer. */
        virtual void read( s16 &value ) = 0;
        /** @brief Reads an unsigned 16-bit integer. */
        virtual void read( u16 &value ) = 0;
        /** @brief Reads a signed 32-bit integer. */
        virtual void read( s32 &value ) = 0;
        /** @brief Reads an unsigned 32-bit integer. */
        virtual void read( u32 &value ) = 0;
        /** @brief Reads a 32-bit floating-point number. */
        virtual void read( f32 &value ) = 0;
        /** @brief Reads a boolean value. */
        virtual void read( bool &value ) = 0;
        /** @brief Reads a UTF-8 string (length-prefixed). */
        virtual void read( String &value ) = 0;
        /** @brief Reads a 2D integer vector. */
        virtual void read( Vector2I &value ) = 0;
        /** @brief Reads a 2D floating-point vector. */
        virtual void read( Vector2<real_Num> &value ) = 0;
        /** @brief Reads a 3D integer vector. */
        virtual void read( Vector3I &value ) = 0;
        /** @brief Reads a 3D floating-point vector. */
        virtual void read( Vector3<real_Num> &value ) = 0;

        // ------------------------------------------------------------------
        // Typed write overloads
        // ------------------------------------------------------------------

        /** @brief Writes a signed 8-bit integer. */
        virtual void write( s8 value ) = 0;
        /** @brief Writes an unsigned 8-bit integer. */
        virtual void write( u8 value ) = 0;
        /** @brief Writes a signed 16-bit integer. */
        virtual void write( s16 value ) = 0;
        /** @brief Writes an unsigned 16-bit integer. */
        virtual void write( u16 value ) = 0;
        /** @brief Writes a signed 32-bit integer. */
        virtual void write( s32 value ) = 0;
        /** @brief Writes an unsigned 32-bit integer. */
        virtual void write( u32 value ) = 0;
        /** @brief Writes a 32-bit floating-point number. */
        virtual void write( f32 value ) = 0;
        /** @brief Writes a boolean value. */
        virtual void write( bool value ) = 0;
        /** @brief Writes a UTF-8 string (length-prefixed). */
        virtual void write( const String &value ) = 0;
        /** @brief Writes a 2D integer vector. */
        virtual void write( const Vector2I &value ) = 0;
        /** @brief Writes a 2D floating-point vector. */
        virtual void write( const Vector2<real_Num> &value ) = 0;
        /** @brief Writes a 3D integer vector. */
        virtual void write( const Vector3I &value ) = 0;
        /** @brief Writes a 3D floating-point vector. */
        virtual void write( const Vector3<real_Num> &value ) = 0;

        // ------------------------------------------------------------------
        // Buffer management
        // ------------------------------------------------------------------

        /**
         * @brief Returns the total number of bytes currently held in the stream buffer.
         *
         * In write mode this is the number of bytes serialised so far.
         * In read mode this is the total size of the received payload.
         *
         * @return Buffer size in bytes.
         */
        virtual size_t getSize() const = 0;

        /**
         * @brief Returns the current read/write cursor position.
         *
         * @return Byte offset from the start of the buffer.
         */
        virtual size_t getPosition() const = 0;

        /**
         * @brief Resets the cursor to the start of the buffer without clearing data.
         *
         * Useful for re-reading a received payload multiple times or rewinding
         * a write buffer for retransmission.
         */
        virtual void reset() = 0;

        /**
         * @brief Returns a read-only pointer to the underlying byte buffer.
         *
         * The pointer is valid until the next mutating call.  Typically used
         * by the network manager to copy serialised data into an outgoing packet.
         *
         * @return Const pointer to the first byte, or @c nullptr if empty.
         */
        virtual const u8 *getData() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // INetworkStream_h__
