#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/TerrainResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <cstring>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        namespace fs = std::filesystem;
        constexpr char magic[8] = { 'W', 'P', 'T', 'E', 'R', 'R', '0', '1' };

        struct TerrainTextureBudget
        {
            StreamBudget gpu, cpu;

            TerrainTextureBudget( StreamBudget gpuBytes, StreamBudget cpuBytes ) :
                gpu( std::move( gpuBytes ) ),
                cpu( std::move( cpuBytes ) )
            {
            }
        };

        // Base destruction runs in reverse order: ClawTexture retires its GPU view
        // and retained reupload mip cache before the ownership budget is released.
        // Returning a texture through the public resolver therefore retains its own
        // reservation even when the original candidate/resolver has gone away.
        class BudgetedTerrainTexture final : private TerrainTextureBudget, public ClawTexture
        {
        public:
            BudgetedTerrainTexture( StreamBudget gpuBytes, StreamBudget cpuBytes ) :
                TerrainTextureBudget( std::move( gpuBytes ), std::move( cpuBytes ) )
            {
            }
        };

        void require( bool condition, const char *message )
        {
            if( !condition )
            {
                throw std::runtime_error( message );
            }
        }

        Array<resource::ResourceID> textureIDs( const TerrainData &terrain )
        {
            std::set<resource::ResourceID> unique;
            const auto insert = [&]( const String &name ) {
                if( name.empty() )
                {
                    return;
                }
                resource::ResourceID id;
                String error;
                require( id.set( name, &error ) && !id.isSubResource() &&
                             id.type() == resource::ResourceTypeID( "texres" ),
                         "Cooked terrain maps require data:// texture resource IDs with type texres" );
                unique.insert( id );
            };
            for( const auto &layer : terrain.layers )
            {
                for( const auto *name : { &layer.albedo, &layer.normal, &layer.orm } )
                {
                    insert( *name );
                }
            }
            insert( terrain.appearance.detailNormal );
            return Array<resource::ResourceID>( unique.begin(), unique.end() );
        }

        TerrainData readTerrain( const resource::CompileContext &context )
        {
            require( context.resourceId.type() == resource::ResourceTypeID( "terrain" ) &&
                         !context.resourceId.isSubResource(),
                     "Terrain compiler requires a terrain resource" );
            std::error_code ec;
            const auto length = fs::file_size( fs::u8path( context.sourcePath.c_str() ), ec );
            require( !ec && length > 0 && length <= terrainResourceByteLimit,
                     "Terrain source exceeds its byte limit or is unavailable" );
            String source( static_cast<size_t>( length ), '\0' );
            std::ifstream file( fs::u8path( context.sourcePath.c_str() ), std::ios::binary );
            file.read( source.data(), static_cast<std::streamsize>( source.size() ) );
            require( file.good() && file.peek() == std::char_traits<char>::eof(),
                     "Terrain source read failed" );
            TerrainData terrain;
            String error;
            if( !deserializeTerrainData( source, terrain, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            return terrain;
        }

        void writeNumber( std::ostream &out, u64 value, size_t size )
        {
            for( size_t i = 0; i < size; ++i )
            {
                out.put( static_cast<char>( ( value >> ( i * 8 ) ) & 255 ) );
            }
        }

        void writeString( std::ostream &out, const String &value )
        {
            writeNumber( out, value.size(), 4 );
            out.write( value.data(), static_cast<std::streamsize>( value.size() ) );
        }

        void writeFloat( std::ostream &out, f32 value )
        {
            u32 bits = 0;
            std::memcpy( &bits, &value, sizeof( bits ) );
            writeNumber( out, bits, 4 );
        }

        void writeTiles( std::ostream &out, const Array<TerrainTileInfo> &tiles )
        {
            writeNumber( out, 1, 4 );
            writeNumber( out, terrainTileCells, 4 );
            writeNumber( out, terrainTileLods, 4 );
            writeNumber( out, 1,
                         4 );  // Border policy 1 retains every source border sample at every LOD.
            writeNumber( out, tiles.size(), 4 );
            for( const auto &tile : tiles )
            {
                writeNumber( out, tile.id, 4 );
                writeNumber( out, tile.first.x, 4 );
                writeNumber( out, tile.first.y, 4 );
                writeNumber( out, tile.last.x, 4 );
                writeNumber( out, tile.last.y, 4 );
                for( auto value : { tile.minimum.x, tile.minimum.y, tile.minimum.z, tile.maximum.x,
                                    tile.maximum.y, tile.maximum.z } )
                {
                    writeFloat( out, value );
                }
                for( auto value : tile.errors )
                {
                    writeFloat( out, value );
                }
                writeNumber( out, tile.hasHoles ? 1 : 0, 4 );
            }
        }

        struct Reader
        {
            const Array<u8> &bytes;
            size_t offset = 0;

            u64 number( size_t count )
            {
                require( count <= bytes.size() - offset, "Truncated cooked terrain payload" );
                u64 result = 0;
                for( size_t i = 0; i < count; ++i )
                {
                    result |= u64( bytes[offset++] ) << ( i * 8 );
                }
                return result;
            }

            String string( size_t limit )
            {
                const auto count = number( 4 );
                require( count <= limit && count <= bytes.size() - offset,
                         "Cooked terrain string exceeds its limit" );
                String value( reinterpret_cast<const char *>( bytes.data() + offset ),
                              static_cast<size_t>( count ) );
                offset += static_cast<size_t>( count );
                require( value.find( '\0' ) == String::npos, "Embedded NUL in cooked terrain payload" );
                return value;
            }

            f32 floating()
            {
                const auto bits = u32( number( 4 ) );
                f32 value;
                std::memcpy( &value, &bits, sizeof( value ) );
                return value;
            }
        };

        void verifyTiles( Reader &read, const Array<TerrainTileInfo> &tiles )
        {
            require( read.number( 4 ) == 1 && read.number( 4 ) == terrainTileCells &&
                         read.number( 4 ) == terrainTileLods && read.number( 4 ) == 1,
                     "Unsupported cooked terrain tile manifest or LOD border policy" );
            require( read.number( 4 ) == tiles.size(),
                     "Cooked terrain tile manifest coverage mismatch" );
            for( const auto &tile : tiles )
            {
                require( read.number( 4 ) == tile.id && read.number( 4 ) == u32( tile.first.x ) &&
                             read.number( 4 ) == u32( tile.first.y ) &&
                             read.number( 4 ) == u32( tile.last.x ) &&
                             read.number( 4 ) == u32( tile.last.y ),
                         "Cooked terrain tile coordinates mismatch source" );
                for( auto value : { tile.minimum.x, tile.minimum.y, tile.minimum.z, tile.maximum.x,
                                    tile.maximum.y, tile.maximum.z } )
                {
                    require( read.floating() == value, "Cooked terrain tile bounds mismatch source" );
                }
                for( auto value : tile.errors )
                {
                    require( read.floating() == value,
                             "Cooked terrain LOD error bounds mismatch source" );
                }
                require( read.number( 4 ) == ( tile.hasHoles ? 1 : 0 ),
                         "Cooked terrain hole metadata mismatch source" );
            }
        }

        void validateTexture( const resource::RuntimeResource &dependency,
                              const resource::ResourceID &id, const String &target, bool packaged,
                              const TerrainData &terrain )
        {
            CookedTextureData texture;
            String error;
            require( dependency.header.resourceId == id && dependency.dependencies.empty(),
                     "Cooked terrain texture identity/closure mismatch" );
            if( !decodeCookedTexture( dependency, texture, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            require( texture.target == target && texture.packagedBuild == packaged,
                     "Cooked terrain texture target/build mismatch" );
            if( terrain.appearance.detailNormal == id.str() )
            {
                require( texture.mipSettings.filter == TextureMipFilter::None ||
                             texture.mipSettings.filter == TextureMipFilter::Normal,
                         "Terrain detail normal mip chain requires normal filtering" );
            }
            // No-mip assets remain valid. Generated mip chains must respect map semantics:
            // colour filters decode sRGB; normal and packed ORM channels remain linear.
            for( const auto &layer : terrain.layers )
            {
                const auto filter = texture.mipSettings.filter;
                if( layer.albedo == id.str() )
                {
                    require( filter == TextureMipFilter::None || filter == TextureMipFilter::Colour,
                             "Terrain albedo mip chain requires colour filtering" );
                }
                if( layer.normal == id.str() )
                {
                    require( filter == TextureMipFilter::None || filter == TextureMipFilter::Normal,
                             "Terrain normal mip chain requires normal filtering" );
                }
                if( layer.orm == id.str() )
                {
                    require( filter == TextureMipFilter::None || filter == TextureMipFilter::Data,
                             "Terrain packed ORM mip chain requires linear data filtering" );
                }
            }
        }
    }  // namespace

    String TerrainResourceCompiler::name() const
    {
        return "WPGraphics.TerrainResourceCompiler";
    }

    Array<resource::CompilerOutput> TerrainResourceCompiler::outputs() const
    {
        return { { resource::ResourceTypeID( "terrain" ), 3 } };
    }

    bool TerrainResourceCompiler::getDependencies( const resource::CompileContext &context,
                                                   resource::DependencySet &dependencies,
                                                   String &error ) const
    {
        error.clear();
        dependencies = {};
        try
        {
            for( const auto &id : textureIDs( readTerrain( context ) ) )
            {
                dependencies.compileDependencies.push_back(
                    resource::CompileDependency::resource( id ) );
                dependencies.installDependencies.push_back( id );
            }
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    resource::CompilationStatus TerrainResourceCompiler::compile(
        const resource::CompileContext &context, std::ostream &output, Array<String> &messages ) const
    {
        try
        {
            auto terrain = readTerrain( context );
            const auto ids = textureIDs( terrain );
            const auto json = serializeTerrainData( terrain );
            require( !json.empty() && json.size() <= terrainResourceByteLimit - 262144,
                     "Serialized terrain exceeds payload byte limit" );
            require( !context.target.empty() && context.target.size() <= 128,
                     "Invalid terrain build target" );
            Array<TerrainTileInfo> tiles;
            String tileError;
            if( !describeTerrainTiles( terrain, tiles, tileError ) )
            {
                throw std::runtime_error( tileError.c_str() );
            }
            output.write( magic, 8 );
            writeNumber( output, 3, 4 );
            writeNumber( output, context.packagedBuild ? 1 : 0, 4 );
            writeString( output, context.target );
            writeString( output, json );
            writeTiles( output, tiles );
            writeNumber( output, ids.size(), 4 );
            for( const auto &id : ids )
            {
                const auto pinned = context.dependencyArtifacts.find( id.str() );
                require( pinned != context.dependencyArtifacts.end(),
                         "Terrain compiler requires pinned texture artifacts" );
                AssetCatalogPath relative;
                String error;
                if( !canonicalAssetCatalogPath( context.compiledRoot, pinned->second, relative, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                const auto path =
                    fs::u8path( context.compiledRoot.c_str() ) / fs::u8path( relative.path.c_str() );
                resource::RuntimeResource texture;
                if( !resource::CompiledResourceIO::read( path.generic_u8string().c_str(), texture, error,
                                                         graphicsTextureByteLimit + 4096 ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                validateTexture( texture, id, context.target, context.packagedBuild, terrain );
                writeString( output, id.str() );
                writeNumber( output, texture.header.compilerVersion, 8 );
                writeNumber( output, texture.header.sourceHash, 8 );
                writeNumber( output, texture.header.payloadHash, 8 );
            }
            require( output.good(), "Writing cooked terrain payload failed" );
            return resource::CompilationStatus::Success;
        }
        catch( const std::exception &e )
        {
            messages.push_back( e.what() );
            return resource::CompilationStatus::Failure;
        }
    }

    bool decodeCookedTerrain( const resource::RuntimeResource &resource, CookedTerrainData &output,
                              String &error )
    {
        error.clear();
        try
        {
            const resource::ResourceTypeID terrainType( "terrain" );
            const TerrainResourceCompiler compiler;
            const auto currentCompilerVersion = compiler.versionFor( terrainType );
            // V1 used IResourceCompiler's original container/output/type hash.
            // Preserve that historical fingerprint independently of future cooks.
            const auto legacyCompilerVersion = resource::combineHash(
                resource::combineHash( resource::combineHash( 14695981039346656037ull, 1 ), 1 ),
                terrainType.value() );
            const auto tileCompilerVersion = resource::combineHash(
                resource::combineHash( resource::combineHash( 14695981039346656037ull, 1 ), 2 ),
                terrainType.value() );
            require( resource.header.isValid() && !resource.header.resourceId.isSubResource() &&
                         resource.header.resourceType == terrainType &&
                         resource.header.resourceId.type() == terrainType &&
                         ( resource.header.compilerVersion == legacyCompilerVersion ||
                           resource.header.compilerVersion == tileCompilerVersion ||
                           resource.header.compilerVersion == currentCompilerVersion ) &&
                         resource.payload.size() >= 24 &&
                         resource.payload.size() <= terrainResourceByteLimit,
                     "Unsupported cooked terrain header/size" );
            require( resource.header.payloadSize == resource.payload.size() &&
                         resource.header.payloadHash ==
                             resource::hashBytes( resource.payload.data(), resource.payload.size() ),
                     "Cooked terrain payload hash/size mismatch" );
            require( std::memcmp( resource.payload.data(), magic, 8 ) == 0,
                     "Invalid cooked terrain magic" );
            Reader read{ resource.payload, 8 };
            const auto version = read.number( 4 );
            require( ( version == 1 && resource.header.compilerVersion == legacyCompilerVersion ) ||
                         ( version == 2 && resource.header.compilerVersion == tileCompilerVersion ) ||
                         ( version == 3 && resource.header.compilerVersion == currentCompilerVersion ),
                     "Unsupported cooked terrain version" );
            const auto mode = read.number( 4 );
            require( mode <= 1, "Invalid cooked terrain build mode" );
            CookedTerrainData result;
            result.packagedBuild = mode != 0;
            result.target = read.string( 128 );
            require( !result.target.empty(), "Empty cooked terrain target" );
            const auto json = read.string( static_cast<size_t>( terrainResourceByteLimit ) );
            if( !deserializeTerrainData( json, result.terrain, error ) )
            {
                return false;
            }
            if( !describeTerrainTiles( result.terrain, result.tiles, error ) )
            {
                return false;
            }
            if( version >= 2 )
            {
                verifyTiles( read, result.tiles );
            }
            const auto ids = textureIDs( result.terrain );
            require( read.number( 4 ) == ids.size() && resource.header.installDependencies == ids &&
                         resource.dependencies.size() == ids.size(),
                     "Cooked terrain must install its exact texture dependency closure" );
            for( size_t i = 0; i < ids.size(); ++i )
            {
                require( read.string( 4096 ) == ids[i].str() && resource.dependencies[i],
                         "Cooked terrain dependency identity missing" );
                const auto &dependency = *resource.dependencies[i];
                require( read.number( 8 ) == dependency.header.compilerVersion &&
                             read.number( 8 ) == dependency.header.sourceHash &&
                             read.number( 8 ) == dependency.header.payloadHash,
                         "Cooked terrain texture generation mismatch" );
                validateTexture( dependency, ids[i], result.target, result.packagedBuild,
                                 result.terrain );
            }
            require( read.offset == resource.payload.size(), "Trailing cooked terrain payload bytes" );
            output = std::move( result );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    DecodedCookedTerrain::DecodedCookedTerrain( CookedTerrainData value,
                                                std::shared_ptr<const resource::RuntimeResource> source,
                                                Array<CookedTextureData> maps ) :
        data( std::move( value ) ),
        resource( std::move( source ) ),
        textures( std::move( maps ) )
    {
    }

    CookedTerrainCandidate::CookedTerrainCandidate( std::shared_ptr<const DecodedCookedTerrain> source,
                                                    ClawTerrainMaterial::TextureResolver maps ) :
        decoded( std::move( source ) ),
        terrain( decoded->data.terrain ),
        tiles( decoded->data.tiles ),
        resolver( std::move( maps ) ),
        resource( decoded->resource )
    {
    }

    std::shared_ptr<const DecodedCookedTerrain> decodeCookedTerrainResource(
        std::shared_ptr<const resource::RuntimeResource> resource, const String &target,
        bool packagedBuild, String &error )
    {
        error.clear();
        try
        {
            if( !resource )
            {
                error = "Cooked terrain resource is missing.";
                return {};
            }
            CookedTerrainData decoded;
            if( !decodeCookedTerrain( *resource, decoded, error ) )
            {
                return {};
            }
            require( decoded.target == target && decoded.packagedBuild == packagedBuild,
                     "Cooked terrain consumer target/build mismatch" );
            Array<CookedTextureData> textures;
            for( const auto &dependency : resource->dependencies )
            {
                CookedTextureData texture;
                if( !decodeCookedTexture( *dependency, texture, error ) )
                {
                    return {};
                }
                textures.push_back( std::move( texture ) );
            }
            return std::shared_ptr<const DecodedCookedTerrain>( new DecodedCookedTerrain(
                std::move( decoded ), std::move( resource ), std::move( textures ) ) );
        }
        catch( const std::exception &e )
        {
            error = String( "Cooked terrain decode failed: " ) + e.what();
            return {};
        }
    }

    bool queueCookedTerrainDecode( WorldStreamProducer &producer,
                                   std::shared_ptr<const resource::RuntimeResource> resource,
                                   const String &target, bool packagedBuild,
                                   const CookedTerrainDecodeOptions &options, String &error )
    {
        error.clear();
        try
        {
            require( resource && resource->dependencies.size() <= 13 && !target.empty() &&
                         target.size() <= 128,
                     "Invalid terrain decode request" );
            // Terrain JSON has no expansion/compression. cJSON nodes plus output arrays
            // require fewer than 64 bytes per encoded byte, including one-character
            // numbers; decoded vectors require fewer than eight. Texture payloads
            // already contain their uncompressed mip bytes. Fixed slack covers small
            // structures, strings and verified tile metadata. Reject before parsing.
            u64 sourceBytes = 4096, textureBytes = 0;
            const auto account = [&]( const resource::RuntimeResource &item ) {
                require( item.payload.size() <= terrainResourceByteLimit &&
                             item.header.installDependencies.size() <= 13 &&
                             item.header.installArtifacts.size() <= 13 &&
                             item.header.resourceId.str().size() <= 4096,
                         "Terrain decode input exceeds closure limits" );
                sourceBytes +=
                    sizeof( resource::RuntimeResource ) + item.payload.capacity() +
                    item.header.resourceId.str().capacity() + 1 +
                    item.header.resourceId.type().str().capacity() + 1 +
                    item.header.resourceType.str().capacity() + 1 +
                    item.dependencies.capacity() * sizeof( item.dependencies[0] ) +
                    item.header.installDependencies.capacity() * sizeof( resource::ResourceID ) +
                    item.header.installArtifacts.capacity() *
                        sizeof( resource::InstallDependencyArtifact );
                for( const auto &id : item.header.installDependencies )
                {
                    require( id.str().size() <= 4096, "Terrain dependency ID exceeds its limit" );
                    sourceBytes += id.str().capacity() + id.type().str().capacity() + 2;
                }
                for( const auto &pin : item.header.installArtifacts )
                {
                    require( pin.path.size() <= 4096, "Terrain dependency path exceeds its limit" );
                    sourceBytes += pin.path.capacity() + 1;
                }
            };
            account( *resource );
            for( const auto &dependency : resource->dependencies )
            {
                require( dependency && dependency->dependencies.empty(),
                         "Terrain decode requires direct immutable texture dependencies" );
                account( *dependency );
                textureBytes += dependency->payload.size();
            }
            const u64 workingBytes =
                ( 1ull << 20 ) + resource->payload.size() * 64ull + textureBytes * 2;
            const u64 resultBytes = ( 1ull << 20 ) + resource->payload.size() * 8ull + textureBytes;
            require( sourceBytes <= options.maximumSourceBytes &&
                         workingBytes <= options.maximumWorkingBytes &&
                         resultBytes <= options.maximumResultBytes,
                     "Cooked terrain exceeds declared source, decode scratch or result budget" );
            auto sourceBudget = producer.reserve( StreamMemoryClass::SourceCpu, sourceBytes, error );
            if( !sourceBudget )
            {
                return false;
            }

            struct InputOwner
            {
                StreamBudget budget;
                std::shared_ptr<const resource::RuntimeResource> resource;
            };

            auto owner =
                std::make_shared<InputOwner>( InputOwner{ std::move( sourceBudget ), resource } );
            auto leasedResource =
                std::shared_ptr<const resource::RuntimeResource>( owner, resource.get() );
            WorldStreamRequest request;
            request.key = options.key;
            request.priority = options.priority;
            request.pendingBytes = 512 + target.size();
            request.workingBytes = workingBytes;
            request.maximumResultBytes = resultBytes;
            request.execute = [leasedResource = std::move( leasedResource ), target, packagedBuild,
                               resultBytes]( const StreamCancellation &cancel ) {
                WorldStreamResult result;
                if( cancel.cancelled() )
                {
                    return result;
                }
                auto decoded =
                    decodeCookedTerrainResource( leasedResource, target, packagedBuild, result.error );
                if( cancel.cancelled() )
                {
                    return WorldStreamResult{};
                }
                if( decoded )
                {
                    result.payload = std::move( decoded );
                    result.bytes = resultBytes;
                }
                return result;
            };
            return producer.replace( { request }, error );
        }
        catch( const std::exception &e )
        {
            error = String( "Terrain decode submission failed: " ) + e.what();
            return false;
        }
    }

    std::shared_ptr<const CookedTerrainCandidate> stageCookedTerrainMaterial(
        ClawRendererDX11 &renderer, std::shared_ptr<const DecodedCookedTerrain> decoded, String &error )
    {
        error.clear();
        try
        {
            require( decoded != nullptr, "Cooked terrain CPU preparation is missing" );
            auto native =
                renderer.isLoaded() ? wp_renderer_get_dx11( renderer.getNativeRenderer() ) : nullptr;
            require( native != nullptr, "Cooked terrain staging requires a loaded DX11 renderer" );
            u64 retainedBytes = 4096;
            for( size_t i = 0; i < decoded->textures.size(); ++i )
            {
                const auto &texture = decoded->textures[i];
                // Retained map keys, texture objects and cache/map node bookkeeping
                // accompany the reupload mip copy, including maximum-length IDs.
                retainedBytes += sizeof( BudgetedTerrainTexture ) +
                                 decoded->resource->dependencies[i]->header.resourceId.str().capacity() +
                                 1 + 512;
            }
            // ClawTexture admits and owns each retained mip copy and physical
            // GPU allocation; this lease covers only resolver bookkeeping.
            auto cpuBudget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::SourceCpu,
                                                                         retainedBytes, error );
            if( !cpuBudget )
            {
                return {};
            }
            auto textures = std::make_shared<std::map<String, SmartPtr<ITexture>>>();
            for( size_t i = 0; i < decoded->textures.size(); ++i )
            {
                const auto &dependency = decoded->resource->dependencies[i];
                const auto &decodedTexture = decoded->textures[i];
                auto texture = make_ptr<BudgetedTerrainTexture>( StreamBudget{}, cpuBudget );
                if( !texture->uploadCookedMips( decodedTexture.levels, decodedTexture.mipSettings,
                                                native, error ) )
                {
                    return {};
                }
                textures->emplace( dependency->header.resourceId.str(), texture );
            }
            ClawTerrainMaterial::TextureResolver resolver = [textures, resource = decoded->resource](
                                                                const String &id,
                                                                String &error ) -> SmartPtr<ITexture> {
                const auto found = textures->find( id );
                if( found == textures->end() )
                {
                    error = "Terrain texture is outside the retained cooked closure: " + id;
                    return {};
                }
                return found->second;
            };
            return std::shared_ptr<const CookedTerrainCandidate>(
                new CookedTerrainCandidate( std::move( decoded ), std::move( resolver ) ) );
        }
        catch( const std::exception &e )
        {
            error = String( "Cooked terrain staging failed: " ) + e.what();
            return {};
        }
    }

    std::shared_ptr<const CookedTerrainCandidate> stageCookedTerrainMaterial(
        ClawRendererDX11 &renderer, std::shared_ptr<const resource::RuntimeResource> resource,
        const String &target, bool packagedBuild, String &error )
    {
        auto decoded =
            decodeCookedTerrainResource( std::move( resource ), target, packagedBuild, error );
        return decoded ? stageCookedTerrainMaterial( renderer, std::move( decoded ), error ) : nullptr;
    }
}  // namespace workphone::render
