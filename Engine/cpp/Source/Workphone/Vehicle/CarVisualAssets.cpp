#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/CarVisualAssets.hpp>
#include <meshoptimizer.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace workphone::vehiclevisual
{
    namespace
    {
        using Json = nlohmann::json;
        constexpr std::size_t limit = 512u * 1024u * 1024u;

        std::uint32_t u32( const std::vector<unsigned char> &data, std::size_t p )
        {
            if( p > data.size() || data.size() - p < 4 )
            {
                throw std::runtime_error( "Truncated CarVisual GLB" );
            }
            return data[p] | ( std::uint32_t( data[p + 1] ) << 8 ) |
                   ( std::uint32_t( data[p + 2] ) << 16 ) | ( std::uint32_t( data[p + 3] ) << 24 );
        }

        void append32( std::vector<unsigned char> &data, std::uint32_t v )
        {
            for( unsigned i = 0; i < 4; ++i )
            {
                data.push_back( static_cast<unsigned char>( v >> ( i * 8 ) ) );
            }
        }

        std::size_t append( std::vector<unsigned char> &out, const unsigned char *data,
                            std::size_t size )
        {
            while( out.size() % 4 )
            {
                out.push_back( 0 );
            }
            if( size > limit - out.size() )
            {
                throw std::runtime_error( "CarVisual GLB exceeds decoded size limit" );
            }
            auto offset = out.size();
            out.insert( out.end(), data, data + size );
            return offset;
        }

        void removeExtension( Json &doc, const char *name )
        {
            for( const char *key : { "extensionsUsed", "extensionsRequired" } )
            {
                if( doc.contains( key ) )
                {
                    auto &list = doc[key];
                    for( auto it = list.begin(); it != list.end(); )
                    {
                        if( *it == name )
                        {
                            it = list.erase( it );
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }
        }

        template <class T>
        T read( const unsigned char *p )
        {
            T value;
            std::memcpy( &value, p, sizeof( value ) );
            return value;
        }
    }  // namespace

    std::string prepareCarVisualGLB( const std::string &sourcePath, const std::string &cacheDirectory )
    {
        auto source = std::filesystem::u8path( sourcePath );
        auto extension = source.extension().u8string();
        std::transform( extension.begin(), extension.end(), extension.begin(),
                        []( unsigned char c ) { return char( std::tolower( c ) ); } );
        if( extension != ".glb" )
        {
            return sourcePath;
        }
        std::ifstream input( source, std::ios::binary | std::ios::ate );
        if( !input )
        {
            throw std::runtime_error( "Cannot read CarVisual GLB: " + sourcePath );
        }
        auto length = input.tellg();
        if( length < 20 || length > static_cast<std::streamoff>( limit ) )
        {
            throw std::runtime_error( "Invalid CarVisual GLB size" );
        }
        std::vector<unsigned char> data( static_cast<std::size_t>( length ) );
        input.seekg( 0 );
        input.read( reinterpret_cast<char *>( data.data() ), length );
        if( !input || u32( data, 0 ) != 0x46546c67 || u32( data, 4 ) != 2 ||
            u32( data, 8 ) != data.size() )
        {
            throw std::runtime_error( "Invalid CarVisual GLB header" );
        }
        Json doc;
        std::vector<unsigned char> original;
        for( std::size_t p = 12; p < data.size(); )
        {
            auto size = u32( data, p ), type = u32( data, p + 4 );
            p += 8;
            if( size > data.size() - p )
            {
                throw std::runtime_error( "Invalid CarVisual GLB chunk" );
            }
            if( type == 0x4e4f534a )
            {
                doc = Json::parse( data.begin() + p, data.begin() + p + size );
            }
            else if( type == 0x004e4942 )
            {
                original.assign( data.begin() + p, data.begin() + p + size );
            }
            p += size;
        }
        if( !doc.is_object() || !doc.contains( "bufferViews" ) )
        {
            throw std::runtime_error( "CarVisual GLB has no buffer views" );
        }
        bool compressed = false;
        for( const auto &view : doc["bufferViews"] )
        {
            if( view.contains( "extensions" ) &&
                view["extensions"].contains( "EXT_meshopt_compression" ) )
            {
                compressed = true;
            }
        }
        if( !compressed )
        {
            return sourcePath;
        }
        // Content identity includes the converter schema/version; no path-only stale cache.
        std::uint64_t hash = 14695981039346656037ull;
        for( auto byte : data )
        {
            hash ^= byte;
            hash *= 1099511628211ull;
        }
        std::ostringstream key;
        key << "meshopt022-float-v3-" << std::hex << hash << ".glb";
        auto directory = std::filesystem::absolute( std::filesystem::u8path( cacheDirectory ) );
        std::filesystem::create_directories( directory );
        auto destination = directory / key.str();
        auto imageName = [&]( std::size_t index, const Json &image ) {
            auto mime = image.value( "mimeType", std::string{} );
            const char *suffix = mime == "image/png"    ? ".png"
                                 : mime == "image/jpeg" ? ".jpg"
                                 : mime == "image/webp" ? ".webp"
                                                        : nullptr;
            if( !suffix )
            {
                throw std::runtime_error( "Unsupported embedded CarVisual image type: " + mime );
            }
            return destination.stem().u8string() + "-image-" + std::to_string( index ) + suffix;
        };
        bool imagesCached = true;
        if( doc.contains( "images" ) )
        {
            for( std::size_t i = 0; i < doc["images"].size(); ++i )
            {
                const auto &image = doc["images"][i];
                if( image.contains( "bufferView" ) &&
                    !std::filesystem::is_regular_file( directory / imageName( i, image ) ) )
                {
                    imagesCached = false;
                }
            }
        }
        if( std::filesystem::is_regular_file( destination ) && imagesCached )
        {
            return destination.u8string();
        }
        auto binary = original;
        for( auto &view : doc["bufferViews"] )
        {
            if( !view.contains( "extensions" ) ||
                !view["extensions"].contains( "EXT_meshopt_compression" ) )
            {
                if( view.value( "buffer", 0 ) != 0 )
                {
                    throw std::runtime_error( "External GLB fallback buffers are unsupported" );
                }
                continue;
            }
            auto ext = view["extensions"]["EXT_meshopt_compression"];
            const auto count = ext.at( "count" ).get<std::size_t>(),
                       stride = ext.at( "byteStride" ).get<std::size_t>();
            const auto offset = ext.value( "byteOffset", std::size_t( 0 ) ),
                       size = ext.at( "byteLength" ).get<std::size_t>();
            if( ext.value( "buffer", 0 ) != 0 || !count || !stride || stride > 256 ||
                count > limit / stride || offset > original.size() || size > original.size() - offset )
            {
                throw std::runtime_error( "Invalid meshopt buffer range" );
            }
            auto mode = ext.at( "mode" ).get<std::string>(),
                 filter = ext.value( "filter", std::string( "NONE" ) );
            std::vector<unsigned char> decoded( count * stride );
            int status = -1;
            if( mode == "ATTRIBUTES" && stride % 4 == 0 )
            {
                status = meshopt_decodeVertexBuffer( decoded.data(), count, stride,
                                                     original.data() + offset, size );
            }
            else if( mode == "TRIANGLES" && ( stride == 2 || stride == 4 ) && count % 3 == 0 )
            {
                status = meshopt_decodeIndexBuffer( decoded.data(), count, stride,
                                                    original.data() + offset, size );
            }
            else if( mode == "INDICES" && ( stride == 2 || stride == 4 ) )
            {
                status = meshopt_decodeIndexSequence( decoded.data(), count, stride,
                                                      original.data() + offset, size );
            }
            if( status != 0 )
            {
                throw std::runtime_error( "Cannot decode meshopt GLB stream (codec v0.22)" );
            }
            if( filter == "OCTAHEDRAL" && ( stride == 4 || stride == 8 ) )
            {
                meshopt_decodeFilterOct( decoded.data(), count, stride );
            }
            else if( filter == "QUATERNION" && stride == 8 )
            {
                meshopt_decodeFilterQuat( decoded.data(), count, stride );
            }
            else if( filter == "EXPONENTIAL" && stride % 4 == 0 )
            {
                meshopt_decodeFilterExp( decoded.data(), count, stride );
            }
            else if( filter != "NONE" )
            {
                throw std::runtime_error( "Unsupported meshopt GLB filter" );
            }
            view["buffer"] = 0;
            view["byteOffset"] = append( binary, decoded.data(), decoded.size() );
            view["byteLength"] = decoded.size();
            view["extensions"].erase( "EXT_meshopt_compression" );
            if( view["extensions"].empty() )
            {
                view.erase( "extensions" );
            }
        }
        // Convert quantized primitive attributes, while retaining each node's scale
        // and translation. Index buffers and joint indices retain integer storage.
        std::set<std::size_t> attributes;
        for( const auto &mesh : doc["meshes"] )
        {
            for( const auto &primitive : mesh["primitives"] )
            {
                for( auto it = primitive["attributes"].begin(); it != primitive["attributes"].end();
                     ++it )
                {
                    if( it.key().rfind( "JOINTS_", 0 ) != 0 )
                    {
                        attributes.insert( it.value().get<std::size_t>() );
                    }
                }
            }
        }
        for( auto index : attributes )
        {
            auto &a = doc["accessors"].at( index );
            int type = a.at( "componentType" ).get<int>();
            if( type == 5126 )
            {
                continue;
            }
            auto shape = a.at( "type" ).get<std::string>();
            unsigned components = shape == "SCALAR" ? 1
                                  : shape == "VEC2" ? 2
                                  : shape == "VEC3" ? 3
                                  : shape == "VEC4" ? 4
                                                    : 0;
            unsigned bytes = type == 5120 || type == 5121   ? 1
                             : type == 5122 || type == 5123 ? 2
                             : type == 5125                 ? 4
                                                            : 0;
            if( !components || !bytes || a.contains( "sparse" ) )
            {
                throw std::runtime_error( "Unsupported quantized CarVisual attribute" );
            }
            const auto &view = doc["bufferViews"].at( a.at( "bufferView" ).get<std::size_t>() );
            auto count = a.at( "count" ).get<std::size_t>(),
                 stride = view.value( "byteStride", std::size_t( bytes * components ) );
            const auto viewOffset = view.value( "byteOffset", std::size_t( 0 ) );
            const auto viewLength = view.at( "byteLength" ).get<std::size_t>();
            const auto accessorOffset = a.value( "byteOffset", std::size_t( 0 ) );
            if( viewOffset > binary.size() || viewLength > binary.size() - viewOffset ||
                accessorOffset > viewLength )
            {
                throw std::runtime_error( "Invalid CarVisual buffer view range" );
            }
            auto offset = viewOffset + accessorOffset;
            if( !count || count > limit / ( components * sizeof( float ) ) || offset > binary.size() ||
                stride < bytes * components || ( count - 1 ) > ( binary.size() - offset ) / stride ||
                bytes * components > binary.size() - offset - ( count - 1 ) * stride ||
                ( count - 1 ) > ( viewLength - accessorOffset ) / stride ||
                bytes * components > viewLength - accessorOffset - ( count - 1 ) * stride )
            {
                throw std::runtime_error( "Invalid quantized CarVisual accessor" );
            }
            std::vector<float> values( count * components );
            bool normalized = a.value( "normalized", false );
            for( std::size_t i = 0; i < count; ++i )
            {
                for( unsigned c = 0; c < components; ++c )
                {
                    auto p = binary.data() + offset + i * stride + c * bytes;
                    double v = 0;
                    switch( type )
                    {
                    case 5120:
                        v = read<std::int8_t>( p );
                        if( normalized )
                        {
                            v = std::max( -1.0, v / 127 );
                        }
                        break;
                    case 5121:
                        v = *p;
                        if( normalized )
                        {
                            v /= 255;
                        }
                        break;
                    case 5122:
                        v = read<std::int16_t>( p );
                        if( normalized )
                        {
                            v = std::max( -1.0, v / 32767 );
                        }
                        break;
                    case 5123:
                        v = read<std::uint16_t>( p );
                        if( normalized )
                        {
                            v /= 65535;
                        }
                        break;
                    case 5125:
                        v = read<std::uint32_t>( p );
                        if( normalized )
                        {
                            v /= 4294967295.0;
                        }
                        break;
                    }
                    values[i * components + c] = static_cast<float>( v );
                }
            }
            auto start = append( binary, reinterpret_cast<const unsigned char *>( values.data() ),
                                 values.size() * sizeof( float ) );
            a["bufferView"] = doc["bufferViews"].size();
            a["byteOffset"] = 0;
            a["componentType"] = 5126;
            a.erase( "normalized" );
            std::vector<float> minimum( components, std::numeric_limits<float>::max() ),
                maximum( components, -std::numeric_limits<float>::max() );
            for( std::size_t i = 0; i < count; ++i )
            {
                for( unsigned c = 0; c < components; ++c )
                {
                    minimum[c] = std::min( minimum[c], values[i * components + c] );
                    maximum[c] = std::max( maximum[c], values[i * components + c] );
                }
            }
            a["min"] = minimum;
            a["max"] = maximum;
            doc["bufferViews"].push_back( { { "buffer", 0 },
                                            { "byteOffset", start },
                                            { "byteLength", values.size() * sizeof( float ) } } );
        }
        for( auto &texture : doc["textures"] )
        {
            if( texture.contains( "extensions" ) &&
                texture["extensions"].contains( "EXT_texture_webp" ) )
            {
                if( !texture.contains( "source" ) )
                {
                    texture["source"] = texture["extensions"]["EXT_texture_webp"]["source"];
                }
                texture["extensions"].erase( "EXT_texture_webp" );
                if( texture["extensions"].empty() )
                {
                    texture.erase( "extensions" );
                }
            }
        }
        removeExtension( doc, "EXT_meshopt_compression" );
        removeExtension( doc, "KHR_mesh_quantization" );
        removeExtension( doc, "EXT_texture_webp" );
        // The native Assimp material path skips embedded textures. Expose image
        // sidecars in the cache so it can register their normal resource paths.
        if( doc.contains( "images" ) )
        {
            for( std::size_t i = 0; i < doc["images"].size(); ++i )
            {
                auto &image = doc["images"][i];
                if( image.contains( "bufferView" ) )
                {
                    const auto &view = doc["bufferViews"].at( image["bufferView"].get<std::size_t>() );
                    const auto offset = view.value( "byteOffset", std::size_t( 0 ) );
                    const auto size = view.at( "byteLength" ).get<std::size_t>();
                    if( view.value( "buffer", 0 ) != 0 || offset > binary.size() ||
                        size > binary.size() - offset )
                    {
                        throw std::runtime_error( "Invalid embedded CarVisual image range" );
                    }
                    auto name = imageName( i, image );
                    std::ofstream output( directory / name, std::ios::binary | std::ios::trunc );
                    output.write( reinterpret_cast<const char *>( binary.data() + offset ), size );
                    if( !output )
                    {
                        throw std::runtime_error( "Cannot write CarVisual image cache" );
                    }
                    image.erase( "bufferView" );
                    image["uri"] = name;
                }
                else if( image.contains( "uri" ) )
                {
                    const auto uri = image["uri"].get<std::string>();
                    if( uri.rfind( "data:", 0 ) != 0 && uri.find( "://" ) == std::string::npos )
                    {
                        image["uri"] = std::filesystem::absolute( source.parent_path() /
                                                                  std::filesystem::u8path( uri ) )
                                           .generic_u8string();
                    }
                }
            }
        }
        doc["buffers"] = Json::array( { { { "byteLength", binary.size() } } } );
        auto text = doc.dump();
        while( text.size() % 4 )
        {
            text.push_back( ' ' );
        }
        while( binary.size() % 4 )
        {
            binary.push_back( 0 );
        }
        std::vector<unsigned char> result;
        append32( result, 0x46546c67 );
        append32( result, 2 );
        append32( result, static_cast<std::uint32_t>( 28 + text.size() + binary.size() ) );
        append32( result, static_cast<std::uint32_t>( text.size() ) );
        append32( result, 0x4e4f534a );
        result.insert( result.end(), text.begin(), text.end() );
        append32( result, static_cast<std::uint32_t>( binary.size() ) );
        append32( result, 0x004e4942 );
        result.insert( result.end(), binary.begin(), binary.end() );
        auto temporary = destination;
        temporary += ".tmp";
        {
            std::ofstream output( temporary, std::ios::binary | std::ios::trunc );
            output.write( reinterpret_cast<const char *>( result.data() ), result.size() );
            if( !output )
            {
                throw std::runtime_error( "Cannot write CarVisual GLB cache" );
            }
        }
        std::error_code error;
        std::filesystem::rename( temporary, destination, error );
        if( error && !std::filesystem::is_regular_file( destination ) )
        {
            throw std::runtime_error( "Cannot publish CarVisual GLB cache: " + error.message() );
        }
        return destination.u8string();
    }
}  // namespace workphone::vehiclevisual
