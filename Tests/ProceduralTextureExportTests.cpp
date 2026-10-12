#include "../Engine/cpp/Include/Workphone/Graphics/ProceduralTextureExport.hpp"
#include <Workphone/Graphics/ProceduralSurfaceSourceExport.hpp>
#include <cstdlib>
#define CHECK(condition) do { if( !(condition) ) std::abort(); } while( false )
#include <fstream>

struct Pixel { double r, g, b, a; };

int main( int argc, char **argv )
{
    using workphone::procedural::texture_export::encode;
    const std::vector<Pixel> pixels = { { -1, 0.5, 2, 0.25 }, { 1, 0, 0, 1 },
                                         { 0, 1, 0, 0 }, { 0, 0, 1, 1 } };
    std::vector<unsigned char> bytes;
    CHECK( !encode( 2, 2, pixels, "dds", bytes ) && bytes.empty() );
    CHECK( !encode( 0, 2, pixels, "png", bytes ) );
    CHECK( !encode( 1, 2, pixels, "png", bytes ) );
    CHECK( !encode( 2, 2, pixels, "ppm", bytes ) );
    CHECK( encode( 2, 2, pixels, ".TGA", bytes ) );
    CHECK( bytes.size() == 34 && bytes[17] == 0x28 );
    CHECK( bytes[18] == 255 && bytes[19] == 128 && bytes[20] == 0 && bytes[21] == 64 );
    CHECK( encode( 2, 2, pixels, "pam", bytes ) );
    CHECK( encode( 2, 2, pixels, "PNG", bytes ) );
    const auto first = bytes;
    CHECK( encode( 2, 2, pixels, "png", bytes ) && first == bytes );
    struct Buffer { unsigned width, height; std::vector<unsigned char> pixels; };
    Buffer buffer = { 2, 2, { 0, 128, 255, 64, 255, 0, 0, 255, 0, 255, 0, 0, 0, 0, 255, 255 } };
    CHECK( workphone::procedural::texture_export::encodeTextureBuffer( buffer, "png", bytes ) );
    CHECK( bytes == first );
    buffer.pixels.pop_back();
    CHECK( !workphone::procedural::texture_export::encodeTextureBuffer( buffer, "png", bytes ) && bytes.empty() );
    const std::string directory = argc > 1 ? argv[1] : ".";
    for( const auto *format : { "png", "tga", "pam" } )
    {
        CHECK( encode( 2, 2, pixels, format, bytes ) );
        std::ofstream out( directory + "/texture-export-test." + format, std::ios::binary );
        out.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
        CHECK( out.good() );
    }
    std::vector<Pixel> large( 257 * 257, { 0.5, 0.25, 1, 0.5 } );
    CHECK( encode( 257, 257, large, "png", bytes ) );
    std::ofstream out( directory + "/texture-export-blocks.png", std::ios::binary );
    out.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
    CHECK( out.good() );
    for( auto &p : large ) p.a = 1;
    CHECK( encode( 257, 257, large, "ppm", bytes ) );

    namespace fs = std::filesystem;
    const auto source = fs::path( directory ) / "texture-export-asphalt-source";
    CHECK( !fs::exists( source ) );
    struct Surface { Buffer albedo, normal, orm; };
    Surface surface = { { 1, 1, { 32, 64, 128, 255 } },
                        { 1, 1, { 128, 128, 255, 255 } },
                        { 1, 1, { 128, 192, 0, 255 } } };
    std::string error;
    CHECK( workphone::procedural::texture_export::exportAsphaltSource( surface, source, 42, error ) );
    CHECK( error.empty() );
    const auto read = [&]( const char *name ) {
        std::ifstream stream( source / name, std::ios::binary );
        CHECK( stream.good() );
        return std::string( std::istreambuf_iterator<char>( stream ), {} );
    };
    const auto albedo = read( "asphalt-albedo.tga" );
    CHECK( albedo.size() == 22 && static_cast<unsigned char>( albedo[18] ) == 128 );
    CHECK( static_cast<unsigned char>( albedo[19] ) == 64 && static_cast<unsigned char>( albedo[20] ) == 32 );
    const auto roughness = read( "asphalt-roughness.tga" );
    CHECK( static_cast<unsigned char>( roughness[18] ) == 192 &&
           static_cast<unsigned char>( roughness[19] ) == 192 &&
           static_cast<unsigned char>( roughness[20] ) == 192 );
    CHECK( read( "asphalt-normal.texres" ).find( "\"mipFilter\":\"normal\"" ) != std::string::npos );
    CHECK( read( "asphalt-orm.texres" ).find( "\"mipFilter\":\"data\"" ) != std::string::npos );
    CHECK( read( "asphalt.matres" ).find( "data://asphalt-albedo.texres" ) != std::string::npos );
    CHECK( read( "material-source.json" ).find( "\"seed\":42" ) != std::string::npos );
    CHECK( read( "material-source.json" ).find( "\"auxiliaryMapsBoundByCookedMaterial\":false" ) != std::string::npos );
    CHECK( !workphone::procedural::texture_export::exportAsphaltSource( surface, source, 43, error ) );
    CHECK( !error.empty() && read( "asphalt-albedo.tga" ) == albedo );
    fs::remove_all( source ); // Only the new fixture directory created above.
    surface.orm.pixels.pop_back();
    CHECK( !workphone::procedural::texture_export::exportAsphaltSource( surface, source, 42, error ) );
    CHECK( !fs::exists( source ) );
}
