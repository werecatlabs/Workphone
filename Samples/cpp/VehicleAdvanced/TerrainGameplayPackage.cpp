#include "TerrainGameplayPackage.h"

#if WP_TERRAIN_GAMEPLAY_PACKAGE
#    include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#    include <Workphone/Core/StringUtil.hpp>
#    include <Workphone/System/ResourceCompilerRegistry.hpp>
#    include <WPSQLite/ResourceCompilationDatabase.hpp>
#    include <boost/json.hpp>
#    include <algorithm>
#    include <array>
#    include <charconv>
#    include <cmath>
#    include <fstream>
#    include <iomanip>
#    include <limits>
#    include <set>
#    include <sstream>
#    include <stdexcept>
#    include <string>

namespace workphone::advanced::terrain_gameplay
{
    namespace
    {
        namespace fs = std::filesystem;
        namespace json = boost::json;
        using namespace resource;
        constexpr char rootID[] = "data://terrain-gameplay.gameplay";
        constexpr char terrainID[] = "data://ground.terrain";
        constexpr char target[] = "terrain-gameplay-dx11";
        constexpr char recipe[] = "ProceduralRaceSceneBuilder.buildCar/defaults-v1";
        constexpr u64 descriptorByteLimit = 16 * 1024;
        constexpr u64 schemaVersion = 1, generatorVersion = 1, compilerVersion = 1;
        constexpr int dimension = 129;

        void require( bool value, const std::string &message )
        {
            if( !value )
                throw std::runtime_error( message );
        }

        std::string standard( const String &value )
        {
            return { value.data(), value.size() };
        }

        void requireResource( bool value, const String &error )
        {
            // Preserve the referenced diagnostic until after the resource call
            // has completed; argument evaluation order must not copy it early.
            require( value, standard( error ) );
        }

        String pathString( const fs::path &value )
        {
            return value.generic_u8string().c_str();
        }

        fs::path cleanupIOPath( const fs::path &path )
        {
#if defined( _WIN32 )
            // Ownership and containment checks use canonical portable paths.
            // Apply the extended spelling only at the recursive IO boundary so
            // Windows can retire hashed artifact children beyond MAX_PATH.
            if( path.is_absolute() )
            {
                auto native = path.native();
                std::replace( native.begin(), native.end(), L'/', L'\\' );
                if( native.rfind( L"\\\\?\\", 0 ) == 0 )
                    return fs::path( native );
                if( native.rfind( L"\\\\", 0 ) == 0 )
                    return fs::path( L"\\\\?\\UNC\\" + native.substr( 2 ) );
                return fs::path( L"\\\\?\\" + native );
            }
#endif
            return path;
        }

        std::string mapStem( int layer, int map )
        {
            return "layer" + std::to_string( layer ) + "_" + std::to_string( map );
        }

        String mapID( int layer, int map )
        {
            return ( "data://" + mapStem( layer, map ) + ".texres" ).c_str();
        }

        int textureResolution( int quality )
        {
            require( quality >= 0 && quality <= 3, "Gameplay vehicle quality must be 0..3" );
            // Exactly the existing buildCar recipe, including the Cinematic cap.
            return quality == 0 ? 256 : quality == 1 ? 512 : 1024;
        }

        std::string hashText( u64 value )
        {
            require( value != 0, "Gameplay requires the actual generated vehicle content hash" );
            std::ostringstream stream;
            stream << std::hex << std::setfill( '0' ) << std::setw( 16 ) << value;
            return stream.str();
        }

        u64 parseHash( const json::value &value )
        {
            require( value.is_string() && value.as_string().size() == 16,
                     "Gameplay vehicle hash must be a 16-digit hexadecimal string" );
            const auto &text = value.as_string();
            for( const auto character : text )
                require( ( character >= '0' && character <= '9' ) ||
                             ( character >= 'a' && character <= 'f' ),
                         "Gameplay vehicle hash must use lowercase hexadecimal" );
            u64 result = 0;
            const auto parsed = std::from_chars( text.data(), text.data() + text.size(), result, 16 );
            require( parsed.ec == std::errc() && parsed.ptr == text.data() + text.size() && result,
                     "Gameplay vehicle hash is invalid" );
            return result;
        }

