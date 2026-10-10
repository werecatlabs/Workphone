#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <boost/json.hpp>
#include <FreeImage.h>

#include <cmath>
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
                throw std::runtime_error( message );
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
            int depth = 0;
            std::set<std::string> keys;
            for( size_t i = 0; i < text.size(); ++i )
            {
                if( text[i] == '"' )
                {
                    const auto start = i;
                    for( ++i; i < text.size(); ++i )
                    {
                        if( text[i] == '\\' )
                            ++i;
                        else if( text[i] == '"' )
                            break;
                    }
                    auto next = i + 1;
                    while( next < text.size() && ( text[next] == ' ' || text[next] == '\t' ||
                                                   text[next] == '\r' || text[next] == '\n' ) )
                        ++next;
                    if( depth == 1 && next < text.size() && text[next] == ':' )
                    {
                        const auto key =
                            json::parse( json::string_view( text.data() + start, i - start + 1 ) )
                                .as_string();
                        require( keys.insert( std::string( key.data(), key.size() ) ).second,
                                 "Duplicate graphics descriptor field" );
                    }
                }
                else if( text[i] == '{' || text[i] == '[' )
                    ++depth;
                else if( text[i] == '}' || text[i] == ']' )
                    --depth;
            }
        }

        json::object readDescriptor( const resource::CompileContext &context,
                                     const std::set<std::string> &fields )
        {
            auto bytes =
                readFile( fs::u8path( context.sourcePath.c_str() ), graphicsResourceDescriptorLimit );
            const std::string text( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
            json::parse_options options;
            options.max_depth = 4;
            auto value = json::parse( text, {}, options );
            require( value.is_object(), "Graphics descriptor must be a JSON object" );
            validateUniqueKeys( text );
            const auto &object = value.as_object();
            require( object.size() == fields.size(),
                     "Graphics descriptor has missing or unsupported fields" );
            for( const auto &field : object )
                require( fields.count( std::string( field.key().data(), field.key().size() ) ) != 0,
                         "Unsupported graphics descriptor field" );
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
        };

        TextureRecipe readTextureRecipe( const resource::CompileContext &context )
        {
            const auto descriptor = readDescriptor(
                context, { "version", "source", "mipFilter", "atlasColumns", "alphaCutoff" } );
            TextureRecipe recipe;
            auto path = stringField( descriptor, "source" );
            require( path.size() > 7 && path.substr( 0, 7 ) == "data://",
                     "Texture source must be a data:// path under the project source root" );
            AssetCatalogPath canonical;
            String error;
            if( !canonicalAssetCatalogPath( context.sourceRoot, path.substr( 7 ), canonical, error ) )
                throw std::runtime_error( error.c_str() );
            recipe.source = "data://" + canonical.path;
            const auto filter = stringField( descriptor, "mipFilter" );
            if( filter == "none" )
                recipe.settings.filter = TextureMipFilter::None;
            else if( filter == "colour" )
                recipe.settings.filter = TextureMipFilter::Colour;
            else if( filter == "data" )
                recipe.settings.filter = TextureMipFilter::Data;
            else if( filter == "normal" )
                recipe.settings.filter = TextureMipFilter::Normal;
            else if( filter == "cutout" )
                recipe.settings.filter = TextureMipFilter::Cutout;
            else if( filter == "roughness" )
                recipe.settings.filter = TextureMipFilter::Roughness;
            else
                throw std::runtime_error( "Unknown texture mipFilter" );
            const auto &columnValue = descriptor.at( "atlasColumns" );
            require( columnValue.is_number(), "Texture atlasColumns must be numeric" );
            const auto columns = columnValue.to_number<double>();
            require( std::isfinite( columns ) && columns >= 1 && columns <= maxTextureDimension,
                     "Texture atlasColumns is outside its supported range" );
            require( std::floor( columns ) == columns, "Texture atlasColumns must be an integer" );
            recipe.settings.atlasColumns = static_cast<u32>( columns );
            recipe.settings.alphaCutoff = scalar( descriptor.at( "alphaCutoff" ), 0.0f, 1.0f );
            require( recipe.settings.alphaCutoff > 0.0f && recipe.settings.alphaCutoff < 1.0f,
                     "Texture alphaCutoff must be strictly between zero and one" );
            return recipe;
        }

        CookedMaterialData readMaterialRecipe( const resource::CompileContext &context )
        {
            const auto descriptor = readDescriptor(
                context, { "version", "baseColour", "metalness", "roughness", "baseColourTexture" } );
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
                throw std::runtime_error( error.c_str() );
            recipe.target = context.target;
            recipe.packagedBuild = context.packagedBuild;
            return recipe;
        }

        void validateImageSize( u32 width, u32 height, const TextureMipSettings &settings )
        {
            require( width && height && width <= maxTextureDimension && height <= maxTextureDimension &&
                         settings.atlasColumns && width % settings.atlasColumns == 0,
                     "Image dimensions or atlas columns are unsupported" );
            u64 total = 0;
            for( ;; )
            {
                const auto bytes = u64( width ) * height * 4u;
                require( bytes <= graphicsTextureByteLimit - total,
                         "Decoded image and mip chain exceed the aggregate byte limit" );
                total += bytes;
                if( settings.filter == TextureMipFilter::None || ( width == 1 && height == 1 ) )
                    break;
                width = std::max( width / 2u, 1u );
                height = std::max( height / 2u, 1u );
                if( width < settings.atlasColumns || width % settings.atlasColumns )
                    break;
            }
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
                    FreeImage_CloseMemory( memory );
            }
        };
        struct BitmapDeleter
        {
            void operator()( FIBITMAP *bitmap ) const
            {
                if( bitmap )
                    FreeImage_Unload( bitmap );
            }
        };
        using Bitmap = std::unique_ptr<FIBITMAP, BitmapDeleter>;

        CookedTextureData compileTexture( const resource::CompileContext &context )
        {
            const auto recipe = readTextureRecipe( context );
            const auto sourcePath = fs::u8path( context.sourceRoot.c_str() ) /
                                    fs::u8path( recipe.source.substr( 7 ).c_str() );
            auto encoded = readFile( sourcePath, encodedImageByteLimit );
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
            validateImageSize( width, height, recipe.settings );
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
            auto levels = generateTextureMips( pixels.data(), width, height, recipe.settings );
            data.levels.reserve( levels.size() );
            for( auto &level : levels )
                data.levels.push_back( std::move( level ) );
            return data;
        }

        CookedMaterialData compileMaterial( const resource::CompileContext &context )
        {
            auto data = readMaterialRecipe( context );
            AssetCatalogPath compiledPath;
            String error;
            const auto pinned=context.dependencyArtifacts.find(data.textureId.str());
            if(pinned==context.dependencyArtifacts.end())
                throw std::runtime_error("Material compiler requires a pinned texture artifact");
            if( !canonicalAssetCatalogPath( context.compiledRoot, pinned->second,
                                            compiledPath, error ) )
                throw std::runtime_error( error.c_str() );
            const auto path =
                fs::u8path( context.compiledRoot.c_str() ) / fs::u8path( compiledPath.path.c_str() );
            resource::RuntimeResource dependency;
            if( !resource::CompiledResourceIO::read( path.generic_u8string().c_str(), dependency, error,
                                                     graphicsTextureByteLimit + 4096u ) )
                throw std::runtime_error( error.c_str() );
            CookedTextureData texture;
            if( !decodeCookedTexture( dependency, texture, error ) )
                throw std::runtime_error( error.c_str() );
            require( dependency.header.resourceId == data.textureId &&
                         texture.target == context.target &&
                         texture.packagedBuild == context.packagedBuild,
                     "Compiled material dependency identity, target or build mode is incompatible" );
            data.textureSourceHash = dependency.header.sourceHash;
            data.texturePayloadHash = dependency.header.payloadHash;
            data.textureCompilerVersion = dependency.header.compilerVersion;
            return data;
        }
    }  // namespace

    String GraphicsResourceCompiler::name() const
    {
        return "WPGraphics.GraphicsResourceCompiler";
    }

    Array<resource::CompilerOutput> GraphicsResourceCompiler::outputs() const
    {
        return { { resource::ResourceTypeID( "texres" ), 1 },
                 { resource::ResourceTypeID( "matres" ), 1 } };
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
            if( context.resourceId.type() == resource::ResourceTypeID( "texres" ) )
            {
                const auto recipe = readTextureRecipe( context );
                dependencies.compileDependencies.push_back(
                    resource::CompileDependency::data( recipe.source ) );
            }
            else if( context.resourceId.type() == resource::ResourceTypeID( "matres" ) )
            {
                const auto recipe = readMaterialRecipe( context );
                dependencies.compileDependencies.push_back(
                    resource::CompileDependency::resource( recipe.textureId ) );
                dependencies.installDependencies.push_back( recipe.textureId );
            }
            else
                throw std::runtime_error( "Unsupported graphics resource type" );
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
            if( context.resourceId.type() == resource::ResourceTypeID( "texres" ) )
                succeeded = writeCookedTexture( compileTexture( context ), output, error );
            else if( context.resourceId.type() == resource::ResourceTypeID( "matres" ) )
                succeeded = writeCookedMaterial( compileMaterial( context ), output, error );
            else
                error = "Unsupported graphics resource type";
            if( !succeeded )
            {
                messages.push_back( error );
                return resource::CompilationStatus::Failure;
            }
            return resource::CompilationStatus::Success;
        }
        catch( const std::exception &exception )
        {
            messages.push_back( exception.what() );
            return resource::CompilationStatus::Failure;
        }
    }
}  // namespace workphone::render
