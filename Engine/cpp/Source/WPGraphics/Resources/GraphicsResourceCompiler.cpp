#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/DdsTexture.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <boost/json.hpp>
#include <FreeImage.h>
#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
#include <d3dcompiler.h>
#include <wrl/client.h>
#endif

#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        namespace fs = std::filesystem;
        namespace json = boost::json;
        constexpr u64 encodedImageByteLimit = 64ull * 1024ull * 1024ull;
        constexpr u32 maxTextureDimension = 16384;
        std::mutex imageCodecMutex;

        void require( bool condition, const char *message )
        {
            if( !condition )
            {
                throw std::runtime_error( message );
            }
        }

        std::vector<u8> readFile( const fs::path &path, u64 limit )
        {
            std::error_code error;
            const auto size = fs::file_size( path, error );
            require( !error && size > 0 && size <= limit,
                     "Graphics source is missing, empty or exceeds its byte limit" );
            std::ifstream stream( path, std::ios::binary );
            require( stream.good(), "Cannot open graphics source file" );
            std::vector<u8> bytes( static_cast<size_t>( size ) );
            stream.read( reinterpret_cast<char *>( bytes.data() ),
                         static_cast<std::streamsize>( size ) );
            require( stream.good() && stream.peek() == std::char_traits<char>::eof(),
                     "Graphics source changed size or could not be read completely" );
            return bytes;
        }

        // Boost.JSON deliberately accepts duplicate object keys. Descriptor recipes
        // reject them so two tools cannot interpret the same authored settings differently.
        void validateUniqueKeys( const std::string &text )
        {
            struct Frame { bool object; std::set<std::string> keys; };
            std::vector<Frame> frames;
            for(size_t i=0;i<text.size();++i)
            {
                if(text[i]=='"')
                {
                    const auto start=i;
                    for(++i;i<text.size();++i)
                    { if(text[i]=='\\')++i;else if(text[i]=='"')break; }
                    auto next=i+1;
                    while(next<text.size() && (text[next]==' ' || text[next]=='\t' || text[next]=='\r' || text[next]=='\n'))++next;
                    if(!frames.empty() && frames.back().object && next<text.size() && text[next]==':')
                    {
                        const auto key=json::parse(json::string_view(text.data()+start,i-start+1)).as_string();
                        require(frames.back().keys.insert(std::string(key.data(),key.size())).second,"Duplicate graphics descriptor field");
                    }
                }
                else if(text[i]=='{' || text[i]=='[')frames.push_back({text[i]=='{',{}});
                else if(text[i]=='}' || text[i]==']') { require(!frames.empty(),"Unbalanced graphics descriptor");frames.pop_back(); }
            }
        }

        json::object readDescriptor( const resource::CompileContext &context,
                                     const std::set<std::string> &fields,
                                     const std::set<std::string> &optional = {} )
        {
            auto bytes =
                readFile( fs::u8path( context.sourcePath.c_str() ), graphicsResourceDescriptorLimit );
            const std::string text( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
            json::parse_options options;
            options.max_depth = 8;
            auto value = json::parse( text, {}, options );
            require( value.is_object(), "Graphics descriptor must be a JSON object" );
            validateUniqueKeys( text );
            const auto &object = value.as_object();
            for( const auto &field : fields )
                require( object.contains( field ), "Graphics descriptor has missing required fields" );
            for( const auto &field : object )
            {
                const std::string name( field.key().data(), field.key().size() );
                require( fields.count( name ) != 0 || optional.count( name ) != 0,
                         "Unsupported graphics descriptor field" );
            }
            const auto &version = object.at( "version" );
            require( version.is_number() && version.to_number<double>() == 1.0,
                     "Unsupported graphics descriptor version" );
            return object;
        }

        String stringField( const json::object &object, const char *name )
        {
            const auto &value = object.at( name );
            require( value.is_string(), "Graphics descriptor field must be a string" );
            const auto &text = value.as_string();
            require( !text.empty() && text.size() <= 1024,
                     "Invalid graphics descriptor path or string length" );
            String result( text.data(), text.size() );
            require( result.find( '\0' ) == String::npos, "Embedded NUL in graphics descriptor string" );
            return result;
        }

        float scalar( const json::value &value, float minimum, float maximum )
        {
            require( value.is_number(), "Graphics descriptor scalar must be a number" );
            const auto number = value.to_number<double>();
            require( std::isfinite( number ) && number >= minimum && number <= maximum,
                     "Graphics descriptor scalar is outside its supported range" );
            return static_cast<float>( number );
        }

        struct TextureRecipe
        {
            String source;
            TextureMipSettings settings;
            String compression;
            u32 maximumTextureSize = 0;
        };

        constexpr const char *projectPolicyInput = "data://.workphone/ProjectAssetPolicy.settings";
        ProjectAssetPolicy projectAssetPolicy( const resource::CompileContext &context )
        {
            auto path = fs::u8path( context.sourceRoot.c_str() ) / ".workphone/ProjectAssetPolicy.settings";
            if( context.snapshotInput )
            {
                String captured, error;
                if( !context.snapshotInput( projectPolicyInput, false, captured, error ) )
                    throw std::runtime_error( error.c_str() );
                if( captured.empty() ) return {};
                path = fs::u8path( captured.c_str() );
            }
            ProjectAssetPolicy policy; String error;
            if( !readProjectAssetPolicy( path.u8string().c_str(), policy, error ) ) throw std::runtime_error( error.c_str() );
            return policy;
        }

        TextureRecipe readTextureRecipe( const resource::CompileContext &context )
        {
            const auto descriptor = readDescriptor(
                context, { "version", "source", "mipFilter", "atlasColumns", "alphaCutoff" }, { "compression", "maxTextureSize" } );
            TextureRecipe recipe;
            auto path = stringField( descriptor, "source" );
            require( path.size() > 7 && path.substr( 0, 7 ) == "data://",
                     "Texture source must be a data:// path under the project source root" );
            AssetCatalogPath canonical;
            String error;
            if( !canonicalAssetCatalogPath( context.sourceRoot, path.substr( 7 ), canonical, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            recipe.source = "data://" + canonical.path;
            const auto filter = stringField( descriptor, "mipFilter" );
            if( filter == "none" )
            {
                recipe.settings.filter = TextureMipFilter::None;
            }
            else if( filter == "colour" )
            {
                recipe.settings.filter = TextureMipFilter::Colour;
            }
            else if( filter == "data" )
            {
                recipe.settings.filter = TextureMipFilter::Data;
            }
            else if( filter == "normal" )
            {
                recipe.settings.filter = TextureMipFilter::Normal;
            }
            else if( filter == "cutout" )
            {
                recipe.settings.filter = TextureMipFilter::Cutout;
            }
            else if( filter == "roughness" )
            {
                recipe.settings.filter = TextureMipFilter::Roughness;
            }
            else
            {
                throw std::runtime_error( "Unknown texture mipFilter" );
            }
            const auto &columnValue = descriptor.at( "atlasColumns" );
            require( columnValue.is_number(), "Texture atlasColumns must be numeric" );
            const auto columns = columnValue.to_number<double>();
            require( std::isfinite( columns ) && columns >= 1 && columns <= maxTextureDimension,
                     "Texture atlasColumns is outside its supported range" );
            require( std::floor( columns ) == columns, "Texture atlasColumns must be an integer" );
            recipe.settings.atlasColumns = static_cast<u32>( columns );
            recipe.settings.alphaCutoff = scalar( descriptor.at( "alphaCutoff" ), 0.0f, 1.0f );
            if( descriptor.contains( "compression" ) )
            {
                recipe.compression = stringField( descriptor, "compression" );
                require( recipe.compression == "none" || recipe.compression == "bc1" || recipe.compression == "bc3" ||
                         recipe.compression == "bc7" || recipe.compression == "astc" || recipe.compression == "pvrtc" || recipe.compression == "etc2",
                         "Unknown texture recipe compression format" );
            }
            if( descriptor.contains( "maxTextureSize" ) )
            {
                const auto maximum = scalar( descriptor.at( "maxTextureSize" ), 1, 16384 );
                require( std::floor( maximum ) == maximum, "Texture maxTextureSize must be an integer" );
                recipe.maximumTextureSize = static_cast<u32>( maximum );
            }
            const auto gpuFormat = ddsFormatForTarget( context.target );
            const bool srgb = gpuFormat == 72 || gpuFormat == 78 || gpuFormat == 91;
            require( !srgb || recipe.settings.filter == TextureMipFilter::Colour ||
                         recipe.settings.filter == TextureMipFilter::Cutout ||
                         recipe.settings.filter == TextureMipFilter::None,
                     "SRGB DDS profiles require colour/cutout/none mip semantics" );
            require( !dds::blockBytes( gpuFormat ) || recipe.settings.filter != TextureMipFilter::Normal,
                     "Normal maps require the lossless BGRA profile until a normal-map compression "
                     "profile is supported" );
            require( recipe.settings.alphaCutoff > 0.0f && recipe.settings.alphaCutoff < 1.0f,
                     "Texture alphaCutoff must be strictly between zero and one" );
            return recipe;
        }

        void exactFields(const json::object &object, const std::set<std::string> &required,
            const std::set<std::string> &optional={})
        {
            for(const auto &name:required)require(object.contains(name),"Missing shader descriptor field");
            for(const auto &field:object)
            { const std::string name(field.key().data(),field.key().size());
              require(required.count(name) || optional.count(name),"Unsupported shader descriptor field"); }
        }
        u32 integer(const json::value &value, double minimum, double maximum)
        {
            require(value.is_number(),"Shader integer must be numeric");
            const double number=value.to_number<double>();
            require(std::isfinite(number) && std::floor(number)==number && number>=minimum && number<=maximum,
                "Shader integer is outside its exact supported range");
            return minimum<0 ? static_cast<u32>(static_cast<s32>(number)) : static_cast<u32>(number);
        }
        struct ShaderInput
        {
            String logical;
            std::vector<u8> bytes;
        };
        ShaderInput shaderInput(const resource::CompileContext &context,const String &relative,Array<String> *dependencies)
        {
            AssetCatalogPath path;String error;
            if(!canonicalAssetCatalogPath(context.sourceRoot,relative,path,error))throw std::runtime_error(error.c_str());
            ShaderInput input;input.logical="data://"+path.path;
            String captured;
            if(context.snapshotInput)
            {
                if(!context.snapshotInput(input.logical,true,captured,error) || captured.empty())
                    throw std::runtime_error(error.empty()?"Shader input snapshot failed":error.c_str());
            }
            const auto physical=captured.empty()?fs::u8path(context.sourceRoot.c_str())/fs::u8path(path.path.c_str()):fs::u8path(captured.c_str());
            input.bytes=readFile(physical,graphicsMaterialShaderByteLimit/2);
            if(dependencies && std::find(dependencies->begin(),dependencies->end(),input.logical)==dependencies->end())
                dependencies->push_back(input.logical);
            return input;
        }
#if defined(WP_PLATFORM_WIN32) && WP_BUILD_RENDERER_DX11
        class ShaderIncludes final : public ID3DInclude
        {
        public:
            ShaderIncludes(const resource::CompileContext &context,const ShaderInput &root,Array<String> *dependencies) :
                context(context),root(root),dependencies(dependencies),total(root.bytes.size()) {}
            HRESULT __stdcall Open(D3D_INCLUDE_TYPE,const char *name,const void *parent,const void **output,UINT *size) override
            {
                try
                {
                    require(name && *name && output && size && depth<32,"Shader include depth or arguments are invalid");
                    String parentId=root.logical;
                    if(parent && parent!=root.bytes.data())
                    {
                        bool found=false;
                        for(const auto &file:files)if(file->bytes.data()==parent){parentId=file->logical;found=true;break;}
                        require(found,"Shader include has an unknown parent snapshot");
                    }
                    require(std::strlen(name)<=1024,"Shader include path exceeds its limit");
                    const auto include=fs::u8path(name);
                    require(!include.is_absolute(),"Absolute shader includes are not portable asset inputs");
                    const auto relative=(fs::u8path(parentId.substr(7).c_str()).parent_path()/include).lexically_normal().generic_u8string();
                    AssetCatalogPath canonical;String error;
                    if(!canonicalAssetCatalogPath(context.sourceRoot,relative.c_str(),canonical,error))throw std::runtime_error(error.c_str());
                    const String logical="data://"+canonical.path;
                    const ShaderInput *input=nullptr;
                    for(const auto &file:files)if(file->logical==logical){input=file.get();break;}
                    if(!input)
                    {
                        require(files.size()<128,"Shader include file count exceeds its limit");
                        auto file=std::make_unique<ShaderInput>(shaderInput(context,canonical.path,dependencies));
                        require(file->bytes.size()<=8ull*1024*1024-total,"Shader source/include closure exceeds its byte limit");
                        total+=file->bytes.size();input=file.get();files.push_back(std::move(file));
                    }
                    *output=input->bytes.data();*size=static_cast<UINT>(input->bytes.size());++depth;return S_OK;
                }
                catch(const std::exception &failure)
                {
                    try { diagnostic=failure.what(); } catch(...) { return E_OUTOFMEMORY; }
                    return E_FAIL;
                }
                catch(...){return E_OUTOFMEMORY;}
            }
            HRESULT __stdcall Close(const void *) override { if(depth)--depth;return S_OK; }
            const resource::CompileContext &context;
            const ShaderInput &root;
            Array<String> *dependencies;
            std::vector<std::unique_ptr<ShaderInput>> files;
            size_t total;
            u32 depth=0;
            String diagnostic;
        };
        std::vector<u8> compileShaderInput(const resource::CompileContext &context,const String &source,
            const String &entry,const char *profile,Array<String> *dependencies)
        {
            require(source.substr(0,7)=="data://","HLSL source must use data://");
            auto input=shaderInput(context,source.substr(7),dependencies);
            ShaderIncludes includes(context,input,dependencies);
            Microsoft::WRL::ComPtr<ID3DBlob> bytecode,diagnostics;
            const auto result=D3DCompile(input.bytes.data(),input.bytes.size(),input.logical.c_str(),nullptr,&includes,
                entry.c_str(),profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&bytecode,&diagnostics);
            if(FAILED(result))
            {
                if(!includes.diagnostic.empty())throw std::runtime_error(includes.diagnostic.c_str());
                throw std::runtime_error(diagnostics?static_cast<const char *>(diagnostics->GetBufferPointer()):"Cooked HLSL compilation failed");
            }
            require(bytecode && bytecode->GetBufferSize()<=graphicsMaterialShaderByteLimit/2,"Compiled shader exceeds its byte limit");
            const auto begin=static_cast<const u8 *>(bytecode->GetBufferPointer());
            return std::vector<u8>(begin,begin+bytecode->GetBufferSize());
        }
#else
        std::vector<u8> compileShaderInput(const resource::CompileContext &,const String &,
            const String &,const char *,Array<String> *)
        { throw std::runtime_error("Cooked HLSL compilation requires the enabled Windows DX11 toolchain"); }
#endif
        CookedMaterialShaderData readShaderRecipe(const resource::CompileContext &context,
            const json::value &value, Array<String> *dependencies)
        {
            require(value.is_object(),"Material shader must be an object");const auto &object=value.as_object();
            exactFields(object,{"id","schemaVersion","parameters","pixelBuffers","pixelResources","vertexBuffers","vertexResources"},
                {"pixel","vertex","pixelSource","vertexSource","pixelEntry","vertexEntry"});
            require(object.contains("pixel") != object.contains("pixelSource"),"Specify exactly one pixel DXBC or HLSL source");
            require(!(object.contains("vertex") && object.contains("vertexSource")),"Vertex DXBC and HLSL source are mutually exclusive");
            require(!object.contains("pixelEntry") || object.contains("pixelSource"),"pixelEntry requires pixelSource");
            require(!object.contains("vertexEntry") || object.contains("vertexSource"),"vertexEntry requires vertexSource");
            CookedMaterialShaderData shader;shader.shaderID=stringField(object,"id");
            shader.schemaVersion=integer(object.at("schemaVersion"),1,4294967295.0);
            const auto bytecode=[&](const char *name,const char *hlsl,const char *entry,const char *profile,std::vector<u8> &bytes) {
                if(object.contains(hlsl))
                {
                    bytes=compileShaderInput(context,stringField(object,hlsl),object.contains(entry)?stringField(object,entry):String("main"),profile,dependencies);
                    return;
                }
                if(!object.contains(name))return;
                const auto source=stringField(object,name);
                require(source.substr(0,7)=="data://","Shader bytecode source must use data://");
                bytes=shaderInput(context,source.substr(7),dependencies).bytes;
            };
            bytecode("vertex","vertexSource","vertexEntry","vs_5_0",shader.vertexDXBC);
            bytecode("pixel","pixelSource","pixelEntry","ps_5_0",shader.pixelDXBC);
            const auto selection=[&](const char *name,Array<String> &output) {
                const auto &values=object.at(name);require(values.is_array() && values.as_array().size()<=144,"Invalid shader ownership selection");
                for(const auto &v:values.as_array())
                { require(v.is_string() && !v.as_string().empty() && v.as_string().size()<=255,"Invalid shader ownership name");
                  String item(v.as_string().data(),v.as_string().size());
                  require(item.find('\0')==String::npos && std::find(output.begin(),output.end(),item)==output.end(),"Duplicate/invalid shader ownership name");output.push_back(item); }
            };
            selection("vertexBuffers",shader.vertexSelection.constantBuffers);selection("vertexResources",shader.vertexSelection.resources);
            selection("pixelBuffers",shader.pixelSelection.constantBuffers);selection("pixelResources",shader.pixelSelection.resources);
            const auto &parameters=object.at("parameters");require(parameters.is_array() && parameters.as_array().size()<=64,"Invalid shader parameter count");
            for(const auto &v:parameters.as_array())
            {
                require(v.is_object(),"Shader parameter must be an object");const auto &p=v.as_object();
                exactFields(p,{"name","type","value"});CookedMaterialShaderParameter parameter;parameter.name=stringField(p,"name");
                const auto type=stringField(p,"type");using T=MaterialShaderParameterType;
                u32 count=1;bool floats=false,signedValues=false;
                if(type=="float") {parameter.type=T::Scalar;floats=true;}
                else if(type=="colour") {parameter.type=T::Colour;count=4;floats=true;}
                else if(type=="float2") {parameter.type=T::Vector2;count=2;floats=true;}
                else if(type=="float4") {parameter.type=T::Vector4;count=4;floats=true;}
                else if(type=="matrix4") {parameter.type=T::Matrix;count=16;floats=true;}
                else if(type=="int") {parameter.type=T::Int;signedValues=true;}
                else if(type=="uint")parameter.type=T::UInt;
                else if(type=="int2") {parameter.type=T::IntVector2;count=2;signedValues=true;}
                else if(type=="int4") {parameter.type=T::IntVector4;count=4;signedValues=true;}
                else if(type=="bool")parameter.type=T::Bool;
                else if(type=="sampler") {parameter.type=T::Sampler;count=4;}
                else if(type=="texture")parameter.type=T::Texture;
                else throw std::runtime_error("Unsupported persistent shader parameter/resource kind");
                const auto &raw=p.at("value");
                if(parameter.type==T::Bool) { require(raw.is_bool(),"Shader bool must be boolean");parameter.words[0]=raw.as_bool()?1:0; }
                else if(parameter.type==T::Texture)
                {
                    require(raw.is_string(),"Shader texture value must be a texres ResourceID");
                    const String id(raw.as_string().data(),raw.as_string().size());AssetCatalogPath path;String error;
                    require(id.find('\0')==String::npos && id.substr(0,7)=="data://" && parameter.texture.id.set(id) && !parameter.texture.id.isSubResource() &&
                        parameter.texture.id.type()==resource::ResourceTypeID("texres"),"Invalid shader texture ResourceID");
                    if(!canonicalAssetCatalogPath(context.sourceRoot,parameter.texture.id.sourceRelativePath(),path,error))throw std::runtime_error(error.c_str());
                }
                else
                {
                    require(count==1 || (raw.is_array() && raw.as_array().size()==count),"Shader value has wrong component count");
                    for(u32 i=0;i<count;++i)
                    {
                        const auto &component=count==1?raw:raw.as_array()[i];
                        if(floats) { const auto f=scalar(component,-std::numeric_limits<float>::max(),std::numeric_limits<float>::max());std::memcpy(&parameter.words[i],&f,4); }
                        else parameter.words[i]=integer(component,signedValues?-2147483648.0:0.0,signedValues?2147483647.0:4294967295.0);
                    }
                }
                shader.parameters.push_back(std::move(parameter));
            }
            String error;
            if(!validateCookedMaterialShaderRecipe(shader,error))throw std::runtime_error(error.c_str());
            return shader;
        }

        CookedMaterialData readMaterialRecipe( const resource::CompileContext &context, Array<String> *shaderSources=nullptr )
        {
            const auto descriptor = readDescriptor(
                context, { "version", "baseColour", "metalness", "roughness", "baseColourTexture" },
                { "normalTexture", "ormTexture", "shader" } );
            CookedMaterialData recipe;
            const auto &colour = descriptor.at( "baseColour" );
            require( colour.is_array() && colour.as_array().size() == 4,
                     "Material baseColour must contain four numeric RGBA components" );
            const auto &values = colour.as_array();
            recipe.baseColour = ColourF( scalar( values[0], 0, 1 ), scalar( values[1], 0, 1 ),
                                         scalar( values[2], 0, 1 ), scalar( values[3], 0, 1 ) );
            recipe.metalness = scalar( descriptor.at( "metalness" ), 0, 1 );
            recipe.roughness = scalar( descriptor.at( "roughness" ), 0, 1 );
            const auto path = stringField( descriptor, "baseColourTexture" );
            require( path.substr( 0, 7 ) == "data://" && recipe.textureId.set( path ) &&
                         !recipe.textureId.isSubResource() &&
                         recipe.textureId.type() == resource::ResourceTypeID( "texres" ),
                     "Material baseColourTexture must identify a texres file resource" );
            AssetCatalogPath canonical;
            String error;
            if( !canonicalAssetCatalogPath( context.sourceRoot, recipe.textureId.sourceRelativePath(),
                                            canonical, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            const auto optionalTexture = [&]( const char *name, CookedMaterialTextureReference &ref ) {
                if( !descriptor.contains( name ) ) return;
                const auto path = stringField( descriptor, name );
                require( path.substr( 0, 7 ) == "data://" && ref.id.set( path ) &&
                             !ref.id.isSubResource() && ref.id.type() == resource::ResourceTypeID( "texres" ),
                         "Optional material texture must identify a texres file resource" );
                if( !canonicalAssetCatalogPath( context.sourceRoot, ref.id.sourceRelativePath(), canonical, error ) )
                    throw std::runtime_error( error.c_str() );
            };
            optionalTexture( "normalTexture", recipe.normalTexture );
            optionalTexture( "ormTexture", recipe.ormTexture );
            if(descriptor.contains("shader"))recipe.shader=readShaderRecipe(context,descriptor.at("shader"),shaderSources);
            recipe.target = context.target;
            recipe.packagedBuild = context.packagedBuild;
            return recipe;
        }

        u64 validateImageSize( u32 width, u32 height, const TextureMipSettings &settings,
                               u64 maximumBytes = graphicsTextureByteLimit )
        {
            require( width && height && width <= maxTextureDimension && height <= maxTextureDimension &&
                         settings.atlasColumns && width % settings.atlasColumns == 0,
                     "Image dimensions or atlas columns are unsupported" );
            u64 total = 0;
            for( ;; )
            {
                const auto bytes = u64( width ) * height * 4u;
                require( total <= maximumBytes && bytes <= maximumBytes - total,
                         "Decoded image and mip chain exceed the aggregate byte limit" );
                total += bytes;
                if( settings.filter == TextureMipFilter::None || ( width == 1 && height == 1 ) )
                {
                    break;
                }
                width = std::max( width / 2u, 1u );
                height = std::max( height / 2u, 1u );
                if( width < settings.atlasColumns || width % settings.atlasColumns )
                {
                    break;
                }
            }
            return total;
        }

        struct ImageLifetime
        {
            ImageLifetime()
            {
                FreeImage_Initialise( FALSE );
            }

            ~ImageLifetime()
            {
                FreeImage_DeInitialise();
            }
        };

        struct MemoryDeleter
        {
            void operator()( FIMEMORY *memory ) const
            {
                if( memory )
                {
                    FreeImage_CloseMemory( memory );
                }
            }
        };

        struct BitmapDeleter
        {
            void operator()( FIBITMAP *bitmap ) const
            {
                if( bitmap )
                {
                    FreeImage_Unload( bitmap );
                }
            }
        };

        using Bitmap = std::unique_ptr<FIBITMAP, BitmapDeleter>;

        CookedTextureData compileTexture( const resource::CompileContext &context,
                                          u64 maximumOutputBytes = graphicsTextureByteLimit,
                                          const std::function<void( u64 )> &admitEncoded = {},
                                          const std::function<void( u64 )> &admitDecoded = {}, bool portableDecode = false )
        {
            const auto recipe = readTextureRecipe( context );
            const auto policy = projectAssetPolicy( context );
            const auto sourcePath = fs::u8path( context.sourceRoot.c_str() ) /
                                    fs::u8path( recipe.source.substr( 7 ).c_str() );
            u64 encodedLimit = encodedImageByteLimit;
            if( admitEncoded )
            {
                std::error_code error;
                const auto size = fs::file_size( sourcePath, error );
                require( !error && size && size <= encodedImageByteLimit,
                         "Graphics source is missing, empty or exceeds its byte limit" );
                admitEncoded( size );
                encodedLimit = size;  // A file growing after admission must be rejected.
            }
            auto encoded = readFile( sourcePath, encodedLimit );
            if( sourcePath.extension() == ".dds" && ddsFormatForTarget( context.target ) )
            {
                const auto image = dds::inspect( encoded );
                require( image.format == ddsFormatForTarget( context.target ),
                         "Authored DDS format does not match its target profile" );
                require( !dds::blockBytes( image.format ) ||
                             ( image.mips[0].width % 4 == 0 && image.mips[0].height % 4 == 0 ),
                         "DX11 block-compressed base dimensions must be multiples of four" );
                CookedTextureData data;
                data.target = context.target;
                data.packagedBuild = context.packagedBuild;
                data.mipSettings = recipe.settings;
                data.ddsBytes = std::move( encoded );
                String error;
                require( validateCookedTextureData( data, error ), error.c_str() );
                return data;
            }
            std::lock_guard<std::mutex> codecLock( imageCodecMutex );
            static ImageLifetime imageLifetime;
            std::unique_ptr<FIMEMORY, MemoryDeleter> memory(
                FreeImage_OpenMemory( encoded.data(), static_cast<DWORD>( encoded.size() ) ) );
            require( bool( memory ), "Cannot open image decoder stream" );
            const auto format = FreeImage_GetFileTypeFromMemory( memory.get(), 0 );
            require( format != FIF_UNKNOWN && FreeImage_FIFSupportsReading( format ) &&
                         FreeImage_FIFSupportsNoPixels( format ),
                     "Image format must support bounded header inspection before decoding" );
            Bitmap header( FreeImage_LoadFromMemory( format, memory.get(), FIF_LOAD_NOPIXELS ) );
            require( bool( header ) && FreeImage_GetImageType( header.get() ) == FIT_BITMAP &&
                         FreeImage_GetBPP( header.get() ) <= 32,
                     "Only integer colour images with up to 32 bits per pixel are supported" );
            const auto width = FreeImage_GetWidth( header.get() );
            const auto height = FreeImage_GetHeight( header.get() );
            const auto decodedBytes =
                validateImageSize( width, height, recipe.settings, maximumOutputBytes );
            if( admitDecoded )
            {
                admitDecoded( decodedBytes );
            }
            header.reset();
            require( FreeImage_SeekMemory( memory.get(), 0, SEEK_SET ) != 0,
                     "Cannot rewind image decoder stream" );
            Bitmap bitmap( FreeImage_LoadFromMemory( format, memory.get(), 0 ) );
            require( bool( bitmap ) && FreeImage_GetWidth( bitmap.get() ) == width &&
                         FreeImage_GetHeight( bitmap.get() ) == height,
                     "Image decoding failed or changed the inspected dimensions" );
            Bitmap converted( FreeImage_ConvertTo32Bits( bitmap.get() ) );
            require( bool( converted ), "Image conversion to BGRA8 failed" );
            bitmap.reset();
            std::vector<std::uint8_t> pixels( size_t( width ) * height * 4u );
            for( u32 y = 0; y < height; ++y )
            {
                const auto *scanline = FreeImage_GetScanLine( converted.get(), height - 1u - y );
                require( scanline != nullptr, "Missing decoded image scanline" );
                for( u32 x = 0; x < width; ++x )
                {
                    auto *pixel = pixels.data() + ( size_t( y ) * width + x ) * 4u;
                    pixel[0] = scanline[x * 4u + FI_RGBA_BLUE];
                    pixel[1] = scanline[x * 4u + FI_RGBA_GREEN];
                    pixel[2] = scanline[x * 4u + FI_RGBA_RED];
                    pixel[3] = scanline[x * 4u + FI_RGBA_ALPHA];
                }
            }
            converted.reset();
            CookedTextureData data;
            data.target = context.target;
            data.packagedBuild = context.packagedBuild;
            data.mipSettings = recipe.settings;
            auto mipSettings = recipe.settings;
            const bool resize = recipe.maximumTextureSize || policy.present;
            if( resize && mipSettings.filter == TextureMipFilter::None ) mipSettings.filter = TextureMipFilter::Data;
            auto levels = generateTextureMips( pixels.data(), width, height, mipSettings );
            size_t firstMip = 0;
            const auto maximum = recipe.maximumTextureSize ? recipe.maximumTextureSize : policy.maximumTextureSize;
            if( resize )
            {
                const auto qualityDrop = recipe.maximumTextureSize || !policy.present ? 0 : policy.textureQuality == 0 ? 2 : policy.textureQuality == 1 ? 1 : 0;
                while( firstMip + 1 < levels.size() &&
                    ( firstMip < static_cast<size_t>( qualityDrop ) || levels[firstMip].width > maximum || levels[firstMip].height > maximum ) ) ++firstMip;
                require( levels[firstMip].width <= maximum && levels[firstMip].height <= maximum,
                         "Texture size limit conflicts with its atlas layout" );
            }
            if( firstMip ) levels.erase( levels.begin(), levels.begin() + firstMip );
            if( recipe.settings.filter == TextureMipFilter::None && levels.size() > 1 ) levels.resize( 1 );
            data.levels.reserve( levels.size() );
            for( auto &level : levels )
            {
                data.levels.push_back( std::move( level ) );
            }
            u32 gpuFormat = portableDecode ? 0 : ddsFormatForTarget( context.target );
            if( !portableDecode && ( !recipe.compression.empty() || policy.present ) )
            {
                bool alpha = false;
                for( size_t i = 3; i < data.levels.front().bgra.size(); i += 4 )
                    if( data.levels.front().bgra[i] != 255 ) { alpha = true; break; }
                auto selected = recipe.compression.empty() ? projectTextureCompressionName( policy, alpha ) : recipe.compression;
                if( recipe.compression.empty() && recipe.settings.filter == TextureMipFilter::Normal ) selected = "none";
                if( selected == "none" ) gpuFormat = 87;
                else if( selected == "bc1" ) gpuFormat = 71;
                else if( selected == "bc3" ) gpuFormat = 77;
                else
                {
                    TextureCompressionRequest request; request.format = selected; request.effort = policy.compression;
                    request.levels = data.levels.data(); request.levelCount = data.levels.size();
                    TextureCompressionOutput encoded; String error;
                    if( !encodeTextureCompression( request, encoded, error ) ) throw std::runtime_error( error.c_str() );
                    require( encoded.container == "dds", "A platform KTX2/PVR runtime texture installer is required for this encoder output" );
                    const auto image = dds::inspect( encoded.bytes ); gpuFormat = image.format;
                    require( selected != "bc7" || gpuFormat == 98 || gpuFormat == 99, "BC7 provider returned a different GPU format" );
                    data.ddsBytes = std::move( encoded.bytes );
                }
                if( recipe.compression.empty() && dds::blockBytes( gpuFormat ) &&
                    ( data.levels.front().width % 4 || data.levels.front().height % 4 ) ) gpuFormat = 87;
                data.storageFormat = gpuFormat;
            }
            if( gpuFormat )
            {
                require( !dds::blockBytes( gpuFormat ) || ( data.levels.front().width % 4 == 0 && data.levels.front().height % 4 == 0 ),
                         "DX11 block-compressed base dimensions must be multiples of four" );
                if( data.ddsBytes.empty() ) data.ddsBytes = dds::encode( data.levels, gpuFormat );
                data.levels.clear();
            }
            return data;
        }

        CookedMaterialData compileMaterial( const resource::CompileContext &context )
        {
            auto data = readMaterialRecipe( context );
            struct CachedPin { resource::ResourceID id;u64 source,payload,compiler;TextureMipFilter filter; };
            Array<CachedPin> pinnedTextures;
            const auto pin = [&]( const resource::ResourceID &id, MaterialTextureSlot slot,
                                  u64 &sourceHash, u64 &payloadHash, u64 &compilerVersion ) {
                AssetCatalogPath compiledPath;
                String error;
                for(const auto &cached:pinnedTextures)if(cached.id==id)
                {
                    CookedTextureData semantic;semantic.target=context.target;semantic.mipSettings.filter=cached.filter;
                    if(!validateMaterialTextureSemantic(semantic,slot,error))throw std::runtime_error(error.c_str());
                    sourceHash=cached.source;payloadHash=cached.payload;compilerVersion=cached.compiler;return;
                }
                const auto pinned = context.dependencyArtifacts.find( id.str() );
                if( pinned == context.dependencyArtifacts.end() )
                    throw std::runtime_error( "Material compiler requires a pinned texture artifact" );
                if( !canonicalAssetCatalogPath( context.compiledRoot, pinned->second, compiledPath, error ) )
                    throw std::runtime_error( error.c_str() );
                const auto path = fs::u8path( context.compiledRoot.c_str() ) / fs::u8path( compiledPath.path.c_str() );
                resource::RuntimeResource dependency;
                if( !resource::CompiledResourceIO::read( path.generic_u8string().c_str(), dependency, error,
                                                         graphicsTextureByteLimit + 4096u ) )
                    throw std::runtime_error( error.c_str() );
                CookedTextureData texture;
                if( !decodeCookedTexture( dependency, texture, error ) )
                    throw std::runtime_error( error.c_str() );
                require( dependency.header.resourceId == id && texture.target == context.target &&
                             texture.packagedBuild == context.packagedBuild,
                         "Compiled material dependency identity, target or build mode is incompatible" );
                if( !validateMaterialTextureSemantic( texture, slot, error ) )
                    throw std::runtime_error( error.c_str() );
                sourceHash = dependency.header.sourceHash;
                payloadHash = dependency.header.payloadHash;
                compilerVersion = dependency.header.compilerVersion;
                pinnedTextures.push_back({id,sourceHash,payloadHash,compilerVersion,texture.mipSettings.filter});
            };
            pin( data.textureId, MaterialTextureSlot::BaseColour, data.textureSourceHash,
                 data.texturePayloadHash, data.textureCompilerVersion );
            for( const auto slot : { MaterialTextureSlot::Normal, MaterialTextureSlot::ORM } )
            {
                auto &ref = slot == MaterialTextureSlot::Normal ? data.normalTexture : data.ormTexture;
                if( ref.id.isValid() ) pin( ref.id, slot, ref.sourceHash, ref.payloadHash, ref.compilerVersion );
            }
            for(auto &p:data.shader.parameters)if(p.type==MaterialShaderParameterType::Texture)
                pin(p.texture.id,MaterialTextureSlot::BaseColour,p.texture.sourceHash,p.texture.payloadHash,p.texture.compilerVersion);
            if(data.shader.present())
            {
                std::unique_ptr<wp_shader_program,decltype(&wp_shader_program_destroy)> definition(wp_shader_program_create(),&wp_shader_program_destroy);
                MaterialShaderParametersInstance values;String error;
                if(!populateCookedMaterialShader(data.shader,values,definition.get(),error))throw std::runtime_error(error.c_str());
                for(size_t stage=0;stage<2;++stage)
                {
                    const auto &code=stage==0?data.shader.vertexDXBC:data.shader.pixelDXBC;
                    const auto &selection=stage==0?data.shader.vertexSelection:data.shader.pixelSelection;
                    if(code.empty())continue;
                    if(!CreateDX11MaterialProgramBindingMap(values.GetLayout(),definition.get(),1,
                        stage==0?WORKPHONE_SHADER_TYPE_VERTEX:WORKPHONE_SHADER_TYPE_PIXEL,code.data(),code.size(),error,&selection))
                        throw std::runtime_error(error.c_str());
                }
            }
            return data;
        }
    }  // namespace

    bool decodeGraphicsTextureSource( const String &sourceRoot, const resource::ResourceID &id,
                                      CookedTextureData &output, String &error, u64 maximumOutputBytes,
                                      StreamBudget *retainedBudget )
    {
        error.clear();
        try
        {
            require(
                id.isValid() && !id.isSubResource() && id.type() == resource::ResourceTypeID( "texres" ),
                "Graphics source decoding requires a texres file resource" );
            require( maximumOutputBytes && maximumOutputBytes <= graphicsTextureByteLimit,
                     "Graphics source output budget must be between one byte and 256 MiB" );
            resource::CompileContext context;
            if( !canonicalAssetCatalogRoot( sourceRoot, context.sourceRoot, error ) )
            {
                return false;
            }
            AssetCatalogPath descriptor;
            if( !canonicalAssetCatalogPath( context.sourceRoot, id.sourceRelativePath(), descriptor,
                                            error ) )
            {
                return false;
            }
            context.resourceId = id;
            context.sourcePath =
                ( fs::u8path( context.sourceRoot.c_str() ) / fs::u8path( descriptor.path.c_str() ) )
                    .generic_u8string()
                    .c_str();
            context.target = "windows-dx11";
            context.packagedBuild = false;
            auto scheduler = WorldStreamingScheduler::shared();
            auto metadataBudget = scheduler->reserve( StreamMemoryClass::WorkingCpu, 4ull << 20, error );
            if( !metadataBudget )
            {
                return false;
            }
            StreamBudget encodedBudget, scratchBudget, sourceBudget;
            auto candidate = compileTexture(
                context, maximumOutputBytes,
                [&]( u64 bytes ) {
                    encodedBudget = scheduler->reserve( StreamMemoryClass::WorkingCpu, bytes, error );
                    if( !encodedBudget )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                },
                [&]( u64 bytes ) {
                    // Decoder source/conversion, top-down pixels and mip generation
                    // can overlap. Retained result bytes are admitted separately.
                    scratchBudget =
                        scheduler->reserve( StreamMemoryClass::WorkingCpu, bytes * 5, error );
                    if( !scratchBudget )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                    if( retainedBudget )
                    {
                        sourceBudget =
                            scheduler->reserve( StreamMemoryClass::SourceCpu, bytes + 4096, error );
                        if( !sourceBudget )
                        {
                            throw std::runtime_error( error.c_str() );
                        }
                    }
                }, true );
            output = std::move( candidate );
            if( retainedBudget )
            {
                *retainedBudget = std::move( sourceBudget );
            }
            error.clear();
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    String GraphicsResourceCompiler::name() const
    {
        return "WPGraphics.GraphicsResourceCompiler";
    }

    Array<resource::CompilerOutput> GraphicsResourceCompiler::outputs() const
    {
        return { { resource::ResourceTypeID( "texres" ), 3 },
                 { resource::ResourceTypeID( "matres" ), 5 } };
    }

    bool GraphicsResourceCompiler::getDependencies( const resource::CompileContext &context,
                                                    resource::DependencySet &dependencies,
                                                    String &error ) const
    {
        dependencies = resource::DependencySet();
        error.clear();
        try
        {
            require( !context.resourceId.isSubResource(),
                     "Graphics descriptors cannot be subresources" );
            projectAssetPolicy( context );
            auto policyDependency = resource::CompileDependency::data( projectPolicyInput );
            policyDependency.optional = true;
            dependencies.compileDependencies.push_back( policyDependency );
            if( context.resourceId.type() == resource::ResourceTypeID( "texres" ) )
            {
                const auto recipe = readTextureRecipe( context );
                dependencies.compileDependencies.push_back(
                    resource::CompileDependency::data( recipe.source ) );
            }
            else if( context.resourceId.type() == resource::ResourceTypeID( "matres" ) )
            {
                Array<String> shaderSources;
                const auto recipe = readMaterialRecipe( context, &shaderSources );
                for(const auto &source:shaderSources)dependencies.compileDependencies.push_back(resource::CompileDependency::data(source));
                dependencies.compileDependencies.push_back(
                    resource::CompileDependency::resource( recipe.textureId ) );
                dependencies.installDependencies.push_back( recipe.textureId );
                for( const auto &ref : { recipe.normalTexture, recipe.ormTexture } )
                {
                    if( !ref.id.isValid() || std::find( dependencies.installDependencies.begin(),
                            dependencies.installDependencies.end(), ref.id ) != dependencies.installDependencies.end() ) continue;
                    dependencies.compileDependencies.push_back( resource::CompileDependency::resource( ref.id ) );
                    dependencies.installDependencies.push_back( ref.id );
                }
                for(const auto &p:recipe.shader.parameters)
                    if(p.type==MaterialShaderParameterType::Texture && std::find(dependencies.installDependencies.begin(),dependencies.installDependencies.end(),p.texture.id)==dependencies.installDependencies.end())
                    { dependencies.compileDependencies.push_back(resource::CompileDependency::resource(p.texture.id));dependencies.installDependencies.push_back(p.texture.id); }
            }
            else
            {
                throw std::runtime_error( "Unsupported graphics resource type" );
            }
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    resource::CompilationStatus GraphicsResourceCompiler::compile(
        const resource::CompileContext &context, std::ostream &output, Array<String> &messages ) const
    {
        try
        {
            require( !context.resourceId.isSubResource(),
                     "Graphics descriptors cannot be subresources" );
            String error;
            bool succeeded = false;
            std::ostringstream raw( std::ios::binary );
            if( context.resourceId.type() == resource::ResourceTypeID( "texres" ) )
            {
                succeeded = writeCookedTexture( compileTexture( context ), raw, error );
            }
            else if( context.resourceId.type() == resource::ResourceTypeID( "matres" ) )
            {
                succeeded = writeCookedMaterial( compileMaterial( context ), raw, error );
            }
            else
            {
                error = "Unsupported graphics resource type";
            }
            if( !succeeded )
            {
                messages.push_back( error );
                return resource::CompilationStatus::Failure;
            }
            const auto text = raw.str();
            Array<u8> bytes( text.begin(), text.end() );
            const auto encoded = compressProjectAssetPayload( bytes, projectAssetPolicy( context ).compression );
            output.write( reinterpret_cast<const char *>( encoded.data() ), encoded.size() );
            require( output.good(), "Failed writing compressed graphics payload" );
            return resource::CompilationStatus::Success;
        }
        catch( const std::exception &exception )
        {
            messages.push_back( exception.what() );
            return resource::CompilationStatus::Failure;
        }
    }
}  // namespace workphone::render
