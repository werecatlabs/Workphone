#pragma once

#include <boost/json.hpp>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

namespace workphone::mesh_generation::detail
{
    namespace json = boost::json;
    constexpr std::size_t maximumJsonBytes = 1024u * 1024u;

    struct UniqueKeys
    {
        static constexpr std::size_t max_object_size = maximumJsonBytes;
        static constexpr std::size_t max_array_size = maximumJsonBytes;
        static constexpr std::size_t max_key_size = 1024;
        static constexpr std::size_t max_string_size = maximumJsonBytes;
        std::vector<std::unordered_set<std::string>> keys;
        std::string partial;
        bool on_object_begin( boost::system::error_code & ) { keys.emplace_back(); return true; }
        bool on_object_end( std::size_t, boost::system::error_code & ) { keys.pop_back(); return true; }
        bool on_key_part( json::string_view s, std::size_t, boost::system::error_code & )
        { partial.append( s.data(), s.size() ); return true; }
        bool on_key( json::string_view s, std::size_t, boost::system::error_code &error )
        {
            partial.append( s.data(), s.size() );
            const bool unique = keys.back().insert( partial ).second;
            partial.clear();
            if( !unique ) error = json::error::syntax;
            return unique;
        }
        bool on_document_begin( boost::system::error_code & ) { return true; }
        bool on_document_end( boost::system::error_code & ) { return true; }
        bool on_array_begin( boost::system::error_code & ) { return true; }
        bool on_array_end( std::size_t, boost::system::error_code & ) { return true; }
        bool on_string_part( json::string_view, std::size_t, boost::system::error_code & ) { return true; }
        bool on_string( json::string_view, std::size_t, boost::system::error_code & ) { return true; }
        bool on_number_part( json::string_view, boost::system::error_code & ) { return true; }
        bool on_int64( std::int64_t, json::string_view, boost::system::error_code & ) { return true; }
        bool on_uint64( std::uint64_t, json::string_view, boost::system::error_code & ) { return true; }
        bool on_double( double, json::string_view, boost::system::error_code & ) { return true; }
        bool on_bool( bool, boost::system::error_code & ) { return true; }
        bool on_null( boost::system::error_code & ) { return true; }
        bool on_comment_part( json::string_view, boost::system::error_code & ) { return true; }
        bool on_comment( json::string_view, boost::system::error_code & ) { return true; }
    };

    inline bool parseJson( const char *bytes, std::size_t size, json::value &value )
    {
        if( !bytes || size == 0 || size > maximumJsonBytes ) return false;
        boost::system::error_code error;
        json::parse_options options;
        options.max_depth = 32;
        json::basic_parser<UniqueKeys> validator( options );
        const auto consumed = validator.write_some( false, bytes, size, error );
        if( error || !validator.done() || consumed != size ) return false;
        value = json::parse( json::string_view( bytes, size ), error, {}, options );
        return !error;
    }

    inline std::string stringField( const json::object &object, const char *key )
    {
        const auto field = object.if_contains( key );
        if( !field || !field->is_string() ) return {};
        return std::string( field->as_string().data(), field->as_string().size() );
    }

    inline bool unsignedValue( const json::value *value, std::uint64_t &number )
    {
        if( !value ) return false;
        if( value->is_uint64() ) { number = value->as_uint64(); return true; }
        if( value->is_int64() && value->as_int64() >= 0 )
        { number = static_cast<std::uint64_t>( value->as_int64() ); return true; }
        return false;
    }

    inline bool optionalUnsigned( const json::object &object, const char *key,
                                  std::uint64_t &number, std::uint64_t fallback = 0 )
    {
        const auto field = object.if_contains( key );
        if( !field ) { number = fallback; return true; }
        return unsignedValue( field, number );
    }

    inline std::uint32_t little( const std::uint8_t *p )
    { return p[0] | ( std::uint32_t( p[1] ) << 8 ) | ( std::uint32_t( p[2] ) << 16 ) | ( std::uint32_t( p[3] ) << 24 ); }
    inline std::uint32_t big( const std::uint8_t *p )
    { return ( std::uint32_t( p[0] ) << 24 ) | ( std::uint32_t( p[1] ) << 16 ) | ( std::uint32_t( p[2] ) << 8 ) | p[3]; }

    inline bool imageDimensions( std::uint32_t width, std::uint32_t height )
    { return width >= 32 && height >= 32 && std::uint64_t( width ) * height <= 178956970u; }

