#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/NetworkStream.hpp>

#include <cstring>
#include <stdexcept>

namespace workphone
{
    namespace scene
    {

        WP_CLASS_REGISTER_DERIVED( workphone, NetworkStream, INetworkStream );

        NetworkStream::NetworkStream( bool writing ) : m_writing( writing )
        {
        }

        NetworkStream::~NetworkStream() = default;

        // ------------------------------------------------------------------
        // Private helpers
        // ------------------------------------------------------------------

        void NetworkStream::appendBytes( const void *src, size_t count )
        {
            const auto *bytes = static_cast<const u8 *>( src );
            m_buffer.insert( m_buffer.end(), bytes, bytes + count );
            m_position += count;
        }

        void NetworkStream::extractBytes( void *dst, size_t count )
        {
            if( m_position + count > m_buffer.size() )
            {
                throw std::out_of_range( "NetworkStream: read past end of buffer" );
            }

            std::memcpy( dst, m_buffer.data() + m_position, count );
            m_position += count;
        }

        // ------------------------------------------------------------------
        // Mode
        // ------------------------------------------------------------------

        bool NetworkStream::isWriting() const
        {
            return m_writing;
        }

        bool NetworkStream::isReading() const
        {
            return !m_writing;
        }

        // ------------------------------------------------------------------
        // Raw bytes
        // ------------------------------------------------------------------

        size_t NetworkStream::read( void *buffer, size_t size )
        {
            const size_t available = m_buffer.size() - m_position;
            const size_t toRead = ( size < available ) ? size : available;

            if( toRead == 0 )
            {
                return 0;
            }

            std::memcpy( buffer, m_buffer.data() + m_position, toRead );
            m_position += toRead;
            return toRead;
        }

        size_t NetworkStream::write( const void *buffer, size_t size )
        {
            appendBytes( buffer, size );
            return size;
        }

        // ------------------------------------------------------------------
        // Typed reads
        // ------------------------------------------------------------------

        void NetworkStream::read( s8 &value )
        {
            extractBytes( &value, sizeof( s8 ) );
        }
        void NetworkStream::read( u8 &value )
        {
            extractBytes( &value, sizeof( u8 ) );
        }
        void NetworkStream::read( s16 &value )
        {
            extractBytes( &value, sizeof( s16 ) );
        }
        void NetworkStream::read( u16 &value )
        {
            extractBytes( &value, sizeof( u16 ) );
        }
        void NetworkStream::read( s32 &value )
        {
            extractBytes( &value, sizeof( s32 ) );
        }
        void NetworkStream::read( u32 &value )
        {
            extractBytes( &value, sizeof( u32 ) );
        }
        void NetworkStream::read( f32 &value )
        {
            extractBytes( &value, sizeof( f32 ) );
        }

        void NetworkStream::read( bool &value )
        {
            u8 raw = 0;
            extractBytes( &raw, sizeof( u8 ) );
            value = ( raw != 0 );
        }

        void NetworkStream::read( String &value )
        {
            u32 len = 0;
            extractBytes( &len, sizeof( u32 ) );

            if( m_position + len > m_buffer.size() )
            {
                throw std::out_of_range( "NetworkStream: string read past end of buffer" );
            }

            value.assign( reinterpret_cast<const char *>( m_buffer.data() + m_position ), len );
            m_position += len;
        }

        void NetworkStream::read( Vector2I &value )
        {
            extractBytes( &value.x, sizeof( s32 ) );
            extractBytes( &value.y, sizeof( s32 ) );
        }

        void NetworkStream::read( Vector2<real_Num> &value )
        {
            extractBytes( &value.x, sizeof( real_Num ) );
            extractBytes( &value.y, sizeof( real_Num ) );
        }

        void NetworkStream::read( Vector3I &value )
        {
            extractBytes( &value.x, sizeof( s32 ) );
            extractBytes( &value.y, sizeof( s32 ) );
            extractBytes( &value.z, sizeof( s32 ) );
        }

        void NetworkStream::read( Vector3<real_Num> &value )
        {
            extractBytes( &value.x, sizeof( real_Num ) );
            extractBytes( &value.y, sizeof( real_Num ) );
            extractBytes( &value.z, sizeof( real_Num ) );
        }

        // ------------------------------------------------------------------
        // Typed writes
        // ------------------------------------------------------------------

        void NetworkStream::write( s8 value )
        {
            appendBytes( &value, sizeof( s8 ) );
        }
        void NetworkStream::write( u8 value )
        {
            appendBytes( &value, sizeof( u8 ) );
        }
        void NetworkStream::write( s16 value )
        {
            appendBytes( &value, sizeof( s16 ) );
        }
        void NetworkStream::write( u16 value )
        {
            appendBytes( &value, sizeof( u16 ) );
        }
        void NetworkStream::write( s32 value )
        {
            appendBytes( &value, sizeof( s32 ) );
        }
        void NetworkStream::write( u32 value )
        {
            appendBytes( &value, sizeof( u32 ) );
        }
        void NetworkStream::write( f32 value )
        {
            appendBytes( &value, sizeof( f32 ) );
        }

        void NetworkStream::write( bool value )
        {
            const u8 raw = value ? 1 : 0;
            appendBytes( &raw, sizeof( u8 ) );
        }

        void NetworkStream::write( const String &value )
        {
            const auto len = static_cast<u32>( value.size() );
            appendBytes( &len, sizeof( u32 ) );
            appendBytes( value.data(), len );
        }

        void NetworkStream::write( const Vector2I &value )
        {
            appendBytes( &value.x, sizeof( s32 ) );
            appendBytes( &value.y, sizeof( s32 ) );
        }

        void NetworkStream::write( const Vector2<real_Num> &value )
        {
            appendBytes( &value.x, sizeof( real_Num ) );
            appendBytes( &value.y, sizeof( real_Num ) );
        }

        void NetworkStream::write( const Vector3I &value )
        {
            appendBytes( &value.x, sizeof( s32 ) );
            appendBytes( &value.y, sizeof( s32 ) );
            appendBytes( &value.z, sizeof( s32 ) );
        }

        void NetworkStream::write( const Vector3<real_Num> &value )
        {
            appendBytes( &value.x, sizeof( real_Num ) );
            appendBytes( &value.y, sizeof( real_Num ) );
            appendBytes( &value.z, sizeof( real_Num ) );
        }

        // ------------------------------------------------------------------
        // Buffer management
        // ------------------------------------------------------------------

        size_t NetworkStream::getSize() const
        {
            return m_buffer.size();
        }

        size_t NetworkStream::getPosition() const
        {
            return m_position;
        }

        void NetworkStream::reset()
        {
            m_position = 0;
        }

        const u8 *NetworkStream::getData() const
        {
            return m_buffer.empty() ? nullptr : m_buffer.data();
        }

        void NetworkStream::setData( const void *data, size_t size )
        {
            const auto *bytes = static_cast<const u8 *>( data );
            m_buffer.clear();
            if( size > 0 )
            {
                m_buffer.insert( m_buffer.begin(), bytes, bytes + size );
            }
            m_position = 0;
        }

    }  // namespace scene
}  // namespace workphone
