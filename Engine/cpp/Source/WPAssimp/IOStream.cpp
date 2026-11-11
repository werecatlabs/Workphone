#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/IOStream.hpp>
#include <limits>
#include <utility>

namespace workphone
{

    IOStream::IOStream( SmartPtr<IStream> stream ) : m_stream( std::move( stream ) )
    {
    }

    auto IOStream::Read( void *pvBuffer, size_t pSize, size_t pCount ) -> size_t
    {
        if( pSize == 0 || pCount > std::numeric_limits<size_t>::max() / pSize )
        {
            return 0;
        }
        const auto bytes = m_stream->read( pvBuffer, pSize * pCount );
        return bytes / pSize;
    }

    auto IOStream::Tell() const -> size_t
    {
        return m_stream->tell();
    }

    auto IOStream::FileSize() const -> size_t
    {
        return m_stream->size();
    }

    auto IOStream::Write( const void *pvBuffer, size_t pSize, size_t pCount ) -> size_t
    {
        return 0;
    }

    auto IOStream::Seek( size_t pOffset, aiOrigin pOrigin ) -> aiReturn
    {
        const auto size = m_stream->size();
        size_t position = pOffset;
        if( pOrigin == aiOrigin_CUR || pOrigin == aiOrigin_END )
        {
            const auto base = pOrigin == aiOrigin_CUR ? m_stream->tell() : size;
            // Assimp represents negative relative offsets in the unsigned API.
            if( pOffset > std::numeric_limits<size_t>::max() / 2 )
            {
                const auto distance = size_t( 0 ) - pOffset;
                if( distance > base ) return AI_FAILURE;
                position = base - distance;
            }
            else
            {
                if( base > size || pOffset > size - base ) return AI_FAILURE;
                position = base + pOffset;
            }
        }
        else if( pOrigin != aiOrigin_SET )
        {
            return AI_FAILURE;
        }

        return position <= size && m_stream->seek( position ) ? AI_SUCCESS : AI_FAILURE;
    }

    void IOStream::Flush()
    {
        if( m_stream )
        {
            //m_stream->flush();
        }
    }

}  // namespace workphone
