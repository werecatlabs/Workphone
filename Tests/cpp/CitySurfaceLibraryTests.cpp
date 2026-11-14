#include <Workphone/Scene/Components/CitySurfaceLibrary.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <FreeImage.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace workphone;
using namespace workphone::scene;
using namespace workphone::procedural;
namespace fs = std::filesystem;
namespace
{
    void need( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &message ) { if( !value ) throw std::runtime_error( message.c_str() ); }
    std::vector<u8> readBytes( const fs::path &path )
    {
        std::ifstream file( path, std::ios::binary | std::ios::ate );
        need( bool( file ), "Cannot read image fixture bytes" );
        const auto length = static_cast<std::streamoff>( file.tellg() );
        need( length > 0, "Image fixture is empty" );
        std::vector<u8> bytes( static_cast<size_t>( length ) ); file.seekg( 0 );
        file.read( reinterpret_cast<char *>( bytes.data() ), std::streamsize( bytes.size() ) );
        need( bool( file ), "Image fixture read was truncated" ); return bytes;
    }
    void writeBytes( const fs::path &path, const std::vector<u8> &bytes )
    {
        std::ofstream file( path, std::ios::binary );
        file.write( reinterpret_cast<const char *>( bytes.data() ), std::streamsize( bytes.size() ) );
        need( bool( file ), "Cannot write image fixture bytes" );
    }
    std::vector<u8> pngChunk( const std::array<u8, 4> &type, const std::vector<u8> &payload )
    {
        std::vector<u8> result;
        const auto append = [&]( u32 value ) { for( int shift : { 24, 16, 8, 0 } ) result.push_back( u8( value >> shift ) ); };
        append( u32( payload.size() ) ); result.insert( result.end(), type.begin(), type.end() );
        result.insert( result.end(), payload.begin(), payload.end() );
        u32 crc = 0xffffffffu;
        for( size_t i = 4; i < result.size(); ++i )
        {
            crc ^= result[i];
            for( int bit = 0; bit < 8; ++bit ) crc = ( crc >> 1 ) ^ ( ( crc & 1 ) ? 0xedb88320u : 0 );
        }
        append( crc ^ 0xffffffffu ); return result;
    }
    std::vector<u8> thumbnailMarker()
    {
        std::unique_ptr<FIBITMAP, decltype( &FreeImage_Unload )> thumbnail( FreeImage_Allocate( 8, 8, 24 ), &FreeImage_Unload );
        std::unique_ptr<FIMEMORY, decltype( &FreeImage_CloseMemory )> memory( FreeImage_OpenMemory(), &FreeImage_CloseMemory );
        need( thumbnail && memory, "Could not allocate bounded thumbnail fixture" );
        std::memset( FreeImage_GetBits( thumbnail.get() ), 0, FreeImage_GetPitch( thumbnail.get() ) * 8 );
        need( FreeImage_SaveToMemory( FIF_JPEG, thumbnail.get(), memory.get(), JPEG_BASELINE | JPEG_QUALITYNORMAL ),
              "Could not encode bounded thumbnail fixture" );
        BYTE *bytes = nullptr; DWORD size = 0;
        need( FreeImage_AcquireMemory( memory.get(), &bytes, &size ) && size < 65527,
              "Thumbnail fixture exceeds JPEG marker bounds" );
        const u32 length = size + 8;
        std::vector<u8> marker{ 255, 224, u8( length >> 8 ), u8( length ), 'J','F','X','X',0,16 };
        marker.insert( marker.end(), bytes, bytes + size ); return marker;
    }
    u64 hash( const SurfaceBakeResult &surface )
    {
        u64 value = 1469598103934665603ull;
        for( const auto *map : { &surface.albedo, &surface.normal, &surface.orm } )
        {
            value = ( value ^ map->width ) * 1099511628211ull;
            value = ( value ^ map->height ) * 1099511628211ull;
            for( auto byte : map->pixels ) value = ( value ^ byte ) * 1099511628211ull;
        }
        return value ^ surface.height.values.size();
    }
    struct Image
    {
        std::unique_ptr<FIBITMAP, decltype( &FreeImage_Unload )> bitmap{ nullptr, &FreeImage_Unload };
        explicit Image( const fs::path &path )
        {
            const auto format = FreeImage_GetFIFFromFilename( path.string().c_str() );
            bitmap.reset( FreeImage_Load( format, path.string().c_str(), format == FIF_PNG ? PNG_IGNOREGAMMA : JPEG_DEFAULT ) );
            need( bool( bitmap ), "Independent fixture image decode failed" );
            need( FreeImage_GetWidth( bitmap.get() ) == 1024 && FreeImage_GetHeight( bitmap.get() ) == 1024,
                  "Downloaded city fixture is not1024square" );
        }
        std::array<double, 3> rgb( unsigned x, unsigned y ) const
        {
            const auto row = FreeImage_GetScanLine( bitmap.get(), 1023 - y );
            const auto type = FreeImage_GetImageType( bitmap.get() );
            if( type == FIT_RGB16 )
            { const auto p = reinterpret_cast<const FIRGB16 *>( row )[x]; return { p.red / 65535., p.green / 65535., p.blue / 65535. }; }
            if( type == FIT_RGBA16 )
            { const auto p = reinterpret_cast<const FIRGBA16 *>( row )[x]; return { p.red / 65535., p.green / 65535., p.blue / 65535. }; }
            const auto bpp = FreeImage_GetBPP( bitmap.get() );
            if( bpp == 8 )
            { const auto p = FreeImage_GetPalette( bitmap.get() )[row[x]]; return { p.rgbRed / 255., p.rgbGreen / 255., p.rgbBlue / 255. }; }
            need( bpp == 24 || bpp == 32, "Unexpected independent source pixel format" );
            const auto p = row + size_t( x ) * ( bpp / 8 );
            return { p[FI_RGBA_RED] / 255., p[FI_RGBA_GREEN] / 255., p[FI_RGBA_BLUE] / 255. };
        }
    };
    u8 byte( double value ) { return u8( std::lround( std::clamp( value, 0., 1. ) * 255 ) ); }
    double linear( double value ) { return value <= .04045 ? value / 12.92 : std::pow( ( value + .055 ) / 1.055, 2.4 ); }
    double encoded( double value ) { return value <= .0031308 ? value * 12.92 : 1.055 * std::pow( value, 1 / 2.4 ) - .055; }
    void compareSource( const fs::path &root, CitySurface id, const SurfaceBakeResult &surface )
    {
        const std::string stem = citySurfaceAssetId( id );
        const Image diffuse( root / ( stem + "_diff_1k.jpg" ) );
        const Image normal( root / ( stem + "_nor_gl_1k." + ( id == CitySurface::Ground ? "jpg" : "png" ) ) );
        const Image rough( root / ( stem + "_rough_1k.jpg" ) ), ao( root / ( stem + "_ao_1k.jpg" ) );
        const unsigned step = 1024 / surface.albedo.width;
        for( unsigned y : { 0u, 71u, 153u, 255u } ) for( unsigned x : { 0u, 43u, 167u, 255u } )
        {
            std::array<double, 3> colour{}, n{}; double r = 0, occlusion = 0;
            for( unsigned dy = 0; dy < step; ++dy ) for( unsigned dx = 0; dx < step; ++dx )
            {
                const auto p = diffuse.rgb( x * step + dx, y * step + dy );
                const auto v = normal.rgb( x * step + dx, y * step + dy );
                for( size_t c = 0; c < 3; ++c )
                { colour[c] += linear( p[c] ); n[c] += ( 2 * v[c] - 1 ) * ( c == 1 ? -1 : 1 ); }
                r += std::pow( rough.rgb( x * step + dx, y * step + dy )[0], 4 );
                occlusion += ao.rgb( x * step + dx, y * step + dy )[0];
            }
            const double count = step * step, length = std::sqrt( n[0] * n[0] + n[1] * n[1] + n[2] * n[2] );
            const auto a = surface.albedo.pixel( x, y ), b = surface.normal.pixel( x, y ), o = surface.orm.pixel( x, y );
            for( size_t c = 0; c < 3; ++c )
            {
                need( std::abs( int( a[c] ) - byte( encoded( colour[c] / count ) ) ) <= 1,
                      "Albedo transfer, row orientation or area filtering changed source colours" );
                need( std::abs( int( b[c] ) - byte( n[c] / length * .5 + .5 ) ) <= 1,
                      "Source normal precision, OpenGL green conversion or vector filtering is wrong" );
            }
            need( std::abs( int( o[0] ) - byte( occlusion / count ) ) <= 1 &&
                  std::abs( int( o[1] ) - byte( std::pow( r / count, .25 ) ) ) <= 1 && o[2] == 0 && o[3] == 255,
                  "ORM channels are not linear AO/roughness/zero metalness" );
        }
    }
    void actualPack( const fs::path &root )
    {
        String error;
        for( u32 i = 0; i < u32( CitySurface::Count ); ++i )
        {
            SurfaceBakeResult surface;
            const auto id = CitySurface( i );
            check( loadCitySurfaceFromDirectory( root.generic_u8string().c_str(), id, 512, surface, error ), error );
            for( const auto *map : { &surface.albedo, &surface.normal, &surface.orm } )
                need( map->width == 512 && map->height == 512 && map->pixels.size() == 512 * 512 * 4,
                      "Actual city pack escaped requested resolution" );
            need( surface.height.values.empty() && !surface.height.width && !surface.height.height,
                  "Appearance textures unexpectedly introduced displacement geometry" );
            compareSource( root, id, surface );
            std::cout << "PASS " << citySurfaceAssetId( id ) << "512pixel colour/normal/ORM contracts\n";
        }
        SurfaceBakeResult full;
        check( loadCitySurfaceFromDirectory( root.generic_u8string().c_str(), CitySurface::Asphalt, 1024, full, error ), error );
        compareSource( root, CitySurface::Asphalt, full );
        check( loadCitySurfaceFromDirectory( root.generic_u8string().c_str(), CitySurface::Asphalt, 256, full, error ), error );
        compareSource( root, CitySurface::Asphalt, full );
        need( citySurfaceTileMetres( CitySurface::Brick ) == 1.5f && citySurfaceTileMetres( CitySurface::Ground ) == 2.51f &&
              citySurfaceTileMetres( CitySurface::Count ) == 0, "City physical material scale contract changed" );
    }
    void failures( const fs::path &root, const fs::path &scratch )
    {
        SurfaceBakeResult retained; String error;
        check( loadCitySurfaceFromDirectory( root.generic_u8string().c_str(), CitySurface::Asphalt, 256, retained, error ), error );
        const auto before = hash( retained );
        fs::create_directories( scratch );
        const auto reject = [&]( const fs::path &directory, u32 size ) {
            need( !loadCitySurfaceFromDirectory( directory.generic_u8string().c_str(), CitySurface::Asphalt, size, retained, error ) &&
                  !error.empty() && hash( retained ) == before, "Failed city surface load replaced resident output" );
        };
        reject( scratch / "missing", 512 ); reject( root, 2048 ); reject( root, 0 );
        const auto fixture = scratch / "clean_asphalt_diff_1k.jpg";
        { std::ofstream file( fixture, std::ios::binary ); file << "corrupt city texture"; }
        reject( scratch, 512 );
        need( error.find( "PNG/JPEG" ) != String::npos, "Present corrupt image silently became a missing-pack fallback" );
        // Oversized PNG header must fail before even allocating codec metadata.
        const std::array<u8, 24> oversized{ 137,80,78,71,13,10,26,10, 0,0,0,13, 'I','H','D','R', 0,0,8,0, 0,0,4,0 };
        { std::ofstream file( fixture, std::ios::binary ); file.write( reinterpret_cast<const char *>( oversized.data() ), oversized.size() ); }
        reject( scratch, 512 );
        need( error.find( "1024" ) != String::npos, "Oversized source was not rejected by raw dimension preflight" );
        auto jpeg = readBytes( root / "clean_asphalt_diff_1k.jpg" );
        jpeg.resize( jpeg.size() - 2 ); writeBytes( fixture, jpeg );
        reject( scratch, 512 );
        need( error.find( "EOI" ) != String::npos, "Truncated JPEG was recovered silently by the codec" );
        jpeg = readBytes( root / "clean_asphalt_diff_1k.jpg" );
        const std::array<u8, 2> baselineMarker{ 255, 192 };
        const auto frame = std::search( jpeg.begin(), jpeg.end(), baselineMarker.begin(), baselineMarker.end() );
        need( frame != jpeg.end(), "Fixture has no baseline JPEG frame" ); *( frame + 1 ) = 194;
        writeBytes( fixture, jpeg ); reject( scratch, 512 );
        need( error.find( "baseline SOF0" ) != String::npos, "Progressive JPEG escaped the bounded baseline contract" );
        for( u8 marker : { u8( 225 ), u8( 218 ) } )
        {
            jpeg = readBytes( root / "clean_asphalt_diff_1k.jpg" );
            jpeg.insert( jpeg.end() - 2, { 255, marker, 0, 4, 0, 0 } );
            writeBytes( fixture, jpeg ); reject( scratch, 512 );
            need( error.find( "single baseline scan" ) != String::npos,
                  "Late JPEG metadata or additional scan escaped the preflight bound" );
        }
        // Preserve a valid PNG and its pixels, adding a valid compressed-text
        // chunk. Metadata expansion must be rejected before codec allocation.
        const auto originalPNG = readBytes( root / "brick_wall_001_nor_gl_1k.png" );
        auto png = originalPNG;
        const auto textChunk = pngChunk( { 'z', 'T', 'X', 't' },
            { 'f','i','x','t','u','r','e',0,0, 0x78,0x9c,0x03,0,0,0,0,1 } );
        png.insert( png.begin() + 33, textChunk.begin(), textChunk.end() ); writeBytes( fixture, png );
        reject( scratch, 512 );
        need( error.find( "compressed ancillary" ) != String::npos,
              "Compressed PNG metadata escaped bounded preflight" );
        png = originalPNG;
        const auto oversizedText = pngChunk( { 't', 'E', 'X', 't' }, std::vector<u8>( 64u << 10, 'a' ) );
        png.insert( png.begin() + 33, oversizedText.begin(), oversizedText.end() ); writeBytes( fixture, png );
        reject( scratch, 512 );
        need( error.find( "64 KiB" ) != String::npos, "PNG metadata escaped its independent allocation bound" );
        jpeg = readBytes( root / "clean_asphalt_diff_1k.jpg" );
        std::vector<u8> jpegMetadata( 65537, 0 );
        jpegMetadata[0] = 255; jpegMetadata[1] = 225; jpegMetadata[2] = 255; jpegMetadata[3] = 255;
        jpeg.insert( jpeg.begin() + 2, jpegMetadata.begin(), jpegMetadata.end() ); writeBytes( fixture, jpeg );
        reject( scratch, 512 );
        need( error.find( "64 KiB" ) != String::npos, "JPEG metadata escaped its independent allocation bound" );
        jpeg = readBytes( root / "clean_asphalt_diff_1k.jpg" );
        const auto thumbnail = thumbnailMarker();
        jpeg.insert( jpeg.begin() + 2, thumbnail.begin(), thumbnail.end() ); writeBytes( fixture, jpeg );
        for( const auto *name : { "clean_asphalt_nor_gl_1k.png", "clean_asphalt_rough_1k.jpg", "clean_asphalt_ao_1k.jpg" } )
            fs::copy_file( root / name, scratch / name, fs::copy_options::overwrite_existing );
        {
            SurfaceBakeResult stripped;
            check( loadCitySurfaceFromDirectory( scratch.generic_u8string().c_str(), CitySurface::Asphalt, 256, stripped, error ), error );
            need( hash( stripped ) == before && readBytes( fixture ) == jpeg,
                  "Stripping thumbnail metadata changed surface pixels or rewrote the original file" );
        }
        const auto scheduler = WorldStreamingScheduler::shared(); const auto baseline = scheduler->stats();
        // The final AO map fails only after albedo, normal and ORM candidate
        // buffers exist. Their teardown must retain both resident pixels and
        // exact admission, including when a reused fixture directory exists.
        const auto late = scratch / "late-map";
        fs::create_directories( late );
        for( const auto *name : { "clean_asphalt_diff_1k.jpg", "clean_asphalt_nor_gl_1k.png",
                                 "clean_asphalt_rough_1k.jpg" } )
            fs::copy_file( root / name, late / name, fs::copy_options::overwrite_existing );
        const auto lateAO = late / "clean_asphalt_ao_1k.jpg";
        fs::remove( lateAO );
        reject( late, 256 );
        need( error.find( "clean_asphalt_ao_1k.jpg" ) != String::npos &&
              error.find( "missing or unreadable" ) != String::npos && scheduler->stats().used == baseline.used,
              "Missing final city map did not release its partially loaded candidate" );
        { std::ofstream file( lateAO, std::ios::binary ); file << "corrupt final AO map"; }
        reject( late, 256 );
        need( error.find( "clean_asphalt_ao_1k.jpg" ) != String::npos &&
              error.find( "PNG/JPEG" ) != String::npos && scheduler->stats().used == baseline.used,
              "Corrupt final city map did not release its partially loaded candidate" );
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler; WorldStreamStats original;
            ~Restore() { String ignored; for( auto type : { StreamMemoryClass::SourceCpu, StreamMemoryClass::WorkingCpu } )
                scheduler->setBudget( type, original.limits[size_t( type )], ignored ); }
        } restore{ scheduler, baseline };
        for( auto type : { StreamMemoryClass::SourceCpu, StreamMemoryClass::WorkingCpu } )
        {
            check( scheduler->setBudget( type, baseline.used[size_t( type )], error ), error );
            reject( root, 512 );
            check( scheduler->setBudget( type, baseline.limits[size_t( type )], error ), error );
            need( scheduler->stats().used == baseline.used, "City surface decoder denial leaked CPU admission" );
        }
        check( loadCitySurfaceFromDirectory( root.generic_u8string().c_str(), CitySurface::Asphalt, 256, retained, error ), error );
        need( hash( retained ) == before && scheduler->stats().used == baseline.used,
              "Retry after restored city texture budgets changed output or retained storage" );
    }
}
int main( int argc, char **argv )
{
    int result = 0; std::cout << std::unitbuf;
    FreeImage_Initialise( FALSE );
    try
    {
        need( argc == 3, "Provide source pack and scratch directories" );
        const auto scheduler = WorldStreamingScheduler::shared(); const auto before = scheduler->stats();
        actualPack( fs::u8path( argv[1] ) ); failures( fs::u8path( argv[1] ), fs::u8path( argv[2] ) );
        need( scheduler->stats().used == before.used, "City surface result teardown retained CPU storage" );
        std::cout << "City photographic surface library: source mapping, precision, caps, fallback and admission PASS\n";
    }
    catch( const std::exception &error ) { std::cerr << error.what() << '\n'; result = 1; }
    FreeImage_DeInitialise(); WorldStreamingScheduler::shutdownShared(); return result;
}
