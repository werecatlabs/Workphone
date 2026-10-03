#ifndef NetworkStream_h__
#define NetworkStream_h__

#include <Workphone/Interface/Net/INetworkStream.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Concrete implementation of INetworkStream.
         *
         * NetworkStream mirrors Unity PUN's @c PhotonStream: a mode-aware,
         * typed serialisation buffer exchanged during @c OnPhotonSerializeView
         * (here: @c NetworkView::serializeView / @c deserializeView).
         *
         * ### Write mode
         * Constructed with @c isWriting = @c true.  Each typed @c write() call
         * appends the value's raw bytes to an internal @c Array<u8> buffer.
         * When serialisation is complete the network manager copies getData()
         * into an outgoing IPacket.
         *
         * ### Read mode
         * Constructed with @c isWriting = @c false and an existing byte payload
         * (set via setData()).  Each typed @c read() call extracts bytes from
         * the buffer at the current cursor position.
         *
         * ### Value layout
         * All arithmetic types are stored in their native byte representation
         * (no endian conversion).  Strings are length-prefixed with a @c u32
         * character count followed by the raw UTF-8 bytes.  Composite types
         * (Vector2, Vector3) are stored as sequential components.
         *
         * @see INetworkStream, NetworkView
         */
        class WPCore_API NetworkStream : public INetworkStream
        {
        public:
            /**
             * @brief Constructs a stream in write or read mode.
             *
             * @param writing @c true to create a write (send) stream,
             *                @c false to create a read (receive) stream.
             */
            explicit NetworkStream( bool writing = true );

            ~NetworkStream() override;

            // ------------------------------------------------------------------
            // INetworkStream — Mode
            // ------------------------------------------------------------------

            bool isWriting() const override;
            bool isReading() const override;

            // ------------------------------------------------------------------
            // INetworkStream — Raw bytes
            // ------------------------------------------------------------------

            size_t read( void *buffer, size_t size ) override;
            size_t write( const void *buffer, size_t size ) override;

            // ------------------------------------------------------------------
            // INetworkStream — Typed reads
            // ------------------------------------------------------------------

            void read( s8 &value ) override;
            void read( u8 &value ) override;
            void read( s16 &value ) override;
            void read( u16 &value ) override;
            void read( s32 &value ) override;
            void read( u32 &value ) override;
            void read( f32 &value ) override;
            void read( bool &value ) override;
            void read( String &value ) override;
            void read( Vector2I &value ) override;
            void read( Vector2<real_Num> &value ) override;
            void read( Vector3I &value ) override;
            void read( Vector3<real_Num> &value ) override;

            // ------------------------------------------------------------------
            // INetworkStream — Typed writes
            // ------------------------------------------------------------------

            void write( s8 value ) override;
            void write( u8 value ) override;
            void write( s16 value ) override;
            void write( u16 value ) override;
            void write( s32 value ) override;
            void write( u32 value ) override;
            void write( f32 value ) override;
            void write( bool value ) override;
            void write( const String &value ) override;
            void write( const Vector2I &value ) override;
            void write( const Vector2<real_Num> &value ) override;
            void write( const Vector3I &value ) override;
            void write( const Vector3<real_Num> &value ) override;

            // ------------------------------------------------------------------
            // INetworkStream — Buffer management
            // ------------------------------------------------------------------

            size_t getSize() const override;
            size_t getPosition() const override;
            void reset() override;
            const u8 *getData() const override;

            /**
             * @brief Replaces the internal buffer with the supplied data.
             *
             * Typically called by the network manager after receiving a packet,
             * before passing the stream to a NetworkView in read mode.  The
             * cursor is reset to the start of the new buffer.
             *
             * @param data Pointer to the source bytes.
             * @param size Number of bytes to copy.
             */
            void setData( const void *data, size_t size );

            WP_CLASS_REGISTER_DECL;

        private:
            /** Helper: append @p count raw bytes of @p src to the write buffer. */
            void appendBytes( const void *src, size_t count );

            /** Helper: copy @p count raw bytes from the read buffer into @p dst. */
            void extractBytes( void *dst, size_t count );

            Array<u8> m_buffer;
            size_t m_position = 0;
            bool m_writing = true;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // NetworkStream_h__
