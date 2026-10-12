#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/FoliageResource.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <cJSON.h>
#include "../../Workphone/Graphics/FoliageJsonBudget.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#ifdef _WIN32
#    include <windows.h>
#endif

namespace workphone::render
{
    namespace
    {
        namespace fs = std::filesystem;
        using namespace resource;
        constexpr u64 metadataLimit = 16ull << 20;
        constexpr u64 speciesLimit = 16ull << 20;
        constexpr u64 pageLimit = 8ull << 20;
        constexpr char magic[8] = { 'W', 'P', 'F', 'O', 'L', '0', '1', '\0' };

        void check( bool ok, const char *message )
        {
            if( !ok )
            {
                throw std::runtime_error( message );
            }
        }

        void checked( bool ok, const String &error )
        {
            if( !ok )
            {
                throw std::runtime_error( error.c_str() );
            }
        }

        struct Pin
        {
            ResourceID id;
            u64 version = 0, source = 0, hash = 0, bytes = 0;
        };

        struct Entry
        {
            FoliageCellKey cell;
            ResourceID id;
            u32 count = 0;
        };

        struct Document
        {
            String speciesJson;
            f32 pageSize = 64;
            f32 geometryRadius = 0, bendRadius = 0;
            Array<ResourceID> species;
            Array<Entry> pages;
            FoliagePageData page;
        };

        struct Envelope
        {
            String target;
            bool packaged = false;
            Array<Pin> pins;
            Document data;
        };

        using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;

        const cJSON *field( const cJSON *o, const char *key )
        {
            check( cJSON_IsObject( o ), "Expected foliage descriptor object" );
            auto p = cJSON_GetObjectItemCaseSensitive( o, key );
            check( p != nullptr, "Missing foliage descriptor field" );
            return p;
        }

        double numeric( const cJSON *v )
        {
            check( cJSON_IsNumber( v ) && std::isfinite( v->valuedouble ),
                   "Expected finite foliage number" );
            return v->valuedouble;
        }

        u32 unsignedValue( const cJSON *v )
        {
            auto n = numeric( v );
            check( n >= 0 && n <= std::numeric_limits<u32>::max() && n == std::floor( n ),
                   "Invalid foliage unsigned integer" );
            return u32( n );
        }

        s32 signedValue( const cJSON *v )
        {
            auto n = numeric( v );
            check( n >= std::numeric_limits<s32>::min() && n <= std::numeric_limits<s32>::max() &&
                       n == std::floor( n ),
                   "Invalid foliage cell integer" );
            return s32( n );
        }

        String text( const cJSON *v )
        {
            check( cJSON_IsString( v ) && v->valuestring && std::strlen( v->valuestring ) <= 4096,
                   "Invalid foliage descriptor string" );
            return v->valuestring;
        }

        size_t count( const cJSON *v, size_t limit )
        {
            check( cJSON_IsArray( v ), "Expected foliage descriptor array" );
            auto n = cJSON_GetArraySize( v );
            check( n >= 0 && size_t( n ) <= limit, "Foliage descriptor array exceeds limit" );
            return size_t( n );
        }

        std::array<f32, 4> floats( const cJSON *v, size_t n )
        {
            check( count( v, n ) == n, "Invalid foliage vector width" );
            std::array<f32, 4> a{};
            size_t i = 0;
            for( auto p = v->child; p; p = p->next )
            {
                auto x = numeric( p );
                check( std::abs( x ) <= std::numeric_limits<f32>::max(), "Foliage float overflow" );
                a[i++] = f32( x );
            }
            return a;
        }

        ResourceID idFor( const String &s, const char *type )
        {
            ResourceID id;
            String error;
            checked( id.set( s, &error ), error );
            check( !id.isSubResource() && id.type() == ResourceTypeID( type ),
                   "Incorrect foliage dependency resource type" );
            return id;
        }

        Array<ResourceID> speciesIDs( const cJSON *v )
        {
            count( v, foliageMaximumSpecies );
            Array<ResourceID> ids;
            std::set<ResourceID> unique;
            for( auto p = v->child; p; p = p->next )
            {
                auto id = idFor( text( p ), "folspec" );
                check( unique.insert( id ).second, "Duplicate foliage species resource" );
                ids.push_back( id );
            }
            return ids;
        }

        FoliageCellKey cell( const cJSON *v )
        {
            check( count( v, 2 ) == 2, "Invalid foliage cell width" );
            return { signedValue( v->child ), signedValue( v->child->next ) };
        }

        FoliageInstance instance( const cJSON *v )
        {
            FoliageInstance out;
            const auto s = text( field( v, "id" ) );
            const auto parsed = std::from_chars( s.data(), s.data() + s.size(), out.id );
            check( parsed.ec == std::errc{} && parsed.ptr == s.data() + s.size(),
                   "Invalid foliage instance ID" );
            out.species = unsignedValue( field( v, "species" ) );
            auto p = floats( field( v, "position" ), 3 );
            out.position = { p[0], p[1], p[2] };
            auto q = floats( field( v, "rotation" ), 4 );
            out.rotation = { q[0], q[1], q[2], q[3] };
            auto scale = floats( field( v, "scale" ), 3 );
            out.scale = { scale[0], scale[1], scale[2] };
            auto tint = floats( field( v, "tint" ), 4 );
            out.tint = { tint[0], tint[1], tint[2], tint[3] };
            out.seed = unsignedValue( field( v, "seed" ) );
            out.flags = unsignedValue( field( v, "flags" ) );
            return out;
        }

        String sourceText( const CompileContext &context )
        {
            std::error_code ec;
            auto n = fs::file_size( fs::u8path( context.sourcePath.c_str() ), ec );
            check( !ec && n > 0 && n <= foliageMaximumSourceBytes,
                   "Foliage source unavailable or oversized" );
            String s( size_t( n ), '\0' );
            std::ifstream f( fs::u8path( context.sourcePath.c_str() ), std::ios::binary );
            f.read( s.data(), std::streamsize( n ) );
            check(
                f.good() && f.peek() == std::char_traits<char>::eof() && s.find( '\0' ) == String::npos,
                "Foliage source read failed" );
            detail::checkFoliageJsonBudget( { s.data(), s.size() } );
            return s;
        }

