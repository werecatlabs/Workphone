#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/FoliageImpostorBaker.hpp>
#include <cJSON.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <limits>
#include <cstring>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        std::mutex resolverMutex;
        FoliageBakeTextureResolver installedResolver;

        void require( bool value, const char *message )
        {
            if( !value )
            {
                throw std::runtime_error( message );
            }
        }

        u8 byte( double value )
        {
            return u8( std::lround( std::clamp( value, 0.0, 1.0 ) * 255 ) );
        }

        ColourF sample( const TextureMipLevel *image, Vector2F uv, ColourF fallback,
                        bool colour = false )
        {
            if( !image )
            {
                return fallback;
            }
            const double u = double( uv.x ) - std::floor( double( uv.x ) ),
                         v = double( uv.y ) - std::floor( double( uv.y ) );
            const auto x = u * image->width - .5, y = v * image->height - .5;
            const auto ix = s32( std::floor( x ) ), iy = s32( std::floor( y ) );
            const auto fx = x - ix, fy = y - iy;
            double result[4]{};
            for( int z = 0; z < 2; ++z )
            {
                for( int a = 0; a < 2; ++a )
                {
                    const auto px = ( ix + a + s32( image->width ) ) % s32( image->width ),
                               py = ( iy + z + s32( image->height ) ) % s32( image->height );
                    const auto *p = image->bgra.data() + ( size_t( py ) * image->width + px ) * 4;
                    const auto w = ( a ? fx : 1 - fx ) * ( z ? fy : 1 - fy );
                    for( int c = 0; c < 4; ++c )
                    {
                        result[c] +=
                            w * ( colour && c != 3 ? std::pow( p[c] / 255.0, 2.2 ) : p[c] / 255.0 );
                    }
                }
            }
            return { f32( result[2] ), f32( result[1] ), f32( result[0] ), f32( result[3] ) };
        }

        double edge( Vector2F a, Vector2F b, Vector2F p )
        {
            return ( double( p.x ) - a.x ) * ( double( b.y ) - a.y ) -
                   ( double( p.y ) - a.y ) * ( double( b.x ) - a.x );
        }

        void write( TextureMipLevel &level, size_t pixel, ColourF colour )
        {
            auto p = level.bgra.data() + pixel * 4;
            p[0] = byte( colour.b );
            p[1] = byte( colour.g );
            p[2] = byte( colour.r );
            p[3] = byte( colour.a );
        }

        void dilate( std::array<TextureMipLevel *, 4> levels, u32 tile, u32 tileSize, u32 distance )
        {
            const auto width = levels[0]->width;
            std::vector<u8> known( size_t( tileSize ) * tileSize );
            for( u32 y = 0; y < tileSize; ++y )
            {
                for( u32 x = 0; x < tileSize; ++x )
                {
                    known[size_t( y ) * tileSize + x] =
                        levels[0]->bgra[( size_t( y ) * width + tile * tileSize + x ) * 4 + 3] != 0;
                }
            }
            for( u32 pass = 0; pass < distance; ++pass )
            {
                auto next = known;
                for( u32 y = 0; y < tileSize; ++y )
                {
                    for( u32 x = 0; x < tileSize; ++x )
                    {
                        const auto local = size_t( y ) * tileSize + x;
                        if( known[local] )
                        {
                            continue;
                        }
                        s32 neighbour = -1;
                        if( x && known[local - 1] )
                        {
                            neighbour = s32( local - 1 );
                        }
                        else if( x + 1 < tileSize && known[local + 1] )
                        {
                            neighbour = s32( local + 1 );
                        }
                        else if( y && known[local - tileSize] )
                        {
                            neighbour = s32( local - tileSize );
                        }
                        else if( y + 1 < tileSize && known[local + tileSize] )
                        {
                            neighbour = s32( local + tileSize );
                        }
                        if( neighbour < 0 )
                        {
                            continue;
                        }
                        const auto src = ( size_t( neighbour / s32( tileSize ) ) * width +
                                           tile * tileSize + neighbour % tileSize ) *
                                         4;
                        const auto dst = ( size_t( y ) * width + tile * tileSize + x ) * 4;
                        for( auto level : levels )
                        {
                            for( u32 c = 0; c < 3; ++c )
                            {
                                level->bgra[dst + c] = level->bgra[src + c];
                            }
                        }
                        next[local] = 1;
                    }
                }
                known.swap( next );
            }
        }
    }  // namespace

    void setFoliageImpostorTextureResolver( FoliageBakeTextureResolver resolver )
    {
        std::lock_guard<std::mutex> guard( resolverMutex );
        installedResolver = std::move( resolver );
    }

    FoliageBakeTextureResolver getFoliageImpostorTextureResolver()
    {
        std::lock_guard<std::mutex> guard( resolverMutex );
        return installedResolver;
    }

    bool bakeFoliageImpostor( const FoliageSpecies &species, const FoliageBakeTextureResolver &resolve,
                              const FoliageImpostorBakeOptions &options, FoliageImpostorBake &output,
                              String &error, const std::atomic<bool> *cancel )
    {
        try
        {
            if( !validateFoliageSpecies( species, error ) )
            {
                return false;
            }
            require( options.tileSize >= 16 && options.tileSize <= 512 &&
                         !( options.tileSize & ( options.tileSize - 1 ) ) && options.gutter >= 2 &&
                         options.gutter <= 16 && options.gutter * 2 + 4 <= options.tileSize,
                     "Impostor tile size must be a power of two 16..512 with a 2..16 pixel gutter." );
            require(
                !options.directions.empty() && options.directions.size() <= 32 &&
                    !options.resourcePrefix.empty() &&
                    options.resourcePrefix.find( '\0' ) == String::npos &&
                    options.maximumRasterSamples > 0 &&
                    options.maximumRasterSamples <= 128ull * 1024 * 1024,
                "Impostor bake requires 1..32 directions, resource IDs and a bounded raster budget." );
            const auto width = options.tileSize * u32( options.directions.size() ),
                       height = options.tileSize;
            require( width <= 8192 && u64( width ) * height * 4 * 4 * 4 / 3 <= 128ull * 1024 * 1024,
                     "Impostor output exceeds the 128 MiB mip-chain budget." );
            const auto check = [&] {
                require( !cancel || !cancel->load(),
                         "Foliage impostor bake cancelled; previous result retained." );
            };
            std::map<String, std::shared_ptr<const TextureMipLevel>> textures;
            u64 textureBytes = 0;
            const auto texture = [&]( const String &id ) -> const TextureMipLevel * {
                if( id.empty() )
                {
                    return nullptr;
                }
                auto found = textures.find( id );
                if( found != textures.end() )
                {
                    return found->second.get();
                }
                require( bool( resolve ), "Textured impostor baking requires a texture resolver." );
                auto image = resolve( id, error );
                require( image && image->width > 0 && image->height > 0 && image->width <= 16384 &&
                             image->height <= 16384 &&
                             u64( image->width ) * image->height * 4 == image->bgra.size(),
                         "Cannot resolve valid immutable BGRA foliage texture pixels." );
                textureBytes += image->bgra.size();
                require( textureBytes <= 128ull * 1024 * 1024,
                         "Impostor source textures exceed 128 MiB." );
                textures.emplace( id, image );
                return image.get();
            };

            struct Material
            {
                const TextureMipLevel *albedo, *normal, *orm;
            };

            Array<Material> materials;
            for( const auto &m : species.materials )
            {
                check();
                materials.push_back( { texture( m.albedo ), texture( m.normal ), texture( m.orm ) } );
            }
            FoliageImpostorBake result;
            result.centre = ( species.boundsMinimum + species.boundsMaximum ) * .5f;
            result.radius = ( species.boundsMaximum - species.boundsMinimum ).length() * .5f;
            require( std::isfinite( result.radius ) && result.radius > 0,
                     "Impostor bounds require a finite positive sphere." );
            auto level = [&] {
                return TextureMipLevel{ width, height,
                                        std::vector<u8>( size_t( width ) * height * 4, 0 ) };
            };
            auto albedo = level(), normal = level(), orm = level(), depth = level();
            for( size_t i = 0; i < size_t( width ) * height; ++i )
            {
                write( normal, i, { .5f, .5f, 1, 0 } );
                write( orm, i, { 1, 1, 0, 0 } );
                write( depth, i, { 1, 1, 1, 0 } );
            }
            result.metadata.width = width;
            result.metadata.height = height;
            result.metadata.gutter = options.gutter;
            result.metadata.relightable = true;
            result.metadata.albedo = options.resourcePrefix + "_albedo.texres";
            result.metadata.normal = options.resourcePrefix + "_normal.texres";
            result.metadata.orm = options.resourcePrefix + "_orm.texres";
            result.metadata.depth = options.resourcePrefix + "_depth.texres";
            const auto &mesh = species.lods[0];
            auto sections = mesh.sections;
            if( sections.empty() )
            {
                sections.push_back( { 0, u32( mesh.indices.size() ), 0 } );
            }
            for( u32 view = 0; view < options.directions.size(); ++view )
            {
                check();
                auto toward = options.directions[view];
                require( toward.isFinite() && toward.lengthSquared() > 1e-12f,
                         "Impostor capture direction must be finite and nonzero." );
                toward.normalise();
                auto up = std::abs( toward.y ) > .98f ? Vector3F( 0, 0, 1 ) : Vector3F( 0, 1, 0 );
                auto right = up.crossProduct( toward ).normaliseCopy();
                up = toward.crossProduct( right );
                FoliageImpostorView metadata;
                metadata.direction = toward;
                metadata.uvMinimum = { ( view * options.tileSize + options.gutter + .5f ) / width,
                                       ( options.gutter + .5f ) / height };
                metadata.uvMaximum = { ( ( view + 1 ) * options.tileSize - options.gutter - .5f ) /
                                           width,
                                       ( height - options.gutter - .5f ) / height };
                result.metadata.views.push_back( metadata );
                std::vector<f32> zbuffer( size_t( height ) * height,
                                          -std::numeric_limits<f32>::infinity() );
                const float span = f32( options.tileSize - 2 * options.gutter - 1 );
                for( const auto &section : sections )
                {
                    for( u32 index = section.firstIndex; index < section.firstIndex + section.indexCount;
                         index += 3 )
                    {
                        check();
                        const auto &m = species.materials[section.material];
                        const auto &maps = materials[section.material];
                        Vector3F p[3], n[3];
                        Vector2F screen[3], uv[3];
                        ColourF colour[3];
                        f32 z[3];
                        for( int corner = 0; corner < 3; ++corner )
                        {
                            const auto vertex = mesh.indices[index + corner];
                            p[corner] = mesh.positions[vertex];
                            n[corner] = mesh.normals[vertex];
                            uv[corner] = mesh.uvs[vertex];
                            colour[corner] =
                                mesh.colours.empty() ? ColourF::White : mesh.colours[vertex];
                            const auto delta = p[corner] - result.centre;
                            z[corner] = delta.dotProduct( toward );
                            screen[corner] = {
                                options.gutter + .5f +
                                    ( delta.dotProduct( right ) / result.radius * .5f + .5f ) * span,
                                options.gutter + .5f +
                                    ( .5f - delta.dotProduct( up ) / result.radius * .5f ) * span
                            };
                        }
                        const auto area = edge( screen[0], screen[1], screen[2] );
                        if( std::abs( area ) < 1e-10 )
                        {
                            continue;
                        }
                        auto geometric = ( p[1] - p[0] ).crossProduct( p[2] - p[0] );
                        if( !m.twoSided && geometric.dotProduct( toward ) <= 0 )
                        {
                            continue;
                        }
                        const auto du1 = uv[1] - uv[0], du2 = uv[2] - uv[0];
                        const auto determinant = du1.x * du2.y - du2.x * du1.y;
                        Vector3F tangent = right, bitangent = -up;
                        if( std::abs( determinant ) > 1e-10f )
                        {
                            tangent =
                                ( ( p[1] - p[0] ) * du2.y - ( p[2] - p[0] ) * du1.y ) / determinant;
                            bitangent =
                                ( ( p[2] - p[0] ) * du1.x - ( p[1] - p[0] ) * du2.x ) / determinant;
                        }
                        const auto minX = std::max(
                                       s32( options.gutter ),
                                       s32( std::floor(
                                           std::min( { screen[0].x, screen[1].x, screen[2].x } ) ) ) ),
                                   maxX = std::min( s32( height - options.gutter - 1 ),
                                                    s32( std::ceil( std::max( { screen[0].x, screen[1].x,
                                                                                screen[2].x } ) ) ) );
                        const auto minY = std::max(
                                       s32( options.gutter ),
                                       s32( std::floor(
                                           std::min( { screen[0].y, screen[1].y, screen[2].y } ) ) ) ),
                                   maxY = std::min( s32( height - options.gutter - 1 ),
                                                    s32( std::ceil( std::max( { screen[0].y, screen[1].y,
                                                                                screen[2].y } ) ) ) );
                        for( s32 y = minY; y <= maxY; ++y )
                        {
                            check();
                            for( s32 x = minX; x <= maxX; ++x )
                            {
                                require(
                                    ++result.rasterSamples <= options.maximumRasterSamples,
                                    "Impostor raster work budget exhausted; previous result retained." );
                                const Vector2F point( x + .5f, y + .5f );
                                const auto a = edge( screen[1], screen[2], point ) / area,
                                           b = edge( screen[2], screen[0], point ) / area, c = 1 - a - b;
                                if( a < -1e-8 || b < -1e-8 || c < -1e-8 )
                                {
                                    continue;
                                }
                                const auto distance = f32( a * z[0] + b * z[1] + c * z[2] );
                                const auto local = size_t( y ) * height + x;
                                if( distance <= zbuffer[local] )
                                {
                                    continue;
                                }
                                const auto texcoord =
                                    uv[0] * f32( a ) + uv[1] * f32( b ) + uv[2] * f32( c );
                                auto base = sample( maps.albedo, texcoord, ColourF::White );
                                const ColourF vertex(
                                    f32( a * colour[0].r + b * colour[1].r + c * colour[2].r ),
                                    f32( a * colour[0].g + b * colour[1].g + c * colour[2].g ),
                                    f32( a * colour[0].b + b * colour[1].b + c * colour[2].b ),
                                    f32( a * colour[0].a + b * colour[1].a + c * colour[2].a ) );
                                const auto alpha = base.a * m.tint.a * vertex.a;
                                if( alpha < m.alphaCutoff )
                                {
                                    continue;
                                }
                                // Match the native foliage material convention: multiply texture/tint/vertex
                                // colour in sRGB, then the runtime shader performs its existing decode.
                                base = { base.r * m.tint.r * vertex.r, base.g * m.tint.g * vertex.g,
                                         base.b * m.tint.b * vertex.b, 1 };
                                auto worldNormal = n[0] * f32( a ) + n[1] * f32( b ) + n[2] * f32( c );
                                worldNormal.normalise();
                                if( maps.normal )
                                {
                                    auto normalMap = sample( maps.normal, texcoord, { .5f, .5f, 1, 1 } );
                                    auto t = tangent - worldNormal * tangent.dotProduct( worldNormal );
                                    t.normalise();
                                    auto bt = worldNormal.crossProduct( t );
                                    if( bt.dotProduct( bitangent ) < 0 )
                                    {
                                        bt = -bt;
                                    }
                                    worldNormal = t * ( 2 * normalMap.r - 1 ) +
                                                  bt * ( 2 * normalMap.g - 1 ) +
                                                  worldNormal * ( 2 * normalMap.b - 1 );
                                    worldNormal.normalise();
                                }
                                if( m.twoSided && worldNormal.dotProduct( toward ) < 0 )
                                {
                                    worldNormal = -worldNormal;
                                }
                                auto packed = sample( maps.orm, texcoord, { 1, 1, 1, 1 } );
                                packed.g *= m.roughness;
                                packed.b *= m.metalness;
                                packed.a = 1;
                                const auto pixel = size_t( y ) * width + view * height + x;
                                const auto linearDepth =
                                    std::clamp( .5f - distance / ( 2 * result.radius ), 0.f, 1.f );
                                write( albedo, pixel, base );
                                write( normal, pixel,
                                       { .5f + .5f * worldNormal.dotProduct( right ),
                                         .5f - .5f * worldNormal.dotProduct( up ),
                                         .5f + .5f * worldNormal.dotProduct( toward ), 1 } );
                                write( orm, pixel, packed );
                                write( depth, pixel, { linearDepth, linearDepth, linearDepth, 1 } );
                                zbuffer[local] = distance;
                            }
                        }
                    }
                }
                dilate( { &albedo, &normal, &orm, &depth }, view, options.tileSize, options.gutter );
            }
            check();
            const auto columns = u32( options.directions.size() );
            result.albedo = generateTextureMips( albedo.bgra.data(), width, height,
                                                 { TextureMipFilter::Cutout, columns, .5f } );
            result.normal = generateTextureMips( normal.bgra.data(), width, height,
                                                 { TextureMipFilter::Normal, columns, .5f } );
            result.orm = generateTextureMips( orm.bgra.data(), width, height,
                                              { TextureMipFilter::Data, columns, .5f } );
            auto roughness = generateTextureMips( orm.bgra.data(), width, height,
                                                  { TextureMipFilter::Roughness, columns, .5f } );
            for( size_t mip = 1; mip < result.orm.size(); ++mip )
            {
                for( size_t pixel = 0; pixel < result.orm[mip].bgra.size(); pixel += 4 )
                {
                    result.orm[mip].bgra[pixel + 1] = roughness[mip].bgra[pixel + 1];
                }
            }
            result.depth = generateTextureMips( depth.bgra.data(), width, height,
                                                { TextureMipFilter::Data, columns, .5f } );
            check();
            output = std::move( result );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool saveFoliageImpostorSource( const FoliageSpecies &source, const FoliageImpostorBake &bake,
                                    const String &directory, String &speciesPath, String &error )
    {
        std::filesystem::path folder;
        Array<std::filesystem::path> created;
        bool ownDirectory = false;
        try
        {
            require( !directory.empty() && directory.find( '\0' ) == String::npos &&
                         !bake.albedo.empty() && !bake.normal.empty() && !bake.orm.empty() &&
                         !bake.depth.empty(),
                     "Impostor export requires a fresh directory and a completed bake." );
            auto species = source;
            species.impostor = bake.metadata;
            species.farBillboard = true;
            if( !validateFoliageSpecies( species, error ) )
            {
                return false;
            }
            String json;
            if( !serializeFoliageSpecies( species, json, error ) )
            {
                return false;
            }
            folder = std::filesystem::absolute( std::filesystem::u8path( directory.c_str() ) );
            require( !std::filesystem::exists( folder ),
                     "Impostor export destination must be a fresh asset directory." );
            ownDirectory = std::filesystem::create_directory( folder );
            require( ownDirectory, "Cannot create impostor asset directory." );
            const auto writeFile = [&]( const std::filesystem::path &path, const char *bytes,
                                        size_t count ) {
                created.push_back( path );
                std::ofstream file( path, std::ios::binary | std::ios::trunc );
                file.write( bytes, std::streamsize( count ) );
                file.flush();
                require( bool( file ), "Cannot write impostor source artifact." );
                file.close();
                require( bool( file ), "Cannot close impostor source artifact." );
            };
            const String ids[] = { bake.metadata.albedo, bake.metadata.normal, bake.metadata.orm,
                                   bake.metadata.depth };
            // Source export writes base images. Recooking regenerates isolated view mips:
            // cutout albedo, normalized normals, and linear data for packed ORM/depth.
            // The existing Roughness policy changes all RGB channels and would corrupt
            // AO/metalness, so exported ORM does not retain the in-memory green-only
            // roughness-filtered chain until a packed ORM policy is available.
            const std::vector<TextureMipLevel> *chains[] = { &bake.albedo, &bake.normal, &bake.orm,
                                                             &bake.depth };
            const char *filters[] = { "cutout", "normal", "data", "data" };
            for( u32 channel = 0; channel < 4; ++channel )
            {
                const auto &image = chains[channel]->front();
                require( image.width == bake.metadata.width && image.height == bake.metadata.height &&
                             u64( image.width ) * image.height * 4 == image.bgra.size(),
                         "Impostor channel dimensions disagree." );
                auto name = ids[channel].substr( ids[channel].find_last_of( '/' ) + 1 );
                require( name.size() > 7 && name.substr( name.size() - 7 ) == ".texres",
                         "Impostor texture IDs must end in .texres." );
                auto imageName = name.substr( 0, name.size() - 7 ) + ".tga";
                std::vector<u8> tga( 18 + image.bgra.size(), 0 );
                tga[2] = 2;
                tga[12] = u8( image.width );
                tga[13] = u8( image.width >> 8 );
                tga[14] = u8( image.height );
                tga[15] = u8( image.height >> 8 );
                tga[16] = 32;
                tga[17] = 0x28;
                std::copy( image.bgra.begin(), image.bgra.end(), tga.begin() + 18 );
                writeFile( folder / imageName.c_str(), reinterpret_cast<const char *>( tga.data() ),
                           tga.size() );
                auto descriptor = cJSON_CreateObject();
                require( descriptor, "Cannot allocate texture descriptor." );
                std::unique_ptr<cJSON, decltype( &cJSON_Delete )> owner( descriptor, cJSON_Delete );
                auto imageId = ids[channel].substr( 0, ids[channel].size() - 7 ) + ".tga";
                cJSON_AddNumberToObject( descriptor, "version", 1 );
                cJSON_AddStringToObject( descriptor, "source", imageId.c_str() );
                cJSON_AddStringToObject( descriptor, "mipFilter", filters[channel] );
                cJSON_AddNumberToObject( descriptor, "atlasColumns", bake.metadata.views.size() );
                cJSON_AddNumberToObject( descriptor, "alphaCutoff", .5 );
                std::unique_ptr<char, decltype( &cJSON_free )> text(
                    cJSON_PrintUnformatted( descriptor ), cJSON_free );
                require( bool( text ), "Cannot encode texture descriptor." );
                writeFile( folder / name.c_str(), text.get(), std::strlen( text.get() ) );
            }
            const auto path = folder / "species.folspec";
            writeFile( path, json.data(), json.size() );
            speciesPath = path.u8string().c_str();
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            if( ownDirectory )
            {
                std::error_code ignored;
                for( const auto &path : created )
                {
                    std::filesystem::remove( path, ignored );
                }
                std::filesystem::remove( folder, ignored );
            }
            return false;
        }
    }
}  // namespace workphone::render