    inline std::uint32_t crc32( const std::uint8_t *bytes, std::size_t size )
    {
        std::uint32_t crc = 0xffffffffu;
        for( std::size_t i = 0; i < size; ++i )
        {
            crc ^= bytes[i];
            for( unsigned bit = 0; bit < 8; ++bit )
                crc = ( crc >> 1 ) ^ ( 0xedb88320u & ( 0u - ( crc & 1u ) ) );
        }
        return crc ^ 0xffffffffu;
    }

    inline bool png( const std::uint8_t *bytes, std::size_t size )
    {
        static constexpr std::uint8_t signature[] = {137,80,78,71,13,10,26,10};
        if( size < 57 || !std::equal( std::begin( signature ), std::end( signature ), bytes ) ) return false;
        bool header = false, data = false;
        for( std::size_t offset = 8; offset <= size - 12; )
        {
            const std::uint32_t length = big( bytes + offset );
            if( length > size - offset - 12 ) return false;
            const auto type = big( bytes + offset + 4 );
            if( crc32( bytes + offset + 4, std::size_t( length ) + 4 ) != big( bytes + offset + 8 + length ) ) return false;
            if( !header )
            {
                if( type != 0x49484452 || length != 13 ||
                    !imageDimensions( big( bytes + offset + 8 ), big( bytes + offset + 12 ) ) ) return false;
                header = true;
            }
            else if( type == 0x49484452 ) return false;
            if( type == 0x49444154 ) data = true;
            if( type == 0x49454e44 ) return data && length == 0 && offset + 12 == size;
            offset += std::size_t( length ) + 12;
        }
        return false;
    }

    inline bool jpeg( const std::uint8_t *bytes, std::size_t size )
    {
        if( size < 12 || bytes[0] != 0xff || bytes[1] != 0xd8 ||
            bytes[size - 2] != 0xff || bytes[size - 1] != 0xd9 ) return false;
        bool frame = false;
        for( std::size_t offset = 2; offset + 3 < size; )
        {
            if( bytes[offset++] != 0xff ) return false;
            while( offset < size && bytes[offset] == 0xff ) ++offset;
            if( offset >= size ) return false;
            const auto marker = bytes[offset++];
            if( marker == 0xda ) return frame; // Entropy-coded data is checked by Meshy.
            if( marker == 0 || marker == 0xd8 || marker == 0xd9 || marker == 0x01 ||
                ( marker >= 0xd0 && marker <= 0xd7 ) || offset + 2 > size ) return false;
            const std::size_t length = ( std::size_t( bytes[offset] ) << 8 ) | bytes[offset + 1];
            if( length < 2 || length > size - offset ) return false;
            const bool sof = ( marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc );
            if( sof )
            {
                if( length < 8 || !imageDimensions( ( bytes[offset + 5] << 8 ) | bytes[offset + 6],
                                                   ( bytes[offset + 3] << 8 ) | bytes[offset + 4] ) ) return false;
                frame = true;
            }
            offset += length;
        }
        return false;
    }

    inline bool hasUri( const json::value &value )
    {
        if( value.is_object() )
            for( const auto &entry : value.as_object() )
                if( entry.key() == "uri" || hasUri( entry.value() ) ) return true;
        if( value.is_array() )
            for( const auto &entry : value.as_array() ) if( hasUri( entry ) ) return true;
        return false;
    }

    struct Accessor
    {
        std::uint64_t count = 0, component = 0;
        std::string type;
    };

