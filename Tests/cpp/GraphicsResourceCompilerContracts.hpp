#ifndef WPGraphicsResourceCompilerContracts_h__
#define WPGraphicsResourceCompilerContracts_h__

#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/DdsTexture.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace graphics_compiler_contracts
{
    using namespace workphone;
    using namespace workphone::resource;
    using namespace workphone::render;
    namespace fs = std::filesystem;

    inline void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    inline void write( const fs::path &path, const std::string &bytes )
    {
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream.write( bytes.data(), static_cast<std::streamsize>( bytes.size() ) );
        require( stream.good(), "Graphics compiler fixture must be writable" );
    }

    inline void refreshIntegrity( RuntimeResource &value )
    {
        value.header.payloadSize = value.payload.size();
        value.header.payloadHash = hashBytes( value.payload.data(), value.payload.size() );
    }

    inline RuntimeResource wrap( const ResourceID &id, const std::string &bytes )
    {
        RuntimeResource value;
        value.header.resourceId = id;
        value.header.resourceType = id.type();
        value.header.compilerVersion = GraphicsResourceCompiler().versionFor( id.type() );
        value.header.sourceHash = 12345;
        value.payload.resize( bytes.size() );
        std::memcpy( value.payload.data(), bytes.data(), bytes.size() );
        refreshIntegrity( value );
        return value;
    }

    inline void overwrite32( RuntimeResource &value, size_t offset, u32 bits )
    {
        require( offset + 4 <= value.payload.size(), "Invalid payload fixture offset" );
        for( size_t i = 0; i < 4; ++i )
            value.payload[offset + i] = static_cast<u8>( bits >> ( i * 8 ) );
        refreshIntegrity( value );
    }

    inline RuntimeResource cook( GraphicsResourceCompiler &compiler, const CompileContext &context )
    {
        std::ostringstream output( std::ios::binary );
        Array<String> messages;
        require( compiler.compile( context, output, messages ) == CompilationStatus::Success,
                 "Valid graphics descriptor must compile" );
        return wrap( context.resourceId, output.str() );
    }

    inline void run( const fs::path &folder )
    {
        const auto root = folder / "graphics-compiler-contracts";
        const auto source = root / "source";
        const auto compiled = root / "compiled";
        fs::create_directories( source );
        fs::create_directories( compiled );
        // A top-origin, uncompressed BGRA TGA has explicit bytes and no encoder dependency.
        std::string tga( 18 + 4 * 4 * 4, '\0' );
        tga[2] = 2;
        tga[12] = 4;
        tga[14] = 4;
        tga[16] = 32;
        tga[17] = 0x28;
        for( size_t i = 18; i < tga.size(); i += 4 )
        {
            tga[i] = static_cast<char>( i );
            tga[i + 1] = static_cast<char>( i + 30 );
            tga[i + 2] = static_cast<char>( i + 60 );
            tga[i + 3] = static_cast<char>( 255 );
        }
        write( source / "image.tga", tga );
        const std::string textureJson =
            R"({"version":1,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})";
        const std::string materialJson =
            R"({"version":1,"baseColour":[0.8,0.6,0.4,1],"metalness":0.2,"roughness":0.7,"baseColourTexture":"data://image.texres"})";
        write( source / "image.texres", textureJson );
        write( source / "surface.matres", materialJson );
        GraphicsResourceCompiler compiler;
        CompileContext context;
        context.resourceId = ResourceID( "data://image.texres" );
        context.sourcePath = ( source / "image.texres" ).u8string().c_str();
        // ResourceSystem supplies canonical roots, including expanded Windows 8.3
        // components. Direct compiler fixtures must honor the same contract.
        context.sourceRoot = fs::canonical( source ).generic_u8string().c_str();
        context.compiledRoot = fs::canonical( compiled ).generic_u8string().c_str();
        context.target = "dx11-contract";
        context.packagedBuild = true;
        DependencySet dependencies;
        String error;
        if( !compiler.getDependencies( context, dependencies, error ) )
            throw std::runtime_error(
                ( String( "Texture dependency discovery failed: " ) + error ).c_str() );
        require( dependencies.compileDependencies.size() == 2 &&
                     !dependencies.compileDependencies.front().isResource &&
                     dependencies.compileDependencies.front().path == "data://image.tga" &&
                     dependencies.compileDependencies.back().optional && dependencies.installDependencies.empty(),
                 "Texture must expose its authored image as a raw compile dependency" );
        auto texture = cook( compiler, context );
        CookedTextureData textureData;
        require( decodeCookedTexture( texture, textureData, error ) && textureData.levels.size() == 3 &&
                     textureData.target == context.target && textureData.packagedBuild,
                 "Texture cook must carry its full mip chain and build configuration" );
        require( std::memcmp( textureData.levels.front().bgra.data(), tga.data() + 18, 64 ) == 0,
                 "Texture cook must preserve top-origin BGRA channel order" );

        for(const char *profile : {"windows-dx11-dds-bgra8-v1","windows-dx11-dds-bgra8-srgb-v1",
                                  "windows-dx11-dds-bc1-v1","windows-dx11-dds-bc1-srgb-v1",
                                  "windows-dx11-dds-bc3-v1","windows-dx11-dds-bc3-srgb-v1"})
        {
            auto nativeContext=context;nativeContext.target=profile;
            auto native=cook(compiler,nativeContext);
            CookedTextureData nativeData;
            require(decodeCookedTexture(native,nativeData,error) && nativeData.levels.empty(),
                    "DDS profiles must carry native payloads without decoded BGRA copies");
            auto image=dds::inspect(nativeData.ddsBytes);
            require(image.format==ddsFormatForTarget(profile) && image.mips.size()==3 &&
                    image.mips[0].width==4 && image.mips[2].width==1,
                    "DDS profile, dimensions and mip tails must match");
            require(cook(compiler,nativeContext).payload==native.payload,
                    "Repeated DDS compilation must produce identical bytes");
            const auto rejectDds=[&](std::vector<u8> bytes){
                bool rejected=false;try{dds::inspect(bytes);}catch(const std::exception &){rejected=true;}
                require(rejected,"Malformed DDS must fail inspection");
            };
            auto bytes=nativeData.ddsBytes;bytes.pop_back();rejectDds(bytes);
            bytes=nativeData.ddsBytes;bytes.push_back(0);rejectDds(bytes);
            for(auto field : {4u,76u,84u,128u,132u,140u})
            { bytes=nativeData.ddsBytes;dds::put(bytes,field,0);rejectDds(bytes); }
            bytes=nativeData.ddsBytes;dds::put(bytes,16,0xffffffff);rejectDds(bytes);
            bytes=nativeData.ddsBytes;dds::put(bytes,20,1);rejectDds(bytes);
            bytes=nativeData.ddsBytes;dds::put(bytes,136,4);rejectDds(bytes);
            bytes=nativeData.ddsBytes;dds::put(bytes,140,2);rejectDds(bytes);
            nativeData.target="portable-profile";
            require(!validateCookedTextureData(nativeData,error),"DDS format cannot be installed under a portable target");
            nativeData.target=profile;
            dds::put(nativeData.ddsBytes,144,2);
            require(!validateCookedTextureData(nativeData,error),"DDS alpha mode cannot silently change profile semantics");
            dds::put(nativeData.ddsBytes,144,dds::blockBytes(image.format)==8?3u:1u);
            // Passthrough is byte-exact, including the full mip chain.
            write(source/"native.dds",std::string(reinterpret_cast<const char *>(nativeData.ddsBytes.data()),nativeData.ddsBytes.size()));
            write(source/"image.texres",R"({"version":1,"source":"data://native.dds","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})");
            CookedTextureData passthrough;
            require(decodeCookedTexture(cook(compiler,nativeContext),passthrough,error) &&
                    passthrough.ddsBytes==nativeData.ddsBytes,"Authored DDS passthrough must retain exact native bytes");
            write(source/"image.texres",textureJson);
        }
        {
            auto policy=make_ptr<Properties>();policy->setProperty("assets.compression",s32(3));
            policy->setProperty("assets.textureCompression.pc",s32(1));policy->setProperty("platform.active",s32(0));
            policy->setProperty("quality.texture",s32(2));policy->setProperty("quality.maxTextureSize",s32(4096));
            require(writeProjectAssetPolicy(context.sourceRoot,policy,error),error.c_str());
            const auto compressed=cook(compiler,context);CookedTextureData encoded;
            require(compressed.payload.size()>4 && std::memcmp(compressed.payload.data(),"WPZ1",4)==0 &&
                decodeCookedTexture(compressed,encoded,error) && encoded.storageFormat==71 &&
                dds::inspect(encoded.ddsBytes).format==71,
                "Project texture preference produces actual BC1 and bounded lossless payload compression");
            write(source/"image.texres",R"({"version":1,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5,"compression":"none"})");
            require(decodeCookedTexture(cook(compiler,context),encoded,error) && encoded.storageFormat==87,
                "Explicit asset compression recipe overrides the project default");
            write(source/"image.texres",textureJson);policy->setProperty("quality.texture",s32(1));
            require(writeProjectAssetPolicy(context.sourceRoot,policy,error),error.c_str());
            require(decodeCookedTexture(cook(compiler,context),encoded,error) && dds::inspect(encoded.ddsBytes).mips.front().width==2,
                "Selected texture quality resizes cooked data while retaining valid mip tails");
            policy->setProperty("assets.textureCompression.pc",s32(2));
            require(writeProjectAssetPolicy(context.sourceRoot,policy,error),error.c_str());
            std::ostringstream rejected;Array<String> diagnostics;
            require(compiler.compile(context,rejected,diagnostics)==CompilationStatus::Failure && rejected.str().empty(),
                "Missing BC7 encoder rejects without silently writing BC1 data");
            bool dispatched=false;
            registerTextureCompressionEncoder("bc7",[&](const TextureCompressionRequest &request,TextureCompressionOutput &,String &message){
                dispatched=request.format=="bc7" && request.levels && request.levelCount>0;
                message="Controlled provider failure";return false;});
            require(compiler.compile(context,rejected,diagnostics)==CompilationStatus::Failure && dispatched && rejected.str().empty(),
                "A registered provider receives the real image request and failure remains transactional");
            registerTextureCompressionEncoder("bc7",{});
            fs::remove(source/".workphone/ProjectAssetPolicy.settings");
        }
        {
            Array<TextureMipLevel> odd;
            for(auto size : {std::pair<u32,u32>{3,5},{1,2},{1,1}})
            {
                TextureMipLevel mip;mip.width=size.first;mip.height=size.second;
                mip.bgra.assign(mip.width*mip.height*4,255);odd.push_back(std::move(mip));
            }
            auto bytes=dds::encode(odd,77);
            auto image=dds::inspect(bytes);
            require(image.mips.size()==3 && image.mips[0].rowPitch==16 && image.mips[0].size==32 &&
                    image.mips[2].size==16,"DDS inspection must compute odd dimensions and block mip tails exactly");
            require(dds::inspect(dds::encode(odd,87)).mips[0].size==60,
                    "Lossless DDS must preserve odd dimensions without block padding");
            odd[0].bgra[3]=0;
            bool rejected=false;try{dds::encode(odd,71);}catch(const std::exception &){rejected=true;}
            require(rejected,"Opaque BC1 must reject alpha instead of dropping it");
        }

        const auto rejectDescriptor = [&]( const std::string &json ) {
            write( source / "image.texres", json );
            require( !compiler.getDependencies( context, dependencies, error ) && !error.empty(),
                     "Malformed or unsupported texture descriptor must fail dependency discovery" );
            std::ostringstream output;
            Array<String> messages;
            require( compiler.compile( context, output, messages ) == CompilationStatus::Failure &&
                         !messages.empty() && output.str().empty(),
                     "Malformed texture descriptor must fail without publishing payload bytes" );
        };
        rejectDescriptor( textureJson.substr( 0, textureJson.size() - 1 ) );
        rejectDescriptor(
            R"({"version":2,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})" );
        rejectDescriptor(
            R"({"version":1,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5,"unknown":true})" );
        rejectDescriptor(
            R"({"version":1,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5,"\u0076ersion":1})" );
        rejectDescriptor(
            R"({"version":1,"source":"data://../image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})" );
        rejectDescriptor(
            R"({"version":1,"source":"data://image.tga","mipFilter":"bogus","atlasColumns":1,"alphaCutoff":0.5})" );
        rejectDescriptor(
            R"({"version":1,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1.00000001,"alphaCutoff":0.5})" );
        rejectDescriptor(
            R"({"version":1,"source":"data://image.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":1e400})" );
        rejectDescriptor( std::string( graphicsResourceDescriptorLimit + 1, ' ' ) );
        write( source / "image.texres", textureJson );
        auto subresource = context;
        subresource.resourceId = ResourceID( "data://image.texres:child.texres" );
        require( !compiler.getDependencies( subresource, dependencies, error ),
                 "Graphics dependency discovery must reject subresource aliases" );
        std::ostringstream ignored;
        Array<String> messages;
        require( compiler.compile( subresource, ignored, messages ) == CompilationStatus::Failure,
                 "Graphics compilation must reject subresource aliases" );
        // A giant header must fail before FreeImage tries to allocate decoded pixels.
        auto hugeTga = tga;
        hugeTga[12] = static_cast<char>( 255 );
        hugeTga[13] = static_cast<char>( 255 );
        write( source / "image.tga", hugeTga );
        require( compiler.compile( context, ignored, messages ) == CompilationStatus::Failure,
                 "Oversized image header must fail before decoded pixel allocation" );
        write( source / "image.tga", tga );

        const auto rejectTexture = [&]( RuntimeResource value ) {
            CookedTextureData result;
            result.target = "previous";
            require( !decodeCookedTexture( value, result, error ) && !error.empty() &&
                         result.levels.empty() && result.target.empty(),
                     "Invalid texture payload must fail and clear output" );
        };
        auto invalid = texture;
        invalid.payload.back() ^= 1;
        rejectTexture( invalid );  // Public reader checks integrity, even without container I/O.
        invalid = texture;
        ++invalid.header.payloadSize;
        rejectTexture( invalid );
        invalid = texture;
        ++invalid.header.compilerVersion;
        rejectTexture( invalid );
        invalid = texture;
        invalid.header.resourceType = ResourceTypeID( "matres" );
        rejectTexture( invalid );
        invalid = texture;
        overwrite32( invalid, 4, 2 );
        rejectTexture( invalid );
        invalid = texture;
        invalid.payload.resize( invalid.payload.size() - 1 );
        refreshIntegrity( invalid );
        rejectTexture( invalid );
        invalid = texture;
        invalid.payload.push_back( 0 );
        refreshIntegrity( invalid );
        rejectTexture( invalid );
        const size_t prefix = 16 + context.target.size();
        for( const auto corruption : { std::pair<size_t, u32>{ prefix, 100 },
                                       { prefix + 8, 0x7fc00000u },
                                       { prefix + 12, 0xffffffffu },
                                       { prefix + 16, 16385u },
                                       { prefix + 24, 0xffffffffu } } )
        {
            invalid = texture;
            overwrite32( invalid, corruption.first, corruption.second );
            rejectTexture( invalid );
        }

        const auto payloadFile = root / "texture.payload";
        write( payloadFile, std::string( reinterpret_cast<const char *>( texture.payload.data() ),
                                         texture.payload.size() ) );
        require(
            CompiledResourceIO::writeAtomic( ( compiled / "image.texres" ).u8string().c_str(),
                                             texture.header, payloadFile.u8string().c_str(), error ),
            "Material fixture requires a real compiled texture container" );
        auto materialContext = context;
        materialContext.resourceId = ResourceID( "data://surface.matres" );
        materialContext.sourcePath = ( source / "surface.matres" ).u8string().c_str();
        materialContext.dependencyArtifacts["data://image.texres"]=(compiled / "image.texres").u8string().c_str();
        require( compiler.getDependencies( materialContext, dependencies, error ) &&
                     dependencies.compileDependencies.size() == 2 &&
                     dependencies.compileDependencies.front().isResource &&
                     dependencies.installDependencies.size() == 1 &&
                     dependencies.installDependencies.front() == texture.header.resourceId,
                 "Material must expose its texture as compile and install dependency" );
        auto material = cook( compiler, materialContext );
        material.header.installDependencies = dependencies.installDependencies;
        material.dependencies.push_back( std::make_shared<const RuntimeResource>( texture ) );
        CookedMaterialData materialData;
        require( decodeCookedMaterial( material, materialData, error ) &&
                     materialData.textureId == texture.header.resourceId &&
                     materialData.textureSourceHash == texture.header.sourceHash &&
                     materialData.texturePayloadHash == texture.header.payloadHash &&
                     materialData.textureCompilerVersion == texture.header.compilerVersion &&
                     materialData.target == context.target && materialData.packagedBuild,
                 "Material must pin and validate the exact cooked texture generation" );
        const auto rejectMaterial = [&]( RuntimeResource value ) {
            CookedMaterialData result;
            result.target = "previous";
            require(
                !decodeCookedMaterial( value, result, error ) && !error.empty() && result.target.empty(),
                "Invalid or incoherent material must fail and clear output" );
        };
        invalid = material;
        overwrite32( invalid, prefix, 0x7fc00000u );
        rejectMaterial( invalid );
        invalid = material;
        invalid.dependencies.clear();
        rejectMaterial( invalid );
        invalid = material;
        invalid.header.installDependencies.front() = ResourceID( "data://other.texres" );
        rejectMaterial( invalid );
        for( unsigned pin = 0; pin < 3; ++pin )
        {
            auto wrongTexture = texture;
            if( pin == 0 )
                ++wrongTexture.header.sourceHash;
            else if( pin == 1 )
                ++wrongTexture.header.payloadHash;
            else
                ++wrongTexture.header.compilerVersion;
            invalid = material;
            invalid.dependencies.front() = std::make_shared<const RuntimeResource>( wrongTexture );
            rejectMaterial( invalid );
        }
        // Keep the generation pins correct: these specifically test typed dependency
        // validation and target/mode consistency after the pin comparisons succeed.
        auto malformedTexture = texture;
        overwrite32( malformedTexture, 4, 2 );
        auto malformedPin = materialData;
        malformedPin.texturePayloadHash = malformedTexture.header.payloadHash;
        std::ostringstream malformedOutput;
        require( writeCookedMaterial( malformedPin, malformedOutput, error ),
                 "Malformed dependency fixture material must encode" );
        invalid = wrap( material.header.resourceId, malformedOutput.str() );
        invalid.header.installDependencies = material.header.installDependencies;
        invalid.dependencies.push_back( std::make_shared<const RuntimeResource>( malformedTexture ) );
        rejectMaterial( invalid );
        for( bool changeTarget : { false, true } )
        {
            auto mismatch = materialData;
            if( changeTarget )
                mismatch.target = "another-target";
            else
                mismatch.packagedBuild = !mismatch.packagedBuild;
            std::ostringstream output;
            require( writeCookedMaterial( mismatch, output, error ), "Mismatch fixture must encode" );
            invalid = wrap( material.header.resourceId, output.str() );
            invalid.header.installDependencies = material.header.installDependencies;
            invalid.dependencies = material.dependencies;
            rejectMaterial( invalid );
        }
        auto mismatchContext = materialContext;
        mismatchContext.target = "another-target";
        require( compiler.compile( mismatchContext, ignored, messages ) == CompilationStatus::Failure,
                 "Material cooker must reject a texture cooked for a different target" );
        write(
            source / "surface.matres",
            R"({"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":-1,"baseColourTexture":"data://image.texres"})" );
        require( !compiler.getDependencies( materialContext, dependencies, error ),
                 "Out-of-range material scalar must fail descriptor validation" );
    }
}  // namespace graphics_compiler_contracts

inline void runGraphicsResourceCompilerContracts( const std::filesystem::path &folder )
{
    graphics_compiler_contracts::run( folder );
}

#endif
