#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, FileDataStream, DataStream );

    FileDataStream::FileDataStream() : DataStream()
    {
    }

    FileDataStream::FileDataStream( std::ifstream *s, bool freeOnClose ) :
        DataStream(),
        m_inStream( s ),
        m_fstreamRO( s ),

        m_freeOnClose( freeOnClose )
    {
        if( s && s->is_open() )
        {
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

    FileDataStream::FileDataStream( const String &name, std::ifstream *s, bool freeOnClose ) :
        DataStream( name ),
        m_inStream( s ),
        m_fstreamRO( s ),

        m_freeOnClose( freeOnClose )
    {
        if( s && s->is_open() )
        {
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

    FileDataStream::FileDataStream( const String &name, std::ifstream *s, size_Num inSize,
                                    bool freeOnClose ) :
        DataStream( name ),
        m_inStream( s ),
        m_fstreamRO( s ),

        m_freeOnClose( freeOnClose )
    {
        // Size is passed in
        m_size = inSize;

        WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );

        determineAccess();
    }

    FileDataStream::FileDataStream( std::fstream *s, bool freeOnClose ) :
        DataStream( false ),
        m_inStream( s ),

        m_fstream( s ),
        m_freeOnClose( freeOnClose )
    {
        if( s && s->is_open() )
        {
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

    FileDataStream::FileDataStream( const String &name, std::fstream *s, bool freeOnClose ) :
        DataStream( name, false ),
        m_inStream( s ),

        m_fstream( s ),
        m_freeOnClose( freeOnClose )
    {
        if( s && s->is_open() )
        {
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

    FileDataStream::FileDataStream( const String &name, std::fstream *s, size_t inSize,
                                    bool freeOnClose ) :
        DataStream( name, false ),
        m_inStream( s ),

        m_fstream( s ),
        m_freeOnClose( freeOnClose )
    {
        // writeable!
        // Size is passed in
        m_size = inSize;

        WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );

        determineAccess();
    }

    void FileDataStream::determineAccess()
    {
        m_access = 0;

        if( m_inStream )
        {
            m_access |= static_cast<u16>( AccessMode::Read );
        }

        if( m_fstream )
        {
            m_access |= static_cast<u16>( AccessMode::Write );
        }
    }

    FileDataStream::~FileDataStream()
    {
        close();
    }

    auto FileDataStream::read( void *buf, size_t count ) -> size_t
    {
        m_inStream->read( static_cast<char *>( buf ), static_cast<std::streamsize>( count ) );
        return static_cast<size_t>( m_inStream->gcount() );
    }

    auto FileDataStream::write( const void *buf, size_t count ) -> size_t
    {
        size_t written = 0;
        if( isWriteable() && m_fstream )
        {
            m_fstream->write( static_cast<const char *>( buf ), static_cast<std::streamsize>( count ) );
            written = count;
        }
        return written;
    }

    String FileDataStream::getLine( bool trimAfter )
    {
        std::string line;
        line.reserve( 512 );

        if( m_fstream )
        {
            if( m_fstream->is_open() )
            {
                std::getline( *m_fstream, line );
            }
        }
        else if( m_fstreamRO )
        {
            if( m_fstreamRO->is_open() )
            {
                std::getline( *m_fstreamRO, line );
            }
        }

        if( trimAfter )
        {
            return StringUtil::trim( String( line.c_str(), line.length() ) );
        }

        return String( line.c_str(), line.length() );
    }

    auto FileDataStream::readLine( c8 *buf, size_Num maxCount, const String &delim ) -> size_t
    {
        if( delim.empty() )
        {
            throw Exception( "No delimiter provided FileStreamDataStream::readLine" );
        }

        if( delim.size() > 1 )
        {
            WP_LOG_ERROR( "WARNING: FileStreamDataStream::readLine - using only first delimeter" );
        }

        // Deal with both Unix & Windows LFs
        bool trimCR = false;
        if( delim.at( 0 ) == '\n' )
        {
            trimCR = true;
        }
        // maxCount + 1 since count excludes terminator in getline
        m_inStream->getline( buf, static_cast<std::streamsize>( maxCount + 1 ), delim.at( 0 ) );
        auto ret = static_cast<size_t>( m_inStream->gcount() );
        // three options
        // 1) we had an eof before we read a whole line
        // 2) we ran out of buffer space
        // 3) we read a whole line - in this case the delim character is taken from the stream but not
        // written in the buffer so the read data is of length ret-1 and thus ends at index ret-2 in all
        // cases the buffer will be null terminated for us

        if( m_inStream->eof() )
        {
            // no problem
        }
        else if( m_inStream->fail() )
        {
            // Did we fail because of maxCount hit? No - no terminating character
            // in included in the count in this case
            if( ret == maxCount )
            {
                // clear failbit for next time
                m_inStream->clear();
            }
            else
            {
                throw Exception( "Streaming error occurred FileStreamDataStream::readLine" );
            }
        }
        else
        {
            // we need to adjust ret because we want to use it as a
            // pointer to the terminating null character and it is
            // currently the length of the data read from the stream
            // i.e. 1 more than the length of the data in the buffer and
            // hence 1 more than the _index_ of the NULL character
            --ret;
        }

        // trim off CR if we found CR/LF
        if( trimCR && buf[ret - 1] == '\r' )
        {
            --ret;
            buf[ret] = '\0';
        }
        return ret;
    }

    void FileDataStream::skip( size_Num count )
    {
#if defined( STLPORT )
        // Workaround for STLport issues: After reached eof of file stream,
        // it's seems the stream was putted in intermediate state, and will be
        // fail if try to repositioning relative to current position.
        // Note: tellg() fail in this case too.
        if( m_inStream->eof() )
        {
            m_inStream->clear();
            // Use seek relative to either begin or end to bring the stream
            // back to normal state.
            m_inStream->seekg( 0, std::ios::end );
        }
#endif

        m_inStream->clear();  // Clear fail status in case eof was set
        m_inStream->seekg( count, std::ios::cur );
    }

    auto FileDataStream::seek( size_Num pos ) -> bool
    {
        m_inStream->clear();  // Clear fail status in case eof was set
        m_inStream->seekg( pos, std::ios::beg );
        return true;
    }

    auto FileDataStream::tell() const -> size_Num
    {
        m_inStream->clear();  // Clear fail status in case eof was set
        return m_inStream->tellg();
    }

    auto FileDataStream::eof() const -> bool
    {
        return m_inStream->eof();
    }

    void FileDataStream::close()
    {
        if( m_inStream )
        {
            // Unfortunately, there is no file-specific shared class hierarchy between fstream and
            // ifstream (!!)
            if( m_fstreamRO )
            {
                m_fstreamRO->close();
            }
            if( m_fstream )
            {
                m_fstream->flush();
                m_fstream->close();
            }

            if( m_freeOnClose )
            {
                // delete the stream too
                delete m_fstreamRO;
                delete m_fstream;

                m_inStream = nullptr;
                m_fstreamRO = nullptr;
                m_fstream = nullptr;
            }
        }
    }

    auto FileDataStream::isOpen() const -> bool
    {
        if( m_inStream )
        {
            auto s = static_cast<std::ifstream *>( m_inStream );
            return s->is_open();
        }

        if( m_fstreamRO )
        {
            return m_fstreamRO->is_open();
        }

        if( m_fstream )
        {
            return m_fstream->is_open();
        }

        return false;
    }

    auto FileDataStream::isValid() const -> bool
    {
        return isOpen() && ( size() > 0 ) && ( size() < static_cast<size_t>( 2e+9 ) );
    }

    auto FileDataStream::getInStream() const -> std::istream *
    {
        return m_inStream;
    }

    void FileDataStream::setInStream( std::istream *stream )
    {
        m_inStream = stream;
        m_fstreamRO = (std::ifstream *)stream;

        if( m_fstreamRO->is_open() )
        {
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

    auto FileDataStream::getFStreamRO() const -> std::ifstream *
    {
        return m_fstreamRO;
    }

    void FileDataStream::setFStreamRO( std::ifstream *stream )
    {
        m_inStream = stream;
        m_fstreamRO = stream;

        if( stream->is_open() )
        {
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

    auto FileDataStream::getFStream() const -> std::fstream *
    {
        return m_fstream;
    }

    void FileDataStream::setFStream( std::fstream *stream )
    {
        m_inStream = stream;
        m_fstream = stream;

        if( stream->is_open() )
        {
            // calculate the size
            m_inStream->seekg( 0, std::ios_base::end );
            m_size = static_cast<size_t>( m_inStream->tellg() );
            m_inStream->seekg( 0, std::ios_base::beg );

            WP_ASSERT( size() < static_cast<size_t>( 2e+9 ) );
        }

        determineAccess();
    }

}  // namespace workphone
