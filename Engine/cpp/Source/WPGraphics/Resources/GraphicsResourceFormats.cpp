#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <Workphone/Graphics/DdsTexture.hpp>

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace workphone::render
{
    u32 ddsFormatForTarget( const String &target )
    {
        if( target == "windows-dx11-dds-bgra8-v1" )
        {
            return 87;
        }
        if( target == "windows-dx11-dds-bgra8-srgb-v1" )
        {
            return 91;
        }
        if( target == "windows-dx11-dds-bc1-v1" )
        {
            return 71;
        }
        if( target == "windows-dx11-dds-bc1-srgb-v1" )
        {
            return 72;
        }
        if( target == "windows-dx11-dds-bc3-v1" )
        {
            return 77;
        }
        if( target == "windows-dx11-dds-bc3-srgb-v1" )
        {
            return 78;
        }
        if( target.find( "windows-dx11-dds-" ) == 0 )
        {
            throw std::runtime_error( "Unsupported Windows DDS texture profile" );
        }
        return 0;
    }

    namespace
    {
        constexpr u32 textureMagic = 0x58545057u;   // WPTX
        constexpr u32 ddsMagic = 0x58445057u;       // WPDX
        constexpr u32 materialMagic = 0x544d5057u;  // WPMT
        constexpr u32 maxTextureDimension = 16384;
        constexpr u32 maxMipLevels = 15;

        void require( bool condition, const char *message )
        {
            if( !condition )
            {
                throw std::runtime_error( message );
            }
        }

        void validateTarget( const String &target )
        {
            require( !target.empty() && target.size() <= 128, "Invalid cooked graphics target length" );
            for( const unsigned char c : target )
            {
                require( c >= 33 && c <= 126,
                         "Cooked graphics target must use printable ASCII without spaces" );
            }
        }

        void validateSettings( const TextureMipSettings &settings )
        {
            require( settings.filter >= TextureMipFilter::None &&
                         settings.filter <= TextureMipFilter::Roughness,
                     "Unknown cooked texture mip filter" );
            require( settings.atlasColumns > 0 && settings.atlasColumns <= maxTextureDimension &&
                         std::isfinite( settings.alphaCutoff ) && settings.alphaCutoff > 0.0f &&
                         settings.alphaCutoff < 1.0f,
                     "Invalid cooked texture mip settings" );
        }

        void validateTexture( const CookedTextureData &data )
        {
            validateTarget( data.target );
            validateSettings( data.mipSettings );
            if( !data.ddsBytes.empty() )
            {
                const auto image = dds::inspect( data.ddsBytes );
                require( data.levels.empty() && image.format == ddsFormatForTarget( data.target ),
                         "DDS artifact does not match its target profile" );
                require( image.alphaMode == ( dds::blockBytes( image.format ) == 8 ? 3u : 1u ),
                         "DDS alpha mode does not match the opaque BC1 or straight-alpha profile" );
                const auto &first = image.mips.front();
                const bool compressed = dds::blockBytes( image.format ) != 0;
                const bool srgb = image.format == 72 || image.format == 78 || image.format == 91;
                require( !compressed || ( first.width % 4 == 0 && first.height % 4 == 0 ),
                         "DX11 block-compressed base dimensions must be multiples of four" );
                require( !compressed || data.mipSettings.filter != TextureMipFilter::Normal,
                         "Compressed normal-map profiles are unsupported" );
                require( !srgb || data.mipSettings.filter == TextureMipFilter::Colour ||
                             data.mipSettings.filter == TextureMipFilter::Cutout ||
                             data.mipSettings.filter == TextureMipFilter::None,
                         "SRGB profile mip semantics are inconsistent" );
                require( first.width % data.mipSettings.atlasColumns == 0,
                         "DDS atlas dimensions are inconsistent" );
                auto width = first.width, height = first.height;
                size_t count = 1;
                while( data.mipSettings.filter != TextureMipFilter::None &&
                       ( width != 1 || height != 1 ) )
                {
                    width = std::max( width / 2, 1u );
                    height = std::max( height / 2, 1u );
                    if( width < data.mipSettings.atlasColumns || width % data.mipSettings.atlasColumns )
                    {
                        break;
                    }
                    ++count;
                }
                require( image.mips.size() == count, "DDS mip count does not match the recipe" );
                return;
            }
            require( ddsFormatForTarget( data.target ) == 0,
                     "Windows DDS profile requires a native DDS payload" );
            require( !data.levels.empty() && data.levels.size() <= maxMipLevels,
                     "Invalid cooked texture mip count" );
            const auto &first = data.levels.front();
            require( first.width && first.height && first.width <= maxTextureDimension &&
                         first.height <= maxTextureDimension &&
                         first.width % data.mipSettings.atlasColumns == 0,
                     "Invalid cooked texture dimensions" );
            u32 width = first.width, height = first.height;
            u64 total = 0;
            size_t expectedCount = 0;
            for( ;; )
            {
                require( expectedCount < data.levels.size(), "Cooked texture mip chain is incomplete" );
                const auto &level = data.levels[expectedCount++];
                const auto bytes = u64( width ) * height * 4u;
                require( bytes <= graphicsTextureByteLimit - total && level.width == width &&
                             level.height == height && level.bgra.size() == bytes,
                         "Cooked texture mip dimensions, length or aggregate size is invalid" );
                total += bytes;
                if( data.mipSettings.filter == TextureMipFilter::None || ( width == 1 && height == 1 ) )
                {
                    break;
                }
                width = std::max( width / 2u, 1u );
                height = std::max( height / 2u, 1u );
                if( width < data.mipSettings.atlasColumns || width % data.mipSettings.atlasColumns )
                {
                    break;
                }
            }
            require( expectedCount == data.levels.size(),
                     "Cooked texture contains unexpected mip levels" );
        }

        void validateMaterial( const CookedMaterialData &data )
        {
            validateTarget( data.target );
            for( const auto value : { data.baseColour.r, data.baseColour.g, data.baseColour.b,
                                      data.baseColour.a, data.metalness, data.roughness } )
            {
                require( std::isfinite( value ) && value >= 0.0f && value <= 1.0f,
                         "Cooked material values must be finite and within [0,1]" );
            }
            require( data.textureId.isValid() && !data.textureId.isSubResource() &&
                         data.textureId.type() == resource::ResourceTypeID( "texres" ) &&
                         data.textureId.str().size() <= 1024 && data.textureSourceHash &&
                         data.texturePayloadHash && data.textureCompilerVersion,
                     "Cooked material requires one pinned texres dependency" );
            for( const auto &ref : { data.normalTexture, data.ormTexture } )
            {
                if( !ref.id.isValid() )
                {
                    require( !ref.sourceHash && !ref.payloadHash && !ref.compilerVersion,
                             "Absent material texture slot contains generation metadata" );
                    continue;
                }
                require( !ref.id.isSubResource() && ref.id.type() == resource::ResourceTypeID( "texres" ) &&
                             ref.id.str().size() <= 1024 && ref.sourceHash && ref.payloadHash && ref.compilerVersion,
                         "Optional cooked material texture requires a pinned texres dependency" );
            }
        }

        void write32( std::ostream &output, u32 value )
        {
            for( unsigned i = 0; i < 4; ++i )
            {
                output.put( static_cast<char>( ( value >> ( i * 8u ) ) & 255u ) );
            }
        }

        void write64( std::ostream &output, u64 value )
        {
            for( unsigned i = 0; i < 8; ++i )
            {
                output.put( static_cast<char>( ( value >> ( i * 8u ) ) & 255u ) );
            }
        }

        void writeFloat( std::ostream &output, float value )
        {
            static_assert( sizeof( float ) == sizeof( u32 ) && std::numeric_limits<float>::is_iec559,
                           "Cooked graphics formats require IEEE-754 binary32" );
            u32 bits;
            std::memcpy( &bits, &value, sizeof( bits ) );
            write32( output, bits );
        }

        void writeString( std::ostream &output, const String &value )
        {
            write32( output, static_cast<u32>( value.size() ) );
            output.write( value.data(), static_cast<std::streamsize>( value.size() ) );
        }

        class Reader
        {
        public:
            explicit Reader( const Array<u8> &bytes ) : m_bytes( bytes )
            {
            }

            size_t remaining() const
            {
                return m_bytes.size() - m_position;
            }

            u32 read32()
            {
                require( remaining() >= 4, "Truncated cooked graphics payload" );
                u32 value = 0;
                for( unsigned i = 0; i < 4; ++i )
                {
                    value |= u32( m_bytes[m_position++] ) << ( i * 8u );
                }
                return value;
            }

            u64 read64()
            {
                require( remaining() >= 8, "Truncated cooked graphics payload" );
                u64 value = 0;
                for( unsigned i = 0; i < 8; ++i )
                {
                    value |= u64( m_bytes[m_position++] ) << ( i * 8u );
                }
                return value;
            }

            float readFloat()
            {
                const auto bits = read32();
                float value;
                std::memcpy( &value, &bits, sizeof( value ) );
                return value;
            }

            String readString( size_t limit )
            {
                const auto size = read32();
                require( size && size <= limit && size <= remaining(),
                         "Invalid cooked graphics string length" );
                String result( reinterpret_cast<const char *>( m_bytes.data() + m_position ), size );
                require( result.find( '\0' ) == String::npos, "Embedded NUL in cooked graphics string" );
                m_position += size;
                return result;
            }

            void readBytes( std::vector<std::uint8_t> &output, size_t size )
            {
                require( size <= remaining(), "Truncated cooked texture pixels" );
                output.assign( m_bytes.data() + m_position, m_bytes.data() + m_position + size );
                m_position += size;
            }

        private:
            const Array<u8> &m_bytes;
            size_t m_position = 0;
        };

        void validateResource( const resource::RuntimeResource &value, const char *type )
        {
            const resource::ResourceTypeID expected( type );
            const GraphicsResourceCompiler compiler;
            require( value.header.isValid() && !value.header.resourceId.isSubResource() &&
                         value.header.resourceId.type() == expected &&
                         value.header.resourceType == expected &&
                         ( value.header.compilerVersion == compiler.versionFor( expected ) ||
                           ( expected == resource::ResourceTypeID( "matres" ) && value.header.compilerVersion == 2 ) ),
                     "Cooked graphics resource type or compiler version is incompatible" );
            require( value.payload.size() <= graphicsTextureByteLimit + 4096u &&
                         value.header.payloadSize == value.payload.size() &&
                         resource::hashBytes( value.payload.data(), value.payload.size() ) ==
                             value.header.payloadHash,
                     "Cooked graphics payload size or integrity is invalid" );
        }

        u32 readPrefix( Reader &reader, u32 magic, String &target, bool &packagedBuild,
                        u32 maximumVersion = graphicsResourceFormatVersion )
        {
            require( reader.read32() == magic,
                     "Cooked graphics payload magic or format version is incompatible" );
            const auto version = reader.read32();
            require( version >= graphicsResourceFormatVersion && version <= maximumVersion,
                     "Cooked graphics payload format version is incompatible" );
            target = reader.readString( 128 );
            validateTarget( target );
            const auto mode = reader.read32();
            require( mode <= 1, "Unknown cooked graphics build mode" );
            packagedBuild = mode != 0;
            return version;
        }

        void writePrefix( std::ostream &output, u32 magic, const String &target, bool packagedBuild,
                          u32 version = graphicsResourceFormatVersion )
        {
            write32( output, magic );
            write32( output, version );
            writeString( output, target );
            write32( output, packagedBuild ? 1 : 0 );
        }
    }  // namespace

    bool validateCookedTextureData( const CookedTextureData &data, String &error )
    {
        error.clear();
        try
        {
            validateTexture( data );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool writeCookedTexture( const CookedTextureData &data, std::ostream &output, String &error )
    {
        error.clear();
        try
        {
            validateTexture( data );
            writePrefix( output, data.ddsBytes.empty() ? textureMagic : ddsMagic, data.target,
                         data.packagedBuild );
            write32( output, static_cast<u32>( data.mipSettings.filter ) );
            write32( output, data.mipSettings.atlasColumns );
            writeFloat( output, data.mipSettings.alphaCutoff );
            if( !data.ddsBytes.empty() )
            {
                write64( output, data.ddsBytes.size() );
                output.write( reinterpret_cast<const char *>( data.ddsBytes.data() ),
                              data.ddsBytes.size() );
                require( output.good(), "Failed writing native DDS payload" );
                return true;
            }
            write32( output, static_cast<u32>( data.levels.size() ) );
            for( const auto &level : data.levels )
            {
                write32( output, level.width );
                write32( output, level.height );
                write64( output, level.bgra.size() );
                output.write( reinterpret_cast<const char *>( level.bgra.data() ),
                              static_cast<std::streamsize>( level.bgra.size() ) );
            }
            require( output.good(), "Failed writing cooked texture payload" );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool validateMaterialTextureSemantic( const CookedTextureData &texture,
                                          MaterialTextureSlot slot, String &error )
    {
        error.clear();
        try
        {
            const auto format = ddsFormatForTarget( texture.target );
            const bool srgb = format == 72 || format == 78 || format == 91;
            switch( slot )
            {
            case MaterialTextureSlot::BaseColour: break; // Preserve the version-one contract.
            case MaterialTextureSlot::Normal:
                require( texture.mipSettings.filter == TextureMipFilter::Normal && !srgb,
                         "Material normalTexture requires Normal filtering and linear storage" );
                break;
            case MaterialTextureSlot::ORM:
                require( texture.mipSettings.filter == TextureMipFilter::Data && !srgb,
                         "Material ormTexture requires Data filtering and linear storage" );
                break;
            default: throw std::runtime_error( "Unknown material texture semantic slot" );
            }
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool writeCookedMaterial( const CookedMaterialData &data, std::ostream &output, String &error )
    {
        error.clear();
        try
        {
            validateMaterial( data );
            const auto count = u32( data.normalTexture.id.isValid() ) + u32( data.ormTexture.id.isValid() );
            writePrefix( output, materialMagic, data.target, data.packagedBuild, count ? 2 : 1 );
            for( const auto value : { data.baseColour.r, data.baseColour.g, data.baseColour.b,
                                      data.baseColour.a, data.metalness, data.roughness } )
            {
                writeFloat( output, value );
            }
            writeString( output, data.textureId.str() );
            write64( output, data.textureSourceHash );
            write64( output, data.texturePayloadHash );
            write64( output, data.textureCompilerVersion );
            if( count )
            {
                write32( output, count );
                for( const auto slot : { MaterialTextureSlot::Normal, MaterialTextureSlot::ORM } )
                {
                    const auto &ref = slot == MaterialTextureSlot::Normal ? data.normalTexture : data.ormTexture;
                    if( !ref.id.isValid() ) continue;
                    write32( output, static_cast<u32>( slot ) );
                    writeString( output, ref.id.str() );
                    write64( output, ref.sourceHash );
                    write64( output, ref.payloadHash );
                    write64( output, ref.compilerVersion );
                }
            }
            require( output.good(), "Failed writing cooked material payload" );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool decodeCookedTexture( const resource::RuntimeResource &value, CookedTextureData &output,
                              String &error )
    {
        output = CookedTextureData();
        error.clear();
        try
        {
            validateResource( value, "texres" );
            require( value.header.installDependencies.empty() && value.dependencies.empty(),
                     "Cooked textures cannot contain install dependencies" );
            Reader reader( value.payload );
            CookedTextureData data;
            require( value.payload.size() >= 4, "Truncated texture prefix" );
            const auto magic = u32( value.payload[0] ) | u32( value.payload[1] ) << 8 |
                               u32( value.payload[2] ) << 16 | u32( value.payload[3] ) << 24;
            require( magic == textureMagic || magic == ddsMagic, "Unknown texture payload encoding" );
            readPrefix( reader, magic, data.target, data.packagedBuild );
            data.mipSettings.filter = static_cast<TextureMipFilter>( reader.read32() );
            data.mipSettings.atlasColumns = reader.read32();
            data.mipSettings.alphaCutoff = reader.readFloat();
            validateSettings( data.mipSettings );
            if( magic == ddsMagic )
            {
                const auto size = reader.read64();
                require( size <= graphicsTextureByteLimit + 148 && size == reader.remaining(),
                         "Invalid embedded DDS size" );
                reader.readBytes( data.ddsBytes, static_cast<size_t>( size ) );
                validateTexture( data );
                output = std::move( data );
                return true;
            }
            const auto count = reader.read32();
            require( count && count <= maxMipLevels, "Invalid cooked texture mip count" );
            data.levels.reserve( count );
            u64 total = 0;
            for( u32 i = 0; i < count; ++i )
            {
                TextureMipLevel level;
                level.width = reader.read32();
                level.height = reader.read32();
                const auto size = reader.read64();
                require( level.width && level.height && level.width <= maxTextureDimension &&
                             level.height <= maxTextureDimension &&
                             size == u64( level.width ) * level.height * 4u &&
                             size <= graphicsTextureByteLimit - total && size <= reader.remaining(),
                         "Invalid cooked mip size or aggregate allocation limit" );
                total += size;
                reader.readBytes( level.bgra, static_cast<size_t>( size ) );
                data.levels.push_back( std::move( level ) );
            }
            require( reader.remaining() == 0, "Unexpected trailing cooked texture data" );
            validateTexture( data );
            output = std::move( data );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool decodeCookedMaterial( const resource::RuntimeResource &value, CookedMaterialData &output,
                               String &error )
    {
        output = CookedMaterialData();
        error.clear();
        try
        {
            validateResource( value, "matres" );
            require( value.payload.size() <= 4096, "Cooked material payload exceeds its size limit" );
            Reader reader( value.payload );
            CookedMaterialData data;
            const auto version = readPrefix( reader, materialMagic, data.target, data.packagedBuild, 2 );
            require( value.header.compilerVersion != 2 || version == 1,
                     "Legacy material compiler cannot produce extended material payloads" );
            data.baseColour.r = reader.readFloat();
            data.baseColour.g = reader.readFloat();
            data.baseColour.b = reader.readFloat();
            data.baseColour.a = reader.readFloat();
            data.metalness = reader.readFloat();
            data.roughness = reader.readFloat();
            require( data.textureId.set( reader.readString( 1024 ) ),
                     "Invalid cooked material texture ResourceID" );
            data.textureSourceHash = reader.read64();
            data.texturePayloadHash = reader.read64();
            data.textureCompilerVersion = reader.read64();
            if( version == 2 )
            {
                const auto count = reader.read32();
                require( count >= 1 && count <= 2, "Invalid cooked material optional slot count" );
                for( u32 i = 0; i < count; ++i )
                {
                    const auto slot = static_cast<MaterialTextureSlot>( reader.read32() );
                    require( slot == MaterialTextureSlot::Normal || slot == MaterialTextureSlot::ORM,
                             "Unknown cooked material optional semantic slot" );
                    auto &ref = slot == MaterialTextureSlot::Normal ? data.normalTexture : data.ormTexture;
                    require( !ref.id.isValid(), "Duplicate cooked material semantic slot" );
                    require( ref.id.set( reader.readString( 1024 ) ), "Invalid material slot texture ResourceID" );
                    ref.sourceHash = reader.read64();
                    ref.payloadHash = reader.read64();
                    ref.compilerVersion = reader.read64();
                }
            }
            require( reader.remaining() == 0, "Unexpected trailing cooked material data" );
            validateMaterial( data );
            Array<resource::ResourceID> expected{ data.textureId };
            for( const auto &ref : { data.normalTexture, data.ormTexture } )
            {
                if( ref.id.isValid() && std::find( expected.begin(), expected.end(), ref.id ) == expected.end() )
                    expected.push_back( ref.id );
            }
            require( value.header.installDependencies.size() == expected.size() &&
                         value.dependencies.size() == expected.size(),
                     "Cooked material install dependency count does not match its semantic slots" );
            for( const auto &id : expected )
            {
                require( std::count( value.header.installDependencies.begin(),
                                    value.header.installDependencies.end(), id ) == 1,
                         "Cooked material install dependencies differ from its typed payload" );
                size_t matches = 0;
                for( const auto &dependency : value.dependencies )
                    if( dependency && dependency->header.resourceId == id ) ++matches;
                require( matches == 1, "Cooked material requires exactly one installed generation per texture" );
            }
            const auto validateSlot = [&]( const resource::ResourceID &id, u64 sourceHash,
                                           u64 payloadHash, u64 compilerVersion, MaterialTextureSlot slot ) {
                const resource::RuntimeResource *dependency = nullptr;
                for( const auto &candidate : value.dependencies )
                    if( candidate && candidate->header.resourceId == id ) dependency = candidate.get();
                require( dependency && dependency->header.sourceHash == sourceHash &&
                             dependency->header.payloadHash == payloadHash &&
                             dependency->header.compilerVersion == compilerVersion,
                         "Cooked material texture dependency generation does not match" );
                CookedTextureData texture;
                if( !decodeCookedTexture( *dependency, texture, error ) )
                    throw std::runtime_error( error.c_str() );
                require( texture.target == data.target && texture.packagedBuild == data.packagedBuild,
                         "Cooked material and texture target/build mode do not match" );
                if( !validateMaterialTextureSemantic( texture, slot, error ) )
                    throw std::runtime_error( error.c_str() );
            };
            validateSlot( data.textureId, data.textureSourceHash, data.texturePayloadHash,
                          data.textureCompilerVersion, MaterialTextureSlot::BaseColour );
            for( const auto slot : { MaterialTextureSlot::Normal, MaterialTextureSlot::ORM } )
            {
                const auto &ref = slot == MaterialTextureSlot::Normal ? data.normalTexture : data.ormTexture;
                if( ref.id.isValid() ) validateSlot( ref.id, ref.sourceHash, ref.payloadHash, ref.compilerVersion, slot );
            }
            output = std::move( data );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }
}  // namespace workphone::render