        u64 integer( const json::value &value, u64 maximum )
        {
            require( value.is_uint64() || ( value.is_int64() && value.as_int64() >= 0 ),
                     "Gameplay recipe field must be an unsigned integer" );
            const auto result = value.is_uint64() ? value.as_uint64() : u64( value.as_int64() );
            require( result <= maximum, "Gameplay recipe integer is out of range" );
            return result;
        }

        bool equals( const json::value &value, const char *expected )
        {
            return value.is_string() && value.as_string() == expected;
        }

        // The grammar is deliberately flat. After JSON syntax validation, decode
        // each key token so escaped spellings cannot conceal duplicate fields.
        void uniqueFlatKeys( const std::string &text )
        {
            std::set<std::string> keys;
            for( size_t index = 0; index < text.size(); ++index )
            {
                if( text[index] != '"' )
                    continue;
                const auto begin = index;
                for( ++index; index < text.size(); ++index )
                {
                    if( text[index] == '\\' )
                        ++index;
                    else if( text[index] == '"' )
                        break;
                }
                auto next = index + 1;
                while( next < text.size() && ( text[next] == ' ' || text[next] == '\t' ||
                                               text[next] == '\r' || text[next] == '\n' ) )
                    ++next;
                if( next < text.size() && text[next] == ':' )
                {
                    const auto key = json::parse( json::string_view( text.data() + begin,
                                                                    index - begin + 1 ) ).as_string();
                    require( keys.emplace( key.data(), key.size() ).second,
                             "Duplicate gameplay recipe field" );
                }
            }
        }

        json::object readRecipe( const std::string &text, bool cooked )
        {
            require( !text.empty() && text.size() <= descriptorByteLimit,
                     "Gameplay recipe is empty or exceeds its byte limit" );
            json::parse_options options;
            options.max_depth = 2;
            auto parsed = json::parse( text, {}, options );
            require( parsed.is_object(), "Gameplay recipe must be an object" );
            uniqueFlatKeys( text );
            auto object = std::move( parsed.as_object() );
            std::set<std::string> fields{ "version", "generatorVersion", "tier", "generator",
                "vehiclePreset", "recipe", "synchronizeFromPhysics", "seed", "quality",
                "customMaxResolution", "vehicleHash", "terrain" };
            if( cooked )
            {
                fields.insert( "target" );
                fields.insert( "packagedBuild" );
            }
            require( object.size() == fields.size(), "Gameplay recipe has missing or extra fields" );
            for( const auto &entry : object )
                require( fields.count( std::string( entry.key().data(), entry.key().size() ) ) == 1,
                         "Unknown gameplay recipe field" );
            require( integer( object.at( "version" ), schemaVersion ) == schemaVersion &&
                         integer( object.at( "generatorVersion" ), generatorVersion ) == generatorVersion,
                     "Unsupported gameplay schema or generator version" );
            require( equals( object.at( "tier" ), "runtime-generated-vehicle" ) &&
                         equals( object.at( "generator" ), "WPVehicleGenerator" ) &&
                         equals( object.at( "vehiclePreset" ), "GrandPrix" ) &&
                         equals( object.at( "recipe" ), recipe ) &&
                         equals( object.at( "terrain" ), terrainID ),
                     "Gameplay package requires the fixed GrandPrix buildCar recipe and ground terrain" );
            require( object.at( "synchronizeFromPhysics" ).is_bool() &&
                         object.at( "synchronizeFromPhysics" ).as_bool(),
                     "Gameplay recipe requires synchronized physics dimensions" );
            integer( object.at( "seed" ), ( std::numeric_limits<u32>::max )() );
            const auto quality = int( integer( object.at( "quality" ), 3 ) );
            require( integer( object.at( "customMaxResolution" ), 1024 ) ==
                         u64( textureResolution( quality ) ),
                     "Gameplay texture resolution differs from the fixed buildCar recipe" );
            parseHash( object.at( "vehicleHash" ) );
            if( cooked )
                require( equals( object.at( "target" ), target ) &&
                             object.at( "packagedBuild" ).is_bool() &&
                             object.at( "packagedBuild" ).as_bool(),
                         "Gameplay package has the wrong target or build mode" );
            return object;
        }

