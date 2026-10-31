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
#include <set>
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
        constexpr u64 maxEncodedHeaderBytes = 8ull * 1024 * 1024;
        std::atomic<u64> temporaryFileCounter{ 0 };

        // Keep caller paths and container identities unchanged. Only absolute
        // Windows filesystem operations need the extended spelling; relative
        // paths retain their current-directory semantics.
        std::filesystem::path fileSystemIOPath( const std::filesystem::path &path )
        {
#if defined WP_PLATFORM_WIN32
            if( path.is_absolute() )
            {
                auto native = path.native();
                std::replace( native.begin(), native.end(), L'/', L'\\' );
                if( native.rfind( L"\\\\?\\", 0 ) == 0 || native.rfind( L"\\\\.\\", 0 ) == 0 )
                {
                    return std::filesystem::path( native );
                }
                // Extended Win32 paths do not process dot segments. Normalize
                // those before adding the prefix to preserve ordinary path IO.
                native = path.lexically_normal().native();
                std::replace( native.begin(), native.end(), L'/', L'\\' );
                if( native.rfind( L"\\\\", 0 ) == 0 )
                {
                    return std::filesystem::path( L"\\\\?\\UNC\\" + native.substr( 2 ) );
                }
                return std::filesystem::path( L"\\\\?\\" + native );
            }
#endif
            return path;
        }

        bool validArtifactPath( const String &path )
        {
            ResourceID normalized;
            return !path.empty() && path.size() <= 4096 && path.find( ':' ) == String::npos &&
                   path.find( '\\' ) == String::npos && normalized.set( "data://" + path ) &&
                   normalized.sourceRelativePath() == path && path.find( "artifacts/v2/" ) == 0;
        }

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
                {
                    return false;
                }
                value |= static_cast<T>( byte ) << ( i * 8u );
            }
            return true;
        }

        bool validHeaderUTF8(const char *text,size_t size)
        {
            for(size_t at=0;at<size;) {
                const auto lead=static_cast<unsigned char>(text[at++]);
                if(lead<0x80) { if(!lead)return false; continue; }
                unsigned count=0,code=0,minimum=0;
                if(lead>=0xc2&&lead<=0xdf) {count=1;code=lead&31;minimum=0x80;}
                else if(lead>=0xe0&&lead<=0xef) {count=2;code=lead&15;minimum=0x800;}
                else if(lead>=0xf0&&lead<=0xf4) {count=3;code=lead&7;minimum=0x10000;}
                else return false;
                if(count>size-at)return false;
                for(unsigned i=0;i<count;++i) {
                    const auto next=static_cast<unsigned char>(text[at++]);
                    if((next&0xc0)!=0x80)return false;
                    code=(code<<6)|(next&63);
                }
                if(code<minimum||code>0x10ffff||(code>=0xd800&&code<=0xdfff))return false;
            }
            return true;
        }

        bool writeString( std::ostream &stream, const String &value )
        {
            if( value.empty() || value.size() > maxResourceIdBytes || !validHeaderUTF8(value.data(),value.size()) )
            {
                return false;
            }
            return writeLittleEndian<u32>( stream, static_cast<u32>( value.size() ) ) &&
                   ( stream.write( value.data(), static_cast<std::streamsize>( value.size() ) ),
                     stream.good() );
        }

        struct HeaderReadBudget
        {
            u64 encodedRemaining = maxEncodedHeaderBytes;
            u64 fileRemaining = 0;
            bool fits(u64 bytes) const { return bytes<=encodedRemaining && bytes<=fileRemaining; }
            bool consume(u64 bytes)
            {
                if(!fits(bytes)) return false;
                encodedRemaining-=bytes; fileRemaining-=bytes; return true;
            }
        };

        bool readString( std::istream &stream, String &value,HeaderReadBudget &budget,
                         u32 maximumBytes=maxResourceIdBytes )
        {
            u32 size = 0;
            if( !budget.consume(sizeof(u32)) || !readLittleEndian( stream, size ) || size == 0 ||
                size > maximumBytes || !budget.consume(size) )
            {
                return false;
            }
            std::string buffer( size, '\0' );
            stream.read( &buffer[0], static_cast<std::streamsize>( size ) );
            if( !stream )
            {
                return false;
            }
            if(!validHeaderUTF8(buffer.data(),buffer.size())) return false;
            value = buffer.c_str();
            if( value.size() != buffer.size() )
            {
                return false;  // Embedded NULs are never valid in an identifier.
            }
            return true;
        }

        bool writeHeader( std::ostream &stream, const CompiledResourceHeader &header )
        {
            // Keep the writer's admitted envelope consistent with the reader.
            u64 encodedBytes=4+2+2+5*8+4;
            auto add=[&](u64 count) {
                if(count>maxEncodedHeaderBytes-encodedBytes) return false;
                encodedBytes+=count; return true;
            };
            auto addString=[&](const String &value) { return add(sizeof(u32)) && add(value.size()); };
            if(!addString(header.resourceId.str())) return false;
            for(const auto &dependency:header.installDependencies) if(!addString(dependency.str())) return false;
            if(header.formatVersion>=2)
                for(const auto &artifact:header.installArtifacts)
                    if(!addString(artifact.path) || !add(2*sizeof(u64))) return false;
            stream.write( magic, sizeof( magic ) );
            if( !writeLittleEndian<u16>( stream, header.formatVersion ) ||
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
                {
                    return false;
                }
            }
            if( header.formatVersion >= 2 )
            {
                for( const auto &artifact : header.installArtifacts )
                {
                    if( !writeString( stream, artifact.path ) ||
                        !writeLittleEndian<u64>( stream, artifact.sourceHash ) ||
                        !writeLittleEndian<u64>( stream, artifact.payloadHash ) )
                    {
                        return false;
                    }
                }
            }
            return stream.good();
        }

        bool readHeaderFromStream( std::istream &stream, CompiledResourceHeader &header,
                                   u64 &payloadOffset, String &error )
        {
            header = CompiledResourceHeader();
            // Establish file and aggregate metadata bounds before any variable
            // allocation. A tiny payload limit alone cannot bound its header.
            const auto start=stream.tellg();
            if(start<0) { error="Cannot locate compiled resource header"; return false; }
            stream.seekg(0,std::ios::end); const auto end=stream.tellg();
            stream.seekg(start);
            HeaderReadBudget budget;
            constexpr u64 fixedHeaderBytes=4+2+2+5*8+4;
            if(!stream || end<start) { error="Cannot bound compiled resource header"; return false; }
            budget.fileRemaining=static_cast<u64>(end-start);
            if(!budget.consume(fixedHeaderBytes)) { error="Compiled resource fixed header is truncated"; return false; }
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
            if( formatVersion == 0 || formatVersion > CompiledResourceHeader::currentFormatVersion ||
                flags != 0 )
            {
                error = "Unsupported compiled resource container version or flags";
                return false;
            }
            header.formatVersion = formatVersion;
            if( dependencyCount > maxInstallDependencies )
            {
                error = "Compiled resource has too many install dependencies";
                return false;
            }

            String resourceId;
            if( !readString( stream, resourceId,budget ) )
            {
                error = "Compiled resource contains an invalid resource ID";
                return false;
            }
            if( !header.resourceId.set( resourceId, &error ) )
            {
                return false;
            }
            header.resourceType = header.resourceId.type();
            if( encodedType != header.resourceType.value() )
            {
                error = "Compiled resource type does not match its resource ID";
                return false;
            }

            // Each encoded ID needs at least its length word and one byte.
            // V2 additionally requires one artifact string and two u64 hashes.
            const u64 minimumDependencyBytes=formatVersion>=2 ? 26 : 5;
            if(!budget.fits(u64(dependencyCount)*minimumDependencyBytes))
            { error="Compiled resource dependencies exceed remaining header/file bounds"; return false; }
            header.installDependencies.reserve( dependencyCount );
            for( u32 i = 0; i < dependencyCount; ++i )
            {
                String dependencyId;
                if( !readString( stream, dependencyId,budget ) )
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

            if( formatVersion >= 2 )
            {
                if(!budget.fits(u64(dependencyCount)*21))
                { error="Compiled resource artifact pins exceed remaining header/file bounds"; return false; }
                header.installArtifacts.reserve( dependencyCount );
                for( u32 i = 0; i < dependencyCount; ++i )
                {
                    InstallDependencyArtifact artifact;
                    if( !readString( stream, artifact.path,budget,4096 ) || !validArtifactPath( artifact.path ) ||
                        !budget.consume(2*sizeof(u64)) ||
                        !readLittleEndian( stream, artifact.sourceHash ) ||
                        !readLittleEndian( stream, artifact.payloadHash ) )
                    {
                        error = "Compiled resource contains an invalid dependency artifact pin";
                        return false;
                    }
                    header.installArtifacts.push_back( std::move( artifact ) );
                }
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
            sourceHash == 0 || payloadHash == 0 || installDependencies.size() > maxInstallDependencies ||
            formatVersion == 0 || formatVersion > currentFormatVersion ||
            ( formatVersion == 1 && !installArtifacts.empty() ) ||
            ( formatVersion >= 2 && installArtifacts.size() != installDependencies.size() ) )
        {
            return false;
        }
        std::set<String> unique;
        for( const auto &dependency : installDependencies )
        {
            if( !dependency.isValid() || !unique.insert( dependency.str() ).second )
            {
                return false;
            }
        }
        for( const auto &artifact : installArtifacts )
        {
            if( !validArtifactPath( artifact.path ) || artifact.sourceHash == 0 ||
                artifact.payloadHash == 0 )
            {
                return false;
            }
        }
        return true;
    }

    bool CompiledResourceIO::hashFile( const String &path, u64 &hash, u64 &size, String &error )
    {
        hash = 14695981039346656037ull;
        size = 0;
        std::ifstream stream( fileSystemIOPath( std::filesystem::u8path( path.c_str() ) ),
                              std::ios::binary );
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
            {
                error = "Compiled payload changed before it could be packaged";
            }
            return false;
        }

        const auto destination = fileSystemIOPath( std::filesystem::u8path( outputPath.c_str() ) );
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
        const auto temporaryPath =
            fileSystemIOPath( std::filesystem::u8path( temporaryPathString.c_str() ) );

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
        std::ifstream payload( fileSystemIOPath( std::filesystem::u8path( payloadPath.c_str() ) ),
                               std::ios::binary );
        if( !output || !payload || !writeHeader( output, header ) )
        {
            error = "Failed to create the compiled resource container";
            return false;
        }

        // Inserting an empty streambuf sets failbit even though an empty
        // payload with the matching empty hash is a valid resource body.
        if( header.payloadSize != 0 )
        {
            output << payload.rdbuf();
        }
        output.flush();
        if( !output || payload.bad() )
        {
            error = "Failed while writing the compiled resource payload";
            return false;
        }
        output.close();
        payload.close();

        if( !replaceFile( temporaryPath, destination, error ) )
        {
            return false;
        }
        guard.path.clear();
        return true;
    }

    bool CompiledResourceIO::readHeader( const String &path, CompiledResourceHeader &header,
                                         String &error )
    {
        std::ifstream stream( fileSystemIOPath( std::filesystem::u8path( path.c_str() ) ),
                              std::ios::binary );
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
        const auto inputPath = fileSystemIOPath( std::filesystem::u8path( path.c_str() ) );
        std::ifstream stream( inputPath, std::ios::binary );
        if( !stream )
        {
            error = String( "Failed to open compiled resource: " ) + path;
            return false;
        }

        u64 payloadOffset = 0;
        if( !readHeaderFromStream( stream, header, payloadOffset, error ) )
        {
            return false;
        }

        std::error_code filesystemError;
        const u64 fileSize = std::filesystem::file_size( inputPath, filesystemError );
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
        const auto inputPath = fileSystemIOPath( std::filesystem::u8path( path.c_str() ) );
        std::ifstream stream( inputPath, std::ios::binary );
        if( !stream )
        {
            error = String( "Failed to open compiled resource: " ) + path;
            return false;
        }

        u64 payloadOffset = 0;
        if( !readHeaderFromStream( stream, resource.header, payloadOffset, error ) )
        {
            return false;
        }
        if( resource.header.payloadSize > maxPayloadBytes ||
            resource.header.payloadSize > static_cast<u64>( std::numeric_limits<size_t>::max() ) )
        {
            error = "Compiled resource payload exceeds the configured limit";
            return false;
        }

        std::error_code filesystemError;
        const u64 fileSize = std::filesystem::file_size( inputPath, filesystemError );
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
