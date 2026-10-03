#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/ZipFile.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <zzip/zzip.h>
#include <zzip/plugin.h>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ZipFile, DataStream );

    ZipFile::ZipFile() = default;

    ZipFile::ZipFile( ZipArchive *archive, const String &fileName, ZZIP_FILE *zzipFile,
                      u32 uncompressedSize ) :
        m_zipFile( zzipFile ),
        m_archive( archive )
    {
        setFileName( fileName );

        m_size = uncompressedSize;
        zzip_seek( m_zipFile, 0, SEEK_SET );
    }

    ZipFile::~ZipFile()
    {
        unload( nullptr );
    }

    void ZipFile::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        zzip_seek( m_zipFile, 0, SEEK_SET );
        setLoadingState( LoadingState::Loaded );
    }

    void ZipFile::unload( [[maybe_unused]] SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        close();
        setLoadingState( LoadingState::Unloaded );
    }

    auto ZipFile::eof() const -> bool
    {
        auto pos = tell();
        return pos >= m_size;
    }

    auto ZipFile::read( void *buffer, size_t sizeToRead ) -> size_t
    {
        // RecursiveMutex::ScopedLock lock(reader->Mutex);
        zzip_ssize_t r = zzip_file_read( m_zipFile, buffer, sizeToRead );
        if( r < 0 )
        {
            ZZIP_DIR *dir = zzip_dirhandle( m_zipFile );

            auto fileName = getFileName();
            auto msg = zzip_strerror_of( dir );
            auto exceptionMsg = fileName + String( " - error from zziplib: " ) + msg +
                                String( "ObfuscatedZipDataStream::read" );
            WP_EXCEPTION( exceptionMsg.c_str() );
        }

        return r;
    }

    auto ZipFile::write( [[maybe_unused]] const void *buffer, [[maybe_unused]] size_t sizeToWrite )
        -> size_t
    {
        return 0;
    }

    auto ZipFile::seek( size_t pos ) -> bool
    {
        zzip_seek( m_zipFile, static_cast<zzip_off_t>( pos ), SEEK_SET );
        return true;
    }

    void ZipFile::skip( size_Num count )
    {
        if( count > 0 )
        {
            zzip_seek( m_zipFile, (zzip_off_t)count, SEEK_CUR );
        }
        else if( count < 0 )
        {
            zzip_seek( m_zipFile, (zzip_off_t)count, SEEK_CUR );
        }
    }

    auto ZipFile::size() const -> size_t
    {
        return m_size;
    }

    auto ZipFile::tell() const -> size_t
    {
        auto p = zzip_tell( m_zipFile );
        return static_cast<size_t>( p );
    }

    auto ZipFile::isOpen() const -> bool
    {
        return m_zipFile != nullptr;
    }

    void ZipFile::close()
    {
        if( m_zipFile != nullptr )
        {
            zzip_file_close( m_zipFile );
            m_zipFile = nullptr;
        }
    }

    void ZipFile::setFreeMemory( [[maybe_unused]] bool freeMemory )
    {
    }

    auto ZipFile::getZipFile() const -> ZZIP_FILE *
    {
        return m_zipFile;
    }

    void ZipFile::setZipFile( ZZIP_FILE *zipFile )
    {
        m_zipFile = zipFile;
    }

    auto ZipFile::getArchive() const -> ZipArchive *
    {
        return m_archive;
    }

    void ZipFile::setArchive( ZipArchive *archive )
    {
        m_archive = archive;
    }

    void ZipFile::setSize( size_t iSize )
    {
        m_size = iSize;
    }

    auto ZipFile::isReadable() const -> bool
    {
        return true;
    }

    auto ZipFile::isWriteable() const -> bool
    {
        return false;
    }

    auto ZipFile::isValid() const -> bool
    {
        return isOpen() && ( size() > 0 ) && ( size() < static_cast<size_t>( 2e+9 ) );
    }

}  // namespace workphone
