#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/WPOgreDataStream.hpp>
#include <Workphone/Workphone.hpp>
#include <utility>

namespace workphone
{

    WPOgreDataStream::WPOgreDataStream() = default;

    WPOgreDataStream::WPOgreDataStream( SmartPtr<IStream> stream ) :
        Ogre::DataStream(),
        m_stream( std::move( stream ) )
    {
        mSize = m_stream->size();
    }

    WPOgreDataStream::~WPOgreDataStream()
    {
        m_stream = nullptr;
    }

    auto WPOgreDataStream::read( void *buf, size_t count ) -> size_t
    {
        WP_ASSERT( m_stream );

        if( mSize > 0 )
        {
            return m_stream->read( buf, count );
        }

        return 0;
    }

    auto WPOgreDataStream::getLine( bool trimAfter ) -> Ogre::String
    {
        WP_ASSERT( m_stream );
        WP_ASSERT( m_stream->isValid() );

        auto str = m_stream->getLine( trimAfter );
        return str.c_str();
    }

    void WPOgreDataStream::skip( long count )
    {
        WP_ASSERT( m_stream );
        m_stream->skip( count );
    }

    void WPOgreDataStream::seek( size_t pos )
    {
        WP_ASSERT( m_stream );
        m_stream->seek( pos );
    }

    auto WPOgreDataStream::tell() const -> size_t
    {
        WP_ASSERT( m_stream );
        return m_stream->tell();
    }

    auto WPOgreDataStream::eof() const -> bool
    {
        WP_ASSERT( m_stream );
        if( mSize > 0 )
        {
            return m_stream->eof();
        }

        return false;
    }

    void WPOgreDataStream::close()
    {
        WP_ASSERT( m_stream );
        m_stream->close();
    }

    auto WPOgreDataStream::getStream() const -> SmartPtr<IStream>
    {
        return m_stream;
    }

    void WPOgreDataStream::setStream( SmartPtr<IStream> stream )
    {
        m_stream = stream;
    }

}  // namespace workphone