        json::object sourceRecipe( u32 seed, int quality, u64 hash )
        {
            return { { "version", schemaVersion }, { "generatorVersion", generatorVersion },
                { "tier", "runtime-generated-vehicle" }, { "generator", "WPVehicleGenerator" },
                { "vehiclePreset", "GrandPrix" }, { "recipe", recipe },
                { "synchronizeFromPhysics", true }, { "seed", seed }, { "quality", quality },
                { "customMaxResolution", textureResolution( quality ) },
                { "vehicleHash", hashText( hash ) }, { "terrain", terrainID } };
        }

        std::string readText( const String &file )
        {
            const auto path = fs::u8path( file.c_str() );
            std::error_code error;
            const auto bytes = fs::file_size( path, error );
            require( !error && bytes > 0 && bytes <= descriptorByteLimit,
                     "Gameplay source is unavailable or exceeds its byte limit" );
            std::ifstream stream( path, std::ios::binary );
            std::string text( size_t( bytes ), '\0' );
            stream.read( text.data(), std::streamsize( bytes ) );
            require( stream.good() && stream.peek() == std::char_traits<char>::eof(),
                     "Gameplay source changed size or could not be read" );
            return text;
        }

        class GameplayCompiler final : public IResourceCompiler
        {
        public:
            String name() const override { return "VehicleAdvancedGameplayCompiler"; }
            Array<CompilerOutput> outputs() const override
            {
                return { { ResourceTypeID( "gameplay" ), compilerVersion } };
            }
            bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                                  String &error ) const override
            {
                try
                {
                    validateContext( context );
                    readRecipe( readText( context.sourcePath ), false );
                    const ResourceID terrain( terrainID );
                    dependencies.compileDependencies = { CompileDependency::resource( terrain ) };
                    dependencies.installDependencies = { terrain };
                    return true;
                }
                catch( const std::exception &exception )
                {
                    error = exception.what();
                    return false;
                }
            }
            CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                       Array<String> &messages ) const override
            {
                try
                {
                    validateContext( context );
                    auto object = readRecipe( readText( context.sourcePath ), false );
                    require( context.dependencyArtifacts.find( terrainID ) !=
                                 context.dependencyArtifacts.end(),
                             "Gameplay terrain dependency was not compiled" );
                    object["target"] = target;
                    object["packagedBuild"] = true;
                    const auto text = json::serialize( object );
                    require( text.size() <= descriptorByteLimit, "Cooked gameplay recipe exceeds its byte limit" );
                    output.write( text.data(), std::streamsize( text.size() ) );
                    require( output.good(), "Cannot write cooked gameplay recipe" );
                    return CompilationStatus::Success;
                }
                catch( const std::exception &exception )
                {
                    messages.push_back( exception.what() );
                    return CompilationStatus::Failure;
                }
            }
        private:
            static void validateContext( const CompileContext &context )
            {
                require( context.resourceId == ResourceID( rootID ) && context.target == target &&
                             context.packagedBuild,
                         "Gameplay compiler requires its fixed package root, target and package mode" );
            }
        };

        u64 expectedCompilerVersion()
        {
            // IResourceCompiler::versionFor hashes container v1, output version,
            // then type. Runtime validation requires no compiler instance.
            auto value = combineHash( 14695981039346656037ull, 1 );
            value = combineHash( value, compilerVersion );
            return combineHash( value, ResourceTypeID( "gameplay" ).value() );
        }

        void validateSource( const render::TerrainData &source )
        {
            String error;
            requireResource( render::validateTerrainData( source, error ), error );
            require( source.dimensions == Vector2I( dimension, dimension ) &&
                         source.spacing == Vector2F( 4, 4 ) && source.origin == Vector2F( -256, -256 ) &&
                         source.heightScale == 1 && source.layerWeights.size() == dimension * dimension &&
                         source.holes.size() == ( dimension - 1 ) * ( dimension - 1 ),
                     "Gameplay terrain must retain its bounded 512m, four-layer representative layout" );
            for( int layer = 0; layer < 4; ++layer )
                require( source.layers[layer].albedo == mapID( layer, 0 ) &&
                             source.layers[layer].normal == mapID( layer, 1 ) &&
                             source.layers[layer].orm == mapID( layer, 2 ),
                         "Gameplay terrain layer map IDs differ from the fixed cooked closure" );
            require( source.appearance.detailNormal == "data://global_detail.texres",
                     "Gameplay terrain detail normal differs from the fixed cooked closure" );
            bool holePresent = false, hillPresent = false;
            for( int z = 0; z < dimension; ++z )
                for( int x = 0; x < dimension; ++x )
                {
                    const auto height = source.heights[z * dimension + x];
                    require( height >= 0 && height <= 40, "Gameplay terrain height exceeds its fixed bounds" );
                    hillPresent = hillPresent || height > 1;
                    if( std::abs( -256 + x * 4 ) <= 96 && std::abs( -256 + z * 4 ) <= 224 )
                        require( height == 0, "Gameplay drive corridor must remain flat" );
                    if( x < dimension - 1 && z < dimension - 1 &&
                        render::terrainCellIsHole( source, x, z ) )
                    {
                        holePresent = true;
                        require( x >= 112 && x <= 116 && z >= 102 && z <= 106,
                                 "Gameplay holes must remain in the off-corridor validation patch" );
                    }
                }
            require( holePresent && hillPresent && render::terrainCellIsHole( source, 114, 104 ),
                     "Gameplay terrain requires its off-corridor hill and hole validation features" );
        }

        void writeText( const fs::path &file, const std::string &text )
        {
            std::ofstream stream( file, std::ios::binary );
            stream.write( text.data(), std::streamsize( text.size() ) );
            stream.close();
            require( stream.good(), "Cannot write gameplay authoring input: " + file.u8string() );
        }

        void writeImage( const fs::path &file, std::array<u8, 4> rgba, int map )
        {
            // Self-authored 8x8 top-down BGRA TGA; subtle variation makes all
            // four PBR layers useful without depending on an external asset tree.
            std::array<u8, 18 + 8 * 8 * 4> bytes{};
            bytes[2] = 2;
            bytes[12] = bytes[14] = 8;
            bytes[16] = 32;
            bytes[17] = 0x28;
            for( int z = 0; z < 8; ++z )
                for( int x = 0; x < 8; ++x )
                {
                    auto pixel = rgba;
                    const int variation = ( ( x * 13 + z * 7 ) % 9 ) - 4;
                    if( map == 0 )
                        for( int channel = 0; channel < 3; ++channel )
                            pixel[channel] = u8( std::clamp( int( pixel[channel] ) + variation * 2, 0, 255 ) );
                    else if( map == 1 )
                    {
                        pixel[0] = u8( 128 + variation );
                        pixel[1] = u8( 128 + ( ( x * 3 + z * 11 ) % 9 ) - 4 );
                    }
                    else
                    {
                        pixel[1] = u8( std::clamp( int( pixel[1] ) + variation, 0, 255 ) );
                        pixel[3] = u8( 128 + variation * 8 );
                    }
                    const auto offset = 18 + ( z * 8 + x ) * 4;
                    bytes[offset] = pixel[2];
                    bytes[offset + 1] = pixel[1];
                    bytes[offset + 2] = pixel[0];
                    bytes[offset + 3] = pixel[3];
                }
            std::ofstream stream( file, std::ios::binary );
            stream.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
            stream.close();
            require( stream.good(), "Cannot write gameplay texture input: " + file.u8string() );
        }

        void writeTexture( const fs::path &source, const std::string &stem,
                           const std::array<u8, 4> &rgba, int map )
        {
            writeImage( source / ( stem + ".tga" ), rgba, map );
            const auto filter = map == 0 ? "colour" : map == 1 ? "normal" : "data";
            const json::object descriptor{ { "version", 1 }, { "source", "data://" + stem + ".tga" },
                { "mipFilter", filter }, { "atlasColumns", 1 }, { "alphaCutoff", .5 } };
            writeText( source / ( stem + ".texres" ), json::serialize( descriptor ) );
        }
    }

    render::TerrainData makeSource()
    {
        render::TerrainData source;
        source.dimensions = { dimension, dimension };
        source.spacing = { 4, 4 };
        source.origin = { -256, -256 };
        source.heights.resize( dimension * dimension );
        source.layerWeights.resize( dimension * dimension );
        source.surfaceMasks.assign( dimension * dimension, Vector2F( 1, 1 ) );
        source.holes.assign( ( dimension - 1 ) * ( dimension - 1 ), 0 );
        for( int layer = 0; layer < 4; ++layer )
        {
            source.layers[layer].albedo = mapID( layer, 0 );
            source.layers[layer].normal = mapID( layer, 1 );
            source.layers[layer].orm = mapID( layer, 2 );
            source.layers[layer].uvScale = { .25f, .25f };
            source.layers[layer].roughness = 1;
            source.layers[layer].normalStrength = .65f;
            source.layers[layer].heightScale = .2f;
        }
        source.appearance.projection = render::TerrainProjection::Triplanar;
        source.appearance.detailNormal = "data://global_detail.texres";
        source.appearance.detailStrength = .25f;
        source.appearance.macroStrength = .08f;
        source.appearance.macroSeed = 0x475430u;
        for( int z = 0; z < dimension; ++z )
            for( int x = 0; x < dimension; ++x )
            {
                const auto worldX = float( -256 + x * 4 ), worldZ = float( -256 + z * 4 );
                const auto edge = ( std::max )( std::clamp( ( std::abs( worldX ) - 96 ) / 160, 0.f, 1.f ),
                                               std::clamp( ( std::abs( worldZ ) - 224 ) / 32, 0.f, 1.f ) );
                source.heights[z * dimension + x] = edge * edge *
                    ( 12 + 6 * std::sin( worldX * .03f ) + 4 * std::cos( worldZ * .025f ) );
                const auto east = float( x ) / ( dimension - 1 ), north = float( z ) / ( dimension - 1 );
                const auto path = std::clamp( 1 - std::abs( worldX ) / 24, 0.f, 1.f );
                source.layerWeights[z * dimension + x] = render::normalizeTerrainWeights( {
                    .1f + .7f * ( 1 - east ) * ( 1 - north ),
                    .1f + .7f * east * ( 1 - north ),
                    .1f + .7f * ( 1 - east ) * north + path,
                    .1f + .7f * east * north + edge * .5f } );
            }
        for( int z = 102; z <= 106; ++z )
            for( int x = 112; x <= 116; ++x )
                source.holes[z * ( dimension - 1 ) + x] = 1;
        validateSource( source );
        return source;
    }

    void exportPackage( const fs::path &destination, const render::TerrainData &source,
                        u32 seed, int quality, u64 vehicleHash )
    {
        validateSource( source );
        const auto descriptor = sourceRecipe( seed, quality, vehicleHash );
        readRecipe( json::serialize( descriptor ), false );
        const auto output = fs::absolute( destination ).lexically_normal();
        require( !output.filename().empty() && !fs::exists( fs::symlink_status( output ) ) &&
                     fs::is_directory( output.parent_path() ),
                 "Gameplay package destination must be fresh and have an existing parent directory" );
        const auto ownerName = ".terrain-gameplay-author-" + standard( StringUtil::getUUID() );
        const auto owned = output.parent_path() / ownerName;
        require( fs::create_directory( owned ), "Cannot exclusively create gameplay authoring directory" );
        try
        {
            const auto sourceRoot = owned / "source", compiledRoot = owned / "compiled";
            require( fs::create_directory( sourceRoot ), "Cannot create gameplay source directory" );
            const std::array<u8, 4> colours[] = { { 80, 108, 54, 255 }, { 115, 83, 54, 255 },
                                                { 123, 120, 111, 255 }, { 158, 153, 138, 255 } };
            for( int layer = 0; layer < 4; ++layer )
            {
                writeTexture( sourceRoot, mapStem( layer, 0 ), colours[layer], 0 );
                writeTexture( sourceRoot, mapStem( layer, 1 ), { 128, 128, 255, 255 }, 1 );
                writeTexture( sourceRoot, mapStem( layer, 2 ), { 255, u8( 220 - layer * 8 ), 0, 128 }, 2 );
            }
            writeTexture( sourceRoot, "global_detail", { 128, 128, 255, 255 }, 1 );
            writeText( sourceRoot / "ground.terrain", standard( render::serializeTerrainData( source ) ) );
            writeText( sourceRoot / "terrain-gameplay.gameplay", json::serialize( descriptor ) );
            auto registry = std::make_shared<ResourceCompilerRegistry>();
            String error;
            requireResource( registry->registerCompiler( std::make_shared<render::GraphicsResourceCompiler>(), &error ), error );
            requireResource( registry->registerCompiler( std::make_shared<render::TerrainResourceCompiler>(), &error ), error );
            requireResource( registry->registerCompiler( std::make_shared<GameplayCompiler>(), &error ), error );
            ResourceSystem author( registry, std::make_shared<ResourceCompilationDatabase>() );
            ResourceSystemConfig config;
            config.sourceRoot = pathString( sourceRoot );
            config.compiledRoot = pathString( compiledRoot );
            config.target = target;
            config.maxRuntimePayloadBytes = 64ull << 20;
            requireResource( author.initialize( config, error ), error );
            CompilationOptions options;
            options.packagedBuild = true;
            const auto report = author.compile( ResourceID( rootID ), options );
            if( !report.succeeded() )
            {
                std::string messages = "Gameplay package compilation failed";
                for( const auto &message : report.messages )
                    messages += "\n" + standard( message );
                throw std::runtime_error( messages );
            }
            requireResource( author.writeRuntimePackage( pathString( output ), { ResourceID( rootID ) }, {}, error ), error );
            author.shutdown();
            // Validate the exported consumer path before removing any authoring
            // evidence. No source tree, compiler registry or database is mounted.
            const auto verified = loadPackage( output );
            require( verified->seed == seed && verified->quality == quality && verified->vehicleHash == vehicleHash,
                     "Gameplay export did not preserve the pinned vehicle recipe" );
            require( owned.parent_path() == output.parent_path() && owned.filename().u8string() == ownerName &&
                         fs::canonical( owned ).parent_path() == fs::canonical( output.parent_path() ) &&
                         !fs::is_symlink( fs::symlink_status( owned ) ),
                     "Gameplay authoring cleanup ownership check failed" );
            fs::remove_all( cleanupIOPath( owned ) );
        }
        catch( const std::exception &exception )
        {
            throw std::runtime_error( std::string( exception.what() ) +
                "\nGameplay authoring evidence retained at " + owned.u8string() );
        }
    }

    std::unique_ptr<Package> loadPackage( const fs::path &directory )
    {
        const auto input = fs::absolute( directory ).lexically_normal();
        require( fs::is_directory( input ), "Gameplay package directory is unavailable" );
        auto result = std::make_unique<Package>();
        result->runtime = std::make_shared<ResourceSystem>( nullptr, nullptr );
        RuntimeResourceConfig config;
        config.compiledRoot = pathString( input );
        config.manifestPath = pathString( input / "runtime.wprm" );
        config.target = target;
        config.maxPayloadBytes = 64ull << 20;
        config.maxClosurePayloadBytes = 128ull << 20;
        config.maxClosureResources = 15;
        config.maxDependencyDepth = 3;
        String error;
        requireResource( result->runtime->initializeRuntime( config, error ), error );
        const auto root = result->runtime->load( ResourceID( rootID ), error );
        require( bool( root ), standard( error ) );
        require( root->header.resourceId == ResourceID( rootID ) &&
                     root->header.resourceType == ResourceTypeID( "gameplay" ) &&
                     root->header.compilerVersion == expectedCompilerVersion() &&
                     root->payload.size() <= descriptorByteLimit && root->dependencies.size() == 1 &&
                     root->header.installDependencies.size() == 1 &&
                     root->header.installDependencies.front() == ResourceID( terrainID ),
                 "Gameplay root compiler version, bounds or install dependency is invalid" );
        const auto object = readRecipe( std::string( reinterpret_cast<const char *>( root->payload.data() ),
                                                    root->payload.size() ), true );
        const auto &terrain = root->dependencies.front();
        require( terrain && terrain->header.resourceId == ResourceID( terrainID ) && terrain->dependencies.size() == 13,
                 "Gameplay terrain must retain exactly its thirteen cooked texture dependencies" );
        result->decoded = render::decodeCookedTerrainResource( terrain, target, true, error );
        require( bool( result->decoded ), standard( error ) );
        validateSource( result->decoded->data.terrain );
        result->seed = u32( integer( object.at( "seed" ), ( std::numeric_limits<u32>::max )() ) );
        result->quality = int( integer( object.at( "quality" ), 3 ) );
        result->vehicleHash = parseHash( object.at( "vehicleHash" ) );
        return result;
    }
}
#endif
