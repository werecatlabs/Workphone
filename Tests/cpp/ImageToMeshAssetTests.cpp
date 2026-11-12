#include <ui/ImageToMeshAssets.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>

namespace
{
    namespace fs = std::filesystem;
    namespace assets = workphone::editor::mesh_assets;
    using Bytes = std::vector<std::uint8_t>;
    void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }
    struct TemporaryDirectory
    {
        fs::path path;
        TemporaryDirectory()
        {
            const auto nonce = std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
            path = fs::temp_directory_path() /
                   ( "workphone_image_mesh_" + nonce + "_" + std::to_string( std::random_device{}() ) );
            require( fs::create_directory( path ), "Could not create test directory" );
        }
        ~TemporaryDirectory()
        {
            std::error_code ignored;
            if( path.parent_path() == fs::temp_directory_path() &&
                path.filename().string().find( "workphone_image_mesh_" ) == 0 ) fs::remove_all( path, ignored );
        }
    };
    void le32( Bytes &bytes, std::uint32_t value )
    {
        for( unsigned i = 0; i < 4; ++i ) bytes.push_back( static_cast<std::uint8_t>( value >> ( i * 8 ) ) );
    }
    void be32( Bytes &bytes, std::uint32_t value )
    {
        for( int i = 3; i >= 0; --i ) bytes.push_back( static_cast<std::uint8_t>( value >> ( i * 8 ) ) );
    }
    Bytes triangle()
    {
        std::string json = R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0}],"meshes":[{"primitives":[{"attributes":{"POSITION":0},"mode":4}]}],"buffers":[{"byteLength":36}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]}]})";
        while( json.size() % 4 ) json += ' ';
        Bytes bytes;
        le32( bytes, 0x46546c67 ); le32( bytes, 2 );
        le32( bytes, static_cast<std::uint32_t>( 12 + 8 + json.size() + 8 + 36 ) );
        le32( bytes, static_cast<std::uint32_t>( json.size() ) ); le32( bytes, 0x4e4f534a );
        bytes.insert( bytes.end(), json.begin(), json.end() );
        le32( bytes, 36 ); le32( bytes, 0x004e4942 );
        const float positions[] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
        for( const auto position : positions )
        {
            std::uint32_t bits;
            std::memcpy( &bits, &position, sizeof( bits ) );
            le32( bytes, bits );
        }
        return bytes;
    }
    std::uint32_t crc( const std::uint8_t *bytes, std::size_t size )
    {
        std::uint32_t value = 0xffffffffu;
        while( size-- )
        {
            value ^= *bytes++;
            for( int bit = 0; bit < 8; ++bit ) value = ( value >> 1 ) ^ ( ( value & 1 ) ? 0xedb88320u : 0 );
        }
        return value ^ 0xffffffffu;
    }
    void chunk( Bytes &png, const char *type, const Bytes &body )
    {
        be32( png, static_cast<std::uint32_t>( body.size() ) );
        const auto start = png.size();
        png.insert( png.end(), type, type + 4 );
        png.insert( png.end(), body.begin(), body.end() );
        be32( png, crc( png.data() + start, 4 + body.size() ) );
    }
    Bytes sourcePng()
    {
        Bytes png{ 137, 80, 78, 71, 13, 10, 26, 10 }, header;
        be32( header, 32 ); be32( header, 32 );
        header.insert( header.end(), { 8, 2, 0, 0, 0 } );
        chunk( png, "IHDR", header );
        Bytes raw( 32 * ( 1 + 32 * 3 ), 0 );
        Bytes zlib{ 0x78, 0x01, 0x01 };
        const auto count = static_cast<std::uint16_t>( raw.size() );
        zlib.push_back( count & 0xff ); zlib.push_back( count >> 8 );
        zlib.push_back( ( ~count ) & 0xff ); zlib.push_back( ( ~count >> 8 ) & 0xff );
        zlib.insert( zlib.end(), raw.begin(), raw.end() );
        be32( zlib, ( static_cast<std::uint32_t>( raw.size() % 65521 ) << 16 ) | 1 );
        chunk( png, "IDAT", zlib ); chunk( png, "IEND", {} );
        return png;
    }
    void write( const fs::path &path, const Bytes &bytes )
    {
        std::ofstream output( path, std::ios::binary );
        output.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
        require( output.good(), "Could not write fixture" );
    }
    Bytes read( const fs::path &path )
    {
        std::ifstream input( path, std::ios::binary );
        require( input.good(), "Could not reopen generated mesh" );
        return { std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() };
    }
    void contracts()
    {
        TemporaryDirectory temporary;
        const auto project = temporary.path / fs::u8path( "Project caf\xc3\xa9 with spaces" );
        fs::create_directory( project );
        const auto source = temporary.path / "source.PNG";
        const auto png = sourcePng();
        write( source, png );
        Bytes loaded;
        std::string mime, error;
        require( assets::readImage( source, loaded, mime, error ), error.c_str() );
        require( loaded == png && mime == "image/png" && error.empty(), "Selected image was altered" );
        write( temporary.path / "wrong.jpg", png );
        require( !assets::readImage( temporary.path / "wrong.jpg", loaded, mime, error ) && loaded.empty() && mime.empty(),
                 "Image extension did not match encoded format" );
        write( temporary.path / "invalid.png", Bytes{ 1, 2, 3 } );
        for( const auto path : { temporary.path / "missing.png", temporary.path / "invalid.png", project } )
            require( !assets::readImage( path, loaded, mime, error ) && !error.empty() && loaded.empty(),
                     "Bad image input was accepted or retained bytes" );
        write( temporary.path / "oversized.png", png );
        fs::resize_file( temporary.path / "oversized.png", workphone::mesh_generation::maximumSourceImageBytes + 1 );
        require( !assets::readImage( temporary.path / "oversized.png", loaded, mime, error ) && loaded.empty(),
                 "Oversized image was read" );
        const auto glb = triangle();
        fs::path output;
        require( assets::saveGlb( project, "test-mesh.glb", glb.data(), glb.size(), output, error ), error.c_str() );
        require( output.filename() == "test-mesh.glb" && read( output ) == glb && error.empty(),
                 "Mesh name or content changed during save" );
        const auto first = output;
        require( assets::saveGlb( project, "test-mesh", glb.data(), glb.size(), output, error ), error.c_str() );
        require( output.filename() == "test-mesh-2.glb" && read( first ) == glb && read( output ) == glb,
                 "Repeated generation overwrote an existing mesh" );
        require( assets::sanitizeBasename( "" ) == "generated-mesh", "Wrong empty mesh name" );
        for( const auto name : { "../../outside", "C:\\outside\\mesh", "CON.glb", "NUL", "...", "a/b:c*?", "object.GLB" } )
        {
            require( assets::saveGlb( project, name, glb.data(), glb.size(), output, error ), error.c_str() );
            require( output.parent_path() == fs::canonical( project / "Assets/Generated" ) && output.extension() == ".glb",
                     "Mesh name escaped the generated asset directory" );
        }
        const auto before = std::distance( fs::directory_iterator( project / "Assets/Generated" ), fs::directory_iterator() );
        auto corrupt = glb;
        corrupt[8] ^= 1;
        for( const auto &bytes : { Bytes{}, Bytes{ 'n', 'o', 't', 'g', 'l', 'b' }, corrupt } )
        {
            require( !assets::saveGlb( project, "invalid", bytes.data(), bytes.size(), output, error ) && output.empty() && !error.empty(),
                     "Malformed GLB was published" );
        }
        require( before == std::distance( fs::directory_iterator( project / "Assets/Generated" ), fs::directory_iterator() ),
                 "Failed generation left an asset behind" );
        require( !assets::saveGlb( "relative", "bad", glb.data(), glb.size(), output, error ),
                 "Unsaved/relative project path was accepted" );
        const auto blocked = temporary.path / "Blocked";
        fs::create_directory( blocked ); write( blocked / "Assets", Bytes{ 1 } );
        require( !assets::saveGlb( blocked, "mesh", glb.data(), glb.size(), output, error ),
                 "Blocked asset directory did not fail" );
        const auto linked = temporary.path / "Linked";
        const auto outside = temporary.path / "Outside";
        fs::create_directories( linked / "Assets" ); fs::create_directory( outside );
        std::error_code linkError;
        fs::create_directory_symlink( outside, linked / "Assets/Generated", linkError );
        if( !linkError )
        {
            require( !assets::saveGlb( linked, "escape", glb.data(), glb.size(), output, error ) && fs::is_empty( outside ),
                     "Mesh publication escaped through a symlink" );
        }
        else std::cout << "INFO: symlink fixture unavailable: " << linkError.message() << '\n';
    }
}
int main()
{
    try { contracts(); std::cout << "PASS: image input and generated GLB publication contracts\n"; return 0; }
    catch( const std::exception &error ) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
