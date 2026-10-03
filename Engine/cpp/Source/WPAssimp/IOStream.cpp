#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/IOStream.hpp>
#include <utility>

namespace workphone
{

    IOStream::IOStream( SmartPtr<IStream> stream ) : m_stream( std::move( stream ) )
    {
    }

    auto IOStream::Read( void *pvBuffer, size_t pSize, size_t pCount ) -> size_t
    {
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
        if( pOrigin != aiOrigin_SET )
        {
            return AI_FAILURE;
        }

        m_stream->seek( pOffset );
        return AI_SUCCESS;
    }

    void IOStream::Flush()
    {
        if( m_stream )
        {
            //m_stream->flush();
        }
    }

}  // namespace workphone
