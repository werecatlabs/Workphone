#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Core/WPOgreDataStream.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{

    WPOgreDataStream::WPOgreDataStream() = default;

    WPOgreDataStream::WPOgreDataStream( SmartPtr<IStream> stream ) : DataStream(), m_stream( stream )
    {
        mSize = m_stream->size();
    }

    WPOgreDataStream::~WPOgreDataStream()
    {
        m_stream = nullptr;
    }

    size_t WPOgreDataStream::read( void *buf, size_t count )
    {
        WP_ASSERT( m_stream );
        if( m_stream->isValid() )
        {
            return m_stream->read( buf, count );
        }

        return 0;
    }

    Ogre::String WPOgreDataStream::getLine( bool trimAfter )
    {
        if( m_stream )
        {
            auto str = m_stream->getLine( trimAfter );
            return str.c_str();
        }

        return {};
    }

    void WPOgreDataStream::skip( long count )
    {
        WP_ASSERT( m_stream );
        m_stream->skip( count );
    }

    void WPOgreDataStream::seek( size_t pos )
    {
        WP_ASSERT( m_stream );
        m_stream->seek( (long)pos );
    }

    size_t WPOgreDataStream::tell( void ) const
    {
        WP_ASSERT( m_stream );
        return m_stream->tell();
    }

    bool WPOgreDataStream::eof( void ) const
    {
        WP_ASSERT( m_stream );
        if( m_stream->isValid() )
        {
            return m_stream->eof();
        }

        return true;
    }

    void WPOgreDataStream::close( void )
    {
        WP_ASSERT( m_stream );
        m_stream->close();
    }

    SmartPtr<IStream> WPOgreDataStream::getStream() const
    {
        return m_stream;
    }

    void WPOgreDataStream::setStream( SmartPtr<IStream> stream )
    {
        m_stream = stream;
    }
}  // namespace workphone
