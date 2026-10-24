#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <meshoptimizer.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    bool close( float a, float b ) { return std::abs( a - b ) < 0.0001f; }

    enum class Fault
    {
        None,
        CorruptPayload,
        SourceOutsideBuffer,
        InconsistentCount,
        UnknownMode,
        UnknownFilter,
        InvalidFilterStride
    };

    struct View
    {
        size_t offset, length, stride, count, decodedOffset;
        std::string mode, filter;
    };

    struct Fixture
    {
        std::vector<unsigned char> bytes;
        std::vector<View> views;
        size_t decodedLength = 0;

        void attributes( const void *data, size_t count, size_t stride,
                         const char *filter = "NONE" )
        {
            std::vector<unsigned char> encoded( meshopt_encodeVertexBufferBound( count, stride ) );
            const auto size = meshopt_encodeVertexBuffer( encoded.data(), encoded.size(), data, count, stride );
            require( size > 0, "fixture vertex encoding failed" );
            append( encoded.data(), size, count, stride, "ATTRIBUTES", filter );
        }

        void indices( bool sequence )
        {
            const unsigned int indices[] = { 0, 1, 2 };
            std::vector<unsigned char> encoded( sequence ? meshopt_encodeIndexSequenceBound( 3, 3 ) :
                                                           meshopt_encodeIndexBufferBound( 3, 3 ) );
            const auto size = sequence ? meshopt_encodeIndexSequence( encoded.data(), encoded.size(), indices, 3 ) :
                                         meshopt_encodeIndexBuffer( encoded.data(), encoded.size(), indices, 3 );
            require( size > 0, "fixture index encoding failed" );
            append( encoded.data(), size, 3, 2, sequence ? "INDICES" : "TRIANGLES", "NONE" );
        }

        void append( const unsigned char *data, size_t size, size_t count, size_t stride,
                     const char *mode, const char *filter )
        {
            while( bytes.size() % 4 ) bytes.push_back( 0 );
            while( decodedLength % 4 ) ++decodedLength;
            views.push_back( { bytes.size(), size, stride, count, decodedLength, mode, filter } );
            bytes.insert( bytes.end(), data, data + size );
            decodedLength += count * stride;
        }

        static void uint32( std::vector<unsigned char> &output, std::uint32_t value )
        {
            for( unsigned int shift = 0; shift < 32; shift += 8 )
                output.push_back( static_cast<unsigned char>( value >> shift ) );
        }

        std::vector<unsigned char> glb( bool floatingPositions, Fault fault )
        {
            switch( fault )
            {
            case Fault::CorruptPayload: bytes[views[0].offset] = 0xff; break;
            case Fault::SourceOutsideBuffer: views[0].offset = bytes.size() + 16; break;
            case Fault::InconsistentCount: ++views[0].count; break;
            case Fault::UnknownMode: views[0].mode = "INVALID"; break;
            case Fault::UnknownFilter: views[0].filter = "INVALID"; break;
            case Fault::InvalidFilterStride: views[0].filter = "QUATERNION"; break;
            default: break;
            }
            std::ostringstream json;
            json << R"({"asset":{"version":"2.0"},"extensionsUsed":["EXT_meshopt_compression","KHR_mesh_quantization"],)"
                    R"("extensionsRequired":["EXT_meshopt_compression","KHR_mesh_quantization"],"buffers":[{"byteLength":)"
                 << bytes.size() << R"(},{"byteLength":)" << decodedLength
                 << R"(,"extensions":{"EXT_meshopt_compression":{"fallback":true}}}],"bufferViews":[)";
            for( size_t i = 0; i < views.size(); ++i )
            {
                const auto &view = views[i];
                if( i ) json << ',';
                json << R"({"buffer":1,"byteOffset":)" << view.decodedOffset
                     << R"(,"byteLength":)" << ( i == 0 && fault == Fault::InconsistentCount ?
                                                    ( view.count - 1 ) * view.stride : view.count * view.stride );
                if( view.mode == "ATTRIBUTES" || i < 3 ) json << R"(,"byteStride":)" << view.stride;
                json << R"(,"extensions":{"EXT_meshopt_compression":{"buffer":0,"byteOffset":)" << view.offset
                     << R"(,"byteLength":)" << view.length << R"(,"byteStride":)" << view.stride
                     << R"(,"count":)" << view.count << R"(,"mode":")" << view.mode
                     << R"(","filter":")" << view.filter << R"("}}})";
            }
            json << R"(],"accessors":[{"bufferView":0,"componentType":)" << ( floatingPositions ? 5126 : 5122 )
                 << R"(,"normalized":)" << ( floatingPositions ? "false" : "true" )
                 << R"(,"count":3,"type":"VEC3","min":[-1,0,0],"max":[1,1,0]},)"
                    R"({"bufferView":1,"componentType":5120,"normalized":true,"count":3,"type":"VEC3"},)"
                    R"({"bufferView":2,"componentType":5123,"normalized":true,"count":3,"type":"VEC2"},)"
                    R"({"bufferView":3,"componentType":5123,"count":3,"type":"SCALAR"}],)"
                    R"("meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TEXCOORD_0":2},"indices":3}]}],)"
                    R"("nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";
            auto source = json.str();
            while( source.size() % 4 ) source.push_back( ' ' );
            while( bytes.size() % 4 ) bytes.push_back( 0 );
            std::vector<unsigned char> output;
            uint32( output, 0x46546c67 );
            uint32( output, 2 );
            uint32( output, static_cast<std::uint32_t>( 12 + 8 + source.size() + 8 + bytes.size() ) );
            uint32( output, static_cast<std::uint32_t>( source.size() ) );
            uint32( output, 0x4e4f534a );
            output.insert( output.end(), source.begin(), source.end() );
            uint32( output, static_cast<std::uint32_t>( bytes.size() ) );
            uint32( output, 0x004e4942 );
            output.insert( output.end(), bytes.begin(), bytes.end() );
            return output;
        }
    };

    std::vector<unsigned char> fixture( bool sequence = false, bool floatingPositions = false,
                                       Fault fault = Fault::None )
    {
        Fixture value;
        if( floatingPositions )
        {
            const float positions[] = { -1, 0, 0, 1, 0, 0, 0, 1, 0 };
            std::array<std::uint32_t, 9> encoded{};
            meshopt_encodeFilterExp( encoded.data(), 3, 12, 24, positions, meshopt_EncodeExpSeparate );
            value.attributes( encoded.data(), 3, 12, "EXPONENTIAL" );
        }
        else
        {
            const std::int16_t positions[3][4] = { { -32768, 0, 0, 0 }, { 32767, 0, 0, 0 }, { 0, 32767, 0, 0 } };
            value.attributes( positions, 3, 8 );
        }
        const float normals[] = { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
        std::array<std::int8_t, 12> encodedNormals{};
        meshopt_encodeFilterOct( encodedNormals.data(), 3, 4, 8, normals );
        value.attributes( encodedNormals.data(), 3, 4, "OCTAHEDRAL" );
        const std::uint16_t uvs[3][2] = { { 0, 0 }, { 65535, 0 }, { 0, 65535 } };
        value.attributes( uvs, 3, 4 );
        value.indices( sequence );
        return value.glb( floatingPositions, fault );
    }

    void verifyGeometry( bool sequence, bool floatingPositions )
    {
        Assimp::Importer importer;
        auto data = fixture( sequence, floatingPositions );
        const auto scene = importer.ReadFileFromMemory( data.data(), data.size(), 0, "glb" );
        if( !scene ) throw std::runtime_error( importer.GetErrorString() );
        require( scene->mNumMeshes == 1, "compressed fixture must produce one mesh" );
        const auto &mesh = *scene->mMeshes[0];
        require( mesh.mNumVertices == 3 && mesh.mNumFaces == 1, "decoded geometry counts changed" );
        require( mesh.mFaces[0].mNumIndices == 3, "decoded primitive must remain a triangle" );
        for( unsigned int i = 0; i < 3; ++i )
            require( mesh.mFaces[0].mIndices[i] == i, "decoded triangle indices changed" );
        require( close( mesh.mVertices[0].x, -1 ) && close( mesh.mVertices[0].y, 0 ) &&
                 close( mesh.mVertices[1].x, 1 ) && close( mesh.mVertices[2].y, 1 ),
                 "decoded positions must convert signed normalized extrema to floats" );
        require( mesh.HasNormals() && mesh.HasTextureCoords( 0 ), "decoded attributes disappeared" );
        for( unsigned int i = 0; i < 3; ++i )
            require( close( mesh.mNormals[i].x, 0 ) && close( mesh.mNormals[i].y, 0 ) &&
                     close( mesh.mNormals[i].z, 1 ), "octahedral normals must decode to unit Z" );
        require( close( mesh.mTextureCoords[0][0].x, 0 ) && close( mesh.mTextureCoords[0][0].y, 1 ) &&
                 close( mesh.mTextureCoords[0][1].x, 1 ) && close( mesh.mTextureCoords[0][2].y, 0 ) &&
                 close( mesh.mTextureCoords[0][2].z, 0 ),
                 "quantized UV conversion must retain Assimp's Y flip and zero unused components" );
    }

    void rejectMalformed( Fault fault )
    {
        Assimp::Importer importer;
        // InvalidFilterStride uses VEC3 floats (stride 12), illegal for QUATERNION.
        auto data = fixture( false, fault == Fault::InvalidFilterStride, fault );
        require( importer.ReadFileFromMemory( data.data(), data.size(), 0, "glb" ) == nullptr,
                 "malformed compressed fixture must fail import" );
        require( std::string( importer.GetErrorString() ).find( "meshopt" ) != std::string::npos,
                 "malformed compression must report its meshopt error" );
    }

    void verifyUncompressedQuantized( bool normalized )
    {
        const std::int16_t positions[3][4] = { { -32768, 0, 0, 0 }, { 32767, 0, 0, 0 }, { 0, 32767, 0, 0 } };
        const std::int8_t normals[3][4] = { { 0, 0, -128, 0 }, { 0, 0, 127, 0 }, { 0, 127, 0, 0 } };
        const std::uint16_t uvs[3][2] = { { 0, 0 }, { 65535, 0 }, { 0, 65535 } };
        const std::uint16_t indices[] = { 0, 1, 2 };
        std::vector<unsigned char> binary;
        const auto append = [&binary]( const void *data, size_t size ) {
            const auto bytes = static_cast<const unsigned char *>( data );
            binary.insert( binary.end(), bytes, bytes + size );
        };
        append( positions, sizeof( positions ) );
        append( normals, sizeof( normals ) );
        append( uvs, sizeof( uvs ) );
        append( indices, sizeof( indices ) );
        std::string json = R"({"asset":{"version":"2.0"},"extensionsUsed":["KHR_mesh_quantization"],)"
                           R"("extensionsRequired":["KHR_mesh_quantization"],"buffers":[{"byteLength":54}],)"
                           R"("bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":24,"byteStride":8},)"
                           R"({"buffer":0,"byteOffset":24,"byteLength":12,"byteStride":4},)"
                           R"({"buffer":0,"byteOffset":36,"byteLength":12,"byteStride":4},)"
                           R"({"buffer":0,"byteOffset":48,"byteLength":6}],)"
                           R"("accessors":[{"bufferView":0,"componentType":5122,"normalized":)";
        json += normalized ? "true" : "false";
        json += R"(,"count":3,"type":"VEC3","min":[-32768,0,0],"max":[32767,32767,0]},)"
                R"({"bufferView":1,"componentType":5120,"normalized":true,"count":3,"type":"VEC3"},)"
                R"({"bufferView":2,"componentType":5123,"normalized":true,"count":3,"type":"VEC2"},)"
                R"({"bufferView":3,"componentType":5123,"count":3,"type":"SCALAR"}],)"
                R"("meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TEXCOORD_0":2},"indices":3}]}],)"
                R"("nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";
        while( json.size() % 4 ) json.push_back( ' ' );
        while( binary.size() % 4 ) binary.push_back( 0 );
        std::vector<unsigned char> glb;
        Fixture::uint32( glb, 0x46546c67 );
        Fixture::uint32( glb, 2 );
        Fixture::uint32( glb, static_cast<std::uint32_t>( 28 + json.size() + binary.size() ) );
        Fixture::uint32( glb, static_cast<std::uint32_t>( json.size() ) );
        Fixture::uint32( glb, 0x4e4f534a );
        glb.insert( glb.end(), json.begin(), json.end() );
        Fixture::uint32( glb, static_cast<std::uint32_t>( binary.size() ) );
        Fixture::uint32( glb, 0x004e4942 );
        glb.insert( glb.end(), binary.begin(), binary.end() );
        Assimp::Importer importer;
        const auto scene = importer.ReadFileFromMemory( glb.data(), glb.size(), 0, "glb" );
        if( !scene ) throw std::runtime_error( importer.GetErrorString() );
        require( scene->mNumMeshes == 1, "uncompressed quantized fixture must import" );
        const auto &mesh = *scene->mMeshes[0];
        require( close( mesh.mVertices[0].x, normalized ? -1.f : -32768.f ) &&
                 close( mesh.mVertices[1].x, normalized ? 1.f : 32767.f ),
                 "signed quantization normalization flag must be respected" );
        require( mesh.HasNormals() && close( mesh.mNormals[0].z, -1 ) &&
                 close( mesh.mNormals[1].z, 1 ) && close( mesh.mNormals[2].y, 1 ),
                 "signed BYTE normalization must clamp -128 to -1 and map 127 to 1" );
        require( mesh.HasTextureCoords( 0 ) && close( mesh.mTextureCoords[0][1].x, 1 ) &&
                 close( mesh.mTextureCoords[0][2].y, 0 ),
                 "unsigned SHORT normalized UV extrema must convert without compression" );
    }

    void verifyAssets( const std::filesystem::path &directory )
    {
        size_t files = 0;
        for( const auto &entry : std::filesystem::directory_iterator( directory ) )
        {
            if( !entry.is_regular_file() || entry.path().extension() != ".glb" ) continue;
            Assimp::Importer importer;
            const auto scene = importer.ReadFile( entry.path().string(), 0 );
            if( !scene ) throw std::runtime_error( entry.path().string() + ": " + importer.GetErrorString() );
            require( scene->mNumMeshes > 0, "real compressed asset produced no meshes" );
            size_t vertices = 0, faces = 0;
            for( unsigned int m = 0; m < scene->mNumMeshes; ++m )
            {
                const auto &mesh = *scene->mMeshes[m];
                require( mesh.mNumVertices > 0 && mesh.mNumFaces > 0,
                         "real compressed asset produced an empty mesh" );
                vertices += mesh.mNumVertices;
                faces += mesh.mNumFaces;
                for( unsigned int v = 0; v < mesh.mNumVertices; ++v )
                {
                    const auto &position = mesh.mVertices[v];
                    require( std::isfinite( position.x ) && std::isfinite( position.y ) &&
                             std::isfinite( position.z ), "real asset has non-finite decoded positions" );
                    if( mesh.HasNormals() )
                    {
                        const auto &normal = mesh.mNormals[v];
                        require( std::isfinite( normal.x ) && std::isfinite( normal.y ) &&
                                 std::isfinite( normal.z ), "real asset has non-finite decoded normals" );
                    }
                }
                for( unsigned int f = 0; f < mesh.mNumFaces; ++f )
                {
                    const auto &face = mesh.mFaces[f];
                    require( face.mNumIndices == 3, "real car asset has a non-triangle face" );
                    for( unsigned int i = 0; i < face.mNumIndices; ++i )
                        require( face.mIndices[i] < mesh.mNumVertices, "real asset decoded an invalid index" );
                }
            }
            ++files;
            std::printf( "%s: meshes=%u vertices=%zu triangles=%zu textures=%u animations=%u\n",
                         entry.path().filename().string().c_str(), scene->mNumMeshes, vertices,
                         faces, scene->mNumTextures, scene->mNumAnimations );
        }
        require( files > 0, "asset directory contains no GLB files" );
        std::printf( "Validated %zu GLB assets\n", files );
    }
}

int main( int argc, char **argv )
{
    try
    {
        if( argc == 3 && std::string( argv[1] ) == "--assets" )
        {
            verifyAssets( argv[2] );
            return 0;
        }
        require( argc == 1, "usage: WPAssimpCompressedImportTests [--assets <directory>]" );
        verifyGeometry( false, false );
        verifyGeometry( true, false );
        verifyGeometry( false, true );
        verifyUncompressedQuantized( true );
        verifyUncompressedQuantized( false );
        for( auto fault : { Fault::CorruptPayload, Fault::SourceOutsideBuffer, Fault::InconsistentCount,
                           Fault::UnknownMode, Fault::UnknownFilter, Fault::InvalidFilterStride } )
            rejectMalformed( fault );
        std::puts( "Assimp compressed import: 5 geometry contracts and 6 malformed input contracts passed" );
        return 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "Assimp compressed import failed: %s\n", error.what() );
        return 1;
    }
}
