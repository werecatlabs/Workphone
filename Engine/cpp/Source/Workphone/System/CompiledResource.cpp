#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/CompiledResource.hpp>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <limits>
#include <thread>
#include <type_traits>

#if defined WP_PLATFORM_WIN32
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <Windows.h>
#else
#    include <unistd.h>
#endif

namespace workphone::resource
{
    namespace
    {
        constexpr char magic[4] = { 'W', 'P', 'R', 'S' };
        constexpr u32 maxResourceIdBytes = 1024 * 1024;
        constexpr u32 maxInstallDependencies = 4096;
        std::atomic<u64> temporaryFileCounter{ 0 };

        template <typename T>
        bool writeLittleEndian( std::ostream &stream, T value )
        {
            static_assert( std::is_unsigned<T>::value, "Unsigned integer required" );
            for( size_t i = 0; i < sizeof( T ); ++i )
            {
                const char byte = static_cast<char>( value & 0xffu );
                stream.write( &byte, 1 );
                value >>= 8u;
            }
            return stream.good();
        }

        template <typename T>
        bool readLittleEndian( std::istream &stream, T &value )
        {
            static_assert( std::is_unsigned<T>::value, "Unsigned integer required" );
            value = 0;
            for( size_t i = 0; i < sizeof( T ); ++i )
            {
                unsigned char byte = 0;
                stream.read( reinterpret_cast<char *>( &byte ), 1 );
                if( !stream )
                    return false;
                value |= static_cast<T>( byte ) << ( i * 8u );
            }
            return true;
        }

        bool writeString( std::ostream &stream, const String &value )
        {
            if( value.size() > maxResourceIdBytes )
                return false;
            return writeLittleEndian<u32>( stream, static_cast<u32>( value.size() ) ) &&
                   ( stream.write( value.data(), static_cast<std::streamsize>( value.size() ) ),
                     stream.good() );
        }

        bool readString( std::istream &stream, String &value )
        {
            u32 size = 0;
            if( !readLittleEndian( stream, size ) || size == 0 || size > maxResourceIdBytes )
                return false;
            std::string buffer( size, '\0' );
            stream.read( &buffer[0], static_cast<std::streamsize>( size ) );
            if( !stream )
                return false;
            value = buffer.c_str();
            if( value.size() != buffer.size() )
                return false;  // Embedded NULs are never valid in an identifier.
            return true;
        }

        bool writeHeader( std::ostream &stream, const CompiledResourceHeader &header )
        {
            stream.write( magic, sizeof( magic ) );
            if( !writeLittleEndian<u16>( stream, CompiledResourceHeader::currentFormatVersion ) ||
                !writeLittleEndian<u16>( stream, 0 ) ||
                !writeLittleEndian<u64>( stream, header.compilerVersion ) ||
                !writeLittleEndian<u64>( stream, header.sourceHash ) ||
                !writeLittleEndian<u64>( stream, header.payloadHash ) ||
                !writeLittleEndian<u64>( stream, header.payloadSize ) ||
                !writeLittleEndian<u64>( stream, header.resourceType.value() ) ||
                !writeLittleEndian<u32>( stream,
                                         static_cast<u32>( header.installDependencies.size() ) ) ||
                !writeString( stream, header.resourceId.str() ) )
            {
                return false;
            }
            for( const auto &dependency : header.installDependencies )
            {
                if( !writeString( stream, dependency.str() ) )
                    return false;
            }
            return stream.good();
        }

