#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CitySurfaceLibrary.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <FreeImage.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        constexpr u64 maximumEncodedBytes = 12ull << 20;
        constexpr u32 maximumDimension = 1024;
        struct Entry { const char *id; float metres; const char *normalExtension; };
        constexpr std::array<Entry, 6> entries{{
            { "clean_asphalt", 2.1f, "png" }, { "concrete_pavement", 1.8f, "png" },
            // Physical override: five brick pitches across the source should be
            // about 25–30 cm each. Upstream raw provenance retains its 3 m value.
            { "brick_wall_001", 1.5f, "png" }, { "white_plaster_02", 1.f, "png" },
            { "concrete_wall_009", 1.805f, "png" }, { "grass_ground", 2.51f, "jpg" }
        }};
        void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
        struct FreeImageLifetime
        {
            FreeImageLifetime() { FreeImage_Initialise( FALSE ); }
            ~FreeImageLifetime() { FreeImage_DeInitialise(); }
        };
        using Bitmap = std::unique_ptr<FIBITMAP, decltype( &FreeImage_Unload )>;
        using Memory = std::unique_ptr<FIMEMORY, decltype( &FreeImage_CloseMemory )>;
        enum class Map { Colour, NormalGL, Roughness, Occlusion };

        std::array<u32, 2> inspectDimensions( const SourceBudgetVector<u8> &data )
        {
            const auto big = [&]( size_t at ) {
                return ( u32( data[at] ) << 24 ) | ( u32( data[at + 1] ) << 16 ) |
                       ( u32( data[at + 2] ) << 8 ) | data[at + 3];
            };
            const std::array<u8, 8> signature{ 137, 80, 78, 71, 13, 10, 26, 10 };
            if( data.size() >= 24 && std::equal( signature.begin(), signature.end(), data.begin() ) )
            {
                require( big( 8 ) == 13 && big( 12 ) == 0x49484452u, "City PNG requires a leading IHDR." );
                return { big( 16 ), big( 20 ) };
            }
            require( data.size() >= 4 && data[0] == 255 && data[1] == 216,
                     "City surface is not a supported PNG/JPEG image." );
            for( size_t at = 2; at < data.size(); )
            {
                require( data[at++] == 255, "City JPEG has an invalid marker stream." );
                while( at < data.size() && data[at] == 255 ) ++at;
                require( at < data.size(), "City JPEG marker is truncated." );
                const u8 marker = data[at++];
                if( marker == 1 || ( marker >= 208 && marker <= 217 ) ) continue;
                require( marker != 218 && at + 2 <= data.size(), "City JPEG has no bounded frame header." );
                const size_t length = ( size_t( data[at] ) << 8 ) | data[at + 1];
                require( length >= 2 && length <= data.size() - at, "City JPEG segment is truncated." );
                if( marker >= 192 && marker <= 207 && marker != 196 && marker != 200 && marker != 204 )
                {
                    require( length >= 8, "City JPEG frame is truncated." );
                    return { u32( data[at + 5] ) * 256 + data[at + 6],
                             u32( data[at + 3] ) * 256 + data[at + 4] };
                }
                at += length;
            }
            throw std::runtime_error( "City JPEG has no bounded frame header." );
        }

        void inspectContainer( const SourceBudgetVector<u8> &data )
        {
            constexpr size_t maximumMetadata = 64u << 10;
            if( data[0] == 137 )
            {
                const auto big = [&]( size_t at ) {
                    return ( u32( data[at] ) << 24 ) | ( u32( data[at + 1] ) << 16 ) |
                           ( u32( data[at + 2] ) << 8 ) | data[at + 3];
                };
                size_t metadata = 0, chunks = 0; bool hasImage = false;
                for( size_t at = 8; at < data.size(); )
                {
                    require( ++chunks <= 4096 && data.size() - at >= 12,
                             "City PNG chunk count is excessive or a chunk is truncated." );
                    const size_t length = big( at ); const u32 type = big( at + 4 );
                    require( length <= data.size() - at - 12, "City PNG chunk is truncated." );
                    if( type == 0x49444154u ) hasImage = true; // IDAT
                    else
                    {
                        require( length + 12 <= maximumMetadata - metadata,
                                 "City PNG metadata exceeds 64 KiB." );
                        metadata += length + 12;
                        // These chunks can expand independently of pixel dimensions.
                        // The bundled pack has only small, uncompressed metadata.
                        require( type != 0x7a545874u && type != 0x69434350u,
                                 "City PNG compressed ancillary metadata is unsupported." ); // zTXt, iCCP
                        if( type == 0x69545874u ) // iTXt keyword, compression flag, method
                        {
                            size_t text = at + 8, end = text + length;
                            while( text < end && data[text] ) ++text;
                            require( text + 2 < end, "City PNG international text header is truncated." );
                            require( data[text + 1] == 0,
                                     "City PNG compressed ancillary metadata is unsupported." );
                        }
                    }
                    at += length + 12;
                    if( type == 0x49454e44u ) // IEND
                    {
                        require( length == 0 && hasImage && at == data.size(),
                                 "City PNG terminator or image data is invalid." );
                        return;
                    }
                }
                throw std::runtime_error( "City PNG is missing its IEND terminator." );
            }
            // libjpeg normally recovers a truncated scan. This asset loader
            // deliberately requires a complete file before decoding any pixels.
            require( data.size() >= 4 && data[data.size() - 2] == 255 && data.back() == 217,
                     "City JPEG is missing its EOI terminator." );
            size_t metadata = 2; bool baselineFrame = false;
            for( size_t at = 2; at < data.size(); )
            {
                require( data[at++] == 255, "City JPEG has an invalid marker stream." );
                while( at < data.size() && data[at] == 255 ) ++at;
                require( at < data.size(), "City JPEG marker is truncated." );
                const u8 marker = data[at++];
                if( marker == 1 ) continue;
                require( marker != 216 && marker != 217 && at + 2 <= data.size(),
                         "City JPEG header ended before its image scan." );
                const size_t length = ( size_t( data[at] ) << 8 ) | data[at + 1];
                require( length >= 2 && length <= data.size() - at, "City JPEG segment is truncated." );
                require( length + 2 <= maximumMetadata - metadata, "City JPEG metadata exceeds 64 KiB." );
                metadata += length + 2;
                if( marker >= 192 && marker <= 207 && marker != 196 && marker != 200 && marker != 204 )
                {
                    require( marker == 192 && !baselineFrame && length >= 8 && data[at + 2] == 8,
                             "City JPEG requires one 8-bit baseline SOF0 frame." );
                    baselineFrame = true;
                }
                if( marker == 218 )
                {
                    require( baselineFrame, "City JPEG requires a baseline SOF0 frame before its scan." );
                    // Only a complete single baseline scan is accepted. Extra
                    // APP/COM segments between progressive scans could otherwise
                    // allocate metadata outside the pre-scan admission bound.
                    for( size_t scan = at + length; scan < data.size(); )
                    {
                        if( data[scan++] != 255 ) continue;
                        while( scan < data.size() && data[scan] == 255 ) ++scan;
                        require( scan < data.size(), "City JPEG entropy marker is truncated." );
                        const u8 scanMarker = data[scan++];
                        if( scanMarker == 0 || ( scanMarker >= 208 && scanMarker <= 215 ) ) continue;
                        require( scanMarker == 217 && scan == data.size(),
                                 "City JPEG supports a single baseline scan without late metadata." );
                        return;
                    }
                    throw std::runtime_error( "City JPEG entropy data has no EOI terminator." );
                }
                at += length;
            }
            throw std::runtime_error( "City JPEG has no image scan." );
        }

        void removeJPEGMetadata( SourceBudgetVector<u8> &data )
        {
            if( data[0] != 255 ) return;
            // The validated stream is compacted in its existing admitted input
            // buffer. FreeImage must not recursively decode EXIF/JFXX thumbnails
            // or process profiles whose sizes are unrelated to the main pixels.
            size_t output = 2;
            for( size_t at = 2; at < data.size(); )
            {
                const size_t start = at++;
                while( data[at] == 255 ) ++at;
                const u8 marker = data[at++];
                if( marker == 218 )
                {
                    const size_t remaining = data.size() - start;
                    std::memmove( data.data() + output, data.data() + start, remaining );
                    data.resize( output + remaining ); return;
                }
                const size_t length = marker == 1 ? 0 : ( size_t( data[at] ) << 8 ) | data[at + 1];
                const size_t end = at + length;
                const bool jfif = marker == 224 && length >= 16 &&
                                  std::memcmp( data.data() + at + 2, "JFIF\0", 5 ) == 0;
                const bool adobe = marker == 238 && length == 14 &&
                                   std::memcmp( data.data() + at + 2, "Adobe", 5 ) == 0;
                if( jfif )
                {
                    std::memmove( data.data() + output + 4, data.data() + at + 2, 14 );
                    data[output] = 255; data[output + 1] = marker;
                    data[output + 2] = 0; data[output + 3] = 16;
                    data[output + 16] = data[output + 17] = 0; // no JFIF thumbnail
                    output += 18;
                }
                else if( adobe || !( ( marker >= 224 && marker <= 239 ) || marker == 254 ) )
                {
                    std::memmove( data.data() + output, data.data() + start, end - start );
                    output += end - start;
                }
                at = end;
            }
        }

        SourceBudgetVector<u8> readFile( const String &path, IFileSystem *files )
        {
            SourceBudgetVector<u8> data;
            if( files )
            {
                auto stream = files->open( path, true, true, false, false, false );
                require( bool( stream ), "City surface file is missing or unreadable." );
                const auto bytes = stream->size();
                require( bytes > 0 && bytes <= maximumEncodedBytes, "City surface encoded size exceeds 12 MiB or is empty." );
                data.resize( size_t( bytes ) );
                size_t read = 0;
                while( read < data.size() )
                {
                    const auto count = stream->read( data.data() + read, data.size() - read );
                    require( count > 0, "City surface file changed or was truncated while reading." );
                    read += count;
                }
                u8 extra = 0;
                require( stream->read( &extra, 1 ) == 0, "City surface file grew while reading." );
            }
            else
            {
                const auto file = std::filesystem::u8path( path.c_str() );
                std::ifstream stream( file, std::ios::binary | std::ios::ate );
                require( bool( stream ), "City surface file is missing or unreadable." );
                const auto bytes = static_cast<std::streamoff>( stream.tellg() );
                require( bytes > 0 && u64( bytes ) <= maximumEncodedBytes,
                         "City surface encoded size exceeds 12 MiB or is empty." );
                data.resize( size_t( bytes ) ); stream.seekg( 0 );
                stream.read( reinterpret_cast<char *>( data.data() ), std::streamsize( data.size() ) );
                require( bool( stream ), "City surface file changed or was truncated while reading." );
                require( stream.peek() == std::char_traits<char>::eof(), "City surface file grew while reading." );
            }
            return data;
        }

        std::array<double, 3> pixel( FIBITMAP *image, u32 x, u32 y )
        {
            const auto row = FreeImage_GetScanLine( image, FreeImage_GetHeight( image ) - 1 - y );
            switch( FreeImage_GetImageType( image ) )
            {
            case FIT_RGB16:
            {
                const auto &p = reinterpret_cast<const FIRGB16 *>( row )[x];
                return { p.red / 65535., p.green / 65535., p.blue / 65535. };
            }
            case FIT_RGBA16:
            {
                const auto &p = reinterpret_cast<const FIRGBA16 *>( row )[x];
                return { p.red / 65535., p.green / 65535., p.blue / 65535. };
            }
            case FIT_UINT16:
            {
                const double p = reinterpret_cast<const u16 *>( row )[x] / 65535.; return { p, p, p };
            }
            default:
                if( FreeImage_GetBPP( image ) == 8 )
                {
                    const auto palette = FreeImage_GetPalette( image );
                    require( palette != nullptr, "City indexed image has no palette." );
                    const auto p = palette[row[x]];
                    return { p.rgbRed / 255., p.rgbGreen / 255., p.rgbBlue / 255. };
                }
                const auto p = row + size_t( x ) * ( FreeImage_GetBPP( image ) / 8 );
                return { p[FI_RGBA_RED] / 255., p[FI_RGBA_GREEN] / 255., p[FI_RGBA_BLUE] / 255. };
            }
        }

        void decode( const String &path, IFileSystem *files, u32 size, Map kind,
                     procedural::TextureBuffer &output )
        {
            auto encoded = readFile( path, files );
            const auto dimensions = inspectDimensions( encoded );
            require( dimensions[0] && dimensions[1] && dimensions[0] <= maximumDimension && dimensions[1] <= maximumDimension,
                     "City surface image dimensions must not exceed 1024 pixels." );
            require( dimensions[0] >= size && dimensions[1] >= size,
                     "City surface source is smaller than its requested output." );
            inspectContainer( encoded );
            removeJPEGMetadata( encoded );
            String error;
            auto headerBudget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::WorkingCpu, 64u << 10, error );
            if( !headerBudget ) throw std::runtime_error( error.c_str() );
            static FreeImageLifetime lifetime;
            Memory memory( FreeImage_OpenMemory( encoded.data(), DWORD( encoded.size() ) ), &FreeImage_CloseMemory );
            require( bool( memory ), "City surface decoder stream allocation failed." );
            const auto format = FreeImage_GetFileTypeFromMemory( memory.get(), 0 );
            require( format == FIF_PNG || format == FIF_JPEG, "City surface codec must be PNG or JPEG." );
            const int flags = format == FIF_PNG ? PNG_IGNOREGAMMA : JPEG_DEFAULT;
            FreeImage_SeekMemory( memory.get(), 0, SEEK_SET );
            Bitmap header( FreeImage_LoadFromMemory( format, memory.get(), flags | FIF_LOAD_NOPIXELS ), &FreeImage_Unload );
            require( header && FreeImage_GetWidth( header.get() ) == dimensions[0] &&
                     FreeImage_GetHeight( header.get() ) == dimensions[1], "City surface decoder header is invalid." );
            const auto type = FreeImage_GetImageType( header.get() );
            const auto bpp = FreeImage_GetBPP( header.get() );
            require( ( type == FIT_BITMAP && ( bpp == 8 || bpp == 24 || bpp == 32 ) ) ||
                     type == FIT_RGB16 || type == FIT_RGBA16 || type == FIT_UINT16,
                     "City surface requires 8-bit or 16-bit integer image channels." );
            const u64 bitmapBytes = u64( FreeImage_GetPitch( header.get() ) ) * dimensions[1];
            require( bitmapBytes && bitmapBytes <= 8ull * maximumDimension * maximumDimension,
                     "City surface decoder pitch exceeds its admitted dimensions." );
            // The bitmap and bounded codec scratch have a working lease; only
            // the final TextureBuffers and encoded input use SourceCpu storage.
            auto decodeBudget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::WorkingCpu,
                bitmapBytes + ( 4u << 20 ), error );
            if( !decodeBudget ) throw std::runtime_error( error.c_str() );
            header.reset(); FreeImage_SeekMemory( memory.get(), 0, SEEK_SET );
            Bitmap image( FreeImage_LoadFromMemory( format, memory.get(), flags ), &FreeImage_Unload );
            require( image && FreeImage_GetWidth( image.get() ) == dimensions[0] &&
                     FreeImage_GetHeight( image.get() ) == dimensions[1] && FreeImage_GetImageType( image.get() ) == type &&
                     FreeImage_GetBPP( image.get() ) == bpp &&
                     u64( FreeImage_GetPitch( image.get() ) ) * dimensions[1] <= bitmapBytes,
                     "City surface decode failed or changed its admitted header." );
            if( output.pixels.empty() ) output = procedural::TextureBuffer( size, size );
            const auto byte = []( double value ) { return u8( std::lround( std::clamp( value, 0., 1. ) * 255 ) ); };
            for( u32 y = 0; y < size; ++y ) for( u32 x = 0; x < size; ++x )
            {
                std::array<double, 3> sum{}; double weight = 0;
                const double x0 = double( x ) * dimensions[0] / size, x1 = double( x + 1 ) * dimensions[0] / size;
                const double y0 = double( y ) * dimensions[1] / size, y1 = double( y + 1 ) * dimensions[1] / size;
                for( u32 sy = u32( y0 ); sy < u32( std::ceil( y1 ) ); ++sy )
                    for( u32 sx = u32( x0 ); sx < u32( std::ceil( x1 ) ); ++sx )
                    {
                        const double w = ( std::min( x1, double( sx + 1 ) ) - std::max( x0, double( sx ) ) ) *
                                         ( std::min( y1, double( sy + 1 ) ) - std::max( y0, double( sy ) ) );
                        auto p = pixel( image.get(), sx, sy );
                        for( size_t c = 0; c < 3; ++c )
                        {
                            if( kind == Map::Colour ) p[c] = p[c] <= .04045 ? p[c] / 12.92 : std::pow( ( p[c] + .055 ) / 1.055, 2.4 );
                            else if( kind == Map::NormalGL ) p[c] = ( p[c] * 2 - 1 ) * ( c == 1 ? -1 : 1 );
                            else if( kind == Map::Roughness ) p[c] = p[c] * p[c] * p[c] * p[c];
                            sum[c] += p[c] * w;
                        }
                        weight += w;
                    }
                for( auto &c : sum ) c /= weight;
                if( kind == Map::NormalGL )
                {
                    const double length = std::sqrt( sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2] );
                    if( length < 1e-8 ) sum = { 0, 0, 1 };
                    else for( auto &c : sum ) c /= length;
                }
                auto out = output.pixel( x, y );
                if( kind == Map::Colour || kind == Map::NormalGL ) for( size_t c = 0; c < 3; ++c )
                    out[c] = byte( kind == Map::NormalGL ? sum[c] * .5 + .5 :
                        sum[c] <= .0031308 ? sum[c] * 12.92 : 1.055 * std::pow( sum[c], 1 / 2.4 ) - .055 );
                else out[kind == Map::Occlusion ? 0 : 1] = byte( kind == Map::Roughness ? std::pow( sum[0], .25 ) : sum[0] );
                out[3] = 255;
            }
        }

        bool load( const String &directory, IFileSystem *files, CitySurface surface, u32 size,
                   procedural::SurfaceBakeResult &output, String &error )
        {
            String path;
            try
            {
                require( u32( surface ) < entries.size(), "Unknown city surface." );
                require( size == 256 || size == 512 || size == 1024, "City surface output must be 256, 512 or 1024 pixels." );
                require( !directory.empty() && directory.size() <= 4096, "City surface directory is unavailable or excessive." );
                const auto &entry = entries[u32( surface )];
                const String base = directory + "/" + entry.id;
                procedural::SurfaceBakeResult candidate;
                const auto map = [&]( const String &suffix, Map kind, procedural::TextureBuffer &target ) {
                    path = base + suffix; decode( path, files, size, kind, target );
                };
                map( "_diff_1k.jpg", Map::Colour, candidate.albedo );
                map( String( "_nor_gl_1k." ) + entry.normalExtension, Map::NormalGL, candidate.normal );
                map( "_rough_1k.jpg", Map::Roughness, candidate.orm );
                map( "_ao_1k.jpg", Map::Occlusion, candidate.orm );
                output.albedo.swap( candidate.albedo ); output.normal.swap( candidate.normal );
                output.orm.swap( candidate.orm ); output.height.swap( candidate.height );
                error.clear(); return true;
            }
            catch( const std::exception &exception )
            {
                error = String( "City surface " ) + path + ": " + exception.what(); return false;
            }
        }
    }

    const char *citySurfaceAssetId( CitySurface surface ) noexcept
    { return u32( surface ) < entries.size() ? entries[u32( surface )].id : ""; }
    float citySurfaceTileMetres( CitySurface surface ) noexcept
    { return u32( surface ) < entries.size() ? entries[u32( surface )].metres : 0; }

    bool loadCitySurfaceFromDirectory( const String &directory, CitySurface surface, u32 size,
                                       procedural::SurfaceBakeResult &output, String &error )
    { return load( directory, nullptr, surface, size, output, error ); }

    bool loadCitySurface( CitySurface surface, u32 size, procedural::SurfaceBakeResult &output, String &error )
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app || app->getMediaPath().empty() )
        { error = "City surface Media directory is unavailable."; return false; }
        return load( app->getMediaPath() + "/Procedural/City/Textures", app->getFileSystemPtr(), surface, size, output, error );
    }
}  // namespace workphone::scene
