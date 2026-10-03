#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/DataStream.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, DataStream, IStream );

    DataStream::DataStream( const String &name, u16 accessMode /*= READ*/ ) : m_access( accessMode )
    {
        m_fileInfo.fileName = Path::getFileName( name );
    }

    DataStream::DataStream( u16 accessMode /*= READ*/ ) : m_access( accessMode )
    {
    }

    DataStream::~DataStream() = default;

    template <typename T>
    auto DataStream::operator>>( T &value ) -> DataStream &
    {
        read( static_cast<void *>( &value ), sizeof( T ) );
        return *this;
    }

    auto DataStream::getLine( bool trimAfter ) -> String
    {
        const size_Num bufferSize = 128;
        char tmpBuf[bufferSize];

        String retString;
        retString.reserve( bufferSize );

        while( true )
        {
            auto readCount = read( tmpBuf, bufferSize - 1 );
            if( readCount == 0 )
                break;

            // Search for newline character
            auto p = static_cast<char *>( memchr( tmpBuf, '\n', readCount ) );
            if( p != nullptr )
            {
                // Found newline - append up to (not including) the newline
                auto lineLength = p - tmpBuf;
                retString.append( tmpBuf, lineLength );

                // Skip past the newline in the stream
                skip( static_cast<long>( lineLength + 1 - readCount ) );
                break;
            }

            // No newline found - append entire buffer
            retString.append( tmpBuf, readCount );
        }

        // Trim trailing '\r' if exists
        if( !retString.empty() && retString.back() == '\r' )
            retString.pop_back();

        // Trim whitespace if requested
        if( trimAfter )
            StringUtil::trim( retString.c_str() );

        return retString.c_str();
    }

    auto DataStream::isOpen() const -> bool
    {
        return true;
    }

    auto DataStream::getFileName() const -> String
    {
        return m_fileInfo.fileName;
    }

    auto DataStream::getFileName() -> String
    {
        return m_fileInfo.fileName;
    }

    void DataStream::setFileName( const String &fileName )
    {
        m_fileInfo.fileName = fileName;
    }

    auto DataStream::getAccessMode() const -> u16
    {
        return m_access;
    }

    auto DataStream::isReadable() const -> bool
    {
        return ( m_access & static_cast<s32>( AccessMode::Read ) ) != 0;
    }

    auto DataStream::isWriteable() const -> bool
    {
        return ( m_access & static_cast<s32>( AccessMode::Write ) ) != 0;
    }

    auto DataStream::write( const void *buf, size_Num count ) -> size_Num
    {
        (void)buf;
        (void)count;
        // default to not supported
        return 0;
    }

    auto DataStream::readLine( char *buf, size_Num maxCount, const String &delim ) -> size_Num
    {
        const bool trimCR = ( delim.find( '\n' ) != String::npos );

        char tmpBuf[WP_STREAM_BUFFER_SIZE];
        size_t chunkSize = std::min( maxCount, static_cast<size_t>( WP_STREAM_BUFFER_SIZE ) - 1 );
        size_t totalCount = 0;
        size_t readCount;

        while( chunkSize && ( readCount = read( tmpBuf, chunkSize ) ) != 0 )
        {
            tmpBuf[readCount] = '\0';

            size_t pos = strcspn( tmpBuf, delim.c_str() );

            if( buf )
            {
                memcpy( buf + totalCount, tmpBuf, pos );
            }
            totalCount += pos;

            if( pos < readCount )
            {
                skip( static_cast<long>( pos + 1 - readCount ) );

                if( trimCR && totalCount && buf && buf[totalCount - 1] == '\r' )
                {
                    --totalCount;
                }

                break;
            }

            chunkSize =
                std::min( maxCount - totalCount, static_cast<size_t>( WP_STREAM_BUFFER_SIZE ) - 1 );
        }

        if( buf )
        {
            buf[totalCount] = '\0';
        }

        return totalCount;
    }

    auto DataStream::skipLine( const String &delim ) -> size_Num
    {
        char tmpBuf[WP_STREAM_BUFFER_SIZE];
        size_t total = 0;
        size_t readCount;

        // Keep looping while not hitting delimiter
        while( ( readCount = read( tmpBuf, WP_STREAM_BUFFER_SIZE - 1 ) ) != 0 )
        {
            // Terminate string
            tmpBuf[readCount] = '\0';

            // Find first delimiter
            auto pos = strcspn( tmpBuf, delim.c_str() );

            if( pos < readCount )
            {
                // Found terminator, reposition backwards
                skip( static_cast<long>( pos + 1 - readCount ) );

                total += pos + 1;

                // break out
                break;
            }

            total += readCount;
        }

        return total;
    }

    auto DataStream::size() const -> size_Num
    {
        return m_size;
    }

    auto DataStream::getAsString() -> String
    {
        try
        {
            seek( 0 );

            const auto streamSize = size();
            if( streamSize > 0 )
            {
                String result;
                result.reserve( streamSize );

                const auto bufSize = size_Num( WP_STREAM_BUFFER_SIZE );
                char pBuf[bufSize];

                size_t nr;
                while( ( nr = read( pBuf, bufSize ) ) > 0 )
                {
                    result.append( pBuf, nr );
                }

                return result;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void DataStream::setFreeMemory( [[maybe_unused]] bool freeMemory )
    {
    }

    auto DataStream::getFileInfo() const -> FileInfo
    {
        return m_fileInfo;
    }

    void DataStream::setFileInfo( const FileInfo &fileInfo )
    {
        m_fileInfo = fileInfo;
    }

}  // namespace workphone