        Document descriptor( const CompileContext &context )
        {
            check( !context.resourceId.isSubResource(), "Foliage subresources are unsupported" );
            const auto source = sourceText( context );
            Document d;
            if( context.resourceId.type() == ResourceTypeID( "folspec" ) )
            {
                FoliageSpecies species;
                String error;
                checked( deserializeFoliageSpecies( source, species, error ), error );
                checked( serializeFoliageSpecies( species, d.speciesJson, error ), error );
                return d;
            }
            const char *end = nullptr;
            Json json( cJSON_ParseWithLengthOpts( source.c_str(), source.size() + 1, &end, 1 ),
                       cJSON_Delete );
            check( json && unsignedValue( field( json.get(), "version" ) ) == 1,
                   "Unsupported foliage descriptor version" );
            const auto size = numeric( field( json.get(), "pageSize" ) );
            check( size >= 1 && size <= 4096, "Invalid foliage page size" );
            d.pageSize = f32( size );
            d.species = speciesIDs( field( json.get(), "species" ) );
            if( context.resourceId.type() == ResourceTypeID( "folpage" ) )
            {
                d.page.cell = cell( field( json.get(), "cell" ) );
                auto a = field( json.get(), "instances" );
                count( a, foliageMaximumPageInstances );
                d.page.instances.reserve( cJSON_GetArraySize( a ) );
                for( auto p = a->child; p; p = p->next )
                {
                    d.page.instances.push_back( instance( p ) );
                }
            }
            else
            {
                check( context.resourceId.type() == ResourceTypeID( "folworld" ),
                       "Unsupported foliage resource type" );
                auto a = field( json.get(), "pages" );
                count( a, foliageMaximumPages );
                std::set<FoliageCellKey> cells;
                std::set<ResourceID> ids;
                for( auto p = a->child; p; p = p->next )
                {
                    Entry e;
                    e.cell = cell( field( p, "cell" ) );
                    e.id = idFor( text( field( p, "resource" ) ), "folpage" );
                    check( cells.insert( e.cell ).second && ids.insert( e.id ).second,
                           "Duplicate foliage page entry" );
                    d.pages.push_back( e );
                }
            }
            return d;
        }

        Array<ResourceID> textures( const FoliageSpecies &s )
        {
            std::set<ResourceID> ids;
            const auto add = [&]( const String &name ) {
                if( !name.empty() )
                {
                    ids.insert( idFor( name, "texres" ) );
                }
            };
            for( const auto &m : s.materials )
            {
                add( m.albedo );
                add( m.normal );
                add( m.orm );
            }
            add( s.impostor.albedo );
            add( s.impostor.normal );
            add( s.impostor.orm );
            add( s.impostor.depth );
            return { ids.begin(), ids.end() };
        }

        Array<ResourceID> dependencies( const Document &d )
        {
            if( !d.speciesJson.empty() )
            {
                FoliageSpecies s;
                String error;
                checked( deserializeFoliageSpecies( d.speciesJson, s, error ), error );
                return textures( s );
            }
            std::set<ResourceID> ids( d.species.begin(), d.species.end() );
            for( const auto &p : d.pages )
            {
                ids.insert( p.id );
            }
            return { ids.begin(), ids.end() };
        }

        void number( std::ostream &out, u64 value, size_t bytes )
        {
            for( size_t i = 0; i < bytes; ++i )
            {
                out.put( char( ( value >> ( 8 * i ) ) & 255 ) );
            }
        }

        void string( std::ostream &out, const String &s )
        {
            number( out, s.size(), 4 );
            out.write( s.data(), std::streamsize( s.size() ) );
        }

        void floating( std::ostream &out, f32 value )
        {
            u32 bits;
            std::memcpy( &bits, &value, 4 );
            number( out, bits, 4 );
        }

        struct Reader
        {
            const Array<u8> &bytes;
            size_t at = 0;

            u64 number( size_t n )
            {
                check( n <= bytes.size() - at, "Truncated foliage payload" );
                u64 x = 0;
                for( size_t i = 0; i < n; ++i )
                {
                    x |= u64( bytes[at++] ) << ( i * 8 );
                }
                return x;
            }

            String string( size_t limit )
            {
                auto n = number( 4 );
                check( n <= limit && n <= bytes.size() - at, "Oversized foliage payload string" );
                String s( reinterpret_cast<const char *>( bytes.data() + at ), size_t( n ) );
                at += size_t( n );
                check( s.find( '\0' ) == String::npos, "Embedded NUL in foliage payload" );
                return s;
            }

            f32 floating()
            {
                u32 b = u32( number( 4 ) );
                f32 n;
                std::memcpy( &n, &b, 4 );
                return n;
            }
        };

        Pin pin( const RuntimeResource &r )
        {
            return { r.header.resourceId, r.header.compilerVersion, r.header.sourceHash,
                     r.header.payloadHash, r.header.payloadSize };
        }

        void matches( const RuntimeResource &r, const Pin &p )
        {
            check( r.header.resourceId == p.id && r.header.compilerVersion == p.version &&
                       r.header.sourceHash == p.source && r.header.payloadHash == p.hash &&
                       r.header.payloadSize == p.bytes,
                   "Foliage dependency generation mismatch" );
        }

        const Pin &findPin( const Envelope &e, const ResourceID &id )
        {
            for( const auto &p : e.pins )
            {
                if( p.id == id )
                {
                    return p;
                }
            }
            throw std::runtime_error( "Missing foliage dependency pin" );
        }

