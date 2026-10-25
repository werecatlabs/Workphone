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

        u32 shaderWords(MaterialShaderParameterType type)
        {
            using T = MaterialShaderParameterType;
            switch(type)
            {
            case T::Scalar: case T::Int: case T::UInt: case T::Bool: return 1;
            case T::Vector2: case T::IntVector2: return 2;
            case T::Colour: case T::Vector4: case T::IntVector4: case T::Sampler: return 4;
            case T::Matrix: return 16;
            case T::Texture: return 0;
            default: throw std::runtime_error("Cooked shader buffer/resource kind is unsupported");
            }
        }
        u32 shaderWireType(MaterialShaderParameterType type)
        {
            using T = MaterialShaderParameterType;
            switch(type)
            {
            case T::Scalar:return 1; case T::Colour:return 2; case T::Vector2:return 3;
            case T::Vector4:return 4; case T::Matrix:return 5; case T::Texture:return 6;
            case T::Sampler:return 7; case T::Int:return 8; case T::UInt:return 9;
            case T::Bool:return 10; case T::IntVector2:return 11; case T::IntVector4:return 12;
            default: throw std::runtime_error("Unknown cooked material shader parameter type");
            }
        }
        MaterialShaderParameterType shaderType(u32 wire)
        {
            using T = MaterialShaderParameterType;
            switch(wire)
            {
            case 1:return T::Scalar; case 2:return T::Colour; case 3:return T::Vector2;
            case 4:return T::Vector4; case 5:return T::Matrix; case 6:return T::Texture;
            case 7:return T::Sampler; case 8:return T::Int; case 9:return T::UInt;
            case 10:return T::Bool; case 11:return T::IntVector2; case 12:return T::IntVector4;
            default: throw std::runtime_error("Unknown cooked material shader wire type");
            }
        }
        void validateIdentity(const String &name)
        {
            require(!name.empty() && name.size() <= 255 && name.find('\0') == String::npos,
                "Invalid cooked shader identity");
        }
        void validateDXBC(const std::vector<u8> &bytes, bool optional)
        {
            if(bytes.empty() && optional) return;
            require(bytes.size() >= 32 && bytes.size() <= graphicsMaterialShaderByteLimit/2 &&
                !std::memcmp(bytes.data(),"DXBC",4),"Invalid cooked DXBC envelope");
            const auto word = [&](size_t offset) {
                require(offset <= bytes.size()-4,"Truncated DXBC word");
                return u32(bytes[offset]) | u32(bytes[offset+1])<<8 | u32(bytes[offset+2])<<16 | u32(bytes[offset+3])<<24;
            };
            const auto count = word(28);
            require(word(24) == bytes.size() && count > 0 && count <= 64 && 32ull+count*4ull <= bytes.size(),
                "Invalid cooked DXBC length or chunk count");
            for(u32 i=0;i<count;++i)
            {
                const auto offset=word(32+i*4);
                require(offset >= 32+count*4 && offset <= bytes.size()-8,"Invalid DXBC chunk offset");
                const auto size=word(offset+4);
                require(size <= bytes.size()-offset-8,"Truncated DXBC chunk");
                for(u32 j=0;j<i;++j)
                {
                    const auto other=word(32+j*4);const auto end=other+8ull+word(other+4);
                    require(offset+8ull+size <= other || offset >= end,"Overlapping DXBC chunks");
                }
            }
        }
        void validateShader(const CookedMaterialShaderData &shader, bool pinned=true)
        {
            if(!shader.present())
            {
                require(!shader.schemaVersion && shader.parameters.empty() && shader.vertexDXBC.empty() &&
                    shader.pixelDXBC.empty() && shader.vertexSelection.constantBuffers.empty() &&
                    shader.vertexSelection.resources.empty() && shader.pixelSelection.constantBuffers.empty() &&
                    shader.pixelSelection.resources.empty(),"Absent cooked shader contains payload");
                return;
            }
            validateIdentity(shader.shaderID);
            require(shader.schemaVersion > 0 && shader.parameters.size() <= 64,"Invalid cooked material schema version/count");
            validateDXBC(shader.pixelDXBC,false); validateDXBC(shader.vertexDXBC,true);
            Array<String> names;
            for(const auto &p:shader.parameters)
            {
                validateIdentity(p.name); shaderWireType(p.type);
                require(std::find(names.begin(),names.end(),p.name)==names.end(),"Duplicate cooked shader parameter");
                names.push_back(p.name);
                using T=MaterialShaderParameterType;
                const auto words=shaderWords(p.type);
                for(size_t i=words;i<p.words.size();++i) require(!p.words[i],"Nonzero unused cooked shader words");
                if(p.type==T::Bool) require(p.words[0]<=1,"Invalid cooked bool");
                if(p.type==T::Sampler)
                    require(p.words[0]<=4 && p.words[1]<=4 && p.words[2]<=3 && p.words[3]>=1 && p.words[3]<=16,
                        "Invalid portable cooked sampler policy");
                if(p.type==T::Scalar || p.type==T::Colour || p.type==T::Vector2 || p.type==T::Vector4 || p.type==T::Matrix)
                    for(u32 i=0;i<words;++i) { float f;std::memcpy(&f,&p.words[i],4);require(std::isfinite(f),"Nonfinite cooked shader value"); }
                if(p.type==T::Texture)
                    require(p.texture.id.isValid() && !p.texture.id.isSubResource() &&
                        p.texture.id.type()==resource::ResourceTypeID("texres") && p.texture.id.str().size()<=1024 &&
                        (!pinned || (p.texture.sourceHash && p.texture.payloadHash && p.texture.compilerVersion)),
                        "Cooked shader texture requires a pinned durable texres identity");
                else require(!p.texture.id.isValid() && !p.texture.sourceHash && !p.texture.payloadHash && !p.texture.compilerVersion,
                    "Numeric/sampler parameter contains a texture identity");
            }
            for(const auto selection:{&shader.vertexSelection,&shader.pixelSelection})
                for(const auto list:{&selection->constantBuffers,&selection->resources})
                {
                    require(list->size()<=144,"Cooked shader ownership selection is too large");
                    for(size_t i=0;i<list->size();++i)
                    {
                        validateIdentity((*list)[i]);
                        require(std::find(list->begin(),list->begin()+i,(*list)[i])==list->begin()+i,
                            "Duplicate cooked shader ownership selection");
                    }
                }
            require(!shader.vertexDXBC.empty() || (shader.vertexSelection.constantBuffers.empty() &&
                shader.vertexSelection.resources.empty()),"Missing vertex stage has ownership selections");
        }

        u64 materialWireBytes(const CookedMaterialData &data)
        {
            const auto stringBytes=[](const String &value){return 4ull+value.size();};
            u64 bytes=8+stringBytes(data.target)+4+24+stringBytes(data.textureId.str())+24;
            const u32 count=u32(data.normalTexture.id.isValid())+u32(data.ormTexture.id.isValid());
            if(count || data.shader.present())bytes+=4;
            for(const auto &ref:{data.normalTexture,data.ormTexture})if(ref.id.isValid())bytes+=4+stringBytes(ref.id.str())+24;
            if(data.shader.present())
            {
                bytes+=8+stringBytes(data.shader.shaderID)+4+4;
                for(const auto &p:data.shader.parameters)
                {
                    bytes+=stringBytes(p.name)+4+4+u64(shaderWords(p.type))*4;
                    if(p.type==MaterialShaderParameterType::Texture)bytes+=stringBytes(p.texture.id.str())+24;
                }
                bytes+=8+data.shader.vertexDXBC.size()+data.shader.pixelDXBC.size();
                for(const auto selection:{&data.shader.vertexSelection,&data.shader.pixelSelection})
                    for(const auto list:{&selection->constantBuffers,&selection->resources})
                    {bytes+=4;for(const auto &name:*list)bytes+=stringBytes(name);}
            }
            return bytes;
        }

        void validateMaterial( const CookedMaterialData &data )
        {
            validateTarget( data.target );
            validateShader(data.shader);
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
            require(materialWireBytes(data) <= (data.shader.present()?graphicsMaterialShaderByteLimit+65536ull:4096ull),
                "Cooked material aggregate payload exceeds its wire size limit");
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

        void writeShader(std::ostream &output,const CookedMaterialShaderData &shader)
        {
            write32(output,0x01020304u); write32(output,1); // endian marker and portable schema wire version
            writeString(output,shader.shaderID); write32(output,shader.schemaVersion);
            write32(output,static_cast<u32>(shader.parameters.size()));
            for(const auto &p:shader.parameters)
            {
                writeString(output,p.name); write32(output,shaderWireType(p.type));
                const auto count=shaderWords(p.type);write32(output,count);
                for(u32 i=0;i<count;++i)write32(output,p.words[i]);
                if(p.type==MaterialShaderParameterType::Texture)
                {
                    writeString(output,p.texture.id.str());write64(output,p.texture.sourceHash);
                    write64(output,p.texture.payloadHash);write64(output,p.texture.compilerVersion);
                }
            }
            for(const auto code:{&shader.vertexDXBC,&shader.pixelDXBC})
            {
                write32(output,static_cast<u32>(code->size()));
                if(!code->empty())output.write(reinterpret_cast<const char *>(code->data()),code->size());
            }
            for(const auto selection:{&shader.vertexSelection,&shader.pixelSelection})
                for(const auto list:{&selection->constantBuffers,&selection->resources})
                {
                    write32(output,static_cast<u32>(list->size()));
                    for(const auto &name:*list)writeString(output,name);
                }
        }
        CookedMaterialShaderData readShader(Reader &reader)
        {
            require(reader.read32()==0x01020304u && reader.read32()==1,"Unsupported cooked shader endian/schema version");
            CookedMaterialShaderData shader;
            shader.shaderID=reader.readString(255);shader.schemaVersion=reader.read32();
            const auto count=reader.read32();
            require(count<=64 && u64(count)*16<=reader.remaining(),"Invalid cooked shader parameter count");
            for(u32 i=0;i<count;++i)
            {
                CookedMaterialShaderParameter p;p.name=reader.readString(255);p.type=shaderType(reader.read32());
                const auto words=reader.read32();
                require(words==shaderWords(p.type) && u64(words)*4<=reader.remaining(),"Invalid cooked shader value width");
                for(u32 j=0;j<words;++j)p.words[j]=reader.read32();
                if(p.type==MaterialShaderParameterType::Texture)
                {
                    require(p.texture.id.set(reader.readString(1024)),"Invalid cooked shader texture ResourceID");
                    p.texture.sourceHash=reader.read64();p.texture.payloadHash=reader.read64();p.texture.compilerVersion=reader.read64();
                }
                shader.parameters.push_back(std::move(p));
            }
            for(auto code:{&shader.vertexDXBC,&shader.pixelDXBC})
            {
                const auto bytes=reader.read32();
                require(bytes<=graphicsMaterialShaderByteLimit/2 && bytes<=reader.remaining(),"Invalid cooked shader bytecode length");
                reader.readBytes(*code,bytes);
            }
            for(auto selection:{&shader.vertexSelection,&shader.pixelSelection})
                for(auto list:{&selection->constantBuffers,&selection->resources})
                {
                    const auto size=reader.read32();
                    require(size<=144 && u64(size)*4<=reader.remaining(),"Invalid cooked shader ownership count");
                    for(u32 i=0;i<size;++i)list->push_back(reader.readString(255));
                }
            validateShader(shader);
            return shader;
        }

        void validateResource( const resource::RuntimeResource &value, const char *type )
        {
            const resource::ResourceTypeID expected( type );
            const GraphicsResourceCompiler compiler;
            require( value.header.isValid() && !value.header.resourceId.isSubResource() &&
                         value.header.resourceId.type() == expected &&
                         value.header.resourceType == expected &&
                         ( value.header.compilerVersion == compiler.versionFor( expected ) ||
                           ( expected == resource::ResourceTypeID( "matres" ) &&
                             (value.header.compilerVersion == 2 || value.header.compilerVersion == 3) ) ),
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

    bool validateCookedMaterialShaderRecipe(const CookedMaterialShaderData &shader,String &error)
    {
        error.clear();
        try { validateShader(shader,false);return true; }
        catch(const std::exception &exception){error=exception.what();return false;}
    }

    bool populateCookedMaterialShader(const CookedMaterialShaderData &shader,
        MaterialShaderParametersInstance &values, wp_shader_program *program, String &error)
    {
        error.clear();
        try
        {
            validateShader(shader);
            require(shader.present() && program && wp_shader_program_get_param_count(program)==0,"Shader installation requires an empty native definition");
            Array<MaterialShaderParameterInfo> info;
            for(const auto &p:shader.parameters)info.emplace_back(p.type,p.name);
            const auto &existing=values.GetParameterInfo();
            bool same=existing.size()==info.size() && bool(values.GetLayout());
            for(size_t i=0;same && i<info.size();++i)same=existing[i].m_type==info[i].m_type && existing[i].m_name==info[i].m_name;
            if(!same)values.Reset(info);
            require(bool(values.GetLayout()),"Cooked shader CPU schema could not be constructed");
            auto edit=values.Edit();
            for(size_t i=0;i<shader.parameters.size();++i)
            {
                const auto &p=shader.parameters[i];using T=MaterialShaderParameterType;
                wp_uniform_type uniform=WORKPHONE_UNIFORM_TYPE_INT;
                wp_shader_resource_kind resource=WORKPHONE_SHADER_RESOURCE_NONE;
                switch(p.type)
                {
                case T::Scalar:uniform=WORKPHONE_UNIFORM_TYPE_FLOAT;break;
                case T::Colour:case T::Vector4:uniform=WORKPHONE_UNIFORM_TYPE_VEC4;break;
                case T::Vector2:uniform=WORKPHONE_UNIFORM_TYPE_VEC2;break;
                case T::Matrix:uniform=WORKPHONE_UNIFORM_TYPE_MAT4;break;
                case T::UInt:uniform=WORKPHONE_UNIFORM_TYPE_UINT;break;
                case T::Bool:uniform=WORKPHONE_UNIFORM_TYPE_BOOL;break;
                case T::IntVector2:uniform=WORKPHONE_UNIFORM_TYPE_IVEC2;break;
                case T::IntVector4:uniform=WORKPHONE_UNIFORM_TYPE_IVEC4;break;
                case T::Texture:resource=WORKPHONE_SHADER_RESOURCE_TEXTURE;break;
                case T::Sampler:resource=WORKPHONE_SHADER_RESOURCE_SAMPLER;break;
                case T::Int:break;
                default:throw std::runtime_error("Unsupported cooked shader value type");
                }
                require(wp_shader_program_register_param(program,reinterpret_cast<const wp_c8 *>(p.name.c_str()),uniform,1,
                    WORKPHONE_SHADER_PARAM_VARIABILITY_PER_OBJECT,WORKPHONE_SHADER_AUTO_PARAM_NONE,resource)==i,
                    "Cooked shader native parameter registration failed");
                const auto handle=values.GetParameter(static_cast<u32>(i));
                float f[16]{};s32 integers[4]{};
                std::memcpy(f,p.words.data(),64);std::memcpy(integers,p.words.data(),16);
                bool assigned=false;
                switch(p.type)
                {
                case T::Scalar:assigned=values.SetScalar(handle,f[0]);break;
                case T::Int:assigned=values.SetScalar(handle,integers[0]);break;
                case T::UInt:assigned=values.SetScalar(handle,p.words[0]);break;
                case T::Bool:assigned=values.SetBool(handle,p.words[0]!=0);break;
                case T::Colour:assigned=values.SetColour(handle,ColourF(f[0],f[1],f[2],f[3]));break;
                case T::Vector2:assigned=values.SetVector2(handle,Vector2F(f[0],f[1]));break;
                case T::Vector4:assigned=values.SetVector4(handle,Vector4F(f[0],f[1],f[2],f[3]));break;
                case T::IntVector2:assigned=values.SetVector2(handle,Vector2<s32>(integers[0],integers[1]));break;
                case T::IntVector4:assigned=values.SetVector4(handle,Vector4<s32>(integers[0],integers[1],integers[2],integers[3]));break;
                case T::Matrix:assigned=values.SetMatrix(handle,Matrix4F(f));break;
                case T::Texture:assigned=values.SetTexture(handle,i+1);break;
                case T::Sampler:assigned=values.SetSampler(handle,i+1);break;
                default:break;
                }
                require(assigned,"Cooked shader typed value installation failed");
            }
            return true;
        }
        catch(const std::exception &exception){error=exception.what();return false;}
    }

    bool writeCookedMaterial( const CookedMaterialData &data, std::ostream &output, String &error )
    {
        error.clear();
        try
        {
            validateMaterial( data );
            const auto count = u32( data.normalTexture.id.isValid() ) + u32( data.ormTexture.id.isValid() );
            writePrefix( output, materialMagic, data.target, data.packagedBuild, data.shader.present() ? 3 : (count ? 2 : 1) );
            for( const auto value : { data.baseColour.r, data.baseColour.g, data.baseColour.b,
                                      data.baseColour.a, data.metalness, data.roughness } )
            {
                writeFloat( output, value );
            }
            writeString( output, data.textureId.str() );
            write64( output, data.textureSourceHash );
            write64( output, data.texturePayloadHash );
            write64( output, data.textureCompilerVersion );
            if( count || data.shader.present() )
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
            if(data.shader.present())writeShader(output,data.shader);
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
            require( value.payload.size() <= graphicsMaterialShaderByteLimit+65536u, "Cooked material payload exceeds its size limit" );
            validateResource( value, "matres" );
            Reader reader( value.payload );
            CookedMaterialData data;
            const auto version = readPrefix( reader, materialMagic, data.target, data.packagedBuild, 3 );
            require( value.header.compilerVersion != 2 || version == 1,
                     "Legacy material compiler cannot produce extended material payloads" );
            require(version >= 3 || value.payload.size() <= 4096,"Legacy cooked material exceeds its size limit");
            require(value.header.compilerVersion >= 4 || version <= 2,
                "Legacy material compiler cannot produce shader-schema payloads");
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
            if( version >= 2 )
            {
                const auto count = reader.read32();
                require( count <= 2 && (version >= 3 || count >= 1), "Invalid cooked material optional slot count" );
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
            if(version >= 3)data.shader=readShader(reader);
            require( reader.remaining() == 0, "Unexpected trailing cooked material data" );
            validateMaterial( data );
            Array<resource::ResourceID> expected{ data.textureId };
            for( const auto &ref : { data.normalTexture, data.ormTexture } )
            {
                if( ref.id.isValid() && std::find( expected.begin(), expected.end(), ref.id ) == expected.end() )
                    expected.push_back( ref.id );
            }
            for(const auto &p:data.shader.parameters)
                if(p.type==MaterialShaderParameterType::Texture &&
                    std::find(expected.begin(),expected.end(),p.texture.id)==expected.end())expected.push_back(p.texture.id);
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
            Array<std::pair<resource::ResourceID,TextureMipFilter>> validatedTextures;
            const auto validateSlot = [&]( const resource::ResourceID &id, u64 sourceHash,
                                           u64 payloadHash, u64 compilerVersion, MaterialTextureSlot slot ) {
                const resource::RuntimeResource *dependency = nullptr;
                for( const auto &candidate : value.dependencies )
                    if( candidate && candidate->header.resourceId == id ) dependency = candidate.get();
                require( dependency && dependency->header.sourceHash == sourceHash &&
                             dependency->header.payloadHash == payloadHash &&
                             dependency->header.compilerVersion == compilerVersion,
                         "Cooked material texture dependency generation does not match" );
                for(const auto &cached:validatedTextures)if(cached.first==id)
                {
                    CookedTextureData semantic;semantic.target=data.target;semantic.mipSettings.filter=cached.second;
                    if(!validateMaterialTextureSemantic(semantic,slot,error))throw std::runtime_error(error.c_str());
                    return;
                }
                CookedTextureData texture;
                if( !decodeCookedTexture( *dependency, texture, error ) )
                    throw std::runtime_error( error.c_str() );
                require( texture.target == data.target && texture.packagedBuild == data.packagedBuild,
                         "Cooked material and texture target/build mode do not match" );
                if( !validateMaterialTextureSemantic( texture, slot, error ) )
                    throw std::runtime_error( error.c_str() );
                validatedTextures.emplace_back(id,texture.mipSettings.filter);
            };
            validateSlot( data.textureId, data.textureSourceHash, data.texturePayloadHash,
                          data.textureCompilerVersion, MaterialTextureSlot::BaseColour );
            for( const auto slot : { MaterialTextureSlot::Normal, MaterialTextureSlot::ORM } )
            {
                const auto &ref = slot == MaterialTextureSlot::Normal ? data.normalTexture : data.ormTexture;
                if( ref.id.isValid() ) validateSlot( ref.id, ref.sourceHash, ref.payloadHash, ref.compilerVersion, slot );
            }
            for(const auto &p:data.shader.parameters)if(p.type==MaterialShaderParameterType::Texture)
                validateSlot(p.texture.id,p.texture.sourceHash,p.texture.payloadHash,p.texture.compilerVersion,MaterialTextureSlot::BaseColour);
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
