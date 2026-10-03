#include <WPTheoraVideo/WPTheoraDataSource.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/System/ApplicationManager.hpp>

namespace workphone
{
    WPTheoraDataSource::WPTheoraDataSource( const String &filename )
    {
        setFileName( filename );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto fileSystem = applicationManager->getFileSystem() )
        {
            m_stream = fileSystem->open( filename );
        }
    }

    WPTheoraDataSource::~WPTheoraDataSource() = default;

    int WPTheoraDataSource::read( void *output, int nBytes )
    {
        return m_stream ? static_cast<int>( m_stream->read( output, nBytes ) ) : 0;
    }

    void WPTheoraDataSource::seek( unsigned long byteIndex )
    {
        if( m_stream )
        {
            m_stream->seek( byteIndex );
        }
    }

    std::string WPTheoraDataSource::repr()
    {
        return m_fileName.c_str();
    }

    unsigned long WPTheoraDataSource::size()
    {
        return m_stream ? static_cast<unsigned long>( m_stream->size() ) : 0;
    }

    unsigned long WPTheoraDataSource::tell()
    {
        return m_stream ? static_cast<unsigned long>( m_stream->tell() ) : 0;
    }

    String WPTheoraDataSource::getFileName() const
    {
        return m_fileName;
    }

    void WPTheoraDataSource::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }

    SmartPtr<IStream> WPTheoraDataSource::getStream() const
    {
        return m_stream;
    }

    void WPTheoraDataSource::setStream( SmartPtr<IStream> stream )
    {
        m_stream = stream;
    }
}  // namespace workphone