    inline bool glb( const std::uint8_t *bytes, std::size_t size )
    {
        if( !bytes || size < 48 || little( bytes ) != 0x46546c67 || little( bytes + 4 ) != 2 ||
            little( bytes + 8 ) != size ) return false;
        const auto jsonSize = little( bytes + 12 );
        if( jsonSize % 4 || jsonSize > size - 28 || little( bytes + 16 ) != 0x4e4f534a ) return false;
        const std::size_t binOffset = 20u + jsonSize;
        const auto binSize = little( bytes + binOffset );
        if( little( bytes + binOffset + 4 ) != 0x004e4942 || binSize % 4 ||
            std::size_t( binSize ) != size - binOffset - 8 ) return false;
        json::value root;
        if( !parseJson( reinterpret_cast<const char *>( bytes + 20 ), jsonSize, root ) ||
            !root.is_object() || hasUri( root ) ) return false;
        const auto &object = root.as_object();
        const auto asset = object.if_contains( "asset" );
        if( !asset || !asset->is_object() || stringField( asset->as_object(), "version" ) != "2.0" ) return false;
        const auto buffers = object.if_contains( "buffers" );
        const auto views = object.if_contains( "bufferViews" );
        const auto accessors = object.if_contains( "accessors" );
        const auto meshes = object.if_contains( "meshes" );
        if( !buffers || !buffers->is_array() || buffers->as_array().size() != 1 || !buffers->as_array()[0].is_object() ||
            !views || !views->is_array() || views->as_array().empty() ||
            !accessors || !accessors->is_array() || accessors->as_array().empty() ||
            !meshes || !meshes->is_array() || meshes->as_array().empty() ) return false;
        std::uint64_t bufferLength = 0;
        if( !unsignedValue( buffers->as_array()[0].as_object().if_contains( "byteLength" ), bufferLength ) ||
            !bufferLength || bufferLength > binSize || binSize - bufferLength > 3 ) return false;
        for( const auto &view : views->as_array() )
        {
            std::uint64_t buffer = 0, offset = 0, length = 0;
            if( !view.is_object() || !unsignedValue( view.as_object().if_contains( "buffer" ), buffer ) || buffer != 0 ||
                !optionalUnsigned( view.as_object(), "byteOffset", offset ) ||
                !unsignedValue( view.as_object().if_contains( "byteLength" ), length ) || length == 0 ||
                offset > bufferLength || length > bufferLength - offset ) return false;
        }
        std::vector<Accessor> metadata;
        metadata.reserve( accessors->as_array().size() );
        for( const auto &entry : accessors->as_array() )
        {
            std::uint64_t viewIndex = 0, offset = 0, length = 0, stride = 0;
            Accessor info;
            if( !entry.is_object() ) return false;
            const auto &accessor = entry.as_object();
            if( accessor.if_contains( "sparse" ) ||
                !unsignedValue( accessor.if_contains( "bufferView" ), viewIndex ) || viewIndex >= views->as_array().size() ||
                !unsignedValue( accessor.if_contains( "count" ), info.count ) || info.count == 0 ||
                !unsignedValue( accessor.if_contains( "componentType" ), info.component ) ||
                !optionalUnsigned( accessor, "byteOffset", offset ) ) return false;
            info.type = stringField( accessor, "type" );
            const std::uint64_t components = info.type == "SCALAR" ? 1 : info.type == "VEC2" ? 2 :
                                             info.type == "VEC3" ? 3 : info.type == "VEC4" ? 4 :
                                             info.type == "MAT4" ? 16 : 0;
            const std::uint64_t width = info.component == 5120 || info.component == 5121 ? 1 :
                                       info.component == 5122 || info.component == 5123 ? 2 :
                                       info.component == 5125 || info.component == 5126 ? 4 : 0;
            if( !components || !width || offset % width ) return false;
            const auto &view = views->as_array()[static_cast<std::size_t>( viewIndex )].as_object();
            if( !unsignedValue( view.if_contains( "byteLength" ), length ) ||
                !optionalUnsigned( view, "byteStride", stride, components * width ) ||
                stride < components * width || stride % width || stride > 252 ||
                offset > length || components * width > length - offset ||
                info.count - 1 > ( length - offset - components * width ) / stride ) return false;
            metadata.push_back( info );
        }
        bool triangles = false;
        for( const auto &mesh : meshes->as_array() )
        {
            if( !mesh.is_object() ) return false;
            const auto primitives = mesh.as_object().if_contains( "primitives" );
            if( !primitives || !primitives->is_array() || primitives->as_array().empty() ) return false;
            for( const auto &primitive : primitives->as_array() )
            {
                if( !primitive.is_object() ) return false;
                const auto &fields = primitive.as_object();
                const auto attributes = fields.if_contains( "attributes" );
                std::uint64_t position = 0, mode = 4, indices = 0;
                if( !attributes || !attributes->is_object() ||
                    !unsignedValue( attributes->as_object().if_contains( "POSITION" ), position ) || position >= metadata.size() ||
                    !optionalUnsigned( fields, "mode", mode, 4 ) || mode != 4 ) return false;
                const auto &vertices = metadata[static_cast<std::size_t>( position )];
                if( vertices.type != "VEC3" || vertices.component != 5126 || vertices.count < 3 ) return false;
                if( const auto indexed = fields.if_contains( "indices" ) )
                {
                    if( !unsignedValue( indexed, indices ) || indices >= metadata.size() ) return false;
                    const auto &elements = metadata[static_cast<std::size_t>( indices )];
                    if( elements.type != "SCALAR" || ( elements.component != 5121 && elements.component != 5123 && elements.component != 5125 ) ||
                        elements.count < 3 || elements.count % 3 ) return false;
                }
                else if( vertices.count % 3 ) return false;
                triangles = true;
            }
        }
        return triangles;
    }
}
