#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Property.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace workphone
{
    namespace
    {
        constexpr std::array<u8, 8> binaryMagic = { 'F', 'B', 'S', 'C', 'N', 'B', 'I', 'N' };
        constexpr u16 binaryFlags = 0;
        constexpr size_Num headerSize =
            binaryMagic.size() + sizeof( u16 ) + sizeof( u16 ) + sizeof( u64 );
        constexpr u32 maximumStringBytes = 16u * 1024u * 1024u;
        constexpr u32 maximumItemsPerGroup = 1024u * 1024u;
        constexpr u32 maximumTreeDepth = 256u;

        class BinaryWriter
        {
        public:
            void writeU8( u8 value )
            {
                m_bytes.push_back( value );
            }

            void writeU16( u16 value )
            {
                for( u32 i = 0; i < sizeof( value ); ++i )
                {
                    writeU8( static_cast<u8>( value >> ( i * 8u ) ) );
                }
            }

            void writeU32( u32 value )
            {
                for( u32 i = 0; i < sizeof( value ); ++i )
                {
                    writeU8( static_cast<u8>( value >> ( i * 8u ) ) );
                }
            }

            void writeU64( u64 value )
            {
                for( u32 i = 0; i < sizeof( value ); ++i )
                {
                    writeU8( static_cast<u8>( value >> ( i * 8u ) ) );
                }
            }

            void writeString( const String &value )
            {
                if( value.size() > maximumStringBytes )
                {
                    throw std::length_error( "Binary scene string exceeds the format safety limit." );
                }

                writeU32( static_cast<u32>( value.size() ) );
                m_bytes.insert( m_bytes.end(), value.begin(), value.end() );
            }

            void writeBytes( const u8 *bytes, size_Num size )
            {
                m_bytes.insert( m_bytes.end(), bytes, bytes + size );
            }

            void writeProperties( const Properties &properties, u32 depth )
            {
                if( depth > maximumTreeDepth )
                {
                    throw std::length_error( "Binary scene Properties hierarchy is too deep." );
                }

                writeString( properties.getName() );

                auto propertyList = properties.getPropertiesAsArray();
                if( propertyList.size() > std::numeric_limits<u32>::max() )
                {
                    throw std::length_error( "Binary scene has too many properties." );
                }

                writeU32( static_cast<u32>( propertyList.size() ) );
                for( const auto &property : propertyList )
                {
                    writeString( property.getName() );
                    writeString( property.getValue() );
                    writeString( property.getTypeName() );
                    writeU8( property.isReadOnly() ? 1u : 0u );

                    auto attributes = property.getAttributes();
                    if( attributes.size() > std::numeric_limits<u32>::max() )
                    {
                        throw std::length_error( "Binary scene property has too many attributes." );
                    }

                    writeU32( static_cast<u32>( attributes.size() ) );
                    for( const auto &attribute : attributes )
                    {
                        writeString( attribute.first );
                        writeString( attribute.second );
                    }
                }

                auto children = properties.getChildren();
                if( children.size() > std::numeric_limits<u32>::max() )
                {
                    throw std::length_error( "Binary scene has too many child groups." );
                }

                writeU32( static_cast<u32>( children.size() ) );
                for( const auto &child : children )
                {
                    if( !child )
                    {
                        throw std::runtime_error(
                            "Binary scene cannot serialize a null Properties child." );
                    }

                    writeProperties( *child, depth + 1u );
                }
            }

            Array<u8> takeBytes()
            {
                return std::move( m_bytes );
            }

        private:
            Array<u8> m_bytes;
        };

        class BinaryReader
        {
        public:
            BinaryReader( const u8 *bytes, size_Num size ) : m_bytes( bytes ), m_size( size )
            {
            }

            u8 readU8()
            {
                require( sizeof( u8 ) );
                return m_bytes[m_position++];
            }

            u16 readU16()
            {
                u16 value = 0;
                for( u32 i = 0; i < sizeof( value ); ++i )
                {
                    value |= static_cast<u16>( readU8() ) << ( i * 8u );
                }
                return value;
            }

            u32 readU32()
            {
                u32 value = 0;
                for( u32 i = 0; i < sizeof( value ); ++i )
                {
                    value |= static_cast<u32>( readU8() ) << ( i * 8u );
                }
                return value;
            }

            u64 readU64()
            {
                u64 value = 0;
                for( u32 i = 0; i < sizeof( value ); ++i )
                {
                    value |= static_cast<u64>( readU8() ) << ( i * 8u );
                }
                return value;
            }

            String readString()
            {
                const auto length = readU32();
                if( length > maximumStringBytes )
                {
                    throw std::runtime_error( "Binary scene string exceeds the safety limit." );
                }

                require( length );
                String value( reinterpret_cast<const c8 *>( m_bytes + m_position ), length );
                m_position += length;
                return value;
            }

            void readMagic()
            {
                require( binaryMagic.size() );
                if( std::memcmp( m_bytes + m_position, binaryMagic.data(), binaryMagic.size() ) != 0 )
                {
                    throw std::runtime_error( "File is not a binary FireBlade scene." );
                }
                m_position += binaryMagic.size();
            }

            SmartPtr<Properties> readProperties( u32 depth )
            {
                if( depth > maximumTreeDepth )
                {
                    throw std::runtime_error( "Binary scene Properties hierarchy is too deep." );
                }

                auto properties = workphone::make_ptr<Properties>();
                properties->setName( readString() );

                const auto propertyCount = readCount( "property" );
                for( u32 i = 0; i < propertyCount; ++i )
                {
                    Property property;
                    property.setName( readString() );
                    property.setValue( readString() );
                    property.setTypeName( readString() );

                    const auto readOnly = readU8();
                    if( readOnly > 1u )
                    {
                        throw std::runtime_error( "Binary scene has an invalid boolean value." );
                    }
                    property.setReadOnly( readOnly != 0u );

                    const auto attributeCount = readCount( "attribute" );
                    for( u32 attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex )
                    {
                        auto name = readString();
                        auto value = readString();
                        property.setAttribute( name, value );
                    }

                    properties->addProperty( property );
                }

                const auto childCount = readCount( "child" );
                for( u32 i = 0; i < childCount; ++i )
                {
                    properties->addChild( readProperties( depth + 1u ) );
                }

                return properties;
            }

            size_Num position() const
            {
                return m_position;
            }

            size_Num remaining() const
            {
                return m_size - m_position;
            }

        private:
            u32 readCount( const c8 *description )
            {
                const auto count = readU32();
                if( count > maximumItemsPerGroup )
                {
                    throw std::runtime_error( String( "Binary scene " ) + description +
                                              " count exceeds the safety limit." );
                }
                return count;
            }

            void require( size_Num byteCount ) const
            {
                if( byteCount > m_size - m_position )
                {
                    throw std::runtime_error( "Binary scene is truncated." );
                }
            }

            const u8 *m_bytes = nullptr;
            size_Num m_size = 0;
            size_Num m_position = 0;
        };

        void setError( String *error, const String &message )
        {
            if( error )
            {
                *error = message;
            }
        }
    }  // namespace

    Array<u8> PropertiesBinarySerializer::serialize( const Properties &properties )
    {
        BinaryWriter payloadWriter;
        payloadWriter.writeProperties( properties, 0u );
        auto payload = payloadWriter.takeBytes();

        BinaryWriter documentWriter;
        documentWriter.writeBytes( binaryMagic.data(), binaryMagic.size() );
        documentWriter.writeU16( formatVersion );
        documentWriter.writeU16( binaryFlags );
        documentWriter.writeU64( static_cast<u64>( payload.size() ) );
        documentWriter.writeBytes( payload.data(), payload.size() );
        return documentWriter.takeBytes();
    }

    bool PropertiesBinarySerializer::deserialize( const Array<u8> &bytes, Properties &properties,
                                                  String *error )
    {
        return deserialize( bytes.data(), bytes.size(), properties, error );
    }

    bool PropertiesBinarySerializer::deserialize( const u8 *bytes, size_Num size, Properties &properties,
                                                  String *error )
    {
        if( !bytes )
        {
            setError( error, "Binary scene data is null." );
            return false;
        }

        try
        {
            BinaryReader reader( bytes, size );
            reader.readMagic();

            const auto version = reader.readU16();
            if( version != formatVersion )
            {
                setError( error, "Unsupported binary scene version: " +
                                     std::to_string( static_cast<u32>( version ) ) );
                return false;
            }

            const auto flags = reader.readU16();
            if( flags != binaryFlags )
            {
                setError( error, "Binary scene uses unsupported format flags." );
                return false;
            }

            const auto payloadSize = reader.readU64();
            if( payloadSize != static_cast<u64>( reader.remaining() ) )
            {
                setError( error, "Binary scene payload size does not match the file size." );
                return false;
            }

            auto parsedProperties = reader.readProperties( 0u );
            if( reader.position() != size )
            {
                setError( error, "Binary scene contains trailing data." );
                return false;
            }

            properties = *parsedProperties;
            properties.setName( parsedProperties->getName() );
            if( error )
            {
                error->clear();
            }
            return true;
        }
        catch( const std::exception &exception )
        {
            setError( error, exception.what() );
            return false;
        }
    }

    bool PropertiesBinarySerializer::hasBinaryHeader( const u8 *bytes, size_Num size )
    {
        return bytes && size >= headerSize &&
               std::memcmp( bytes, binaryMagic.data(), binaryMagic.size() ) == 0;
    }
}  // namespace workphone