        void write( std::ostream &out, const Envelope &e, const ResourceTypeID &type )
        {
            out.write( magic, 8 );
            number( out, 2, 4 );
            number( out, e.packaged ? 1 : 0, 4 );
            string( out, e.target );
            number( out, e.pins.size(), 4 );
            for( const auto &p : e.pins )
            {
                string( out, p.id.str() );
                number( out, p.version, 8 );
                number( out, p.source, 8 );
                number( out, p.hash, 8 );
                number( out, p.bytes, 8 );
            }
            const auto &d = e.data;
            if( type == ResourceTypeID( "folspec" ) )
            {
                string( out, d.speciesJson );
                return;
            }
            floating( out, d.pageSize );
            number( out, d.species.size(), 4 );
            for( const auto &id : d.species )
            {
                string( out, id.str() );
            }
            if( type == ResourceTypeID( "folworld" ) )
            {
                floating( out, d.geometryRadius );
                floating( out, d.bendRadius );
                number( out, d.pages.size(), 4 );
                for( const auto &p : d.pages )
                {
                    number( out, u32( p.cell.x ), 4 );
                    number( out, u32( p.cell.z ), 4 );
                    string( out, p.id.str() );
                    number( out, p.count, 4 );
                }
                return;
            }
            number( out, u32( d.page.cell.x ), 4 );
            number( out, u32( d.page.cell.z ), 4 );
            number( out, d.page.instances.size(), 4 );
            for( const auto &v : d.page.instances )
            {
                number( out, v.id, 8 );
                number( out, v.species, 4 );
                for( auto x : { v.position.x, v.position.y, v.position.z, v.rotation.w, v.rotation.x,
                                v.rotation.y, v.rotation.z, v.scale.x, v.scale.y, v.scale.z, v.tint.r,
                                v.tint.g, v.tint.b, v.tint.a } )
                {
                    floating( out, x );
                }
                number( out, v.seed, 4 );
                number( out, v.flags, 4 );
            }
        }

        Envelope read( const RuntimeResource &r, const String &target, bool packaged )
        {
            const auto type = r.header.resourceType;
            FoliageResourceCompiler compiler;
            check( r.header.isValid() && !r.header.resourceId.isSubResource() &&
                       r.header.resourceId.type() == type && compiler.versionFor( type ) != 0 &&
                       r.header.compilerVersion == compiler.versionFor( type ),
                   "Unsupported foliage resource header" );
            check( r.payload.size() >= 24 && r.payload.size() <= metadataLimit + speciesLimit &&
                       r.header.payloadSize == r.payload.size() &&
                       r.header.payloadHash == hashBytes( r.payload.data(), r.payload.size() ),
                   "Foliage payload hash/size mismatch" );
            check( std::memcmp( r.payload.data(), magic, 8 ) == 0, "Invalid foliage payload magic" );
            Reader in{ r.payload, 8 };
            Envelope e;
            check( in.number( 4 ) == 2,
                   "Unsupported foliage payload version; recook the foliage sources" );
            auto mode = in.number( 4 );
            check( mode <= 1, "Invalid foliage build mode" );
            e.packaged = mode != 0;
            e.target = in.string( 128 );
            check( !e.target.empty() && e.target == target && e.packaged == packaged,
                   "Foliage target/build mode mismatch" );
            auto n = in.number( 4 );
            check( n <= foliageMaximumPages + foliageMaximumSpecies &&
                       n == r.header.installDependencies.size(),
                   "Foliage install closure mismatch" );
            for( size_t i = 0; i < n; ++i )
            {
                Pin p;
                String error;
                checked( p.id.set( in.string( 4096 ), &error ), error );
                p.version = in.number( 8 );
                p.source = in.number( 8 );
                p.hash = in.number( 8 );
                p.bytes = in.number( 8 );
                check( p.id == r.header.installDependencies[i] &&
                           p.bytes <= graphicsTextureByteLimit + 4096,
                       "Invalid foliage dependency pin" );
                if( r.header.formatVersion >= 2 )
                {
                    check( p.source == r.header.installArtifacts[i].sourceHash &&
                               p.hash == r.header.installArtifacts[i].payloadHash,
                           "Foliage container pin mismatch" );
                }
                e.pins.push_back( p );
            }
            auto &d = e.data;
            if( type == ResourceTypeID( "folspec" ) )
            {
                d.speciesJson = in.string( size_t( speciesLimit ) );
            }
            else
            {
                check( type == ResourceTypeID( "folworld" ) || type == ResourceTypeID( "folpage" ),
                       "Unsupported foliage type" );
                d.pageSize = in.floating();
                check( std::isfinite( d.pageSize ) && d.pageSize >= 1 && d.pageSize <= 4096,
                       "Invalid cooked foliage page size" );
                n = in.number( 4 );
                check( n <= foliageMaximumSpecies, "Too many cooked foliage species" );
                std::set<ResourceID> species;
                for( size_t i = 0; i < n; ++i )
                {
                    auto id = idFor( in.string( 4096 ), "folspec" );
                    check( species.insert( id ).second, "Duplicate cooked foliage species" );
                    d.species.push_back( id );
                }
                if( type == ResourceTypeID( "folworld" ) )
                {
                    d.geometryRadius = in.floating();
                    d.bendRadius = in.floating();
                    check( std::isfinite( d.geometryRadius ) && std::isfinite( d.bendRadius ) &&
                               d.geometryRadius >= 0 && d.bendRadius >= 0,
                           "Invalid cooked foliage paging extents" );
                    n = in.number( 4 );
                    check( n <= foliageMaximumPages, "Too many cooked foliage pages" );
                    std::set<FoliageCellKey> cells;
                    std::set<ResourceID> ids;
                    u64 population = 0;
                    for( size_t i = 0; i < n; ++i )
                    {
                        Entry p;
                        p.cell = { s32( u32( in.number( 4 ) ) ), s32( u32( in.number( 4 ) ) ) };
                        p.id = idFor( in.string( 4096 ), "folpage" );
                        p.count = u32( in.number( 4 ) );
                        population += p.count;
                        check( p.count <= foliageMaximumPageInstances &&
                                   population <= foliageMaximumInstances &&
                                   cells.insert( p.cell ).second && ids.insert( p.id ).second,
                               "Invalid cooked foliage page directory" );
                        d.pages.push_back( p );
                    }
                }
                else
                {
                    d.page.cell = { s32( u32( in.number( 4 ) ) ), s32( u32( in.number( 4 ) ) ) };
                    n = in.number( 4 );
                    check( n <= foliageMaximumPageInstances,
                           "Cooked foliage page population exceeds limit" );
                    d.page.instances.reserve( size_t( n ) );
                    for( size_t i = 0; i < n; ++i )
                    {
                        FoliageInstance v;
                        v.id = in.number( 8 );
                        v.species = u32( in.number( 4 ) );
                        v.position = { in.floating(), in.floating(), in.floating() };
                        v.rotation = { in.floating(), in.floating(), in.floating(), in.floating() };
                        v.scale = { in.floating(), in.floating(), in.floating() };
                        v.tint = { in.floating(), in.floating(), in.floating(), in.floating() };
                        v.seed = u32( in.number( 4 ) );
                        v.flags = u32( in.number( 4 ) );
                        d.page.instances.push_back( v );
                    }
                }
            }
            check( in.at == r.payload.size(), "Trailing foliage payload bytes" );
            if( type != ResourceTypeID( "folspec" ) )
            {
                check( dependencies( d ) == r.header.installDependencies,
                       "Unexpected foliage install dependency" );
            }
            return e;
        }