        bool readHeaderFromStream( std::istream &stream, CompiledResourceHeader &header,
                                   u64 &payloadOffset, String &error )
        {
            header = CompiledResourceHeader();
            char readMagic[4]{};
            stream.read( readMagic, sizeof( readMagic ) );
            if( !stream || !std::equal( std::begin( magic ), std::end( magic ), readMagic ) )
            {
                error = "Invalid compiled resource magic";
                return false;
            }

            u16 formatVersion = 0;
            u16 flags = 0;
            u64 encodedType = 0;
            u32 dependencyCount = 0;
            if( !readLittleEndian( stream, formatVersion ) || !readLittleEndian( stream, flags ) ||
                !readLittleEndian( stream, header.compilerVersion ) ||
                !readLittleEndian( stream, header.sourceHash ) ||
                !readLittleEndian( stream, header.payloadHash ) ||
                !readLittleEndian( stream, header.payloadSize ) ||
                !readLittleEndian( stream, encodedType ) ||
                !readLittleEndian( stream, dependencyCount ) )
            {
                error = "Compiled resource header is truncated";
                return false;
            }
            if( formatVersion != CompiledResourceHeader::currentFormatVersion || flags != 0 )
            {
                error = "Unsupported compiled resource container version or flags";
                return false;
            }
            if( dependencyCount > maxInstallDependencies )
            {
                error = "Compiled resource has too many install dependencies";
                return false;
            }

            String resourceId;
            if( !readString( stream, resourceId ) )
            {
                error = "Compiled resource contains an invalid resource ID";
                return false;
            }
            if( !header.resourceId.set( resourceId, &error ) )
                return false;
            header.resourceType = header.resourceId.type();
            if( encodedType != header.resourceType.value() )
            {
                error = "Compiled resource type does not match its resource ID";
                return false;
            }

            header.installDependencies.reserve( dependencyCount );
            for( u32 i = 0; i < dependencyCount; ++i )
            {
                String dependencyId;
                if( !readString( stream, dependencyId ) )
                {
                    error = "Compiled resource contains an invalid install dependency";
                    return false;
                }
                ResourceID dependency( dependencyId );
                if( !dependency.isValid() )
                {
                    error = "Compiled resource contains an invalid install dependency ID";
                    return false;
                }
                header.installDependencies.push_back( dependency );
            }

            const auto offset = stream.tellg();
            if( offset < 0 || !header.isValid() )
            {
                error = "Compiled resource header is invalid";
                return false;
            }
            payloadOffset = static_cast<u64>( offset );
            return true;
        }

        String makeTemporaryPath( const String &outputPath )
        {
            const u64 counter = ++temporaryFileCounter;
            const auto threadHash = std::hash<std::thread::id>{}( std::this_thread::get_id() );
            const auto processId =
#if defined WP_PLATFORM_WIN32
                static_cast<u64>( GetCurrentProcessId() );
#else
                static_cast<u64>( getpid() );
#endif
            return outputPath + ".tmp." + std::to_string( processId ).c_str() + "." +
                   std::to_string( threadHash ).c_str() + "." + std::to_string( counter ).c_str();
        }

        bool replaceFile( const std::filesystem::path &temporary,
                          const std::filesystem::path &destination, String &error )
        {
#if defined WP_PLATFORM_WIN32
            if( MoveFileExW( temporary.c_str(), destination.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) == 0 )
            {
                error = String( "Failed to atomically replace compiled resource (Win32 error " ) +
                        std::to_string( GetLastError() ).c_str() + ")";
                return false;
            }
            return true;
#else
            if( std::rename( temporary.c_str(), destination.c_str() ) != 0 )
            {
                error = String( "Failed to atomically replace compiled resource: " ) +
                        std::strerror( errno );
                return false;
            }
            return true;
#endif
        }
    }  // namespace

    bool CompiledResourceHeader::isValid() const
    {
        if( !resourceId.isValid() || resourceType != resourceId.type() || compilerVersion == 0 ||
            sourceHash == 0 || payloadHash == 0 || installDependencies.size() > maxInstallDependencies )
        {
            return false;
        }
        for( const auto &dependency : installDependencies )
        {
            if( !dependency.isValid() )
                return false;
        }
        return true;
    }

    bool CompiledResourceIO::hashFile( const String &path, u64 &hash, u64 &size, String &error )
    {
        hash = 14695981039346656037ull;
        size = 0;
        std::ifstream stream( path.c_str(), std::ios::binary );
        if( !stream )
        {
            error = String( "Failed to open file for hashing: " ) + path;
            return false;
        }

        char buffer[64 * 1024];
        while( stream )
        {
            stream.read( buffer, sizeof( buffer ) );
            const auto count = stream.gcount();
            if( count > 0 )
            {
                hash = hashBytes( buffer, static_cast<size_t>( count ), hash );
                size += static_cast<u64>( count );
            }
        }
        if( !stream.eof() )
        {
            error = String( "Failed while hashing file: " ) + path;
            return false;
        }
        return true;
    }

