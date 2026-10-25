#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    struct CityRoadRuntimeState : SourceBudgetObject
    {
        CityRoadTerrainMesh geometry;
        SourceBudgetVector<SmartPtr<IGameActor>> actors;
        SourceBudgetVector<SmartPtr<IMeshResource>> meshes;
        SourceBudgetVector<SmartPtr<render::IMaterial>> materials;
        SourceBudgetVector<SmartPtr<render::ITexture>> textures;
        SourceBudgetVector<SmartPtr<physics::IMeshShape>> shapes;
        SourceBudgetVector<SmartPtr<physics::IRigidStatic3>> bodies;
        SmartPtr<IGameActor> root;
        ~CityRoadRuntimeState()
        {
            auto app = core::IApplicationManager::instancePtr();
            if( !app ) return;
            const auto retire = []( auto operation ) {
                try { operation(); } catch( const std::exception &e ) { WP_LOG_EXCEPTION( e ); }
            };
            if( auto physics = app->getPhysicsManager() )
            {
                for( const auto &body : bodies )
                {
                    retire( [&] {
                        if( auto scene = body->getScene() ) scene->removeActor( body );
                        physics->removePhysicsBody( body );
                    } );
                }
                for( const auto &shape : shapes ) retire( [&] { physics->removeCollisionShape( shape ); } );
            }
            if( auto manager = app->getGameManager() )
                for( auto i = actors.rbegin(); i != actors.rend(); ++i ) retire( [&] { manager->destroyActor( *i ); } );
            if( auto manager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) )
                for( const auto &mesh : meshes ) retire( [&] { manager->removeMeshResource( mesh ); } );
            if( auto graphics = app->getGraphicsSystem() )
            {
                if( auto manager = graphics->getMaterialManager() )
                    for( const auto &material : materials ) retire( [&] { manager->destroyResource( material ); } );
                if( auto manager = graphics->getTextureManager() )
                    for( const auto &texture : textures ) retire( [&] { manager->destroyResource( texture ); } );
            }
        }
    };

    namespace
    {
        void requireRuntime( bool value, const char *message )
        { if( !value ) throw std::runtime_error( message ); }

        SmartPtr<render::ITexture> surfaceTexture( CityRoadRuntimeState &state, const String &prefix,
            const procedural::TextureBuffer &buffer, render::TextureMipFilter filter )
        {
            requireRuntime( buffer.width == 256 && buffer.height == 256 && buffer.pixels.size() == 256 * 256 * 4,
                "Authored road surfaces require complete bounded 256px textures." );
            requireRuntime( state.textures.size() < 16 && state.textures.capacity() >= 16,
                "Authored road texture ownership must be admitted before publication." );
            // Use the same transactional staging path as the procedural scene:
            // SourceCpu owns this BGRA snapshot; the backend owns upload/GPU admission.
            auto bgra = makeSourceBudgetSnapshot<u8>();
            bgra.resize( buffer.pixels.size() );
            for( size_t pixel = 0; pixel < buffer.pixels.size(); pixel += 4 )
            {
                bgra[pixel] = buffer.pixels[pixel + 2]; bgra[pixel + 1] = buffer.pixels[pixel + 1];
                bgra[pixel + 2] = buffer.pixels[pixel]; bgra[pixel + 3] = buffer.pixels[pixel + 3];
            }
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            auto manager = graphics ? graphics->getTextureManager() : nullptr;
            requireRuntime( bool( manager ), "Road texture manager is unavailable." );
            const auto name = prefix + "/surface/" + StringUtil::toString( state.textures.size() );
            auto texture = dynamic_pointer_cast<render::ITexture>( manager->create( name ) );
            requireRuntime( bool( texture ), "Road texture allocation failed." );
            // The owner vector is preallocated. Any following stage failure retires
            // this manager-owned resource with the unpublished candidate generation.
            state.textures.push_back( texture );
            auto properties = texture->getProperties();
            requireRuntime( bool( properties ), "Road texture properties are unavailable." );
            properties->setProperty( "mipmapFilter", static_cast<s32>( filter ) );
            properties->setProperty( "mipmapAtlasColumns", 1u ); texture->setProperties( properties );
            String error;
            requireRuntime( texture->copyByteData( bgra, { 256, 256 }, error ),
                error.empty() ? "Road texture staging failed." : error.c_str() );
            texture->load( nullptr );
            return texture;
        }

        void surfaceMaps( CityRoadRuntimeState &state, const String &prefix,
            SmartPtr<render::IMaterial> material, const procedural::SurfaceBakeResult &surface )
        {
            material->setDiffuse( ColourF::White );
            material->setTexture( surfaceTexture( state, prefix, surface.albedo, render::TextureMipFilter::Colour ), 0 );
            material->setTexture( surfaceTexture( state, prefix, surface.normal, render::TextureMipFilter::Normal ), 1 );
            for( unsigned channel = 0; channel < 3; ++channel )
            {
                procedural::TextureBuffer split( surface.orm.width, surface.orm.height );
                for( size_t pixel = 0; pixel < surface.orm.pixels.size(); pixel += 4 )
                {
                    split.pixels[pixel] = split.pixels[pixel + 1] = split.pixels[pixel + 2] = surface.orm.pixels[pixel + channel];
                    split.pixels[pixel + 3] = 255;
                }
                // Existing material slots read scalar red: AO22, roughness3, metal2.
                material->setTexture( surfaceTexture( state, prefix, split,
                    channel == 1 ? render::TextureMipFilter::Roughness : render::TextureMipFilter::Data ),
                    channel == 0 ? 22 : channel == 1 ? 3 : 2 );
            }
            material->setRoughness( 1 ); material->setMetalness( 1 );
        }

        void roadSurface( CityRoadRuntimeState &state, const String &prefix,
            SmartPtr<render::IMaterial> material, SmartPtr<procedural::ITextureForge> forge, u32 seed,
            CityRoadTerrainTriangleKind kind )
        {
            using namespace procedural;
            const bool asphalt = kind == CityRoadTerrainTriangleKind::Road;
            const bool turf = kind == CityRoadTerrainTriangleKind::Island;
            const bool paint = kind == CityRoadTerrainTriangleKind::Marking;
            forge->setSeed( seed ^ ( u32( kind ) * 0x9e3779b9u ) );
            SurfaceBakeParams params; params.size = 256;
            auto surface = forge->bakeSurface( asphalt ? SurfaceTag::Asphalt : turf ? SurfaceTag::Dirt :
                paint ? SurfaceTag::Paint : SurfaceTag::Concrete, params );
            const auto complete = []( const TextureBuffer &buffer ) {
                return buffer.width == 256 && buffer.height == 256 && buffer.pixels.size() == 256 * 256 * 4;
            };
            requireRuntime( complete( surface.albedo ) && complete( surface.normal ) && complete( surface.orm ) &&
                surface.height.width == 256 && surface.height.height == 256 && surface.height.values.size() == 256 * 256,
                "Road texture forge returned incomplete bounded surface maps." );
            const auto noise = [seed]( u32 x, u32 y, u32 period ) {
                u32 value = ( x % period ) * 0x9e3779b9u ^ ( y % period ) * 0x85ebca6bu ^ seed;
                value ^= value >> 16; value *= 0x7feb352du; value ^= value >> 15; value *= 0x846ca68bu;
                return float( ( value ^ ( value >> 16 ) ) >> 8 ) / 16777215.f;
            };
            const auto smoothNoise = [&]( u32 x, u32 y, u32 cells ) {
                const float u = float( x ) * cells / params.size, v = float( y ) * cells / params.size;
                const u32 ix = u32( u ), iy = u32( v );
                const float fx = u - ix, fy = v - iy;
                const float sx = fx * fx * ( 3 - 2 * fx ), sy = fy * fy * ( 3 - 2 * fy );
                const float a = noise( ix, iy, cells ) * ( 1 - sx ) + noise( ix + 1, iy, cells ) * sx;
                const float b = noise( ix, iy + 1, cells ) * ( 1 - sx ) + noise( ix + 1, iy + 1, cells ) * sx;
                return a * ( 1 - sy ) + b * sy;
            };
            const auto srgb = []( float linear ) {
                linear = std::clamp( linear, 0.f, 1.f );
                return u8( std::lround( ( linear <= .0031308f ? linear * 12.92f :
                    1.055f * std::pow( linear, 1.f / 2.4f ) - .055f ) * 255 ) );
            };
            for( u32 y = 0; y < params.size; ++y ) for( u32 x = 0; x < params.size; ++x )
            {
                const float grain = noise( x, y, params.size ), patch = smoothNoise( x, y, 3 );
                const float aggregate = smoothNoise( x, y, 32 );
                auto pixel = surface.albedo.pixel( x, y ), orm = surface.orm.pixel( x, y );
                auto &height = surface.height.values[size_t( y ) * params.size + x];
                requireRuntime( std::isfinite( height ), "Road texture forge returned nonfinite surface height." );
                if( asphalt )
                {
                    // Reuse the city's isotropic aggregate recipe; directional
                    // generic asphalt ruts read as corrugations at an 8m tile.
                    const float linear = .064f + ( patch - .5f ) * .016f +
                        ( aggregate - .5f ) * .009f + ( grain - .5f ) * .006f;
                    pixel[0] = pixel[1] = srgb( linear ); pixel[2] = srgb( linear * 1.02f );
                    height = .5f + ( grain - .5f ) * .012f;
                    orm[0] = 255; orm[1] = u8( 224 + aggregate * 16 );
                }
                else if( turf )
                {
                    const float variation = .85f + patch * .2f + aggregate * .1f;
                    pixel[0] = srgb( .052f * variation ); pixel[1] = srgb( .082f * variation ); pixel[2] = srgb( .022f * variation );
                    height = .5f + ( height - .5f ) * .05f;
                    orm[0] = u8( 245 + patch * 10 ); orm[1] = u8( 240 + aggregate * 15 );
                }
                else if( paint )
                {
                    const float linear = .72f + ( grain - .5f ) * .035f + ( patch - .5f ) * .02f;
                    pixel[0] = srgb( linear ); pixel[1] = srgb( linear * .99f ); pixel[2] = srgb( linear * .95f );
                }
                else
                {
                    const float linear = .30f + ( height - .5f ) * .045f + ( grain - .5f ) * .012f;
                    pixel[0] = srgb( linear ); pixel[1] = srgb( linear * .99f ); pixel[2] = srgb( linear * .97f );
                    height = .5f + ( height - .5f ) * .03f;
                    orm[0] = u8( 246 + patch * 9 ); orm[1] = u8( 214 + aggregate * 16 );
                }
                pixel[3] = 255; orm[2] = 0; orm[3] = 255;
            }
            if( paint )
            {
                material->setDiffuse( ColourF::White );
                material->setTexture( surfaceTexture( state, prefix, surface.albedo, render::TextureMipFilter::Colour ), 0 );
                material->setRoughness( .78f ); material->setMetalness( 0 );
            }
            else
            {
                forge->heightToNormal( surface.height, surface.normal, asphalt ? .25f : .6f );
                surfaceMaps( state, prefix, material, surface );
                material->setNormalStrength( asphalt ? .2f : turf ? .3f : .22f );
            }
            material->setUVProjection( kind == CityRoadTerrainTriangleKind::Kerb ? 1 : 3 );
            material->setTriplanarScale( asphalt ? 1.f / 8.f : turf ? .25f : .5f );
            material->setEditorUInt( "uvFilter", 3 ); material->setEditorFloat( "uvAniso", 8.f );
        }

        SmartPtr<IMeshResource> runtimeMesh( CityRoadRuntimeState &state, const String &path,
            const CityRoadTerrainMesh &geometry, CityRoadTerrainTriangleKind kind )
        {
            Array<Vector3F> positions{ Allocator<Vector3F>( sourceBudget ) };
            Array<Vector3F> normals{ Allocator<Vector3F>( sourceBudget ) };
            Array<Vector2F> uv{ Allocator<Vector2F>( sourceBudget ) };
            Array<u32> indices{ Allocator<u32>( sourceBudget ) };
            SourceBudgetVector<u32> remap( geometry.vertices.size(), std::numeric_limits<u32>::max() );
            for( size_t triangle = 0; triangle < geometry.triangleKinds.size(); ++triangle )
            {
                if( geometry.triangleKinds[triangle] != kind ) continue;
                for( size_t corner = 0; corner < 3; ++corner )
                {
                    const auto source = geometry.indices[triangle * 3 + corner];
                    auto &destination = remap[source];
                    if( destination == std::numeric_limits<u32>::max() )
                    {
                        const auto &vertex = geometry.vertices[source];
                        destination = u32( positions.size() );
                        positions.push_back( vertex.position ); normals.push_back( vertex.normal ); uv.push_back( vertex.uv );
                    }
                    indices.push_back( destination );
                }
            }
            if( indices.empty() ) return nullptr;
            auto mesh = MeshUtil::createMesh( positions, normals, uv, indices );
            requireRuntime( bool( mesh ), "Road mesh construction failed." );
            mesh->updateAABB( true );
            auto app = core::IApplicationManager::instance();
            auto manager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            requireRuntime( bool( manager ), "Road mesh manager is unavailable." );
            auto resource = make_ptr<MeshResource>();
            resource->setName( path ); resource->setFilePath( path ); resource->setMesh( mesh );
            resource->setLoadingState( LoadingState::Loaded );
            // Inventory capacity is admitted before any manager publication.
            state.meshes.push_back( resource );
            manager->addMeshResource( resource );
            return resource;
        }

        void roadMarkings( CityRoadTerrainMesh &mesh, const procedural::CityRoadSplineSource &source,
            const procedural::CityRoadSplineBake &bake )
        {
            // Index only the already fitted carriageway. Painting its exact
            // triangles preserves cell ridges, Fixed profiles and road boundaries.
            // It also avoids repeating terrain queries with different coverage.
            struct CellTriangle { s32 x, z; u32 triangle; };
            constexpr double cellSize = 8;
            constexpr u64 maximumEntries = 2000000, maximumProjectionVisits = 16000000;
            SourceBudgetVector<CellTriangle> cells;
            const auto bounds = [&]( size_t triangle, s32 &x0, s32 &z0, s32 &x1, s32 &z1 ) {
                const auto a = mesh.vertices[mesh.indices[triangle * 3]].position;
                const auto b = mesh.vertices[mesh.indices[triangle * 3 + 1]].position;
                const auto c = mesh.vertices[mesh.indices[triangle * 3 + 2]].position;
                x0 = s32( std::floor( std::min( { double( a.x ), double( b.x ), double( c.x ) } ) / cellSize ) );
                x1 = s32( std::floor( std::max( { double( a.x ), double( b.x ), double( c.x ) } ) / cellSize ) );
                z0 = s32( std::floor( std::min( { double( a.z ), double( b.z ), double( c.z ) } ) / cellSize ) );
                z1 = s32( std::floor( std::max( { double( a.z ), double( b.z ), double( c.z ) } ) / cellSize ) );
            };
            const size_t roadTriangleCount = mesh.triangleKinds.size();
            u64 entries = 0;
            for( size_t i = 0; i < roadTriangleCount; ++i ) if( mesh.triangleKinds[i] == CityRoadTerrainTriangleKind::Road )
            {
                s32 x0, z0, x1, z1; bounds( i, x0, z0, x1, z1 );
                entries += u64( s64( x1 ) - x0 + 1 ) * u64( s64( z1 ) - z0 + 1 );
                requireRuntime( entries <= maximumEntries, "Road marking spatial-index budget exceeded." );
            }
            cells.reserve( size_t( entries ) );
            for( size_t i = 0; i < roadTriangleCount; ++i ) if( mesh.triangleKinds[i] == CityRoadTerrainTriangleKind::Road )
            {
                s32 x0, z0, x1, z1; bounds( i, x0, z0, x1, z1 );
                for( s32 x = x0; x <= x1; ++x ) for( s32 z = z0; z <= z1; ++z ) cells.push_back( { x, z, u32( i ) } );
            }
            std::sort( cells.begin(), cells.end(), []( const CellTriangle &a, const CellTriangle &b ) {
                if( a.x != b.x ) return a.x < b.x;
                if( a.z != b.z ) return a.z < b.z;
                return a.triangle < b.triangle;
            } );
            SourceBudgetVector<u32> visited( roadTriangleCount, 0 );
            u32 stamp = 0;
            u64 projectionVisits = 0;
            struct PaintPoint { double x, z, u, v; };
            using PaintPolygon = std::array<PaintPoint, 10>;
            const auto area = []( PaintPoint a, PaintPoint b, PaintPoint c ) {
                return ( b.x - a.x ) * ( c.z - a.z ) - ( b.z - a.z ) * ( c.x - a.x );
            };
            const auto clip = [&]( const PaintPolygon &input, size_t count, PaintPoint a, PaintPoint b, PaintPolygon &output ) {
                size_t size = 0;
                if( !count ) return size;
                auto previous = input[count - 1];
                double previousDistance = area( a, b, previous );
                for( size_t i = 0; i < count; ++i )
                {
                    const auto current = input[i];
                    const double distance = area( a, b, current );
                    const bool inside = distance >= 0, previousInside = previousDistance >= 0;
                    if( inside != previousInside )
                    {
                        const double t = std::clamp( previousDistance / ( previousDistance - distance ), 0.0, 1.0 );
                        requireRuntime( size < output.size(), "Road marking clipping polygon budget exceeded." );
                        output[size++] = { previous.x + ( current.x - previous.x ) * t,
                            previous.z + ( current.z - previous.z ) * t,
                            previous.u + ( current.u - previous.u ) * t, previous.v + ( current.v - previous.v ) * t };
                    }
                    if( inside )
                    {
                        requireRuntime( size < output.size(), "Road marking clipping polygon budget exceeded." );
                        output[size++] = current;
                    }
                    previous = current; previousDistance = distance;
                }
                return size;
            };
            size_t quads = 0;
            const auto triangle = [&]( Vector3F a, Vector3F b, Vector3F c ) {
                for( const auto point : { a, b, c } )
                    requireRuntime( std::isfinite( point.x ) && std::isfinite( point.z ) &&
                        std::abs( point.x ) <= 10000000 && std::abs( point.z ) <= 10000000,
                        "Road marking coordinates exceed the supported projection range." );
                PaintPolygon original{}, scratch{}, clipped{};
                original[0] = { a.x, a.z, 0, 0 }; original[1] = { b.x, b.z, 1, 0 }; original[2] = { c.x, c.z, 1, 1 };
                if( std::abs( area( original[0], original[1], original[2] ) ) <= 1e-10 ) return;
                const auto x0 = s32( std::floor( std::min( { a.x, b.x, c.x } ) / cellSize ) );
                const auto x1 = s32( std::floor( std::max( { a.x, b.x, c.x } ) / cellSize ) );
                const auto z0 = s32( std::floor( std::min( { a.z, b.z, c.z } ) / cellSize ) );
                const auto z1 = s32( std::floor( std::max( { a.z, b.z, c.z } ) / cellSize ) );
                requireRuntime( u64( s64( x1 ) - x0 + 1 ) * u64( s64( z1 ) - z0 + 1 ) <= maximumProjectionVisits - projectionVisits,
                    "Road marking projection work budget exceeded." );
                ++stamp;
                for( s32 x = x0; x <= x1; ++x ) for( s32 z = z0; z <= z1; ++z )
                {
                    requireRuntime( ++projectionVisits <= maximumProjectionVisits, "Road marking projection work budget exceeded." );
                    auto cell = std::lower_bound( cells.begin(), cells.end(), std::pair<s32, s32>( x, z ),
                        []( const CellTriangle &entry, const std::pair<s32, s32> &key ) {
                            return entry.x < key.first || ( entry.x == key.first && entry.z < key.second );
                        } );
                    for( ; cell != cells.end() && cell->x == x && cell->z == z; ++cell )
                    {
                        requireRuntime( ++projectionVisits <= maximumProjectionVisits, "Road marking projection work budget exceeded." );
                        const u32 road = cell->triangle;
                        if( visited[road] == stamp ) continue;
                        visited[road] = stamp;
                        auto ra = mesh.vertices[mesh.indices[road * 3]].position;
                        auto rb = mesh.vertices[mesh.indices[road * 3 + 1]].position;
                        auto rc = mesh.vertices[mesh.indices[road * 3 + 2]].position;
                        PaintPoint pa{ ra.x, ra.z, 0, 0 }, pb{ rb.x, rb.z, 0, 0 }, pc{ rc.x, rc.z, 0, 0 };
                        if( area( pa, pb, pc ) < 0 ) { std::swap( pb, pc ); std::swap( rb, rc ); }
                        const double denominator = area( pa, pb, pc );
                        if( denominator <= 1e-10 ) continue;
                        size_t count = clip( original, 3, pa, pb, scratch );
                        count = clip( scratch, count, pb, pc, clipped );
                        count = clip( clipped, count, pc, pa, scratch );
                        auto normal = ( rb - ra ).crossProduct( rc - ra );
                        if( normal.y < 0 ) normal = -normal;
                        normal.normalise();
                        const auto fitted = [&]( const PaintPoint &point ) {
                            const double wa = area( pb, pc, point ) / denominator;
                            const double wb = area( pc, pa, point ) / denominator;
                            return CityRoadTerrainVertex{ { float( point.x ),
                                float( wa * ra.y + wb * rb.y + ( 1 - wa - wb ) * rc.y + .005 ), float( point.z ) },
                                normal, { float( point.u ), float( point.v ) } };
                        };
                        for( size_t i = 1; i + 1 < count; ++i )
                        {
                            if( std::abs( area( scratch[0], scratch[i], scratch[i + 1] ) ) <= 1e-10 ) continue;
                            requireRuntime( mesh.vertices.size() + 3 <= 1000000, "Road marking vertex budget exceeded." );
                            auto aa = fitted( scratch[0] ), bb = fitted( scratch[i] ), cc = fitted( scratch[i + 1] );
                            if( ( bb.position - aa.position ).crossProduct( cc.position - aa.position ).y < 0 ) std::swap( bb, cc );
                            const auto base = u32( mesh.vertices.size() );
                            mesh.vertices.push_back( aa ); mesh.vertices.push_back( bb ); mesh.vertices.push_back( cc );
                            mesh.indices.push_back( base ); mesh.indices.push_back( base + 1 ); mesh.indices.push_back( base + 2 );
                            mesh.triangleKinds.push_back( CityRoadTerrainTriangleKind::Marking );
                        }
                    }
                }
            };
            const auto strip = [&]( Vector3F a, Vector3F b, Vector3F across, float width ) {
                requireRuntime( ++quads <= 32768, "Road marking work budget exceeded." );
                const auto half = across * ( width * .5f );
                Vector3F points[] = { a - half, b - half, b + half, a + half };
                triangle( points[0], points[1], points[2] ); triangle( points[0], points[2], points[3] );
            };
            for( size_t road = 0; road < source.roads.size(); ++road )
            {
                const auto &recipe = source.roads[road]; const auto &points = bake.roads[road].points;
                for( size_t i = 1; i < points.size(); ++i )
                {
                    const auto &a = points[i - 1], &b = points[i];
                    auto direction = b.position - a.position; direction.y = 0;
                    const float runSquared = direction.lengthSquared();
                    if( runSquared < 1e-8f ) continue;
                    direction.normalise(); const Vector3F across( direction.z, 0, -direction.x );
                    for( int side : { -1, 1 } )
                    {
                        const auto offset = across * ( side * ( recipe.width * .5f - .2f ) );
                        strip( a.position + offset, b.position + offset, across, .12f );
                    }
                    const float length = b.distance - a.distance;
                    for( float dash = std::floor( a.distance / 6 ) * 6; dash < b.distance; dash += 6 )
                    {
                        const float start = std::max( dash, a.distance ), end = std::min( dash + 3, b.distance );
                        if( end - start < .01f ) continue;
                        const auto p = a.position + ( b.position - a.position ) * ( ( start - a.distance ) / length );
                        const auto q = a.position + ( b.position - a.position ) * ( ( end - a.distance ) / length );
                        strip( p, q, across, .12f );
                    }
                }
            }
            constexpr float pi = 3.14159265358979323846f;
            for( const auto &junction : source.roundabouts )
            {
                for( float radius : { junction.islandRadius + .45f, junction.outerRadius - .25f } )
                    for( u32 sample = 0; sample < 128; ++sample )
                    {
                        const float angle = ( sample + .5f ) * 2 * pi / 128;
                        bool mouth = false;
                        if( radius > junction.outerRadius - .5f )
                            for( const auto &port : junction.ports )
                            {
                                auto outward = port.outward; outward.normalise();
                                const Vector3F point( std::cos( angle ) * radius, 0, std::sin( angle ) * radius );
                                const float along = point.dotProduct( outward );
                                const float side = std::abs( point.x * outward.z - point.z * outward.x );
                                mouth = mouth || ( along > 0 && side < port.width * .5f + .15f );
                            }
                        if( mouth ) continue;
                        const float a = sample * 2 * pi / 128, b = ( sample + 1 ) * 2 * pi / 128;
                        const auto p = junction.center + Vector3F( std::cos( a ) * radius, 0, std::sin( a ) * radius );
                        const auto q = junction.center + Vector3F( std::cos( b ) * radius, 0, std::sin( b ) * radius );
                        strip( p, q, { std::cos( angle ), 0, std::sin( angle ) }, .12f );
                    }
                for( const auto &port : junction.ports )
                {
                    auto outward = port.outward; outward.normalise();
                    const Vector3F across( outward.z, 0, -outward.x );
                    for( int side : { -1, 1 } )
                    {
                        const auto offset = across * ( side * ( port.width * .5f - .2f ) );
                        strip( junction.center + outward * junction.outerRadius + offset,
                            junction.center + outward * ( junction.outerRadius + port.length ) + offset,
                            across, .12f );
                    }
                    const float incoming = junction.clockwise ? 1.f : -1.f;
                    for( float row : { 1.1f, 1.45f } ) for( int dash = -1; dash <= 1; ++dash )
                    {
                        const auto center = junction.center + outward * ( junction.outerRadius + row ) +
                            across * ( incoming * port.width * .25f + dash * .68f );
                        strip( center - across * .22f, center + across * .22f, outward, .16f );
                    }
                }
                const float radius = ( junction.outerRadius + junction.islandRadius ) * .5f;
                for( int arrow = 0; arrow < 4; ++arrow )
                {
                    const float angle = ( arrow + .5f ) * pi * .5f;
                    const Vector3F radial( std::cos( angle ), 0, std::sin( angle ) );
                    const auto forward = Vector3F( -radial.z, 0, radial.x ) * ( junction.clockwise ? 1.f : -1.f );
                    const auto center = junction.center + radial * radius;
                    Vector3F tip = center + forward * 1.1f, left = center - forward * .8f + radial * .4f,
                             right = center - forward * .8f - radial * .4f;
                    triangle( tip, left, right );
                }
            }
        }
    }

    bool CityRoadAuthoring::publishRuntime( const procedural::CityRoadSplineSource &source,
        const procedural::CityRoadSplineBake &bake, render::TerrainSnapshot terrain,
        const Transform3<real_Num> &placement, String &error )
    {
        try
        {
            const auto task = Thread::getCurrentTask();
            requireRuntime( task != TaskId::Render && task != TaskId::Physics,
                "Road publication must run on the Application task." );
            auto app = core::IApplicationManager::instancePtr();
            requireRuntime( bool( app ) && bool( getActor() ), "Attach road authoring to a live scene actor." );
            const auto pose = getActor()->getTransform()->getWorldTransform();
            requireRuntime( pose.getPosition().lengthSquared() < 1e-10 &&
                ( pose.getScale() - Vector3<real_Num>( 1, 1, 1 ) ).lengthSquared() < 1e-10 &&
                std::abs( pose.getOrientation().W() ) > .999999,
                "Road controls use world metres. Reset the authoring actor transform before publishing." );
            if( source.roads.empty() && source.roundabouts.empty() )
            { clearRuntime(); error.clear(); return true; }
            requireRuntime( bool( terrain ), "A retained terrain snapshot is required for native road publication." );
            auto candidate = std::shared_ptr<CityRoadRuntimeState>( new CityRoadRuntimeState );
            if( !buildCityRoadTerrainMesh( source, bake, terrain, placement, candidate->geometry, error ) ) return false;
            roadMarkings( candidate->geometry, source, bake );
            candidate->actors.reserve( 5 ); candidate->meshes.reserve( 4 ); candidate->materials.reserve( 4 );
            candidate->textures.reserve( 16 );
            candidate->shapes.reserve( 3 ); candidate->bodies.reserve( 3 );
            auto game = app->getGameManager();
            auto scene = game ? game->getCurrentScene() : nullptr;
            auto physics = app->getPhysicsManager();
            requireRuntime( bool( game ) && bool( scene ) && bool( physics ) && bool( physics->getPhysicsScene() ),
                "Roads require the scene and physics services." );
            requireRuntime( getActor()->getScene() == scene, "Road authoring belongs to a different or detached scene." );
            auto factories = app->getFactoryManager();
            requireRuntime( bool( factories ), "Road procedural service factory is unavailable." );
            auto forge = factories->make_object<procedural::ITextureForge>( "ITextureForge" );
            requireRuntime( bool( forge ), "Authored road materials require the WPProcedural texture forge." );
            // Authored identity, not edit/revision order, determines persistent surfaces.
            u32 surfaceSeed = 2166136261u;
            for( const auto character : source.id ) surfaceSeed = ( surfaceSeed ^ u8( character ) ) * 16777619u;
            candidate->root = game->createActor();
            requireRuntime( bool( candidate->root ), "Road generation actor admission failed." );
            candidate->actors.push_back( candidate->root );
            candidate->root->setName( "City road generation " + StringUtil::toString( source.revision ) );
            candidate->root->setFlag( IGameActor::ActorFlagDontSave, true );
            const String prefix = "__procedural/city-roads/" + StringUtil::getUUID();
            for( u32 layer = 0; layer < 4; ++layer )
            {
                const auto kind = static_cast<CityRoadTerrainTriangleKind>( layer );
                auto resource = runtimeMesh( *candidate, prefix + "/" + StringUtil::toString( layer ) + ".meshbin",
                    candidate->geometry, kind );
                if( !resource ) continue;
                auto material = ApplicationUtil::createDefaultMaterial();
                requireRuntime( bool( material ), "Road material allocation failed." );
                candidate->materials.push_back( material );
                material->load( nullptr );
                material->setRoughness( layer == 0 ? .9f : .85f ); material->setMetalness( 0 );
                material->setSpecular( { .04f, .04f, .04f, 1 } ); material->setCullMode( 1 ); material->setDepthTest( 2 );
                roadSurface( *candidate, prefix, material, forge, surfaceSeed, kind );
                auto actor = game->createActor();
                requireRuntime( bool( actor ), "Road surface actor admission failed." );
                candidate->actors.push_back( actor ); candidate->root->addChild( actor );
                actor->setFlag( IGameActor::ActorFlagDontSave, true );
                actor->setName( layer == 0 ? "Authored road carriageway" : layer == 1 ? "Roundabout island" : layer == 2 ? "Roundabout kerb" : "Road markings" );
                auto mesh = actor->addComponent<Mesh>(); mesh->setMeshResource( resource );
                actor->addComponent<Material>()->setMaterial( material );
                auto renderer = actor->addComponent<MeshRenderer>();
                mesh->setState( getState() ); renderer->setState( getState() );
                if( kind == CityRoadTerrainTriangleKind::Marking ) continue;
                auto shape = physics->addCollisionShape<physics::IMeshShape>( nullptr );
                requireRuntime( bool( shape ), "Road mesh collision allocation failed." );
                candidate->shapes.push_back( shape ); shape->setConvex( false ); shape->setMeshResource( resource );
                requireRuntime( shape->hasShapeData(), "Road mesh collision preparation failed." );
                auto body = physics->addRigidStatic( shape );
                requireRuntime( bool( body ) && body->getNumShapes() == 1, "Road static body preparation failed." );
                candidate->bodies.push_back( body );
            }
            requireRuntime( !candidate->bodies.empty(), "Road candidate contains no collision surface." );
            auto tasks = app->getTaskManager();
            auto renderLock = tasks ? tasks->lockTask( TaskId::Render ) : TaskLock();
            auto physicsLock = tasks ? tasks->lockTask( TaskId::Physics ) : TaskLock();
            try
            {
                if( auto bound = getTerrain() )
                {
                    render::TerrainSnapshot current; Transform3<real_Num> currentPlacement; u64 serial = 0; bool detached = false;
                    bound->getTerrainIntegrationState( current, currentPlacement, serial, detached );
                    requireRuntime( !detached && current == terrain && currentPlacement == placement,
                        "Terrain changed before road publication." );
                }
                requireRuntime( getActor()->getScene() == scene, "The authoring scene changed during preparation." );
                scene->addActor( candidate->root );
                for( auto actor : candidate->actors )
                {
                    actor->setScene( scene ); actor->setFlag( IGameActor::ActorFlagInScene, true );
                    actor->setState( getActor()->getState() );
                    scene->registerAllUpdates( actor );
                }
                for( const auto &body : candidate->bodies ) physics->getPhysicsScene()->addActor( body );
                // The inventories are prepared before the swap. A failed scene/body
                // publication destroys the staged generation before releasing task locks.
                m_runtime.swap( candidate ); candidate.reset();
            }
            catch( ... ) { candidate.reset(); throw; }
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void CityRoadAuthoring::clearRuntime()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto tasks = app ? app->getTaskManager() : nullptr;
        auto renderLock = tasks && Thread::getCurrentTask() != TaskId::Render ? tasks->lockTask( TaskId::Render ) : TaskLock();
        auto physicsLock = tasks && Thread::getCurrentTask() != TaskId::Physics ? tasks->lockTask( TaskId::Physics ) : TaskLock();
        m_runtime.reset();
    }

    size_t CityRoadAuthoring::getRuntimeTriangleCount() const
    { return m_runtime ? m_runtime->geometry.triangleKinds.size() : 0; }

    bool CityRoadAuthoring::getRuntimeSurface( f32 x, f32 z, f32 &height, bool &driveable ) const
    {
        if( !m_runtime || !std::isfinite( x ) || !std::isfinite( z ) ) return false;
        bool found = false; f32 top = -std::numeric_limits<f32>::max();
        const auto &g = m_runtime->geometry;
        for( size_t triangle = 0; triangle < g.triangleKinds.size(); ++triangle )
        {
            if( g.triangleKinds[triangle] == CityRoadTerrainTriangleKind::Marking ) continue;
            const auto a = g.vertices[g.indices[triangle * 3]].position;
            const auto b = g.vertices[g.indices[triangle * 3 + 1]].position;
            const auto c = g.vertices[g.indices[triangle * 3 + 2]].position;
            const float denominator = ( b.z - c.z ) * ( a.x - c.x ) + ( c.x - b.x ) * ( a.z - c.z );
            if( std::abs( denominator ) < 1e-8f ) continue;
            const float u = ( ( b.z - c.z ) * ( x - c.x ) + ( c.x - b.x ) * ( z - c.z ) ) / denominator;
            const float v = ( ( c.z - a.z ) * ( x - c.x ) + ( a.x - c.x ) * ( z - c.z ) ) / denominator;
            if( u < -1e-5f || v < -1e-5f || u + v > 1.00001f ) continue;
            const float y = u * a.y + v * b.y + ( 1 - u - v ) * c.y;
            if( y > top ) { top = y; found = true; driveable = g.triangleKinds[triangle] == CityRoadTerrainTriangleKind::Road; }
        }
        if( found ) height = top;
        return found;
    }
}