        RuntimeResource artifact( const CompileContext &context, const ResourceID &id )
        {
            const auto found = context.dependencyArtifacts.find( id.str() );
            check( found != context.dependencyArtifacts.end(),
                   "Foliage compiler requires pinned artifacts" );
            AssetCatalogPath path;
            String error;
            checked( canonicalAssetCatalogPath( context.compiledRoot, found->second, path, error ),
                     error );
            RuntimeResource r;
            checked( CompiledResourceIO::read(
                         ( fs::u8path( context.compiledRoot.c_str() ) / fs::u8path( path.path.c_str() ) )
                             .generic_u8string()
                             .c_str(),
                         r, error, graphicsTextureByteLimit + 4096 ),
                     error );
            check( r.header.resourceId == id, "Foliage compiler artifact identity mismatch" );
            return r;
        }

        std::shared_ptr<const FoliageSpecies> readSpecies( const Envelope &e )
        {
            auto s = std::make_shared<FoliageSpecies>();
            String error;
            checked( deserializeFoliageSpecies( e.data.speciesJson, *s, error ), error );
            auto ids = textures( *s );
            check( ids.size() == e.pins.size(), "Foliage species texture closure mismatch" );
            for( size_t i = 0; i < ids.size(); ++i )
            {
                check( ids[i] == e.pins[i].id, "Unexpected foliage species texture" );
            }
            return s;
        }

        void textureSemantics( const FoliageSpecies &species, const ResourceID &id,
                               const CookedTextureData &data )
        {
            const auto filter = data.mipSettings.filter;
            auto validate = [&]( const String &albedo, const String &normal, const String &orm,
                                 const String &depth ) {
                if( albedo == id.str() )
                {
                    check( filter == TextureMipFilter::None || filter == TextureMipFilter::Colour ||
                               filter == TextureMipFilter::Cutout,
                           "Foliage albedo requires colour/cutout mips" );
                }
                if( normal == id.str() )
                {
                    check( filter == TextureMipFilter::None || filter == TextureMipFilter::Normal,
                           "Foliage normal requires normal mips" );
                }
                if( orm == id.str() || depth == id.str() )
                {
                    check( filter == TextureMipFilter::None || filter == TextureMipFilter::Data,
                           "Foliage ORM/depth requires linear data mips" );
                }
            };
            for( const auto &m : species.materials )
            {
                validate( m.albedo, m.normal, m.orm, {} );
            }
            validate( species.impostor.albedo, species.impostor.normal, species.impostor.orm,
                      species.impostor.depth );
        }
    }  // namespace