    bool CompiledResourceIO::writeAtomic( const String &outputPath, const CompiledResourceHeader &header,
                                          const String &payloadPath, String &error )
    {
        if( !header.isValid() || outputPath.empty() || payloadPath.empty() )
        {
            error = "Cannot write an invalid compiled resource";
            return false;
        }

        u64 payloadHash = 0;
        u64 payloadSize = 0;
        if( !hashFile( payloadPath, payloadHash, payloadSize, error ) ||
            payloadHash != header.payloadHash || payloadSize != header.payloadSize )
        {
            if( error.empty() )
                error = "Compiled payload changed before it could be packaged";
            return false;
        }

        const std::filesystem::path destination( outputPath.c_str() );
        std::error_code filesystemError;
        if( destination.has_parent_path() )
        {
            std::filesystem::create_directories( destination.parent_path(), filesystemError );
            if( filesystemError )
            {
                error = String( "Failed to create compiled resource directory: " ) +
                        filesystemError.message().c_str();
                return false;
            }
        }

        const String temporaryPathString = makeTemporaryPath( outputPath );
        const std::filesystem::path temporaryPath( temporaryPathString.c_str() );
        struct TemporaryFileGuard
        {
            std::filesystem::path path;
            ~TemporaryFileGuard()
            {
                std::error_code ignored;
                std::filesystem::remove( path, ignored );
            }
        } guard{ temporaryPath };

        std::ofstream output( temporaryPath, std::ios::binary | std::ios::trunc );
        std::ifstream payload( payloadPath.c_str(), std::ios::binary );
        if( !output || !payload || !writeHeader( output, header ) )
        {
            error = "Failed to create the compiled resource container";
            return false;
        }

        output << payload.rdbuf();
        output.flush();
        if( !output || payload.bad() )
        {
            error = "Failed while writing the compiled resource payload";
            return false;
        }
        output.close();
        payload.close();

        if( !replaceFile( temporaryPath, destination, error ) )
            return false;
        guard.path.clear();
        return true;
    }

    bool CompiledResourceIO::readHeader( const String &path, CompiledResourceHeader &header,
                                         String &error )
    {
        std::ifstream stream( path.c_str(), std::ios::binary );
        if( !stream )
        {
            error = String( "Failed to open compiled resource: " ) + path;
            return false;
        }
        u64 payloadOffset = 0;
        return readHeaderFromStream( stream, header, payloadOffset, error );
    }

    bool CompiledResourceIO::validate( const String &path, CompiledResourceHeader &header,
                                       String &error )
    {
        std::ifstream stream( path.c_str(), std::ios::binary );
        if( !stream )
        {
            error = String( "Failed to open compiled resource: " ) + path;
            return false;
        }

        u64 payloadOffset = 0;
        if( !readHeaderFromStream( stream, header, payloadOffset, error ) )
            return false;

        std::error_code filesystemError;
        const u64 fileSize = std::filesystem::file_size( path.c_str(), filesystemError );
        if( filesystemError || fileSize != payloadOffset + header.payloadSize )
        {
            error = "Compiled resource payload size does not match the file";
            return false;
        }

        u64 payloadHash = 14695981039346656037ull;
        u64 bytesRead = 0;
        char buffer[64 * 1024];
        while( bytesRead < header.payloadSize )
        {
            const u64 remaining = header.payloadSize - bytesRead;
            const auto requested = static_cast<std::streamsize>(
                std::min<u64>( remaining, static_cast<u64>( sizeof( buffer ) ) ) );
            stream.read( buffer, requested );
            const auto count = stream.gcount();
            if( count <= 0 )
            {
                error = "Compiled resource payload is truncated";
                return false;
            }
            payloadHash = hashBytes( buffer, static_cast<size_t>( count ), payloadHash );
            bytesRead += static_cast<u64>( count );
        }
        if( payloadHash != header.payloadHash )
        {
            error = "Compiled resource payload hash mismatch";
            return false;
        }
        return true;
    }

    bool CompiledResourceIO::read( const String &path, RuntimeResource &resource, String &error,
                                   u64 maxPayloadBytes )
    {
        resource = RuntimeResource();
        std::ifstream stream( path.c_str(), std::ios::binary );
        if( !stream )
        {
            error = String( "Failed to open compiled resource: " ) + path;
            return false;
        }

        u64 payloadOffset = 0;
        if( !readHeaderFromStream( stream, resource.header, payloadOffset, error ) )
            return false;
        if( resource.header.payloadSize > maxPayloadBytes ||
            resource.header.payloadSize > static_cast<u64>( std::numeric_limits<size_t>::max() ) )
        {
            error = "Compiled resource payload exceeds the configured limit";
            return false;
        }

        std::error_code filesystemError;
        const u64 fileSize = std::filesystem::file_size( path.c_str(), filesystemError );
        if( filesystemError || fileSize != payloadOffset + resource.header.payloadSize )
        {
            error = "Compiled resource payload size does not match the file";
            return false;
        }

        resource.payload.resize( static_cast<size_t>( resource.header.payloadSize ) );
        if( !resource.payload.empty() )
        {
            stream.read( reinterpret_cast<char *>( resource.payload.data() ),
                         static_cast<std::streamsize>( resource.payload.size() ) );
        }
        if( !stream )
        {
            error = "Compiled resource payload is truncated";
            resource = RuntimeResource();
            return false;
        }
        if( hashBytes( resource.payload.data(), resource.payload.size() ) !=
            resource.header.payloadHash )
        {
            error = "Compiled resource payload hash mismatch";
            resource = RuntimeResource();
            return false;
        }
        return true;
    }
}  // namespace workphone::resource
