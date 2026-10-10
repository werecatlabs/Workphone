#ifndef WPGraphicsResourceCompilerContracts_h__
#define WPGraphicsResourceCompilerContracts_h__

#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>

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
        require( dependencies.compileDependencies.size() == 1 &&
                     !dependencies.compileDependencies.front().isResource &&
                     dependencies.compileDependencies.front().path == "data://image.tga" &&
                     dependencies.installDependencies.empty(),
                 "Texture must expose its authored image as a raw compile dependency" );
        auto texture = cook( compiler, context );
        CookedTextureData textureData;
        require( decodeCookedTexture( texture, textureData, error ) && textureData.levels.size() == 3 &&
                     textureData.target == context.target && textureData.packagedBuild,
                 "Texture cook must carry its full mip chain and build configuration" );
        require( std::memcmp( textureData.levels.front().bgra.data(), tga.data() + 18, 64 ) == 0,
                 "Texture cook must preserve top-origin BGRA channel order" );

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
                     dependencies.compileDependencies.size() == 1 &&
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