    bool writeFoliageSources( FoliageSnapshot snapshot, const String &sourceRoot,
                              const resource::ResourceID &worldId, String &error )
    {
        std::vector<fs::path> created;
        fs::path temporary;
        try
        {
            check( snapshot && worldId.isValid() && !worldId.isSubResource() &&
                       worldId.type() == resource::ResourceTypeID( "folworld" ),
                   "Foliage export requires a world resource ID" );
            checked( validateFoliageWorld( *snapshot, error ), error );
            String root;
            checked( canonicalAssetCatalogRoot( sourceRoot, root, error ), error );
            AssetCatalogPath relative;
            checked( canonicalAssetCatalogPath( root, worldId.str().substr( 7 ), relative, error ),
                     error );
            const auto destination = fs::u8path( root.c_str() ) / fs::u8path( relative.path.c_str() );
            fs::create_directories( destination.parent_path() );
            auto generation = StringUtil::getUUID();
            generation.erase( std::remove( generation.begin(), generation.end(), '-' ),
                              generation.end() );
            const auto stem = String( ".foliage/" ) + generation + "/";
            const auto generationDirectory =
                fs::u8path( root.c_str() ) / ".foliage" / fs::u8path( generation.c_str() );
            fs::create_directories( generationDirectory.parent_path() );
            check( fs::create_directory( generationDirectory ),
                   "Foliage export generation already exists" );
            const auto quote = []( const String &value ) {
                Json j( cJSON_CreateString( value.c_str() ), cJSON_Delete );
                check( bool( j ), "Foliage JSON allocation failed" );
                char *s = cJSON_PrintUnformatted( j.get() );
                check( s != nullptr, "Foliage JSON print failed" );
                String out( s );
                cJSON_free( s );
                return out;
            };
            const auto publish = [&]( const fs::path &file, const String &data, bool rootFile ) {
                check( data.size() <= foliageMaximumSourceBytes,
                       "Foliage exported descriptor exceeds source limit" );
                temporary = file.parent_path() /
                            fs::u8path( ( String( "wp-fol-" ) + generation + ".tmp" ).c_str() );
                std::ofstream out( temporary, std::ios::binary | std::ios::trunc );
                out.write( data.data(), std::streamsize( data.size() ) );
                out.flush();
                check( bool( out ), "Foliage descriptor write failed" );
                out.close();
                check( bool( out ), "Foliage descriptor close failed" );
#ifdef _WIN32
                check( MoveFileExW( temporary.c_str(), file.c_str(),
                                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) != 0,
                       "Foliage descriptor atomic publication failed" );
#else
                fs::rename( temporary, file );
#endif
                temporary.clear();
                if( !rootFile )
                    created.push_back( file );
            };
            Array<String> species;
            for( size_t i = 0; i < snapshot->species.size(); ++i )
            {
                const auto relativeSpecies = stem + "s" + std::to_string( i ).c_str() + ".folspec";
                String speciesText;
                checked( serializeFoliageSpecies( *snapshot->species[i], speciesText, error ), error );
                publish( fs::u8path( root.c_str() ) / fs::u8path( relativeSpecies.c_str() ), speciesText,
                         false );
                species.push_back( "data://" + relativeSpecies );
            }
            std::ostringstream speciesJson;
            speciesJson << '[';
            for( size_t i = 0; i < species.size(); ++i )
            {
                if( i )
                {
                    speciesJson << ',';
                }
                speciesJson << quote( species[i] );
            }
            speciesJson << ']';
            std::map<FoliageCellKey, Array<const FoliageInstance *>> cells;
            for( const auto &v : snapshot->instances )
            {
                FoliageCellKey key;
                checked( foliageCellForPosition( v.position, snapshot->pageSize, key, error ), error );
                cells[key].push_back( &v );
            }
            std::ostringstream world;
            world << std::setprecision( std::numeric_limits<f32>::max_digits10 )
                  << "{\"version\":1,\"pageSize\":" << snapshot->pageSize
                  << ",\"species\":" << speciesJson.str() << ",\"pages\":[";
            size_t pageIndex = 0;
            for( const auto &entry : cells )
            {
                check( entry.second.size() <= foliageMaximumPageInstances,
                       "Foliage exported page exceeds population limit" );
                const auto relativePage = stem + "p" + std::to_string( pageIndex ).c_str() + ".folpage";
                std::ostringstream page;
                page << std::setprecision( std::numeric_limits<f32>::max_digits10 )
                     << "{\"version\":1,\"pageSize\":" << snapshot->pageSize
                     << ",\"species\":" << speciesJson.str() << ",\"cell\":[" << entry.first.x << ','
                     << entry.first.z << "],\"instances\":[";
                for( size_t i = 0; i < entry.second.size(); ++i )
                {
                    const auto &v = *entry.second[i];
                    if( i )
                    {
                        page << ',';
                    }
                    page << "{\"id\":\"" << v.id << "\",\"species\":" << v.species << ",\"position\":["
                         << v.position.x << ',' << v.position.y << ',' << v.position.z
                         << "],\"rotation\":[" << v.rotation.w << ',' << v.rotation.x << ','
                         << v.rotation.y << ',' << v.rotation.z << "],\"scale\":[" << v.scale.x << ','
                         << v.scale.y << ',' << v.scale.z << "],\"tint\":[" << v.tint.r << ','
                         << v.tint.g << ',' << v.tint.b << ',' << v.tint.a << "],\"seed\":" << v.seed
                         << ",\"flags\":" << v.flags << '}';
                }
                page << "]}";
                publish( fs::u8path( root.c_str() ) / fs::u8path( relativePage.c_str() ),
                         page.str().c_str(), false );
                if( pageIndex++ )
                {
                    world << ',';
                }
                world << "{\"cell\":[" << entry.first.x << ',' << entry.first.z
                      << "],\"resource\":" << quote( "data://" + relativePage ) << '}';
            }
            world << "]}";
            publish( destination, world.str().c_str(), true );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            std::error_code ignored;
            if( !temporary.empty() )
            {
                fs::remove( temporary, ignored );
            }
            for( const auto &file : created )
            {
                fs::remove( file, ignored );
            }
            return false;
        }
    }

    String FoliageResourceCompiler::name() const
    {
        return "WPGraphics.FoliageResourceCompiler";
    }

    Array<resource::CompilerOutput> FoliageResourceCompiler::outputs() const
    {
        return { { resource::ResourceTypeID( "folspec" ), 2 },
                 { resource::ResourceTypeID( "folpage" ), 2 },
                 { resource::ResourceTypeID( "folworld" ), 2 } };
    }

