#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/ObfuscatedZipFile.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <zzip/zzip.h>
#include <zzip/plugin.h>
#include <limits>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ObfuscatedZipFile, DataStream );

    ObfuscatedZipFile::ObfuscatedZipFile() = default;

    ObfuscatedZipFile::ObfuscatedZipFile( ObfuscatedZipArchive *archive, const String &fileName,
                                          ZZIP_FILE *zzipFile, u32 uncompressedSize ) :
        m_zipFile( zzipFile ),
        m_archive( archive )
    {
        setFileName( fileName );

        m_size = uncompressedSize;
    }

    ObfuscatedZipFile::~ObfuscatedZipFile()
    {
        unload( nullptr );
    }

    void ObfuscatedZipFile::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setLoadingState( seek( 0 ) ? LoadingState::Loaded : LoadingState::Unloaded );
    }

    void ObfuscatedZipFile::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        close();
        setLoadingState( LoadingState::Unloaded );
    }

    auto ObfuscatedZipFile::eof() const -> bool
    {
        if( !m_archive )
        {
            return true;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        return !m_zipFile || static_cast<size_Num>( zzip_tell( m_zipFile ) ) >= m_size;
    }

    auto ObfuscatedZipFile::read( void *buffer, size_t sizeToRead ) -> size_t
    {
        if( !m_archive )
        {
            return 0;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        if( !m_zipFile || sizeToRead == 0 )
        {
            return 0;
        }
        zzip_ssize_t r = zzip_file_read( m_zipFile, buffer, sizeToRead );
        if( r < 0 )
        {
            ZZIP_DIR *dir = zzip_dirhandle( m_zipFile );

            auto fileName = getFileName();
            auto msg = zzip_strerror_of( dir );
            auto exceptionMsg = fileName + String( " - error from zziplib: " ) + msg;
            WP_EXCEPTION( exceptionMsg.c_str() );
        }

        return static_cast<size_t>( r );
    }

    auto ObfuscatedZipFile::write( [[maybe_unused]] const void *buffer,
                                   [[maybe_unused]] size_t sizeToWrite ) -> size_t
    {
        return 0;
    }

    auto ObfuscatedZipFile::seek( size_Num finalPos ) -> bool
    {
        if( !m_archive || finalPos > m_size )
        {
            return false;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        return m_zipFile && zzip_seek( m_zipFile, static_cast<zzip_off_t>( finalPos ), SEEK_SET ) >= 0;
    }

    void ObfuscatedZipFile::skip( size_Num count )
    {
        if( !m_archive )
        {
            return;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        if( !m_zipFile ) return;
        const auto current = zzip_tell( m_zipFile );
        if( current < 0 || static_cast<size_Num>( current ) > m_size ) return;
        const auto position = static_cast<size_Num>( current );
        size_Num target;
        // IStream's unsigned signature also carries negative relative offsets.
        // Text line reads and mesh chunk lookahead both rewind through skip().
        if( count > std::numeric_limits<size_Num>::max() / 2 )
        {
            const auto distance = size_Num( 0 ) - count;
            if( distance > position ) return;
            target = position - distance;
        }
        else
        {
            if( count > m_size - position ) return;
            target = position + count;
        }
        zzip_seek( m_zipFile, static_cast<zzip_off_t>( target ), SEEK_SET );
    }

    auto ObfuscatedZipFile::size() const -> size_Num
    {
        return m_size;
    }

    auto ObfuscatedZipFile::tell() const -> size_Num
    {
        if( !m_archive )
        {
            return 0;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        return m_zipFile ? static_cast<size_Num>( zzip_tell( m_zipFile ) ) : 0;
    }

    auto ObfuscatedZipFile::isOpen() const -> bool
    {
        if( !m_archive )
        {
            return false;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        return m_zipFile != nullptr;
    }

    void ObfuscatedZipFile::close()
    {
        if( !m_archive )
        {
            return;
        }
        RecursiveMutex::ScopedLock lock( m_archive->m_mutex );
        if( m_zipFile != nullptr )
        {
            zzip_file_close( m_zipFile );
            m_zipFile = nullptr;
        }
    }

    void ObfuscatedZipFile::setFreeMemory( [[maybe_unused]] bool freeMemory )
    {
    }

    auto ObfuscatedZipFile::isReadable() const -> bool
    {
        return true;
    }

    auto ObfuscatedZipFile::isWriteable() const -> bool
    {
        return false;
    }

    auto ObfuscatedZipFile::isValid() const -> bool
    {
        return isOpen();
    }

}  // namespace workphone