    bool FoliageResourceCompiler::getDependencies( const resource::CompileContext &context,
                                                   resource::DependencySet &out, String &error ) const
    {
        out = {};
        error.clear();
        try
        {
            for( const auto &id : dependencies( descriptor( context ) ) )
            {
                out.compileDependencies.push_back( resource::CompileDependency::resource( id ) );
                out.installDependencies.push_back( id );
            }
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    resource::CompilationStatus FoliageResourceCompiler::compile(
        const resource::CompileContext &context, std::ostream &out, Array<String> &messages ) const
    {
        try
        {
            Envelope e;
            e.target = context.target;
            e.packaged = context.packagedBuild;
            e.data = descriptor( context );
            check( !e.target.empty() && e.target.size() <= 128, "Invalid foliage compiler target" );
            std::map<ResourceID, RuntimeResource> resources;
            for( const auto &id : dependencies( e.data ) )
            {
                auto r = artifact( context, id );
                e.pins.push_back( pin( r ) );
                resources.emplace( id, std::move( r ) );
            }
            if( !e.data.speciesJson.empty() )
            {
                const auto species = readSpecies( e );
                for( const auto &p : e.pins )
                {
                    CookedTextureData texture;
                    String error;
                    checked( decodeCookedTexture( resources.at( p.id ), texture, error ), error );
                    check( texture.target == e.target && texture.packagedBuild == e.packaged,
                           "Foliage texture target mismatch" );
                    textureSemantics( *species, p.id, texture );
                }
            }
            else
            {
                FoliageWorldData definition;
                definition.pageSize = e.data.pageSize;
                for( const auto &id : e.data.species )
                {
                    definition.species.push_back(
                        readSpecies( read( resources.at( id ), e.target, e.packaged ) ) );
                }
                String error;
                checked( validateFoliageWorld( definition, error ), error );
                if( context.resourceId.type() == ResourceTypeID( "folpage" ) )
                {
                    checked( validateFoliagePage( e.data.page, definition, error ), error );
                }
                else
                {
                    std::set<u64> ids;
                    u64 total = 0;
                    for( auto &entry : e.data.pages )
                    {
                        auto page = read( resources.at( entry.id ), e.target, e.packaged );
                        check( page.data.pageSize == e.data.pageSize &&
                                   page.data.species == e.data.species &&
                                   page.data.page.cell == entry.cell,
                               "Foliage page differs from world directory" );
                        for( const auto &id : e.data.species )
                        {
                            const auto &a = findPin( e, id ), &b = findPin( page, id );
                            check( a.version == b.version && a.source == b.source && a.hash == b.hash &&
                                       a.bytes == b.bytes,
                                   "Foliage page pins stale species" );
                        }
                        checked( validateFoliagePage( page.data.page, definition, error ), error );
                        entry.count = u32( page.data.page.instances.size() );
                        total += entry.count;
                        check( total <= foliageMaximumInstances,
                               "Foliage world exceeds instance limit" );
                        f32 geometry = 0, bend = 0;
                        checked( foliageInstanceRadii( definition, page.data.page.instances, geometry,
                                                       bend, error ),
                                 error );
                        e.data.geometryRadius = std::max( e.data.geometryRadius, geometry );
                        e.data.bendRadius = std::max( e.data.bendRadius, bend );
                        for( const auto &v : page.data.page.instances )
                        {
                            check( ids.insert( v.id ).second,
                                   "Foliage world has duplicate instance IDs across pages" );
                        }
                    }
                }
            }
            write( out, e, context.resourceId.type() );
            check( out.good(), "Foliage payload write failed" );
            return resource::CompilationStatus::Success;
        }
        catch( const std::exception &e )
        {
            messages.push_back( e.what() );
            return resource::CompilationStatus::Failure;
        }
    }

    namespace
    {
        using Reserve = std::function<StreamBudget( StreamMemoryClass, u64, String & )>;

        struct ResourceOwner
        {
            StreamBudget budget;
            std::shared_ptr<const resource::RuntimeResource> resource;
        };

        std::shared_ptr<const resource::RuntimeResource> loadResource( resource::ResourceSystem &system,
                                                                       const resource::ResourceID &id,
                                                                       u64 limit, const Reserve &reserve,
                                                                       String &error )
        {
            // Container metadata is bounded by CompiledResourceIO. Reserve its worst
            // direct closure alongside the payload before opening a file.
            const auto metadata =
                id.type() == resource::ResourceTypeID( "folworld" ) ? 20ull << 20 : 1ull << 20;
            auto budget = reserve( StreamMemoryClass::SourceCpu, limit + metadata, error );
            if( !budget )
            {
                return {};
            }
            auto resource = system.loadPayload( id, error, limit );
            if( !resource )
            {
                return {};
            }
            auto owner = std::make_shared<ResourceOwner>(
                ResourceOwner{ std::move( budget ), std::move( resource ) } );
            return std::shared_ptr<const resource::RuntimeResource>( owner, owner->resource.get() );
        }

        struct SpeciesOwner
        {
            StreamBudget budget;
            std::shared_ptr<const FoliageSpecies> species;
        };

        struct WorldOwner
        {
            StreamBudget budget;
            FoliageSnapshot snapshot;
        };

        u64 textureBytes( const CookedTextureData &texture )
        {
            u64 bytes = texture.ddsBytes.capacity() +
                        texture.levels.capacity() * sizeof( TextureMipLevel ) + 4096;
            for( const auto &level : texture.levels )
            {
                bytes += level.bgra.capacity();
            }
            return bytes;
        }

        std::shared_ptr<const DecodedFoliageWorld> decodeWorld(
            std::shared_ptr<resource::ResourceSystem> system, const resource::ResourceID &id,
            const String &target, bool packaged, const FoliageDecodeOptions &options,
            const Reserve &reserve, const StreamCancellation &cancel, String &error )
        {
            try
            {
                check(
                    system && id.type() == resource::ResourceTypeID( "folworld" ) && !id.isSubResource(),
                    "Foliage decode requires a world resource and resource system" );
                check( options.maximumSourceBytes >= metadataLimit + ( 20ull << 20 ) &&
                           options.maximumResultBytes >= 4096 &&
                           options.maximumWorkingBytes >= 1ull << 20,
                       "Foliage decode budgets are too small" );
                auto root = loadResource( *system, id, metadataLimit, reserve, error );
                if( !root )
                {
                    return {};
                }
                auto envelope = read( *root, target, packaged );
                auto result = std::make_shared<DecodedFoliageWorld>();
                auto world = std::make_shared<FoliageWorldData>();
                world->pageSize = envelope.data.pageSize;
                world->revision = root->header.payloadHash;
                const auto directoryBytes =
                    4096 + root->payload.size() * 3 + envelope.data.pages.size() * 512;
                u64 residentBytes = directoryBytes;
                check( residentBytes <= options.maximumResultBytes,
                       "Foliage directory exceeds resident CPU budget" );
                u64 sourceBytes = root->header.payloadSize;
                for( const auto &speciesId : envelope.data.species )
                {
                    if( cancel.cancelled() )
                    {
                        error = "Foliage world decode cancelled";
                        return {};
                    }
                    const auto &p = findPin( envelope, speciesId );
                    check(
                        p.bytes <= speciesLimit && sourceBytes + p.bytes <= options.maximumSourceBytes,
                        "Foliage species exceeds source budget" );
                    check( p.bytes <= options.maximumWorkingBytes / 64,
                           "Foliage species JSON exceeds decode scratch budget" );
                    auto raw = loadResource( *system, p.id, p.bytes, reserve, error );
                    if( !raw )
                    {
                        return {};
                    }
                    matches( *raw, p );
                    sourceBytes += p.bytes;
                    const auto parsedBudget = 4096 + p.bytes * 8;
                    check( parsedBudget <= options.maximumResultBytes - residentBytes,
                           "Foliage shared species exceed resident CPU budget" );
                    auto budget = reserve( StreamMemoryClass::SourceCpu, parsedBudget, error );
                    if( !budget )
                    {
                        return {};
                    }
                    auto speciesEnvelope = read( *raw, target, packaged );
                    auto species = readSpecies( speciesEnvelope );
                    auto owner =
                        std::make_shared<SpeciesOwner>( SpeciesOwner{ std::move( budget ), species } );
                    world->species.emplace_back( owner, species.get() );
                    residentBytes += parsedBudget;
                    for( const auto &texturePin : speciesEnvelope.pins )
                    {
                        auto existing = result->textures.find( texturePin.id.str() );
                        if( existing == result->textures.end() )
                        {
                            check( texturePin.bytes <= graphicsTextureByteLimit + 4096 &&
                                       sourceBytes + texturePin.bytes <= options.maximumSourceBytes,
                                   "Foliage texture exceeds source budget" );
                            check( texturePin.bytes <= options.maximumWorkingBytes / 2,
                                   "Foliage texture exceeds decode scratch budget" );
                            auto textureResource =
                                loadResource( *system, texturePin.id, texturePin.bytes, reserve, error );
                            if( !textureResource )
                            {
                                return {};
                            }
                            matches( *textureResource, texturePin );
                            const auto textureResident = texturePin.bytes + 16384;
                            check( textureResident <= options.maximumResultBytes - residentBytes,
                                   "Foliage textures exceed resident CPU budget" );
                            auto textureBudget =
                                reserve( StreamMemoryClass::SourceCpu, textureResident, error );
                            if( !textureBudget )
                            {
                                return {};
                            }
                            CookedTextureData texture;
                            checked( decodeCookedTexture( *textureResource, texture, error ), error );
                            check( texture.target == target && texture.packagedBuild == packaged &&
                                       textureBytes( texture ) <= textureResident,
                                   "Invalid foliage texture target or resident size" );
                            result->retainedBudgets.push_back( std::move( textureBudget ) );
                            residentBytes += textureResident;
                            sourceBytes += texturePin.bytes;
                            result->retainedResources.push_back( textureResource );
                            existing =
                                result->textures.emplace( texturePin.id.str(), std::move( texture ) )
                                    .first;
                        }
                        else
                        {
                            auto rawTexture = std::find_if(
                                result->retainedResources.begin(), result->retainedResources.end(),
                                [&]( const auto &r ) { return r->header.resourceId == texturePin.id; } );
                            check( rawTexture != result->retainedResources.end(),
                                   "Missing shared foliage texture generation" );
                            matches( **rawTexture, texturePin );
                        }
                        textureSemantics( *species, texturePin.id, existing->second );
                    }
                }
                checked( validateFoliageWorld( *world, error ), error );
                auto worldBudget = reserve( StreamMemoryClass::SourceCpu, directoryBytes, error );
                if( !worldBudget )
                {
                    return {};
                }
                auto owner =
                    std::make_shared<WorldOwner>( WorldOwner{ std::move( worldBudget ), world } );
                result->source.snapshot = FoliageSnapshot( owner, world.get() );
                result->source.maximumGeometryRadius = envelope.data.geometryRadius;
                result->source.maximumBendRadius = envelope.data.bendRadius;
                std::map<FoliageCellKey, Entry> directory;
                for( const auto &entry : envelope.data.pages )
                {
                    directory.emplace( entry.cell, entry );
                    result->source.cells.push_back( entry.cell );
                    result->source.pageInstanceCounts.push_back( entry.count );
                }
                // Only directory/species are retained here. Each demand opens exactly
                // one pinned page container, validates it, then retires its raw bytes.
                result->source.load = [system, definition = result->source.snapshot,
                                       envelope = std::move( envelope ),
                                       directory = std::move( directory ), target, packaged, reserve](
                                          FoliageCellKey key, const StreamCancellation &cancel,
                                          String &error ) -> std::shared_ptr<const FoliagePageData> {
                    try
                    {
                        auto found = directory.find( key );
                        check( found != directory.end(), "Foliage cell is outside the cooked world" );
                        if( cancel.cancelled() )
                        {
                            return {};
                        }
                        const auto &p = findPin( envelope, found->second.id );
                        check( p.bytes <= pageLimit, "Cooked foliage page exceeds byte limit" );
                        auto raw = loadResource( *system, p.id, p.bytes, reserve, error );
                        if( !raw )
                        {
                            return {};
                        }
                        matches( *raw, p );
                        auto page = read( *raw, target, packaged );
                        check( page.data.page.cell == key &&
                                   page.data.pageSize == definition->pageSize &&
                                   page.data.species == envelope.data.species &&
                                   page.data.page.instances.size() == found->second.count,
                               "Cooked foliage page differs from directory" );
                        for( const auto &id : envelope.data.species )
                        {
                            const auto &a = findPin( envelope, id ), &b = findPin( page, id );
                            check( a.version == b.version && a.source == b.source && a.hash == b.hash &&
                                       a.bytes == b.bytes,
                                   "Cooked foliage page pins stale species" );
                        }
                        checked( validateFoliagePage( page.data.page, *definition, error ), error );
                        if( cancel.cancelled() )
                        {
                            return {};
                        }
                        page.data.page.revision = raw->header.payloadHash;
                        error.clear();
                        return std::make_shared<const FoliagePageData>( std::move( page.data.page ) );
                    }
                    catch( const std::exception &e )
                    {
                        error = e.what();
                        return {};
                    }
                };
                result->resultBytes = residentBytes;
                error.clear();
                return result;
            }
            catch( const std::exception &e )
            {
                error = e.what();
                return {};
            }
        }

        struct TextureBudget
        {
            StreamBudget gpu, cpu;
            TextureBudget( StreamBudget g, StreamBudget c ) :
                gpu( std::move( g ) ),
                cpu( std::move( c ) ) {};
        };

        class BudgetedFoliageTexture final : private TextureBudget, public ClawTexture
        {
        public:
            BudgetedFoliageTexture( StreamBudget gpu, StreamBudget cpu ) :
                TextureBudget( std::move( gpu ), std::move( cpu ) ) {};
        };
    }  // namespace

    std::shared_ptr<const DecodedFoliageWorld> decodeCookedFoliageWorld(
        std::shared_ptr<resource::ResourceSystem> system, const resource::ResourceID &id,
        const String &target, bool packaged, String &error, const FoliageDecodeOptions &options )
    {
        auto scheduler = WorldStreamingScheduler::shared();
        auto reserve = [scheduler]( auto kind, u64 bytes, String &error ) {
            return scheduler->reserve( kind, bytes, error );
        };
        auto scratch = reserve( StreamMemoryClass::WorkingCpu, options.maximumWorkingBytes, error );
        if( !scratch )
        {
            return {};
        }
        return decodeWorld( std::move( system ), id, target, packaged, options, reserve,
                            StreamCancellation( {} ), error );
    }

    bool queueCookedFoliageWorld( WorldStreamProducer &producer,
                                  std::shared_ptr<resource::ResourceSystem> system,
                                  const resource::ResourceID &id, const String &target, bool packaged,
                                  const FoliageDecodeOptions &options, String &error )
    {
        error.clear();
        if( !system || !id.isValid() || id.type() != resource::ResourceTypeID( "folworld" ) ||
            !options.maximumWorkingBytes )
        {
            error = "Invalid cooked foliage decode request";
            return false;
        }
        auto reserve = producer.budgetAllocator();
        WorldStreamRequest request;
        request.key = options.key;
        request.priority = options.priority;
        request.workingBytes = options.maximumWorkingBytes;
        request.maximumResultBytes = 4096;
        request.execute = [system = std::move( system ), id, target, packaged, options,
                           reserve]( const StreamCancellation &cancel ) {
            WorldStreamResult result;
            if( cancel.cancelled() )
            {
                return result;
            }
            auto decoded =
                decodeWorld( system, id, target, packaged, options, reserve, cancel, result.error );
            if( cancel.cancelled() )
            {
                return WorldStreamResult{};
            }
            if( decoded )
            {
                result.payload = std::move( decoded );
                result.bytes = 4096;
            }
            return result;
        };
        return producer.replace( { request }, error );
    }

    std::shared_ptr<const CookedFoliageCandidate> stageCookedFoliageWorld(
        ClawRendererDX11 &renderer, std::shared_ptr<const DecodedFoliageWorld> decoded, String &error )
    {
        error.clear();
        try
        {
            check( decoded && decoded->source.snapshot && decoded->source.load,
                   "Cooked foliage CPU preparation is missing" );
            checked( validateFoliageWorld( *decoded->source.snapshot, error ), error );
            auto native =
                renderer.isLoaded() ? wp_renderer_get_dx11( renderer.getNativeRenderer() ) : nullptr;
            check( native != nullptr, "Cooked foliage staging requires a loaded DX11 renderer" );
            u64 gpuBytes = 0, cpuBytes = 4096;
            for( const auto &entry : decoded->textures )
            {
                checked( validateCookedTextureData( entry.second, error ), error );
                cpuBytes += textureBytes( entry.second ) + entry.first.capacity() +
                            sizeof( BudgetedFoliageTexture ) + 512;
                gpuBytes += entry.second.ddsBytes.size();
                for( const auto &level : entry.second.levels )
                {
                    gpuBytes += level.bgra.size();
                }
            }
            auto scheduler = WorldStreamingScheduler::shared();
            auto gpu = scheduler->reserve( StreamMemoryClass::Gpu, gpuBytes, error );
            if( !gpu )
            {
                return {};
            }
            auto cpu = scheduler->reserve( StreamMemoryClass::SourceCpu, cpuBytes, error );
            if( !cpu )
            {
                return {};
            }
            auto maps = std::make_shared<std::map<String, SmartPtr<ITexture>>>();
            for( const auto &entry : decoded->textures )
            {
                auto texture = make_ptr<BudgetedFoliageTexture>( gpu, cpu );
                const auto ok = entry.second.ddsBytes.empty()
                                    ? texture->uploadCookedMips(
                                          entry.second.levels, entry.second.mipSettings, native, error )
                                    : uploadCookedDds( texture.get(), entry.second, native, error );
                if( !ok )
                {
                    return {};
                }
                maps->emplace( entry.first, texture );
            }
            auto result = std::make_shared<CookedFoliageCandidate>();
            result->source = decoded->source;
            result->resolver = [maps]( const String &id, String &error ) -> SmartPtr<ITexture> {
                auto found = maps->find( id );
                if( found == maps->end() )
                {
                    error = "Texture is outside cooked foliage closure: " + id;
                    return {};
                }
                error.clear();
                return found->second;
            };
            return result;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return {};
        }
    }
}  // namespace workphone::render
