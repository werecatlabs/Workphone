#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <Workphone/Interface/Procedural/OpenCityStreetDetails.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/CityRoadParcelSupport.hpp>
#include <Workphone/Scene/Components/CityRoadFootways.hpp>
#include <Workphone/Scene/Components/CollisionMesh.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/CollisionTerrain.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Graphics/ProceduralSurfaceSourceExport.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <stdexcept>
#include <map>
#include <cstdio>

namespace workphone::scene::race
{
    SceneAssets::SceneAssets(const SceneAssets &other) :
        openCity(other.openCity), useFoliageWorld(other.useFoliageWorld), cityNaturalRoads(other.cityNaturalRoads), cityRoadShowcase(other.cityRoadShowcase), cityTerrainRoads(other.cityTerrainRoads), roadAuthoring(other.roadAuthoring), roadTerrain(other.roadTerrain),
        cityLayout(other.cityLayout), cityBuildingSupportChecks(other.cityBuildingSupportChecks), cityCulledParcels(other.cityCulledParcels),
        cityBuildingSupportValid(other.cityBuildingSupportValid), citySourceReused(other.citySourceReused), cityRaceRouteAvailable(other.cityRaceRouteAvailable), cityRaceRouteError(other.cityRaceRouteError), cityAppliedSourceRevision(other.cityAppliedSourceRevision), foliageTerrain(other.foliageTerrain),
        cityBlocks(other.cityBlocks), cityRoute(other.cityRoute), vehicle(other.vehicle), circuit(other.circuit),
        wheels(other.wheels), body(other.body), shadow(other.shadow), reflectionActor(other.reflectionActor), ground(other.ground),
        reflectionTexture(other.reflectionTexture), vehicleMaterials(other.vehicleMaterials), materials(other.materials),
        textures(other.textures), meshes(other.meshes), actors(other.actors), treeLODs(makeSourceBudgetSnapshot(other.treeLODs)),
        vehicleLOD(other.vehicleLOD), sky(other.sky), sun(other.sun), resourcePrefix(other.resourcePrefix),
        exportSourceDirectory(other.exportSourceDirectory), triangles(other.triangles), textureBytes(other.textureBytes) {}

    SceneAssets &SceneAssets::operator=(const SceneAssets &other)
    { if(this!=&other){SceneAssets candidate(other);swap(candidate);} return *this; }

    void SceneAssets::swap(SceneAssets &other) noexcept
    {
        using std::swap;
        swap(openCity,other.openCity);swap(useFoliageWorld,other.useFoliageWorld);swap(cityNaturalRoads,other.cityNaturalRoads);swap(foliageTerrain,other.foliageTerrain);
        swap(cityRoadShowcase,other.cityRoadShowcase);swap(roadAuthoring,other.roadAuthoring);swap(roadTerrain,other.roadTerrain);
        swap(cityTerrainRoads,other.cityTerrainRoads);cityLayout.swap(other.cityLayout);
        swap(cityBuildingSupportChecks,other.cityBuildingSupportChecks);swap(cityCulledParcels,other.cityCulledParcels);
        swap(cityBuildingSupportValid,other.cityBuildingSupportValid);swap(citySourceReused,other.citySourceReused);
        swap(cityRaceRouteAvailable,other.cityRaceRouteAvailable);swap(cityRaceRouteError,other.cityRaceRouteError);swap(cityAppliedSourceRevision,other.cityAppliedSourceRevision);
        swap(cityBlocks,other.cityBlocks);swap(cityRoute,other.cityRoute);swap(vehicle,other.vehicle);circuit.swap(other.circuit);
        wheels.swap(other.wheels);swap(body,other.body);swap(shadow,other.shadow);swap(reflectionActor,other.reflectionActor);swap(ground,other.ground);
        swap(reflectionTexture,other.reflectionTexture);vehicleMaterials.swap(other.vehicleMaterials);materials.swap(other.materials);
        textures.swap(other.textures);meshes.swap(other.meshes);actors.swap(other.actors);treeLODs.swap(other.treeLODs);
        swap(vehicleLOD,other.vehicleLOD);swap(sky,other.sky);swap(sun,other.sun);swap(resourcePrefix,other.resourcePrefix);
        swap(exportSourceDirectory,other.exportSourceDirectory);swap(triangles,other.triangles);swap(textureBytes,other.textureBytes);
    }

    namespace
    {
        template<class C> void reserveOwnerSlot(C &owners)
        {
            if(owners.size()!=owners.capacity()) return;
            if(owners.size()==owners.max_size()) throw std::length_error("Scene inventory capacity overflow");
            const auto next=owners.capacity()>owners.max_size()/2 ? owners.max_size() :
                std::min(owners.max_size(),std::max<size_t>(8,owners.capacity()*2));
            owners.reserve(next);
        }
        using namespace procedural;
        constexpr float pi = 3.14159265358979323846f;

        struct Geometry
        {
            Array<Vector3F> positions{ Allocator<Vector3F>( sourceBudget ) };
            Array<Vector3F> normals{ Allocator<Vector3F>( sourceBudget ) };
            Array<Vector3F> uv{ Allocator<Vector3F>( sourceBudget ) };
            Array<u32> indices{ Allocator<u32>( sourceBudget ) };
            Array<Vector4F> tangents{ Allocator<Vector4F>( sourceBudget ) };

            void quad( Vector3F a, Vector3F b, Vector3F c, Vector3F d, Vector3F normal, float u0 = 0,
                       float u1 = 1, float v0 = 0, float v1 = 1 )
            {
                const auto base = static_cast<u32>( positions.size() );
                auto tangent = b - a;
                if( tangent.lengthSquared() < 1e-8f )
                {
                    tangent = Vector3F( 1, 0, 0 );
                }
                tangent.normalise();
                for( auto p : { a, b, c, d } )
                {
                    positions.push_back( p );
                    normals.push_back( normal );
                    tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                }
                uv.push_back( { u0, v0, 0 } );
                uv.push_back( { u1, v0, 0 } );
                uv.push_back( { u1, v1, 0 } );
                uv.push_back( { u0, v1, 0 } );
                const auto winding = ( b - a ).crossProduct( c - a ).dotProduct( normal ) >= 0;
                for( auto i : winding ? std::array<u32, 6>{ 0, 1, 2, 0, 2, 3 }
                                      : std::array<u32, 6>{ 0, 2, 1, 0, 3, 2 } )
                {
                    indices.push_back( base + i );
                }
            }

            void beam( Vector3F a, Vector3F b, float radius )
            {
                auto axis = b - a;
                if( axis.lengthSquared() < 1e-8f || radius <= 0 )
                {
                    return;
                }
                axis.normalise();
                auto u = axis.crossProduct( std::abs( axis.y ) > .9f ? Vector3F( 1, 0, 0 )
                                                                     : Vector3F( 0, 1, 0 ) );
                u.normalise();
                const auto v = axis.crossProduct( u );
                const auto ur = u * radius, vr = v * radius;
                quad( a + ur - vr, b + ur - vr, b + ur + vr, a + ur + vr, u );
                quad( a - ur + vr, b - ur + vr, b - ur - vr, a - ur - vr, -u );
                quad( a + ur + vr, b + ur + vr, b - ur + vr, a - ur + vr, v );
                quad( a - ur - vr, b - ur - vr, b + ur - vr, a + ur - vr, -v );
                quad( a - ur - vr, a + ur - vr, a + ur + vr, a - ur + vr, -axis );
                quad( b - ur - vr, b + ur - vr, b + ur + vr, b - ur + vr, axis );
            }

            void box( Vector3F p, Vector3F size )
            {
                auto lo = p - size * 0.5f, hi = p + size * 0.5f;
                quad( { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z }, { hi.x, hi.y, hi.z },
                      { lo.x, hi.y, hi.z }, { 0, 1, 0 } );
                quad( { lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z }, { hi.x, lo.y, lo.z },
                      { lo.x, lo.y, lo.z }, { 0, -1, 0 } );
                quad( { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { hi.x, hi.y, lo.z },
                      { lo.x, hi.y, lo.z }, { 0, 0, -1 } );
                quad( { hi.x, lo.y, hi.z }, { lo.x, lo.y, hi.z }, { lo.x, hi.y, hi.z },
                      { hi.x, hi.y, hi.z }, { 0, 0, 1 } );
                quad( { lo.x, lo.y, hi.z }, { lo.x, lo.y, lo.z }, { lo.x, hi.y, lo.z },
                      { lo.x, hi.y, hi.z }, { -1, 0, 0 } );
                quad( { hi.x, lo.y, lo.z }, { hi.x, lo.y, hi.z }, { hi.x, hi.y, hi.z },
                      { hi.x, hi.y, lo.z }, { 1, 0, 0 } );
            }
        };

        SmartPtr<render::IMaterial> material( SceneAssets &assets, ColourF colour, float roughness,
                                              float metalness = 0 )
        {
            reserveOwnerSlot(assets.materials);
            auto m = ApplicationUtil::createDefaultMaterial();
            if(!m) throw std::runtime_error("Cannot create procedural material");
            assets.materials.push_back(m);
            m->load( nullptr );
            m->setCullMode( 1 );
            m->setDepthTest( 2 );
            m->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
            m->setDiffuse( colour );
            m->setRoughness( roughness );
            m->setMetalness( metalness );
            return m;
        }

        SmartPtr<render::ITexture> uploadPixels(
            SceneAssets &assets, const u8 *pixels, size_t bytes, u32 width, u32 height,
            render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1 )
        {
            if( !bytes ) return nullptr;
            if( !pixels || !width || !height || width > 16384 || height > 16384 ||
                u64( width ) * height * 4 != bytes )
                throw std::invalid_argument( "Procedural upload requires exact bounded RGBA pixels." );
            // Admit conversion before creating a manager-owned texture. The
            // caller's atlas and this BGRA snapshot remain independently owned.
            auto bgra = makeSourceBudgetSnapshot<u8>();
            bgra.reserve( bytes );
            bgra.resize( bytes );
            std::copy_n( pixels, bytes, bgra.begin() );
            for( size_t i = 0; i < bgra.size(); i += 4 ) std::swap( bgra[i], bgra[i + 2] );
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            auto manager = graphics->getTextureManager();
            reserveOwnerSlot(assets.textures);
            auto name =
                assets.resourcePrefix + "/texture/" + StringUtil::toString( assets.textures.size() );
            // New sampled textures default to 2D/usage zero. Avoid allocating
            // createManual's complete blank image before replacing its pixels.
            auto texture = dynamic_pointer_cast<render::ITexture>( manager->create( name ) );
            if( !texture ) throw std::runtime_error( "Cannot create procedural texture." );
            try
            {
                auto properties = texture->getProperties();
                properties->setProperty( "mipmapFilter", static_cast<s32>( filter ) );
                properties->setProperty( "mipmapAtlasColumns", atlasColumns );
                texture->setProperties( properties );
                String error;
                if( !texture->copyByteData( bgra, Vector2I( width, height ), error ) )
                    throw std::runtime_error( ( String( "Cannot stage procedural texture: " ) + error ).c_str() );
                texture->load( nullptr );
                assets.textures.push_back( texture );
                assets.textureBytes += bytes;
            }
            catch( ... )
            {
                try { manager->destroyResource( texture ); } catch( ... ) {}
                throw;
            }
            return texture;
        }

        SmartPtr<render::ITexture> upload(
            SceneAssets &assets, const TextureBuffer &buffer,
            render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1 )
        {
            return uploadPixels( assets, buffer.data(), buffer.pixels.size(), buffer.width, buffer.height,
                                 filter, atlasColumns );
        }

        SmartPtr<render::ITexture> uploadLinearRadiance( SceneAssets &assets, const Array<f32> &rgba,
                                                         Vector2I size )
        {
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            reserveOwnerSlot(assets.textures);
            const auto name =
                assets.resourcePrefix + "/radiance/" + StringUtil::toString( assets.textures.size() );
            auto manager=graphics->getTextureManager();
            auto texture=dynamic_pointer_cast<render::ITexture>(manager->create(name));
            if(!texture) throw std::runtime_error("Cannot create HDR reflection texture");
            try
            {
                String error;
                if(!texture->copyLinearFloatData(rgba,size,error))
                    throw std::runtime_error((String("Cannot stage HDR reflection face: ")+error).c_str());
                assets.textures.push_back(texture);
                assets.textureBytes+=rgba.size()*sizeof(f32);
            }
            catch(...){try{manager->destroyResource(texture);}catch(...){}throw;}
            return texture;
        }

        void maps( SceneAssets &assets, SmartPtr<render::IMaterial> m, const TextureBuffer &albedo,
                   const TextureBuffer &normal, const TextureBuffer &orm )
        {
            if( !albedo.pixels.empty() )
            {
                m->setDiffuse( ColourF::White );
                m->setTexture( upload( assets, albedo ), 0 );
            }
            if( !normal.pixels.empty() )
            {
                m->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
            }
            // The generated ORM is AO/Roughness/Metalness. WPGraphics' separate map slots
            // read red, so expand each channel explicitly, without applying an sRGB transfer.
            if( !orm.pixels.empty() )
            {
                for( auto channel : { 0u, 1u, 2u } )
                {
                    TextureBuffer split( orm.width, orm.height );
                    for( size_t i = 0; i < orm.pixels.size(); i += 4 )
                    {
                        split.pixels[i] = split.pixels[i + 1] = split.pixels[i + 2] =
                            orm.pixels[i + channel];
                        split.pixels[i + 3] = 255;
                    }
                    m->setTexture( upload( assets, split,
                                           channel == 1 ? render::TextureMipFilter::Roughness
                                                        : render::TextureMipFilter::Data ),
                                   channel == 0   ? 22
                                   : channel == 1 ? 3
                                                  : 2 );
                }
                m->setRoughness( 1 );
                m->setMetalness( 1 );
            }
        }

        SmartPtr<render::IMaterial> raceAsphalt( SceneAssets &assets, SmartPtr<ITextureForge> forge,
                                                 const SurfaceBakeParams &params, u32 seed )
        {
            auto asphalt = forge->bakeSurface( SurfaceTag::Asphalt, params );
            // Keep the forge's seeded aggregate detail subtle at driving distance.
            // Its general-purpose height/rut contrast is too strong for a race surface.
            for( size_t i = 0; i < asphalt.albedo.pixels.size(); i += 4 )
            {
                for( size_t channel = 0; channel < 3; ++channel )
                {
                    asphalt.albedo.pixels[i + channel] =
                        static_cast<u8>( 60 + asphalt.albedo.pixels[i + channel] / 6 );
                }
                for( size_t channel = 0; channel < 2; ++channel )
                {
                    asphalt.normal.pixels[i + channel] = static_cast<u8>(
                        128 + ( int( asphalt.normal.pixels[i + channel] ) - 128 ) / 10 );
                }
            }
            if( assets.openCity )
            {
                // City streets use isotropic aggregate. The generic asphalt's
                // repeated sinusoidal ruts read as a corrugated road at street level.
                const auto noise = [seed]( u32 x, u32 y, u32 period ) {
                    u32 value = ( x % period ) * 0x9e3779b9u ^
                        ( y % period ) * 0x85ebca6bu ^ seed;
                    value ^= value >> 16; value *= 0x7feb352du;
                    value ^= value >> 15; value *= 0x846ca68bu;
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
                for( u32 y = 0; y < params.size; ++y ) for( u32 x = 0; x < params.size; ++x )
                {
                    const float grain = noise( x, y, params.size );
                    const float patch = smoothNoise( x, y, 3 ), aggregate = smoothNoise( x, y, 32 );
                    const float linear = .064f + ( patch - .5f ) * .016f +
                        ( aggregate - .5f ) * .009f + ( grain - .5f ) * .006f;
                    auto pixel = asphalt.albedo.pixel( x, y );
                    for( u32 channel = 0; channel < 3; ++channel )
                    {
                        const float colour = linear * ( channel == 2 ? 1.02f : 1.f );
                        pixel[channel] = u8( std::clamp( ( 1.055f * std::pow( colour, 1.f / 2.4f ) - .055f ) * 255, 0.f, 255.f ) );
                    }
                    asphalt.height.values[size_t( y ) * params.size + x] = .5f + ( grain - .5f ) * .012f;
                    auto orm = asphalt.orm.pixel( x, y );
                    orm[0] = 255; orm[1] = u8( 224 + aggregate * 16 ); orm[2] = 0; orm[3] = 255;
                }
                forge->heightToNormal( asphalt.height, asphalt.normal, .25f );
            }
            if( !assets.exportSourceDirectory.empty() )
            {
                std::string error;
                if( !texture_export::exportAsphaltSource(
                        asphalt, std::filesystem::u8path( assets.exportSourceDirectory.c_str() ), seed,
                        error ) )
                {
                    throw std::runtime_error( "Asphalt source export failed: " + error );
                }
            }
            auto roadMat = material( assets, { .07f, .075f, .085f, 1 }, .9f );
            maps( assets, roadMat, asphalt.albedo, asphalt.normal, asphalt.orm );
            roadMat->setNormalStrength( .2f );
            roadMat->setEditorUInt( "uvFilter", 3 );
            roadMat->setEditorFloat( "uvAniso", 8.f );
            return roadMat;
        }

        SmartPtr<scene::IGameActor> mesh( SceneAssets &assets, const String &name, const Geometry &g,
                                          SmartPtr<render::IMaterial> m,
                                          SmartPtr<scene::IGameActor> parent = nullptr )
        {
            if( g.indices.empty() )
            {
                return nullptr;
            }
            auto app = core::IApplicationManager::instance();
            auto manager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            if( !manager )
            {
                manager = make_ptr<MeshManager>();
                app->setMeshManager( manager );
            }
            // Preserve outward winding. DX11's material rasterizer uses counterclockwise
            // front faces; reversing these indices also reverses its two-sided lighting normal.
            auto generated = MeshUtil::createMesh( g.positions, g.normals, g.tangents, g.uv, g.indices );
            generated->updateAABB( true );
            auto path = assets.resourcePrefix + "/mesh/" + StringUtil::toString( assets.meshes.size() ) +
                        ".meshbin";
            reserveOwnerSlot(assets.meshes);
            auto resource =
                dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve( path ).first );
            if(!resource) throw std::runtime_error("Cannot create procedural mesh resource");
            assets.meshes.push_back(resource);
            resource->setName( path );
            resource->setFilePath( path );
            resource->setMesh( generated );
            resource->setLoadingState( LoadingState::Loaded );
            assets.triangles += g.indices.size() / 3;
            reserveOwnerSlot(assets.actors);
            auto actor = app->getGameManager()->createActor();
            if(!actor) throw std::runtime_error("Cannot create procedural actor");
            assets.actors.push_back(actor);
            actor->setName( name );
            if( parent )
            {
                parent->addChild( actor );
            }
            actor->addComponent<scene::Mesh>()->setMeshResource( resource );
            actor->addComponent<scene::MeshRenderer>();
            actor->addComponent<scene::Material>()->setMaterial( m );
            if( !parent )
            {
                app->getGameManager()->getCurrentScene()->addActor( actor );
            }
            return actor;
        }

        Vector3F vector( const VehiclePhysicsVector3 &p )
        {
            return { float( p.x ), float( p.y ), float( p.z ) };
        }

        void staticBox( SceneAssets &assets, const String &name, Vector3F centre, Vector3F size,
                        QuaternionF orientation = QuaternionF::identity() )
        {
            auto app = core::IApplicationManager::instance();
            reserveOwnerSlot(assets.actors);
            auto actor = app->getGameManager()->createActor();
            if(!actor) throw std::runtime_error("Cannot create procedural actor");
            assets.actors.push_back(actor);
            // Own these independently of batched render meshes and tree LODs.
            // The normal generated-scene cleanup also removes their physics bodies.
            actor->setName( "Trackside collision: " + name );
            actor->setStatic( true );
            actor->setPosition( centre );
            actor->setOrientation( orientation );
            // CollisionBox takes full dimensions, not half extents.
            actor->addComponent<scene::CollisionBox>()->setExtents( size );
            actor->addComponent<scene::Rigidbody>();
            app->getGameManager()->getCurrentScene()->addActor( actor );
        }

        void railCollision( SceneAssets &assets, Vector3F start, Vector3F end )
        {
            auto direction = end - start;
            direction.y = 0;
            const auto length = direction.length();
            if( length <= 1e-4f )
            {
                return;
            }
            const auto yaw = std::atan2( direction.x, direction.z ) * 180.f / pi;
            staticBox( assets, "Guardrail beam", ( start + end ) * .5f + Vector3F( 0, .5f, 0 ),
                       { .2f, .5f, length }, QuaternionF::eulerDegrees( 0, yaw, 0 ) );
        }

        void pineGeometry( Geometry &trunks, Geometry &leaves, Vector3F p, float h, u32 seed = 0 )
        {
            // Reuse the engine's validated deterministic plant generator for the
            // branching skeleton. Four canonical variants are prepared once and
            // shared by patch batches and the existing far-impostor bake.
            static const auto variants = [] {
                std::array<std::pair<Geometry, Geometry>, 4> result;
                for( u32 variant = 0; variant < result.size(); ++variant )
                {
                    scene::FoliagePlantRecipe recipe;
                    recipe.speciesId = "circuit-pine-" + StringUtil::toString( variant );
                    recipe.seed = 7919u + variant * 104729u;
                    recipe.height = 1;
                    recipe.radius = .22f;
                    recipe.trunkRadius = .025f;
                    recipe.branches = 14;
                    render::FoliageSpecies species;
                    String error;
                    if( !scene::generateFoliagePlant( recipe, species, error ) )
                    {
                        throw std::runtime_error( "Cannot generate circuit pine: " + error );
                    }
                    const auto &source = species.lods[0];
                    auto &bark = result[variant].first;
                    auto &needles = result[variant].second;
                    auto taper = []( Vector3F position ) {
                        // The reusable recipe is a broadleaf skeleton. Narrow
                        // its upper branches into a conifer crown, retaining
                        // open gaps and an exposed lower trunk.
                        const float width = .96f - .68f * std::clamp( position.y, 0.f, 1.f );
                        position.x *= width;
                        position.z *= width;
                        return position;
                    };
                    const auto &woodSection = source.sections[0];
                    // Native plant cylinders contain independent four-vertex
                    // quads. Reconstruct their normals after crown tapering.
                    const auto woodVertices = woodSection.indexCount / 6u * 4u;
                    for( u32 first = 0; first < woodVertices; first += 4u )
                    {
                        const auto a = taper( source.positions[first] );
                        const auto b = taper( source.positions[first + 1u] );
                        const auto c = taper( source.positions[first + 2u] );
                        const auto d = taper( source.positions[first + 3u] );
                        auto normal = ( b - a ).crossProduct( c - a );
                        normal.normalise();
                        bark.quad( a, b, c, d, normal );
                    }
                    // The source's textured leaf cards provide seeded clump
                    // positions/orientations. The sample's opaque palette has
                    // no leaf atlas, so represent each clump with five
                    // overlapping sprigs instead of rendering solid rectangles.
                    // This keeps real silhouette gaps without alpha overdraw.
                    for( u32 first = woodVertices; first + 3u < source.positions.size(); first += 4u )
                    {
                        auto centre =
                            taper( ( source.positions[first] + source.positions[first + 1u] ) * .5f );
                        auto radial = Vector3F( centre.x, 0, centre.z );
                        radial.normalise();
                        auto cardRight = source.positions[first + 1u] - source.positions[first];
                        cardRight.normalise();
                        auto direction = radial * .8f + cardRight * .45f;
                        direction.normalise();
                        const float phase =
                            float( render::foliageStableHash( recipe.seed + first ) >> 40 ) / 16777216.f;
                        // Boughs must cover the branch interior rather than
                        // attaching tiny leaves only to its extreme tip. Their
                        // overlapping lobes build crown mass while retaining
                        // a broken, seeded outer silhouette and tier gaps.
                        centre.x *= .55f;
                        centre.z *= .55f;
                        centre.y += ( phase - .5f ) * .055f;
                        const float crownScale = 1.f - .65f * centre.y;
                        auto bounded = []( Vector3F point ) {
                            const float radius = std::hypot( point.x, point.z );
                            if( radius > .27f )
                            {
                                point.x *= .27f / radius;
                                point.z *= .27f / radius;
                            }
                            return point;
                        };
                        for( int sprig = -2; sprig <= 2; ++sprig )
                        {
                            const float angle = sprig * .55f;
                            const auto axis = Vector3F(
                                direction.x * std::cos( angle ) - direction.z * std::sin( angle ),
                                -.015f + .045f * phase,
                                direction.x * std::sin( angle ) + direction.z * std::cos( angle ) );
                            const auto tip =
                                bounded( centre + axis * ( .20f + .06f * phase ) * crownScale );
                            const auto middle =
                                centre + ( tip - centre ) * .57f + Vector3F( 0, .055f, 0 );
                            auto across = Vector3F( -axis.z, .35f * sprig, axis.x );
                            across.normalise();
                            across *= ( .045f + .020f * phase ) * crownScale;
                            const auto left = bounded( middle + across );
                            const auto right = bounded( middle - across );
                            auto normal = ( left - centre ).crossProduct( tip - centre );
                            normal.normalise();
                            needles.quad( centre, left, tip, right, normal );
                        }
                    }
                }
                return result;
            }();
            const auto &variant = variants[seed % variants.size()];
            auto append = [&]( Geometry &target, const Geometry &source ) {
                const auto base = static_cast<u32>( target.positions.size() );
                for( const auto &position : source.positions )
                {
                    target.positions.push_back( p + position * h );
                }
                target.normals.insert( target.normals.end(), source.normals.begin(),
                                       source.normals.end() );
                target.tangents.insert( target.tangents.end(), source.tangents.begin(),
                                        source.tangents.end() );
                target.uv.insert( target.uv.end(), source.uv.begin(), source.uv.end() );
                for( auto index : source.indices )
                {
                    target.indices.push_back( base + index );
                }
            };
            append( trunks, variant.first );
            append( leaves, variant.second );
        }

        MeshImposterAtlas bakePineImposter( u32 variant )
        {
            Geometry trunks, canopy;
            pineGeometry( trunks, canopy, {}, 1, variant );
            auto triangles = makeSourceBudgetSnapshot<MeshImposterTriangle>();
            triangles.reserve( trunks.indices.size() / 3 + canopy.indices.size() / 3 );
            auto append = [&]( const Geometry &g, ColourF colour ) {
                for( size_t i = 0; i < g.indices.size(); i += 3 )
                {
                    triangles.push_back( { { g.positions[g.indices[i]], g.positions[g.indices[i + 1]],
                                             g.positions[g.indices[i + 2]] },
                                           colour } );
                }
            };
            append( trunks, { .12f, .055f, .018f, 1 } );
            append( canopy, { .18f, .32f, .13f, 1 } );
            return generateMeshImposters( triangles, { -.28f, 0, -.28f }, { .28f, 1.08f, .28f } );
        }

        void pineImposterGeometry( Geometry &g, const MeshImposterAtlas &atlas, Vector3F p, float h,
                                   u32 variant )
        {
            for( size_t i = 0; i < atlas.views.size(); ++i )
            {
                const auto &view = atlas.views[i];
                // Crossed side cards retain a silhouette from every azimuth. The top
                // card preserves the canopy when viewed from the overhead camera.
                const auto centre = p + ( i == 2 ? Vector3F( 0, .7f * h, 0 ) : Vector3F() );
                auto point = [&]( float x, float y ) {
                    return centre + ( view.right * x + view.up * y ) * h;
                };
                const auto base = g.positions.size();
                g.quad( point( view.minimum.x, view.minimum.y ), point( view.maximum.x, view.minimum.y ),
                        point( view.maximum.x, view.maximum.y ), point( view.minimum.x, view.maximum.y ),
                        view.towardCamera );
                // Match the bake's one-pixel transparent border and top-left image origin.
                const float u0 =
                    ( float( variant * atlas.width + i * atlas.tileWidth ) + 1 ) / ( atlas.width * 4 );
                const float u1 = ( float( variant * atlas.width + ( i + 1 ) * atlas.tileWidth ) - 2 ) /
                                 ( atlas.width * 4 );
                const float top = 1.f / atlas.height, bottom = float( atlas.height - 2 ) / atlas.height;
                g.uv[base] = { u0, bottom, 0 };
                g.uv[base + 1] = { u1, bottom, 0 };
                g.uv[base + 2] = { u1, top, 0 };
                g.uv[base + 3] = { u0, top, 0 };
            }
        }

        void buildCar( SceneAssets &assets, SmartPtr<scene::IGameActor> actor, u32 seed,
                       VehicleAppearanceQuality quality )
        {
            auto app = core::IApplicationManager::instance();
            auto generator =
                app->getFactoryManager()->createObjectFromType<IVehicleGenerator>( "IVehicleGenerator" );
            if( !generator )
            {
                throw std::runtime_error( "VehicleAdvanced requires WPProcedural IVehicleGenerator." );
            }
            VehicleGenerationConfig config;
            config.seed = seed;
            config.appearance.quality = quality;
            config.appearance.customMaxResolution = quality == VehicleAppearanceQuality::Preview ? 256
                                                    : quality == VehicleAppearanceQuality::Standard
                                                        ? 512
                                                        : 1024;
            assets.vehicle = generator->generate( config );
            if( !assets.vehicle.isValid() )
            {
                throw std::runtime_error( "Procedural vehicle generation failed." );
            }
            const auto &appearance = assets.vehicle.appearance;
            std::array<SmartPtr<render::IMaterial>, size_t( VehicleMaterialSlot::Count )> materials;
            for( size_t i = 0; i < materials.size(); ++i )
            {
                reserveOwnerSlot(assets.vehicleMaterials);
                const auto &d = appearance.materials[i];
                auto m = material( assets, { d.baseColor.r, d.baseColor.g, d.baseColor.b, d.opacity },
                                   d.roughness, d.metallic );
                m->setEmissive( { d.emissiveColor.r, d.emissiveColor.g, d.emissiveColor.b, 1 } );
                m->setEditorFloat( "emissionIntensity", d.emissiveIntensity );
                m->setEmissionEnabled( d.emissiveIntensity > 0 );
                m->setEditorFloat( "clearCoat", d.clearcoat );
                m->setEditorFloat( "clearCoatRoughness", d.clearcoatRoughness );
                m->setNormalStrength( d.normalScale );
                m->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
                m->setDiffuse( { d.baseColor.r, d.baseColor.g, d.baseColor.b, d.opacity } );
                m->setRoughness( d.roughness );
                m->setMetalness( d.metallic );
                m->setEmissive( { d.emissiveColor.r, d.emissiveColor.g, d.emissiveColor.b, 1 } );
                m->setEmissionEnabled( d.emissiveIntensity > 0 );
                if( d.doubleSided )
                {
                    m->setCullMode( 1 );
                }
                materials[i] = m;
                assets.vehicleMaterials.push_back( m );
            }
            const auto &t = appearance.textures;
            maps( assets, materials[size_t( VehicleMaterialSlot::BodyPaint )], t.bodyLivery,
                  t.bodyNormal, t.bodyORM );
            maps( assets, materials[size_t( VehicleMaterialSlot::CarbonGloss )], t.carbonAlbedo,
                  t.carbonNormal, t.carbonORM );
            // Share the carbon texture objects rather than uploading a second copy.
            auto carbon = materials[size_t( VehicleMaterialSlot::CarbonGloss )];
            auto matte = materials[size_t( VehicleMaterialSlot::CarbonMatte )];
            matte->setTextures( carbon->getTextures() );
            matte->setDiffuse( ColourF::White );
            matte->setRoughness( 1 );
            matte->setMetalness( 1 );
            maps( assets, materials[size_t( VehicleMaterialSlot::Rubber )], t.rubberAlbedo,
                  t.rubberNormal, t.rubberORM );
            TextureBuffer empty;
            maps( assets, materials[size_t( VehicleMaterialSlot::BareMetal )], empty,
                  t.brushedMetalNormal, t.brushedMetalORM );
            auto com = vector( assets.vehicle.physics.massProperties.centreOfMass );
            std::array<Vector3F, 4> hubs;
            for( size_t i = 0; i < 4; ++i )
            {
                hubs[i] = vector( assets.vehicle.physics.wheels[i].hubPosition );
                reserveOwnerSlot(assets.actors);
                auto wheel = app->getGameManager()->createActor();
                if(!wheel) throw std::runtime_error("Cannot create procedural actor");
                assets.actors.push_back(wheel);
                wheel->setName( String( "Generated wheel " ) + StringUtil::toString( i ) );
                actor->addChild( wheel );
                wheel->addComponent<scene::WheelController>();
                assets.wheels[i] = wheel;
            }
            auto vehicleLOD = actor->addComponent<scene::LODGroup>();
            assets.vehicleLOD = vehicleLOD;
            vehicleLOD->setSize( 4.5f );
            vehicleLOD->setHysteresis( .15f );
            vehicleLOD->setCullBelowLastLOD( false );
            const size_t firstLOD = quality == VehicleAppearanceQuality::Preview ? 1 : 0;
            for( size_t lod = firstLOD; lod < assets.vehicle.geometry.lods.size(); ++lod )
            {
                auto renderers=makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                for( const auto &section : assets.vehicle.geometry.lods[lod].sections )
                {
                    auto m = materials[size_t( generator->materialSlotFor( section.material ) )];
                    bool rotating = section.material == VehicleMaterial::Tyre ||
                                    section.material == VehicleMaterial::Rim ||
                                    section.material == VehicleMaterial::Brake;
                    // Wheel sections contain four disconnected assemblies. Assign whole triangles
                    // to their closest authored axle; preserve winding, UVs and normals unchanged.
                    std::array<Geometry, 4> parts;
                    for( size_t index = 0; index < section.indices.size(); index += 3 )
                    {
                        size_t corner = 0;
                        if( rotating )
                        {
                            auto centre = ( section.vertices[section.indices[index]].position +
                                            section.vertices[section.indices[index + 1]].position +
                                            section.vertices[section.indices[index + 2]].position ) /
                                          3.0f;
                            float closest = std::numeric_limits<float>::max();
                            for( size_t i = 0; i < 4; ++i )
                            {
                                auto distance = ( centre - hubs[i] ).length();
                                if( distance < closest )
                                {
                                    closest = distance;
                                    corner = i;
                                }
                            }
                        }
                        auto &g = parts[corner];
                        for( size_t j = 0; j < 3; ++j )
                        {
                            const auto &v = section.vertices[section.indices[index + j]];
                            g.indices.push_back( static_cast<u32>( g.positions.size() ) );
                            g.positions.push_back( v.position - ( rotating ? hubs[corner] : com ) );
                            g.normals.push_back( v.normal );
                            g.uv.push_back( { v.uv.x, v.uv.y, 0 } );
                            g.tangents.push_back(
                                { v.tangent.x, v.tangent.y, v.tangent.z, v.tangentSign } );
                        }
                    }
                    for( size_t i = 0; i < ( rotating ? 4u : 1u ); ++i )
                    {
                        auto part = mesh(
                            assets,
                            "LOD" + StringUtil::toString( lod ) + " " + String( section.name.c_str() ),
                            parts[i], m, rotating ? assets.wheels[i] : actor );
                        if( !assets.body && !rotating )
                        {
                            assets.body = part;
                        }
                        if( part )
                        {
                            renderers.push_back( part->getComponent<scene::MeshRenderer>() );
                        }
                    }
                }
                vehicleLOD->addLevel(
                    lod + 1 == assets.vehicle.geometry.lods.size() ? 0.f : ( lod == 0 ? .32f : .085f ),
                    renderers );
            }
            generator = nullptr;
        }

        void buildFoliageWorld( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality );

        void buildTrack( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality )
        {
            assets.circuit = generateCircuit( seed );
            auto app = core::IApplicationManager::instance();
            auto forge =
                app->getFactoryManager()->createObjectFromType<ITextureForge>( "ITextureForge" );
            if( !forge )
            {
                throw std::runtime_error( "WPProcedural texture forge is unavailable." );
            }
            forge->setSeed( seed );
            SurfaceBakeParams params;
            params.size = quality == VehicleAppearanceQuality::Preview ? 256 : 512;
            auto roadMat = raceAsphalt( assets, forge, params, seed );
            auto grass = material( assets, { .28f, .43f, .16f, 1 }, 1 );
            auto grassMaps = forge->bakeSurface( SurfaceTag::Dirt, params );
            auto gravelMaps = forge->bakeSurface( SurfaceTag::Stone, params );
            for( size_t i = 0; i < grassMaps.albedo.pixels.size(); i += 4 )
            {
                const float variation = .72f + grassMaps.albedo.pixels[i] / 255.f * .55f;
                grassMaps.albedo.pixels[i] = u8( 82 * variation );
                grassMaps.albedo.pixels[i + 1] = u8( 112 * variation );
                grassMaps.albedo.pixels[i + 2] = u8( 48 * variation );
                grassMaps.albedo.pixels[i + 3] = 255;
                const float stone = .7f + gravelMaps.albedo.pixels[i] / 255.f * .6f;
                gravelMaps.albedo.pixels[i] = u8( 142 * stone );
                gravelMaps.albedo.pixels[i + 1] = u8( 127 * stone );
                gravelMaps.albedo.pixels[i + 2] = u8( 104 * stone );
            }
            maps( assets, grass, grassMaps.albedo, grassMaps.normal, grassMaps.orm );
            grass->setNormalStrength( .15f );
            grass->setUVProjection( 3 );
            auto red = material( assets, { .5f, .018f, .013f, 1 }, .72f );
            auto white = material( assets, { .8f, .8f, .72f, 1 }, .8f );
            auto gravel = material( assets, { .35f, .29f, .2f, 1 }, 1 );
            maps( assets, gravel, gravelMaps.albedo, gravelMaps.normal, gravelMaps.orm );
            gravel->setNormalStrength( .3f );
            gravel->setUVProjection( 3 );
            for( auto sampled : { roadMat, grass, gravel } )
            {
                sampled->setEditorUInt( "uvFilter", 3 );
                sampled->setEditorFloat( "uvAniso", 8.f );
                if( sampled != roadMat )
                {
                    sampled->setEditorFloat( "uvTriplanarScale", sampled == grass ? .08f : .25f );
                }
            }
            auto steel = material( assets, { .34f, .37f, .4f, 1 }, .45f, .7f );
            auto dark = material( assets, { .015f, .025f, .035f, 1 }, .55f, .35f );
            Geometry road, kerbRed, kerbWhite, lines, runoff, barriers;
            const auto &points = assets.circuit.samples;
            for( size_t i = 0; i < points.size(); ++i )
            {
                auto a = points[i], b = points[( i + 1 ) % points.size()];
                const auto endDistance = i + 1 == points.size() ? assets.circuit.length : b.distance;
                auto strip = [&]( Geometry &g, float left, float right, float y, float tile = 1.5f ) {
                    auto up = Vector3F( 0, y, 0 );
                    g.quad( a.position + a.right * left + up, a.position + a.right * right + up,
                            b.position + b.right * right + up, b.position + b.right * left + up,
                            { 0, 1, 0 }, left / tile, right / tile, a.distance / tile,
                            endDistance / tile );
                };
                strip( road, -6, 6, .008f );
                for( auto sign : { -1.f, 1.f } )
                {
                    strip( ( int( a.distance / 3 ) % 2 ) ? kerbRed : kerbWhite, sign * 6, sign * 6.85f,
                           .014f );
                    strip( lines, sign * 5.75f, sign * 5.9f, .012f );
                    strip( runoff, sign * 6.85f, sign * 11, .003f );
                    if( i % 6 == 0 )
                    {
                        auto p = a.position + a.right * ( sign * 13.f );
                        barriers.box( p + Vector3F( 0, .45f, 0 ), { .22f, .9f, .22f } );
                        staticBox( assets, "Guardrail post", p + Vector3F( 0, .45f, 0 ),
                                   { .22f, .9f, .22f } );
                        auto next = points[( i + 6 ) % points.size()].position +
                                    points[( i + 6 ) % points.size()].right * ( sign * 13.f );
                        railCollision( assets, p, next );
                        auto side = ( next - p );
                        side.normalise();
                        side = Vector3F( -side.z, 0, side.x ) * .1f;
                        barriers.quad( p - side + Vector3F( 0, .55f, 0 ),
                                       next - side + Vector3F( 0, .55f, 0 ),
                                       next + side + Vector3F( 0, .55f, 0 ),
                                       p + side + Vector3F( 0, .55f, 0 ), { 0, 1, 0 } );
                        barriers.quad( p + Vector3F( 0, .25f, 0 ), next + Vector3F( 0, .25f, 0 ),
                                       next + Vector3F( 0, .75f, 0 ), p + Vector3F( 0, .75f, 0 ),
                                       a.right * sign );
                    }
                }
            }
            mesh( assets, "Continuous circuit", road, roadMat );
            mesh( assets, "Red kerbs", kerbRed, red );
            mesh( assets, "White kerbs", kerbWhite, white );
            mesh( assets, "Edge markings", lines, white );
            mesh( assets, "Gravel runoff", runoff, gravel );
            mesh( assets, "Circuit guardrails", barriers, steel );
            Geometry turf;
            turf.quad( { -550, 0, -550 }, { 550, 0, -550 }, { 550, 0, 550 }, { -550, 0, 550 },
                       { 0, 1, 0 }, 0, 100, 0, 100 );
            mesh( assets, "Circuit meadow", turf, grass );
            Geometry checkerDark, checkerWhite;
            for( int x = 0; x < 12; ++x )
            {
                for( int z = 0; z < 2; ++z )
                {
                    ( ( x + z ) % 2 ? checkerDark : checkerWhite )
                        .quad( { float( x - 6 ), .02f, float( z ) },
                               { float( x - 5 ), .02f, float( z ) },
                               { float( x - 5 ), .02f, float( z + 1 ) },
                               { float( x - 6 ), .02f, float( z + 1 ) }, { 0, 1, 0 } );
                }
            }
            mesh( assets, "Start finish black", checkerDark, dark );
            mesh( assets, "Start finish white", checkerWhite, white );
            Geometry gantry, pits, pitApron, pitTrim, pitRoof, pitGlazing, pitLouvers;
            // Open galvanized truss: the braced structure reads at driving scale without
            // the opaque full-height pillars obscuring the venue and horizon.
            for( float x : { -8.f, 8.f } )
            {
                for( float dx : { -.2f, .2f } )
                {
                    for( float dz : { -.2f, .2f } )
                    {
                        gantry.beam( { x + dx, .15f, 3 + dz }, { x + dx, 5.95f, 3 + dz }, .045f );
                    }
                }
                for( int level = 0; level < 7; ++level )
                {
                    const float y = .15f + level * .8f, top = y + .8f;
                    for( float side : { -.2f, .2f } )
                    {
                        gantry.beam( { x - .2f, y, 3 + side }, { x + .2f, y, 3 + side }, .025f );
                        gantry.beam( { x + side, y, 2.8f }, { x + side, y, 3.2f }, .025f );
                        const float start = level % 2 ? -.2f : .2f;
                        gantry.beam( { x + start, y, 3 + side }, { x - start, top, 3 + side }, .022f );
                        gantry.beam( { x + side, y, 3 + start }, { x + side, top, 3 - start }, .022f );
                    }
                }
            }
            for( float y : { 5.25f, 5.95f } )
            {
                for( float z : { 2.8f, 3.2f } )
                {
                    gantry.beam( { -8.2f, y, z }, { 8.2f, y, z }, .045f );
                }
            }
            for( int panel = 0; panel <= 10; ++panel )
            {
                const float x = -8 + panel * 1.6f;
                for( float z : { 2.8f, 3.2f } )
                {
                    gantry.beam( { x, 5.25f, z }, { x, 5.95f, z }, .025f );
                    if( panel < 10 )
                    {
                        gantry.beam( { x, panel % 2 ? 5.95f : 5.25f, z },
                                     { x + 1.6f, panel % 2 ? 5.25f : 5.95f, z }, .025f );
                    }
                }
                gantry.beam( { x, 5.95f, 2.8f }, { x, 5.95f, 3.2f }, .025f );
            }
            staticBox( assets, "Gantry left post", { -8, 3, 3 }, { .6f, 6, .6f } );
            staticBox( assets, "Gantry right post", { 8, 3, 3 }, { .6f, 6, .6f } );
            staticBox( assets, "Gantry overhead beam", { 0, 5.6f, 3 }, { 16.5f, 1, .5f } );
            mesh( assets, "Start gantry", gantry, steel );
            // A continuous service apron grounds the garages and avoids six isolated boxes.
            pitApron.quad( { -35, .006f, -68 }, { -14, .006f, -68 }, { -14, .006f, -6 },
                           { -35, .006f, -6 }, { 0, 1, 0 }, 0, 14, 0, 41.333f );
            mesh( assets, "Pit service apron", pitApron, roadMat );
            for( int i = 0; i < 6; ++i )
            {
                const float z = float( -12 - i * 10 );
                // Shallow roof, fascia and mullions give readable scale at chase distance.
                pitRoof.box( { -28, 5.12f, z }, { 12.5f, .24f, 8.5f } );
                pitTrim.box( { -21.86f, 4.55f, z }, { .18f, .28f, 8 } );
                pitGlazing.box( { -21.88f, 4.08f, z }, { .12f, .55f, 6 } );
                for( int mullion = -1; mullion <= 1; ++mullion )
                {
                    pitTrim.box( { -21.78f, 4.08f, z + mullion * 2.f }, { .14f, .65f, .08f } );
                }
                for( int slat = 1; slat < 10; ++slat )
                {
                    pitLouvers.box( { -21.81f, slat * .32f, z }, { .08f, .035f, 6 } );
                }
                pits.box( { -28, 2.5f, float( -12 - i * 10 ) }, { 12, 5, 8 } );
                staticBox( assets, "Pit garage", { -28, 2.5f, float( -12 - i * 10 ) }, { 12, 5, 8 } );
            }
            mesh( assets, "Pit garages", pits, white );
            Geometry pitDoors;
            for( int i = 0; i < 6; ++i )
            {
                pitDoors.box( { -21.9f, 1.7f, float( -12 - i * 10 ) }, { .12f, 3.4f, 6 } );
                staticBox( assets, "Pit door", { -21.9f, 1.7f, float( -12 - i * 10 ) },
                           { .12f, 3.4f, 6 } );
            }
            mesh( assets, "Pit doors", pitDoors, dark );
            mesh( assets, "Pit roof caps", pitRoof, steel );
            mesh( assets, "Pit fascia and mullions", pitTrim, red );
            mesh( assets, "Pit door slats", pitLouvers, steel );
            mesh( assets, "Pit clerestory glazing", pitGlazing,
                  material( assets, { .045f, .10f, .13f, 1 }, .24f, .25f ) );
            Geometry hills;
            if( assets.useFoliageWorld )
            {
                buildFoliageWorld( assets, seed, quality );
            }
            else
            {
                // Batch trees into 64-metre patches. The shared data-oriented LODSystem
                // chooses one renderer set per patch, without per-tree update callbacks.
                std::mt19937 rng( seed );
                std::uniform_real_distribution<float> unit( 0, 1 );

                struct TreePatch
                {
                    Geometry trunks, leaves, imposters;
                    Array<scene::LODDetailBound> detailBounds{ Allocator<scene::LODDetailBound>( sourceBudget ) };
                };

                SourceBudgetMap<std::pair<int, int>, TreePatch> patches;
                const std::array<MeshImposterAtlas, 4> atlases = { bakePineImposter( 0 ),
                                                                   bakePineImposter( 1 ),
                                                                   bakePineImposter( 2 ),
                                                                   bakePineImposter( 3 ) };
                const auto &atlas = atlases[0];
                // All four near silhouettes have matching far views in one twelve-
                // column atlas. Keep the existing one-material patch batching.
                auto treePixels = makeSourceBudgetSnapshot<u8>();
                const auto treeWidth = atlas.width * 4;
                treePixels.reserve( size_t( treeWidth ) * atlas.height * 4 );
                treePixels.resize( size_t( treeWidth ) * atlas.height * 4 );
                for( u32 variant = 0; variant < atlases.size(); ++variant )
                {
                    for( u32 y = 0; y < atlas.height; ++y )
                    {
                        std::copy_n(
                            atlases[variant].rgba.begin() + size_t( y ) * atlas.width * 4,
                            size_t( atlas.width ) * 4,
                            treePixels.begin() +
                                ( size_t( y ) * treeWidth + variant * atlas.width ) * 4 );
                    }
                }
                auto imposterMaterial = material( assets, ColourF::White, 1 );
                auto imposterTexture =
                    uploadPixels( assets, treePixels.data(), treePixels.size(), treeWidth, atlas.height,
                                  render::TextureMipFilter::Cutout, 12 );
                imposterMaterial->setTexture( imposterTexture, 0 );
                // Colour is already lit by the CPU bake. Keep its alpha mask in the
                // albedo slot and supply baked colour through the PBS emission slot.
                imposterMaterial->setTexture( imposterTexture, 13 );
                imposterMaterial->setDiffuse( ColourF::Black );
                imposterMaterial->setEmissive( ColourF::White );
                imposterMaterial->setEmissionEnabled( true );
                imposterMaterial->setCutout( true );
                imposterMaterial->setAlphaClip( .5f );
                imposterMaterial->setTransparent( false );
                imposterMaterial->setDepthWrite( true );
                // A two-texel palette preserves both surface colours in one opaque draw.
                // No mip reduction: these are categorical entries, not a continuous image.
                TextureBuffer palette( 2, 1 );
                palette.pixels = { 31, 14, 5, 255, 46, 82, 33, 255 };
                auto nearTreeMaterial = material( assets, ColourF::White, 1 );
                // IMaterial mode 1 maps to WORKPHONE_CULL_MODE_NONE in ClawUtil.
                // Open needle sprigs and far cards remain visible from both sides.
                nearTreeMaterial->setCullMode( 1 );
                nearTreeMaterial->setTexture( upload( assets, palette, render::TextureMipFilter::None ),
                                              0 );
                int treeCount = quality == VehicleAppearanceQuality::Preview ? 90 : 220;
                for( int i = 0; i < treeCount; ++i )
                {
                    Vector3F p( -100 + unit( rng ) * 380, 0, -250 + unit( rng ) * 440 );
                    auto nearest = assets.circuit.nearest( p );
                    if( ( p - points[nearest].position ).length() < 22 ||
                        ( p.x < -15 && p.z > -85 && p.z < 15 ) )
                    {
                        continue;
                    }
                    float h = 4 + unit( rng ) * 7;
                    staticBox( assets, "Tree trunk", p + Vector3F( 0, h * .3f, 0 ),
                               { h * .05f, h * .6f, h * .05f } );
                    const auto key =
                        std::make_pair( int( std::floor( p.x / 64 ) ), int( std::floor( p.z / 64 ) ) );
                    const Vector3F centre( key.first * 64.f + 32, 0, key.second * 64.f + 32 );
                    auto &patch = patches[key];
                    patch.detailBounds.push_back( { p - centre + Vector3F( 0, h * .5f, 0 ), h } );
                    const u32 variant = ( seed ^ ( u32( i ) * 104729u ) ) % 4u;
                    pineGeometry( patch.trunks, patch.leaves, p - centre, h, variant );
                    pineImposterGeometry( patch.imposters, atlases[variant], p - centre, h, variant );
                }
                for( const auto &[key, patch] : patches )
                {
                    reserveOwnerSlot(assets.actors);
                    auto root = app->getGameManager()->createActor();
                    if(!root) throw std::runtime_error("Cannot create procedural actor");
                    assets.actors.push_back(root);
                    root->setName( "Tree LOD patch" );
                    root->setPosition( { key.first * 64.f + 32, 0, key.second * 64.f + 32 } );
                    app->getGameManager()->getCurrentScene()->addActor( root );
                    Geometry nearTrees;
                    auto append = [&]( const Geometry &source, float paletteU ) {
                        const auto base = static_cast<u32>( nearTrees.positions.size() );
                        nearTrees.positions.insert( nearTrees.positions.end(), source.positions.begin(),
                                                    source.positions.end() );
                        nearTrees.normals.insert( nearTrees.normals.end(), source.normals.begin(),
                                                  source.normals.end() );
                        nearTrees.tangents.insert( nearTrees.tangents.end(), source.tangents.begin(),
                                                   source.tangents.end() );
                        nearTrees.uv.insert( nearTrees.uv.end(), source.positions.size(),
                                             Vector3F( paletteU, .5f, 0 ) );
                        for( auto index : source.indices )
                        {
                            nearTrees.indices.push_back( base + index );
                        }
                    };
                    append( patch.trunks, .25f );
                    append( patch.leaves, .75f );
                    auto trees = mesh( assets, "LOD0 pine batch", nearTrees, nearTreeMaterial, root );
                    auto cards =
                        mesh( assets, "LOD1 pine imposters", patch.imposters, imposterMaterial, root );
                    reserveOwnerSlot(assets.treeLODs);
                    auto lod = root->addComponent<scene::LODGroup>();
                    lod->setLocalReferencePoint( { 0, 3.5f, 0 } );
                    lod->setSize( 7 );
                    lod->setDetailBounds( patch.detailBounds );
                    lod->setHysteresis( .15f );
                    lod->setCullBelowLastLOD( false );
                    lod->addLevel( .085f, { trees->getComponent<scene::MeshRenderer>() } );
                    lod->addLevel( 0, { cards->getComponent<scene::MeshRenderer>() } );
                    String error;
                    if( !lod->validate( &error ) )
                    {
                        throw std::runtime_error( "Invalid generated tree LOD group: " + error );
                    }
                    assets.treeLODs.push_back( lod );
                }
            }
            // Keep the track and all authored tree positions on the flat contact plane.
            // The ridge returns to the ground before the square mesh boundary: a raised
            // outer tile edge otherwise becomes an artificial straight skyline under fog.
            const auto height = [seed]( float x, float z ) {
                const float radius = std::hypot( x - 80, z + 25 );
                const float t = std::clamp( ( radius - 300 ) / 140, 0.f, 1.f );
                const float outer = std::clamp( ( 600 - radius ) / 160, 0.f, 1.f );
                const float phase = float( seed % 1000 ) * .073f;
                return -.15f +
                       t * t * ( 3 - 2 * t ) * outer * outer * ( 3 - 2 * outer ) *
                           ( 32 + 11 * std::sin( x * .015f + phase ) +
                             7 * std::cos( z * .020f - phase ) + 4 * std::sin( ( x + z ) * .037f ) );
            };
            const int grid = quality == VehicleAppearanceQuality::Preview ? 32 : 96;
            for( int z = 0; z <= grid; ++z )
            {
                for( int x = 0; x <= grid; ++x )
                {
                    const float px = 80 - 640 + x * 1280.f / grid;
                    const float pz = -25 - 640 + z * 1280.f / grid;
                    hills.positions.push_back( { px, height( px, pz ), pz } );
                    auto normal = Vector3F( height( px - 2, pz ) - height( px + 2, pz ), 4,
                                            height( px, pz - 2 ) - height( px, pz + 2 ) );
                    normal.normalise();
                    hills.normals.push_back( normal );
                    hills.tangents.push_back( { 1, 0, 0, 1 } );
                    hills.uv.push_back( { px * .08f, pz * .08f, 0 } );
                    if( x < grid && z < grid )
                    {
                        const float step = 1280.f / grid;
                        const float radius = std::hypot( px + step * .5f - 80, pz + step * .5f + 25 );
                        const float halfDiagonal = step * .707107f;
                        if( radius + halfDiagonal <= 300 || radius - halfDiagonal >= 600 )
                        {
                            continue;
                        }
                        const u32 a = z * ( grid + 1 ) + x, b = a + 1;
                        const u32 c = a + grid + 1, d = c + 1;
                        for( auto index : { a, c, d, a, d, b } )
                        {
                            hills.indices.push_back( index );
                        }
                    }
                }
            }
            mesh( assets, "Distant terrain", hills, grass );
        }

        void buildFoliageWorld( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality )
        {
            auto app = core::IApplicationManager::instance();
            auto scene = app->getGraphicsSystem()->getGraphicsScene();
            // Reuse the terrain attachment consumed by ClawScene, including its page stream,
            // per-view instance bins, wind, shadow LOD and publication diagnostics.
            // Population transforms below are world-space. Keep the existing
            // identity carrier so a posed terrain cannot transform them twice.
            auto carrier = scene->addGraphicsObjectByType<render::IGraphicsTerrain>();
            assets.foliageTerrain = dynamic_pointer_cast<render::Terrain>( carrier );
            if( !assets.foliageTerrain )
            {
                if( carrier )
                {
                    scene->removeGraphicsObject( carrier );
                }
                throw std::runtime_error( "Graphics backend lacks the foliage terrain attachment." );
            }
            render::TerrainData ground;
            ground.origin = { -550, -550 };
            ground.spacing = { 1100, 1100 };
            ground.holes = { 1 };  // Existing meadow/collision own the ground; no duplicate surface.
            String error;
            if( !assets.foliageTerrain->applyTerrainData( ground, error ) )
            {
                throw std::runtime_error( "Cannot prepare foliage attachment: " + error );
            }
            // Each geometry card represents a small branching spray, not one giant leaf.
            // Seeded individual leaves, interior gaps and twigs retain a broken silhouette
            // through the existing alpha-coverage mip chain.
            constexpr u32 atlasSize = 256;
            TextureBuffer leafAtlas( atlasSize, atlasSize );
            const auto randomLeaf = [seed]( u64 id ) {
                return float( render::foliageStableHash( u64( seed ) ^ 0x4c454146u ^ id ) >> 40 ) /
                       16777216.f;
            };
            const auto ellipse = [&]( float cx, float cy, float rx, float ry, float angle,
                                      float variation ) {
                const float cs = std::cos( angle ), sn = std::sin( angle );
                const float extent = std::max( rx, ry );
                const int minX = std::max( 0, int( ( cx - extent ) * atlasSize ) - 1 );
                const int maxX =
                    std::min( int( atlasSize ) - 1, int( ( cx + extent ) * atlasSize ) + 1 );
                const int minY = std::max( 0, int( ( cy - extent ) * atlasSize ) - 1 );
                const int maxY =
                    std::min( int( atlasSize ) - 1, int( ( cy + extent ) * atlasSize ) + 1 );
                for( int y = minY; y <= maxY; ++y )
                {
                    for( int x = minX; x <= maxX; ++x )
                    {
                        const float dx = ( x + .5f ) / atlasSize - cx;
                        const float dy = ( y + .5f ) / atlasSize - cy;
                        const float u = ( cs * dx + sn * dy ) / rx;
                        const float v = ( -sn * dx + cs * dy ) / ry;
                        const float edge = u * u + v * v;
                        // Small asymmetric serrations avoid a repeating smooth oval outline.
                        const float serration = .94f + .06f * std::cos( u * 24.f );
                        if( edge > serration )
                        {
                            continue;
                        }
                        const float vein = std::abs( v ) < .08f ? 1.13f : 1.f;
                        const float light = ( .68f + .32f * ( 1 - edge ) ) * variation * vein;
                        const auto pixel = ( size_t( y ) * atlasSize + x ) * 4;
                        leafAtlas.pixels[pixel] = u8( 65 * light );
                        leafAtlas.pixels[pixel + 1] = u8( 118 * light );
                        leafAtlas.pixels[pixel + 2] = u8( 34 * light );
                        leafAtlas.pixels[pixel + 3] = 255;
                    }
                }
            };
            const auto twig = [&]( Vector2F start, Vector2F end, float radius ) {
                const auto delta = end - start;
                const float squared = delta.dotProduct( delta );
                for( u32 y = 0; y < atlasSize; ++y )
                {
                    for( u32 x = 0; x < atlasSize; ++x )
                    {
                        const Vector2F p( ( x + .5f ) / atlasSize, ( y + .5f ) / atlasSize );
                        const float t =
                            std::clamp( ( p - start ).dotProduct( delta ) / squared, 0.f, 1.f );
                        if( ( p - ( start + delta * t ) ).lengthSquared() > radius * radius )
                        {
                            continue;
                        }
                        const auto pixel = ( size_t( y ) * atlasSize + x ) * 4;
                        leafAtlas.pixels[pixel] = 74;
                        leafAtlas.pixels[pixel + 1] = 69;
                        leafAtlas.pixels[pixel + 2] = 29;
                        leafAtlas.pixels[pixel + 3] = 255;
                    }
                }
            };
            twig( { .5f, .96f }, { .5f, .08f }, .004f );
            for( u32 branch = 0; branch < 7; ++branch )
            {
                for( int side : { -1, 1 } )
                {
                    const Vector2F start( .5f, .22f + branch * .105f );
                    const Vector2F end( .5f + side * ( .26f + .08f * randomLeaf( branch + 71 ) ),
                                        start.y - .13f );
                    twig( start, end, .003f );
                    for( u32 leaf = 0; leaf < 7; ++leaf )
                    {
                        for( int pair : { -1, 1 } )
                        {
                            const u64 id = branch * 113 + leaf * 7 + ( side + 1 ) * 19 + pair + 1;
                            const float t = .13f + leaf * .125f;
                            const auto centre = start + ( end - start ) * t;
                            ellipse(
                                centre.x + pair * .012f,
                                centre.y + pair * ( .018f + .008f * randomLeaf( id + 701 ) ),
                                .029f + .015f * randomLeaf( id ), .013f + .008f * randomLeaf( id + 313 ),
                                side * ( -.45f + pair * .65f ), .8f + .4f * randomLeaf( id + 991 ) );
                        }
                    }
                }
            }
            const String leafTextureName =
                assets.resourcePrefix + "/texture/" + StringUtil::toString( assets.textures.size() );
            auto leafTexture = upload( assets, leafAtlas, render::TextureMipFilter::Cutout );
            // Use the registered resource identity, not a potentially empty display name
            // returned after a texture property round-trip. An empty albedo is otherwise
            // valid foliage data and silently produces opaque white leaf cards.
            leafTexture->setName( leafTextureName );
            auto registeredLeaf =
                app->getGraphicsSystem()->getTextureManager()->getByName( leafTextureName );
            if( !registeredLeaf || registeredLeaf.get() != leafTexture.get() )
            {
                throw std::runtime_error(
                    "Generated foliage atlas lost its registered texture identity." );
            }
            const auto city = assets.cityTerrainRoads ? assets.cityLayout : assets.openCity
                                  ? OpenCityLayout::generate( seed, assets.cityBlocks, assets.cityRoute, assets.cityNaturalRoads )
                                  : OpenCityLayout{};
            auto world = std::make_shared<render::FoliageWorldData>();
            world->revision = 1;
            // The bounded city overview needs a longer tree view range. Larger city
            // pages keep that range inside the existing 30-page-width pager gate.
            if( assets.openCity ) world->pageSize = 128;
            for( u32 variant = 0; variant < 4; ++variant )
            {
                FoliagePlantRecipe recipe;
                recipe.speciesId =
                    assets.resourcePrefix + "/broadleaf/" + StringUtil::toString( variant );
                recipe.seed = seed ^ ( 7919u + variant * 104729u );
                recipe.height = 6 + variant;
                recipe.radius = 2.6f + variant * .3f;
                recipe.branches = 36 + variant * 4;
                recipe.bark.tint = { .16f, .085f, .035f, 1 };
                recipe.leaves.albedo = leafTextureName;
                recipe.wind.amplitude = .08f;
                recipe.wind.frequency = .65f;
                recipe.wind.flutter = .025f;
                recipe.wind.bendStart = 1.5f;
                recipe.wind.bendEnd = recipe.height;
                auto species = std::make_shared<render::FoliageSpecies>();
                if( !generateFoliagePlant( recipe, *species, error ) )
                {
                    throw std::runtime_error( "Cannot generate racing foliage: " + error );
                }
                if( species->materials.size() != 2 || species->materials[1].albedo != leafTextureName )
                {
                    throw std::runtime_error( "Generated foliage species lost its leaf cutout atlas." );
                }
                species->maximumDistance = assets.openCity ? std::max( 600.f, city.extent * 5.f ) : 600.f;
                species->shadowDistance = 120;
                world->species.push_back( species );
                for( const auto &lod : species->lods )
                {
                    assets.triangles += lod.indices.size() / 3;
                }
            }
            std::mt19937 random( seed );
            std::uniform_real_distribution<float> unit( 0, 1 );
            const int count = quality == VehicleAppearanceQuality::Preview ? 90 : 220;
            SourceBudgetVector<size_t> cityParks;
            if( assets.openCity )
            {
                cityParks.reserve( size_t( std::count_if( city.lots.begin(), city.lots.end(),
                    []( const auto &lot ) { return lot.park; } ) ) );
                for( size_t index = 0; index < city.lots.size(); ++index )
                    if( city.lots[index].park ) cityParks.push_back( index );
                // A bounded preview must not always populate only the first city rows.
                // Every park receives a candidate before another round is distributed.
                std::sort( cityParks.begin(), cityParks.end(), [&]( size_t a, size_t b ) {
                    const auto ha = render::foliageStableHash( city.lots[a].id );
                    const auto hb = render::foliageStableHash( city.lots[b].id );
                    return ha != hb ? ha < hb : city.lots[a].id < city.lots[b].id;
                } );
            }
            for( int i = 0; i < count; ++i )
            {
                Vector3F p( -100 + unit( random ) * 380, 0, -250 + unit( random ) * 440 );
                u64 cityPlacementId = 0;
                if( assets.openCity )
                {
                    if( cityParks.empty() )
                    {
                        continue;
                    }
                    const auto &lot = city.lots[cityParks[size_t( i ) % cityParks.size()]];
                    const auto round = u64( size_t( i ) / cityParks.size() );
                    // Mix the complete parcel identity with an independent round domain.
                    const u64 key = render::foliageStableHash( lot.id ) ^
                        ( ( round + 1 ) * 0x9e3779b97f4a7c15ull );
                    cityPlacementId = render::foliageStableHash( key );
                    if( !cityPlacementId ) cityPlacementId = key;
                    const auto scalar = []( u64 value ) {
                        return float( render::foliageStableHash( value ) >> 40 ) / 16777216.f;
                    };
                    // The 90/220 candidate tiers fit one 16x16 cell permutation per park.
                    // Distinct rounds occupy distinct cells; hashes jitter within each cell.
                    // Fixed cells keep retained candidates stable across quality presets.
                    const auto cell = u32( round * 73 + render::foliageStableHash( lot.id ) ) & 255u;
                    const float u = ( float( cell & 15u ) + .2f + .6f * scalar( key ^ 0xa341316cu ) ) / 16;
                    const float v = ( float( cell >> 4 ) + .2f + .6f * scalar( key ^ 0xc8013ea4u ) ) / 16;
                    p = { lot.parcelX + ( u - .5f ) * std::max( 0.f, lot.parcelWidth - 4 ), lot.groundHeight + .16f,
                          lot.parcelZ + ( v - .5f ) * std::max( 0.f, lot.parcelDepth - 4 ) };
                }
                else if( ( p - assets.circuit.samples[assets.circuit.nearest( p )].position ).length() <
                             22 ||
                         ( p.x < -15 && p.z > -85 && p.z < 15 ) )
                {
                    continue;
                }
                render::FoliageInstance instance;
                instance.id = cityPlacementId ? cityPlacementId :
                    render::foliageStableHash( ( u64( seed ) << 32 ) | u32( i + 1 ) );
                if( !instance.id )
                {
                    instance.id = u64( i + 1 );
                }
                instance.species = u32( i ) % 4;
                instance.seed = u32( render::foliageStableHash( instance.id ) );
                instance.position = p;
                instance.rotation = QuaternionF::eulerDegrees( 0, unit( random ) * 360, 0 );
                const float scale = .75f + unit( random ) * .5f;
                instance.scale = { scale, scale, scale };
                const float tint = .85f + unit( random ) * .15f;
                instance.tint = { tint, tint, tint, 1 };
                world->instances.push_back( instance );
                const float h = ( 6 + instance.species ) * scale;
                staticBox( assets, "Foliage world trunk", p + Vector3F( 0, h * .3f, 0 ),
                           { .24f * scale, h * .6f, .24f * scale } );
            }
            if( !render::validateFoliageWorld( *world, error ) ||
                !assets.foliageTerrain->setFoliageSnapshot( world, error ) )
            {
                throw std::runtime_error( "Cannot publish racing foliage: " + error );
            }
        }

        void buildCityStreetDressing( SceneAssets &assets, const OpenCityLayout &city,
                                     VehicleAppearanceQuality quality )
        {
            Geometry stone, metal, wood, dark, planting, joints;
            const bool preview = quality == VehicleAppearanceQuality::Preview;
            const auto horizontal = []( Geometry &g, Vector3F p, float width, float depth ) {
                const auto h = Vector3F( width * .5f, 0, depth * .5f );
                g.quad( p + Vector3F( -h.x, 0, -h.z ), p + Vector3F( h.x, 0, -h.z ),
                        p + Vector3F( h.x, 0, h.z ), p + Vector3F( -h.x, 0, h.z ), { 0, 1, 0 } );
            };
            const auto axisBox = []( Geometry &g, Vector3F p, float width, float height,
                                     float depth, bool xFace ) {
                g.box( p, xFace ? Vector3F( depth, height, width ) : Vector3F( width, height, depth ) );
            };
            const auto normalFor = []( OpenCityLayout::Frontage frontage ) {
                switch( frontage )
                {
                case OpenCityLayout::Frontage::West: return Vector3F( -1, 0, 0 );
                case OpenCityLayout::Frontage::East: return Vector3F( 1, 0, 0 );
                case OpenCityLayout::Frontage::South: return Vector3F( 0, 0, 1 );
                default: return Vector3F( 0, 0, -1 );
                }
            };
            SourceBudgetVector<const OpenCityLayout::Lot *> parkLots, frontages;
            parkLots.reserve( city.lots.size() ); frontages.reserve( city.lots.size() );
            for( const auto &lot : city.lots )
            {
                if( !lot.park ) { frontages.push_back( &lot ); continue; }
                parkLots.push_back( &lot );
                const float hx = lot.parcelWidth * .5f, hz = lot.parcelDepth * .5f;
                const Vector3F centre( lot.parcelX, lot.groundHeight + .166f, lot.parcelZ );
                // The perimeter loop stays inside the two-metre tree exclusion margin.
                // Corners belong to the transverse strips only, avoiding coplanar overlaps.
                for( int side : { -1, 1 } )
                {
                    horizontal( stone, centre + Vector3F( 0, 0, side * ( hz - 1.4f ) ),
                                lot.parcelWidth - 1.6f, 1.2f );
                    horizontal( stone, centre + Vector3F( side * ( hx - 1.4f ), 0, 0 ),
                                1.2f, lot.parcelDepth - 4.f );
                    horizontal( stone, centre + Vector3F( 0, 0, side * ( hz - .56f ) ),
                                lot.parcelWidth - 1.f, .12f );
                    horizontal( stone, centre + Vector3F( side * ( hx - .56f ), 0, 0 ),
                                .12f, lot.parcelDepth - 1.24f );
                    horizontal( dark, centre + Vector3F( 0, .001f, side * ( hz - 2.f ) ),
                                lot.parcelWidth - 4.f, .025f );
                    horizontal( dark, centre + Vector3F( side * ( hx - 2.f ), .001f, 0 ),
                                .025f, lot.parcelDepth - 4.f );
                }
                if( !city.naturalRoads && !assets.cityTerrainRoads )
                {
                    const auto normal = normalFor( lot.frontage );
                    const auto tangent = Vector3F( normal.z, 0, -normal.x );
                    const bool xFace = std::abs( normal.x ) > .5f;
                    const auto front = centre + normal * ( xFace ? hx : hz );
                    auto inner = front - normal * .8f, outer = front + normal * 2.5f;
                inner.y = .168f; outer.y = .156f;
                    stone.quad( inner - tangent * .6f, outer - tangent * .6f,
                                outer + tangent * .6f, inner + tangent * .6f, { 0, 1, 0 } );
                }
            }
            const auto nearest = []( const auto *a, const auto *b ) {
                const float da = a->x * a->x + a->z * a->z, db = b->x * b->x + b->z * b->z;
                return da == db ? a->id < b->id : da < db;
            };
            std::sort( parkLots.begin(), parkLots.end(), nearest );
            std::sort( frontages.begin(), frontages.end(), nearest );
            const size_t benchCount = std::min<size_t>( parkLots.size(), preview ? 8 : 16 );
            for( size_t i = 0; i < benchCount; ++i )
            {
                const auto &lot = *parkLots[i];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.parcelWidth : lot.parcelDepth ) * .5f;
                const float span = xFace ? lot.parcelDepth : lot.parcelWidth;
                const auto p = Vector3F( lot.parcelX, lot.groundHeight + .15f, lot.parcelZ ) + normal * ( half + ( city.naturalRoads || assets.cityTerrainRoads ? -.8f : 1.1f ) ) -
                    tangent * std::min( 6.f, span * .25f );
                for( int plank = -2; plank <= 2; ++plank )
                    axisBox( wood, p + Vector3F( 0, .46f, 0 ) + normal * ( plank * .085f ),
                             1.8f, .055f, .065f, xFace );
                for( int row = 0; row < 3; ++row )
                    axisBox( wood, p + Vector3F( 0, .70f + row * .13f, 0 ) - normal * .21f,
                             1.8f, .08f, .045f, xFace );
                for( int side : { -1, 1 } )
                {
                    axisBox( metal, p + tangent * ( side * .64f ) + Vector3F( 0, .215f, 0 ),
                             .075f, .43f, .43f, xFace );
                    metal.beam( p + tangent * ( side * .64f ) - normal * .22f + Vector3F( 0, .22f, 0 ),
                                p + tangent * ( side * .64f ) - normal * .22f + Vector3F( 0, .98f, 0 ), .035f );
                }
                staticBox( assets, "City bench", p + Vector3F( 0, .515f, 0 ),
                    xFace ? Vector3F( .55f, 1.03f, 1.84f ) : Vector3F( 1.84f, 1.03f, .55f ) );
            }
            const size_t planterCount = city.naturalRoads || assets.cityTerrainRoads ? 0 : std::min<size_t>( frontages.size(), preview ? 8 : 16 );
            for( size_t i = 0; i < planterCount; ++i )
            {
                const auto &lot = *frontages[i];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.width : lot.depth ) * .5f;
                const float span = xFace ? lot.depth : lot.width;
                const auto p = Vector3F( lot.x, .15f, lot.z ) + normal * ( half + 1.15f ) + tangent * ( span * .35f );
                for( int side : { -1, 1 } )
                {
                    axisBox( stone, p + normal * ( side * .31f ) + Vector3F( 0, .275f, 0 ),
                             1.6f, .55f, .08f, xFace );
                    axisBox( stone, p + tangent * ( side * .76f ) + Vector3F( 0, .275f, 0 ),
                             .08f, .55f, .54f, xFace );
                }
                const auto soil = p + Vector3F( 0, .49f, 0 );
                horizontal( dark, soil, xFace ? .54f : 1.44f, xFace ? 1.44f : .54f );
                for( int blade = 0; blade < ( preview ? 10 : 18 ); ++blade )
                {
                    const float angle = blade * 2.39996323f + float( lot.variant % 127 ) * .07f;
                    const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                    const auto across = Vector3F( direction.z, 0, -direction.x );
                    const auto base = soil + direction * .045f;
                    const float height = .4f + .25f * float( ( blade * 19 + lot.variant ) % 31 ) / 30;
                    for( int segment = 0; segment < 3; ++segment )
                    {
                        const float a = segment / 3.f, b = ( segment + 1 ) / 3.f;
                        const auto lower = base + direction * ( .13f * a * a ) + Vector3F( 0, height * a, 0 );
                        const auto upper = base + direction * ( .13f * b * b ) + Vector3F( 0, height * b, 0 );
                        const auto aWidth = across * ( .023f * ( 1 - .9f * a ) );
                        const auto bWidth = across * ( .023f * ( 1 - .9f * b ) );
                        auto faceNormal = ( lower + aWidth - ( lower - aWidth ) ).crossProduct( upper + bWidth - ( lower - aWidth ) );
                        faceNormal.normalise();
                        planting.quad( lower - aWidth, lower + aWidth, upper + bWidth, upper - bWidth, faceNormal );
                    }
                }
                staticBox( assets, "City planter", p + Vector3F( 0, .275f, 0 ),
                    xFace ? Vector3F( .7f, .55f, 1.6f ) : Vector3F( 1.6f, .55f, .7f ) );
            }
            const size_t binCount = city.naturalRoads || assets.cityTerrainRoads ? 0 : std::min<size_t>( frontages.size(), preview ? 4 : 8 );
            for( size_t i = 0; i < binCount; ++i )
            {
                const auto &lot = *frontages[( i * 3 + 1 ) % frontages.size()];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.width : lot.depth ) * .5f;
                const float span = xFace ? lot.depth : lot.width;
                const auto p = Vector3F( lot.x, .15f, lot.z ) + normal * ( half + 1.1f ) - tangent * ( span * .35f );
                metal.box( p + Vector3F( 0, .45f, 0 ), { .42f, .9f, .42f } );
                metal.box( p + Vector3F( 0, .93f, 0 ), { .47f, .06f, .47f } );
                axisBox( dark, p + Vector3F( 0, .75f, 0 ) + normal * .214f, .26f, .10f, .005f, xFace );
                staticBox( assets, "City waste bin", p + Vector3F( 0, .48f, 0 ), { .48f, .96f, .48f } );
            }
            const auto street = OpenCityStreetDetails::generate( city, 0, false );
            SourceBudgetVector<const OpenCityStreetDetails::Corner *> walks;
            walks.reserve( street.sidewalks.size() );
            for( const auto &walk : street.sidewalks ) walks.push_back( &walk );
            std::sort( walks.begin(), walks.end(), []( const auto *a, const auto *b ) {
                return a->x * a->x + a->z * a->z < b->x * b->x + b->z * b->z;
            } );
            const size_t detailedWalks = assets.cityTerrainRoads ? 0 : std::min<size_t>( walks.size(), preview ? 32 : 64 );
            for( size_t i = 0; i < detailedWalks; ++i )
            {
                const auto &walk = *walks[i];
                const bool horizontalWalk = walk.width > walk.depth;
                const float length = horizontalWalk ? walk.width : walk.depth;
                const float step = preview ? 4.8f : 2.4f;
                for( float t = -length * .5f + step; t < length * .5f - .2f; t += step )
                    horizontal( joints, { walk.x + ( horizontalWalk ? t : 0 ), .156f,
                        walk.z + ( horizontalWalk ? 0 : t ) }, horizontalWalk ? .025f : walk.width,
                        horizontalWalk ? walk.depth : .025f );
                horizontal( joints, { walk.x, .156f, walk.z },
                    horizontalWalk ? length : .025f, horizontalWalk ? .025f : length );
                if( i % 3 != 0 ) continue;
                const Vector3F centre( walk.x, .158f, walk.z );
                const auto vertex = [&]( Vector3F p, Vector3F uv ) {
                    const auto index = u32( metal.positions.size() );
                    metal.positions.push_back( p ); metal.normals.push_back( { 0, 1, 0 } );
                    metal.tangents.push_back( { 1, 0, 0, 1 } ); metal.uv.push_back( uv );
                    return index;
                };
                const auto c = vertex( centre, { .5f, .5f, 0 } );
                const auto first = u32( metal.positions.size() );
                constexpr int sides = 20;
                for( int segment = 0; segment <= sides; ++segment )
                {
                    const float angle = segment * 2 * pi / sides;
                    vertex( centre + Vector3F( std::cos( angle ) * .31f, 0, std::sin( angle ) * .31f ),
                        { .5f + .5f * std::cos( angle ), .5f + .5f * std::sin( angle ), 0 } );
                }
                for( int segment = 0; segment < sides; ++segment )
                { metal.indices.push_back( c ); metal.indices.push_back( first + segment + 1 ); metal.indices.push_back( first + segment ); }
                for( int groove = -2; groove <= 2; ++groove )
                {
                    const float z = groove * .09f;
                    horizontal( dark, centre + Vector3F( 0, .001f, z ),
                        2 * std::sqrt( .31f * .31f - z * z ) - .04f, .012f );
                }
            }
            mesh( assets, "City park paving and planter rims", stone, material( assets, { .49f, .48f, .42f, 1 }, .9f ) );
            mesh( assets, "City bench frames bins and utility covers", metal, material( assets, { .12f, .15f, .16f, 1 }, .62f, .55f ) );
            mesh( assets, "City bench timber", wood, material( assets, { .36f, .21f, .10f, 1 }, .8f ) );
            mesh( assets, "City paving joints and planter soil", dark, material( assets, { .12f, .14f, .11f, 1 }, 1 ) );
            mesh( assets, "City concrete paving joints", joints, material( assets, { .31f, .32f, .30f, 1 }, 1 ) );
            mesh( assets, "City ornamental planter grasses", planting, material( assets, { .24f, .36f, .13f, 1 }, .92f ) );
        }

        void buildNaturalCityStreets( SceneAssets &assets, const OpenCityLayout &city,
                                      Geometry &asphalt, Geometry &pavement, Geometry &markings,
                                      Geometry &streetMetal, Geometry &lampGlass,
                                      VehicleAppearanceQuality quality )
        {
            const auto &network = city.roads;
            if( network.nodes.empty() || network.edges.empty() || network.nodes.size() > 160 || network.edges.size() > 240 )
                throw std::runtime_error( "Natural city requires a bounded connected road graph." );
            const auto position = [&]( u32 node ) {
                if( node >= network.nodes.size() ) throw std::runtime_error( "Natural road endpoint is out of range." );
                const auto p = network.nodes[node];
                if( !std::isfinite( p.x ) || !std::isfinite( p.z ) ) throw std::runtime_error( "Natural road endpoint is not finite." );
                return Vector3F( p.x, 0, p.z );
            };
            SourceBudgetVector<u32> degree( network.nodes.size(), 0 );
            for( const auto &edge : network.edges )
            {
                if( edge.a == edge.b || ( position( edge.b ) - position( edge.a ) ).lengthSquared() < .01f )
                    throw std::runtime_error( "Natural road has an invalid segment." );
                ++degree[edge.a]; ++degree[edge.b];
            }
            const auto stripe = []( Geometry &g, Vector3F a, Vector3F b, Vector3F across,
                                     float width, float y ) {
                a.y = b.y = y;
                const auto half = across * ( width * .5f );
                g.quad( a - half, b - half, b + half, a + half, { 0, 1, 0 } );
            };
            const auto trim = [&]( u32 node, u32 other, Vector3F direction, int side, bool paint ) {
                float distance = paint ? 8.5f : 0;
                for( const auto &edge : network.edges )
                {
                    const u32 neighbour = edge.a == node ? edge.b : edge.b == node ? edge.a : node;
                    if( neighbour == node || neighbour == other ) continue;
                    auto q = position( neighbour ) - position( node ); q.normalise();
                    const float cosine = direction.dotProduct( q );
                    const float sine = direction.x * q.z - direction.z * q.x;
                    if( paint )
                    {
                        if( cosine > 0 && std::abs( sine ) > .001f )
                            distance = std::max( distance, 6.f / std::abs( sine ) + .1f );
                    }
                    else if( sine * side > .001f )
                    {
                        // A branch on this side can cross the walk. Behind-facing
                        // continuations use the inner offset, avoiding enormous trim
                        // distances on gentle bends close to a straight continuation.
                        const float offset = cosine >= 0 ? 8.5f : 6.f;
                        distance = std::max( distance, ( 6.f + offset * cosine ) / std::abs( sine ) + .05f );
                    }
                }
                return distance;
            };
            const auto ribbonClear = [&]( Vector3F a, Vector3F b, Vector3F across ) {
                const auto centre = ( a + b ) * .5f;
                auto along = b - a; const float halfLength = along.length() * .5f; along.normalise();
                constexpr float halfWidth = 1.25f;
                const auto point = [&]( Vector3F p ) {
                    p -= centre; return Vector2F( p.dotProduct( along ), p.dotProduct( across ) );
                };
                const auto distanceToBox = [&]( Vector2F p ) {
                    const float x = std::max( 0.f, std::abs( p.x ) - halfLength );
                    const float y = std::max( 0.f, std::abs( p.y ) - halfWidth );
                    return x * x + y * y;
                };
                for( const auto &edge : network.edges )
                {
                    const auto p = point( position( edge.a ) ), q = point( position( edge.b ) );
                    const auto delta = q - p;
                    float low = 0, high = 1;
                    bool intersects = true;
                    for( int axis = 0; axis < 2; ++axis )
                    {
                        const float start = axis ? p.y : p.x, direction = axis ? delta.y : delta.x;
                        const float extent = axis ? halfWidth : halfLength;
                        if( std::abs( direction ) < 1e-6f )
                        { if( std::abs( start ) > extent ) { intersects = false; break; } }
                        else
                        {
                            float t0 = ( -extent - start ) / direction, t1 = ( extent - start ) / direction;
                            if( t0 > t1 ) std::swap( t0, t1 );
                            low = std::max( low, t0 ); high = std::min( high, t1 );
                            if( low > high ) { intersects = false; break; }
                        }
                    }
                    if( intersects ) return false;
                    float distanceSquared = std::min( distanceToBox( p ), distanceToBox( q ) );
                    const float lengthSquared = delta.x * delta.x + delta.y * delta.y;
                    for( int sx : { -1, 1 } ) for( int sy : { -1, 1 } )
                    {
                        const Vector2F corner( sx * halfLength, sy * halfWidth );
                        const float t = std::clamp( ( ( corner.x - p.x ) * delta.x + ( corner.y - p.y ) * delta.y ) /
                            std::max( lengthSquared, 1e-8f ), 0.f, 1.f );
                        const auto offset = corner - ( p + delta * t );
                        distanceSquared = std::min( distanceSquared, offset.x * offset.x + offset.y * offset.y );
                    }
                    if( distanceSquared < 5.998f * 5.998f ) return false;
                }
                return true;
            };
            u32 lampCount = 0;
            for( const auto &edge : network.edges )
            {
                const auto a = position( edge.a ), b = position( edge.b );
                auto direction = b - a; const float length = direction.length(); direction /= length;
                const Vector3F across( -direction.z, 0, direction.x );
                // Full segment rectangles and round node fans form the road's
                // centreline-buffer union, including split crossings and dead ends.
                stripe( asphalt, a, b, across, 12, .012f );
                const float paintStart = trim( edge.a, edge.b, direction, 0, true );
                const float paintEnd = trim( edge.b, edge.a, -direction, 0, true );
                if( length > paintStart + paintEnd + 2 )
                {
                    const auto start = a + direction * paintStart, end = b - direction * paintEnd;
                    for( int side : { -1, 1 } )
                        stripe( markings, start + across * ( side * 5.7f ), end + across * ( side * 5.7f ), across, .13f, .018f );
                    for( float t = paintStart; t + 2 < length - paintEnd; t += 5 )
                        stripe( markings, a + direction * t, a + direction * ( t + 2 ), across, .15f, .018f );
                }
                for( int side : { -1, 1 } )
                {
                    const float start = trim( edge.a, edge.b, direction, side, false );
                    const float end = trim( edge.b, edge.a, -direction, -side, false );
                    if( length <= start + end + 1 ) continue;
                    auto p = a + direction * start + across * ( side * 7.25f );
                    auto q = b - direction * end + across * ( side * 7.25f );
                    if( !ribbonClear( p, q, across ) ) continue;
                    const auto half = across * 1.25f;
                    const Vector3F up( 0, .15f, 0 );
                    pavement.quad( p - half + up, q - half + up, q + half + up, p + half + up, { 0, 1, 0 } );
                    pavement.quad( p - half, p + half, q + half, q - half, { 0, -1, 0 } );
                    pavement.quad( p + half, p + half + up, q + half + up, q + half, across );
                    pavement.quad( p - half, q - half, q - half + up, p - half + up, -across );
                    pavement.quad( p - half, p - half + up, p + half + up, p + half, -direction );
                    pavement.quad( q - half, q + half, q + half + up, q - half + up, direction );
                    const auto centre = ( p + q ) * .5f;
                    staticBox( assets, "City sidewalk", centre + Vector3F( 0, .075f, 0 ),
                        { 2.5f, .15f, ( q - p ).length() }, QuaternionF::eulerDegrees( 0,
                        std::atan2( direction.x, direction.z ) * 180 / pi, 0 ) );
                    if( lampCount >= ( quality == VehicleAppearanceQuality::Preview ? 24u : 64u ) ||
                        ( q - p ).lengthSquared() < 400 ) continue;
                    const auto base = centre + Vector3F( 0, .15f, 0 ), top = base + Vector3F( 0, 6.2f, 0 );
                    const auto arm = across * ( -side * 1.1f ) + Vector3F( 0, -.15f, 0 );
                    streetMetal.beam( base, top, .055f ); streetMetal.beam( top, top + arm, .045f );
                    lampGlass.box( top + arm, { .5f, .1f, .5f } );
                    staticBox( assets, "City lamp post", base + Vector3F( 0, 3.1f, 0 ), { .12f, 6.2f, .12f } );
                    ++lampCount;
                }
            }
            for( u32 node = 0; node < network.nodes.size(); ++node )
            {
                if( !degree[node] ) continue;
                const auto p = position( node ) + Vector3F( 0, .012f, 0 );
                const auto base = u32( asphalt.positions.size() );
                const auto vertex = [&]( Vector3F v ) {
                    asphalt.positions.push_back( v ); asphalt.normals.push_back( { 0, 1, 0 } );
                    asphalt.tangents.push_back( { 1, 0, 0, 1 } ); asphalt.uv.push_back( { v.x, v.z, 0 } );
                };
                vertex( p );
                constexpr u32 slices = 24;
                for( u32 i = 0; i <= slices; ++i )
                    vertex( p + Vector3F( std::cos( i * 2 * pi / slices ) * 6, 0, std::sin( i * 2 * pi / slices ) * 6 ) );
                for( u32 i = 0; i < slices; ++i )
                { asphalt.indices.push_back( base ); asphalt.indices.push_back( base + i + 2 ); asphalt.indices.push_back( base + i + 1 ); }
            }
        }

        SourceBudgetVector<Vector3F> prepareTerrainCity( SceneAssets &assets, OpenCityLayout &city,
                                                         SmartPtr<IGameActor> vehicle, u32 seed )
        {
            auto app=core::IApplicationManager::instance();auto manager=app->getGameManager();auto scene=manager->getCurrentScene();
            const auto persistentActor=[&](const String &name) {
                auto actor=manager->createActor();if(!actor)throw std::runtime_error("Cannot create persistent city source actor.");
                actor->setName(name);actor->setStatic(true);actor->setPosition(Vector3F::zero());
                actor->setOrientation(QuaternionF::identity());actor->setScale(Vector3F::unit());
                scene->addActor(actor);scene->registerAllUpdates(actor);return actor;
            };
            String error;
            if(!assets.roadTerrain)
            {
                auto actor=persistentActor("Generated city terrain");assets.roadTerrain=actor->addComponent<TerrainSystem>();
                if(!assets.roadTerrain || !actor->addComponent<CollisionTerrain>() || !actor->addComponent<Rigidbody>())
                    throw std::runtime_error("Terrain city requires native terrain and collision components.");
                if(!assets.roadTerrain->isLoaded())assets.roadTerrain->load(nullptr);
                assets.roadTerrain->setTreesEnabled(false);assets.roadTerrain->setGrassEnabled(false);
                const float radius=std::ceil((city.extent+96)/4)*4;const u32 side=u32(radius*.5f)+1;
                render::TerrainData data;data.dimensions={s32(side),s32(side)};data.spacing={4,4};data.origin={-radius,-radius};
                data.heights.resize(size_t(side)*side);data.layerWeights.resize(size_t(side)*side);
                data.layers[0].tint={.30f,.37f,.20f,1};data.layers[1].tint={.35f,.30f,.22f,1};
                data.appearance.macroScale=.022f;data.appearance.macroStrength=.3f;data.appearance.macroSeed=seed;
                data.appearance.macroTint={.29f,.35f,.19f,1};
                for(u32 z=0;z<side;++z)for(u32 x=0;x<side;++x)
                {
                    const float px=-radius+x*4,pz=-radius+z*4;
                    const float outside=std::clamp((std::max(std::abs(px),std::abs(pz))-city.extent-20)/76,0.f,1.f);
                    data.heights[size_t(z)*side+x]=2+.005f*px+.003f*pz+outside*outside*3*std::sin(px*.019f)*std::cos(pz*.017f);
                    data.layerWeights[size_t(z)*side+x]={1-outside*.4f,outside*.4f,0,0};
                }
                auto renderer=dynamic_pointer_cast<render::Terrain>(assets.roadTerrain->getTerrain());
                const bool deferred=renderer && renderer->isTerrainPublicationDeferred();
                if(deferred)renderer->setDeferredTerrainPublication(false);
                const bool applied=assets.roadTerrain->applyTerrainData(data,error);
                if(deferred)renderer->setDeferredTerrainPublication(true);
                if(!applied)throw std::runtime_error(error.c_str());
            }
            const bool newSource=!assets.roadAuthoring;
            if(newSource)assets.roadAuthoring=persistentActor("Editable generated city roads")->addComponent<CityRoadAuthoring>();
            if(!assets.roadAuthoring)throw std::runtime_error("Terrain city requires a canonical road source.");
            if(assets.roadAuthoring->getTerrain()!=assets.roadTerrain && !assets.roadAuthoring->bindTerrain(assets.roadTerrain,error))
                throw std::runtime_error(error.c_str());
            const auto &currentSource=assets.roadAuthoring->getSource();
            const bool seedSource=newSource || (currentSource.roads.empty() && currentSource.roundabouts.empty() && currentSource.revision<=2);
            assets.citySourceReused=!seedSource;
            if(!seedSource && !assets.roadAuthoring->rebuild(error))throw std::runtime_error(error.c_str());
            const auto terrain=assets.roadAuthoring->getTerrainSnapshot();const auto placement=assets.roadAuthoring->getTerrainPlacement();
            if(!terrain)throw std::runtime_error("Terrain city has no retained terrain snapshot.");
            const auto height=[&](const Vector3F &point,float &y){return render::sampleTerrainHeight(*terrain,placement,point,y);};
            SourceBudgetVector<Vector3F> route;
            if(seedSource)
            {
                procedural::OpenCityRoadSourceOptions options;options.seed=seed;
                options.sourceId=assets.roadAuthoring->getSource().id;
                const auto binding=assets.roadAuthoring->getTerrainBindingUUID();options.terrainResourceUuid.assign(binding.data(),binding.size());
                options.terrainRevision=terrain->revision;options.maximumRoundabouts=4;
                procedural::OpenCityRoadSourceResult generated;
                if(!procedural::buildOpenCityRoadSource(city,options,height,generated,error))throw std::runtime_error(error.c_str());
                SourceBudgetText json;if(!procedural::serializeCityRoadSplineSource(generated.source,json,error) ||
                    !assets.roadAuthoring->setSourceJSON(String(json.data(),json.size()),assets.roadAuthoring->getSourceRevision(),error))
                    throw std::runtime_error(error.c_str());
                route=std::move(generated.route);
            }
            else
            {
                assets.cityRaceRouteAvailable=procedural::fitOpenCityRoadRoute(city,assets.roadAuthoring->getSource(),height,route,error);
                if(!assets.cityRaceRouteAvailable)assets.cityRaceRouteError=error;
            }
            if(route.size()>1 && (route.front()-route.back()).lengthSquared()<1e-8f)route.pop_back();
            OpenCityLayout supported;
            if(!prepareCityRoadParcels(city,assets.roadAuthoring->getSource(),assets.roadAuthoring->getBake(),terrain,placement,
                supported,assets.cityCulledParcels,assets.cityBuildingSupportChecks,error))throw std::runtime_error(error.c_str());
            city=std::move(supported);city.route.clear();for(const auto &point:route)city.route.push_back({point.x,point.z});
            assets.cityLayout=city;assets.cityAppliedSourceRevision=assets.roadAuthoring->getSourceRevision();
            assets.ground=assets.roadTerrain->getActor();assets.triangles+=assets.roadAuthoring->getRuntimeTriangleCount()+u64(terrain->dimensions.x-1)*(terrain->dimensions.y-1)*2;
            Vector3F spawn=route.empty()?Vector3F(0,0,0):route.front();bool driveable=false;float y=0;
            if(!assets.roadAuthoring->getRuntimeSurface(spawn.x,spawn.z,y,driveable) || !driveable)
                if(!height(spawn,y))throw std::runtime_error("Edited city has no supported vehicle spawn.");
            spawn.y=y+.42f;vehicle->setPosition(spawn);vehicle->updateTransform();
            if(auto rigidbody=vehicle->getComponent<Rigidbody>())if(auto body=rigidbody->getRigidDynamic())
                body->setTransform(Transform3F(spawn,vehicle->getOrientation()));
            return route;
        }

        void buildCity( SceneAssets &assets, SmartPtr<IGameActor> vehicle, u32 seed, VehicleAppearanceQuality quality )
        {
            auto city = OpenCityLayout::generate( seed, assets.cityBlocks, assets.cityRoute, assets.cityNaturalRoads );
            SourceBudgetVector<Vector3F> fittedRoute;
            if(assets.cityTerrainRoads)fittedRoute=prepareTerrainCity(assets,city,vehicle,seed);
            auto app = core::IApplicationManager::instance();
            auto roads = app->getFactoryManager()->createObjectFromType<IRoadSystem>( "IRoadSystem" );
            if( !roads )
            {
                throw std::runtime_error( "Open city requires the WPProcedural road service." );
            }
            roads->setSeed( seed );
            auto forge =
                app->getFactoryManager()->createObjectFromType<ITextureForge>( "ITextureForge" );
            if( !forge )
            {
                throw std::runtime_error( "Open city requires the WPProcedural texture forge." );
            }
            forge->setSeed( seed );
            SurfaceBakeParams surfaceParams;
            surfaceParams.size = quality == VehicleAppearanceQuality::Preview ? 256 : 512;
            SmartPtr<render::IMaterial> cityAsphalt;
            if(!assets.cityTerrainRoads)cityAsphalt=raceAsphalt( assets, forge, surfaceParams, seed );
            // World projection gives segments and intersections the same physical texture scale.
            if(cityAsphalt)
            {cityAsphalt->setUVProjection( 3 );cityAsphalt->setEditorFloat( "uvTriplanarScale", 1.f / 8.f );}
            Geometry asphalt, pavement, markings, parks, courtyards, streetMetal, lampGlass;
            struct CityPatch
            {
                Geometry walls[3], farWalls[3], glass[3], farGlass, roofs, farRoofs, frames, accents;
                Array<LODDetailBound> bounds{ Allocator<LODDetailBound>( sourceBudget ) };
            };
            SourceBudgetMap<std::pair<int, int>, CityPatch> patches;
            const auto streetDetails = OpenCityStreetDetails::generate( city, seed,
                quality != VehicleAppearanceQuality::Preview );
            // Match the existing flat physics proxy. Preserve library topology,
            // kerbs and markings without introducing unsupported road-height bumps.
            auto append = []( Geometry &g, const RoadMesh &source, bool flat = false, float height = .012f ) {
                const auto base = static_cast<u32>( g.positions.size() );
                for( const auto &v : source.vertices )
                {
                    g.positions.push_back( { float( v.position.x ), flat ? height : float( v.position.y ),
                                             float( v.position.z ) } );
                    g.normals.push_back( flat ? Vector3F( 0, 1, 0 )
                                              : Vector3F( float( v.normal.x ), float( v.normal.y ),
                                                          float( v.normal.z ) ) );
                    g.uv.push_back( { float( v.uv.x ), float( v.uv.y ), 0 } );
                    g.tangents.push_back( { 1, 0, 0, 1 } );
                }
                if( source.indices.size() % 3 ) throw std::runtime_error( "Incomplete city road triangles." );
                for( size_t triangle = 0; triangle < source.indices.size(); triangle += 3 )
                {
                    if( source.indices[triangle] >= source.vertices.size() ||
                        source.indices[triangle + 1] >= source.vertices.size() ||
                        source.indices[triangle + 2] >= source.vertices.size() )
                        throw std::runtime_error( "Invalid city road triangle index." );
                    auto a = base + source.indices[triangle], b = base + source.indices[triangle + 1],
                         c = base + source.indices[triangle + 2];
                    if( a >= g.positions.size() || b >= g.positions.size() || c >= g.positions.size() )
                        throw std::runtime_error( "Invalid city road triangle index." );
                    // Marking meshes can carry the opposite winding to their authored
                    // up normal. Flattened city surfaces always face out of the ground.
                    if( flat && ( g.positions[b] - g.positions[a] ).crossProduct( g.positions[c] - g.positions[a] ).y < 0 )
                        std::swap( b, c );
                    g.indices.push_back( a ); g.indices.push_back( b ); g.indices.push_back( c );
                }
            };
            const int half = city.blocks / 2;
            u32 segmentSeed = seed;
            if( assets.cityTerrainRoads )
            {
                for(const auto &lot:city.lots)
                {
                    const float top=lot.groundHeight+.15f,bottom=lot.groundMinimumHeight-.25f;
                    const Vector3F centre(lot.parcelX,(top+bottom)*.5f,lot.parcelZ);
                    const Vector3F size(lot.parcelWidth,top-bottom,lot.parcelDepth);
                    courtyards.box(centre,size);staticBox(assets,"City terrain foundation",centre,size);
                }
                SourceBudgetVector<Vector3F> lampCandidates;
                const auto &source=assets.roadAuthoring->getSource();const auto &bake=assets.roadAuthoring->getBake();
                for(size_t road=0;road<bake.roads.size();++road)
                {
                    const auto &points=bake.roads[road].points;float next=24;
                    for(size_t i=1;i<points.size();++i)
                    {
                        const auto &a=points[i-1],&b=points[i];
                        while(next<=b.distance)
                        {
                            const float t=std::clamp((next-a.distance)/(b.distance-a.distance),0.f,1.f);next+=60;
                            const auto centre=a.position+(b.position-a.position)*t;
                            auto forward=b.position-a.position;forward.y=0;forward.normalise();
                            const Vector3F across(forward.z,0,-forward.x);
                            for(int side:{-1,1})
                            {
                                auto point=centre+across*(side*(source.roads[road].width*.5f+1.3f));bool clear=true;
                                for(const auto &junction:source.roundabouts)
                                    if(std::hypot(point.x-junction.center.x,point.z-junction.center.z)<junction.outerRadius+2)clear=false;
                                for(size_t other=0;clear && other<bake.roads.size();++other)
                                {
                                    const auto &samples=bake.roads[other].points;
                                    for(size_t j=1;j<samples.size();++j)
                                    {
                                        const auto u=samples[j-1].position,v=samples[j].position;const float dx=v.x-u.x,dz=v.z-u.z;
                                        const float s=std::clamp(((point.x-u.x)*dx+(point.z-u.z)*dz)/(dx*dx+dz*dz),0.f,1.f);
                                        const float x=point.x-u.x-s*dx,z=point.z-u.z-s*dz,r=source.roads[other].width*.5f+.5f;
                                        if(x*x+z*z<r*r){clear=false;break;}
                                    }
                                }
                                float height=0;
                                if(clear && lampCandidates.size()<2048 && render::sampleTerrainHeight(*assets.roadAuthoring->getTerrainSnapshot(),
                                    assets.roadAuthoring->getTerrainPlacement(),point,height))
                                {point.y=height;lampCandidates.push_back(point);}
                            }
                        }
                    }
                }
                std::sort(lampCandidates.begin(),lampCandidates.end(),[](const auto &a,const auto &b){return a.lengthSquared()<b.lengthSquared();});
                for(size_t i=0;i<std::min<size_t>(lampCandidates.size(),quality==VehicleAppearanceQuality::Preview?24:64);++i)
                {
                    const auto ground=lampCandidates[i],base=ground+Vector3F(0,.15f,0),top=base+Vector3F(0,6.2f,0);
                    courtyards.box(ground+Vector3F(0,.075f,0),{.4f,.15f,.4f});
                    streetMetal.beam(base,top,.055f);const Vector3F arm(.8f,-.1f,.45f);
                    streetMetal.beam(top,top+arm,.04f);lampGlass.box(top+arm,{.5f,.1f,.5f});
                    staticBox(assets,"City lamp foundation",ground+Vector3F(0,.075f,0),{.4f,.15f,.4f});
                    staticBox(assets,"City lamp post",base+Vector3F(0,3.1f,0),{.12f,6.2f,.12f});
                }
            }
            else if( city.naturalRoads )
            {
                buildNaturalCityStreets( assets, city, asphalt, pavement, markings,
                    streetMetal, lampGlass, quality );
                // Natural parcels are already filtered against the entire road
                // buffer. Their small raised courts anchor buildings and parks
                // without recreating the old rectangular block grid.
                for( const auto &lot : city.lots )
                {
                    const Vector3F centre( lot.parcelX, .075f, lot.parcelZ );
                    const Vector3F size( lot.parcelWidth, .15f, lot.parcelDepth );
                    courtyards.box( centre, size );
                    staticBox( assets, "City parcel", centre, size );
                }
            }
            else
            {
            for( int row = -half; row <= half; ++row )
            {
                for( int col = -half; col < half; ++col )
                {
                    for( int axis = 0; axis < 2; ++axis )
                    {
                        RoadSegmentSpec spec;
                        const float a = col * city.spacing + 6, b = ( col + 1 ) * city.spacing - 6;
                        spec.start = axis ? Vector3<real_Num>( row * city.spacing, 0, a )
                                          : Vector3<real_Num>( a, 0, row * city.spacing );
                        spec.end = axis ? Vector3<real_Num>( row * city.spacing, 0, b )
                                        : Vector3<real_Num>( b, 0, row * city.spacing );
                        spec.roadClass = RoadClass::Arterial;
                        spec.seed = segmentSeed++;
                        spec.generateDressing = false;
                        spec.generateSidewalks = false;
                        spec.generateKerbs = false;
                        const auto road = roads->generateSegment( spec );
                        append( asphalt, road.roadSurface, true );
                        // Shared city walks have one flat grade and own each corner once.
                        append( markings, road.markings, true, .018f );
                    }
                }
            }
            for( int x = -half; x <= half; ++x )
            {
                for( int z = -half; z <= half; ++z )
                {
                    IntersectionSpec spec;
                    spec.center = { x * city.spacing, 0, z * city.spacing };
                    spec.roadClass = RoadClass::Arterial;
                    spec.seed = segmentSeed++;
                    const auto junction = roads->generateIntersection( spec );
                    append( asphalt, junction.surface, true );
                    append( markings, junction.markings, true, .018f );
                }
            }
            const auto addWalk = [&]( const OpenCityStreetDetails::Corner &walk ) {
                const Vector3F centre( walk.x, ( walk.bottom + walk.top ) * .5f, walk.z );
                const Vector3F size( walk.width, walk.top - walk.bottom, walk.depth );
                pavement.box( centre, size );
            };
            for( const auto &walk : streetDetails.sidewalks ) addWalk( walk );
            for( const auto &walk : streetDetails.corners ) addWalk( walk );
            // The inner block slabs meet the sidewalks at the same authored grade.
            for( int x = -half; x < half; ++x ) for( int z = -half; z < half; ++z )
            {
                const Vector3F centre( ( x + .5f ) * city.spacing, .075f, ( z + .5f ) * city.spacing );
                const Vector3F size( city.spacing - 17, .15f, city.spacing - 17 );
                courtyards.box( centre, size );
            }
            // The adjoining plaza/walk pieces form one solid interior square per
            // block. Use that exact union to avoid thousands of scene-root actors.
            for( const auto &surface : streetDetails.collisionSurfaces( city ) )
                staticBox( assets, "City sidewalk",
                    { surface.x, ( surface.bottom + surface.top ) * .5f, surface.z },
                    { surface.width, surface.top - surface.bottom, surface.depth } );
            for( const auto &item : streetDetails.furniture )
            {
                const Vector3F p( item.x, item.baseY, item.z );
                if( item.kind == OpenCityStreetDetails::FurnitureKind::Lamp )
                {
                    const Vector3F top = p + Vector3F( 0, 6.2f, 0 );
                    const Vector3F arm( std::sin( item.yaw ) * 1.25f, -.15f, std::cos( item.yaw ) * 1.25f );
                    streetMetal.beam( p, top, .055f );
                    streetMetal.beam( top, top + arm, .045f );
                    lampGlass.box( top + arm, { .5f, .10f, .5f } );
                    staticBox( assets, "City lamp post", p + Vector3F( 0, 3.1f, 0 ), { .12f, 6.2f, .12f } );
                }
                else streetMetal.box( p + Vector3F( 0, .45f, 0 ), { .13f, .9f, .13f } );
            }
            }
            for( const auto &lot : city.lots )
            {
                if( lot.park )
                {
                    parks.box( { lot.parcelX, lot.groundHeight + .155f, lot.parcelZ },
                               { lot.parcelWidth - 1, .01f, lot.parcelDepth - 1 } );
                    continue;
                }
                const auto key = std::make_pair( int( std::floor( lot.x / 128 ) ), int( std::floor( lot.z / 128 ) ) );
                const Vector3F origin( key.first * 128.f + 64, 0, key.second * 128.f + 64 );
                auto &patch = patches[key];
                patch.bounds.push_back( { Vector3F( lot.x, lot.groundHeight + lot.height * .5f, lot.z ) - origin,
                    std::max( { lot.width, lot.depth, lot.height } ) } );
                const auto volumes = OpenCityLayout::buildingVolumes( lot );
                const bool office = lot.district == OpenCityLayout::District::Commercial;
                const bool civic = lot.district == OpenCityLayout::District::Civic;
                const int entranceAxis = lot.frontage == OpenCityLayout::Frontage::West ||
                    lot.frontage == OpenCityLayout::Frontage::East ? 1 : 0;
                const int entranceSign = lot.frontage == OpenCityLayout::Frontage::West ||
                    lot.frontage == OpenCityLayout::Frontage::North ? -1 : 1;
                for( const auto &volume : volumes )
                {
                    const Vector3F centre( volume.x, volume.bottom + volume.height * .5f, volume.z );
                    const Vector3F size( volume.width, volume.height, volume.depth );
                    const bool groundFloor = volume.bottom == lot.groundHeight;
                    const u32 family = civic ? 1u : office ?
                        ( groundFloor && lot.variant % 3 == 0 ? 1u : 2u ) : ( lot.variant % 5 == 0 ? 1u : 0u );
                    auto &opaque = patch.walls[family];
                    patch.farWalls[family].box( centre - origin, size );
                    // Geometry and collision consume the identical stepped mass description.
                    staticBox( assets, "City building mass", centre, size );
                    const float cx = centre.x - origin.x, cz = centre.z - origin.z;
                    const float top = volume.bottom + volume.height;
                    // Horizontal caps close the shell. Near walls are segmented around
                    // genuine apertures; a solid wall never covers the recessed glazing.
                    opaque.quad( { cx - size.x * .5f, top, cz - size.z * .5f },
                        { cx + size.x * .5f, top, cz - size.z * .5f },
                        { cx + size.x * .5f, top, cz + size.z * .5f },
                        { cx - size.x * .5f, top, cz + size.z * .5f }, { 0, 1, 0 } );
                    opaque.quad( { cx - size.x * .5f, volume.bottom, cz - size.z * .5f },
                        { cx + size.x * .5f, volume.bottom, cz - size.z * .5f },
                        { cx + size.x * .5f, volume.bottom, cz + size.z * .5f },
                        { cx - size.x * .5f, volume.bottom, cz + size.z * .5f }, { 0, -1, 0 } );
                    patch.roofs.box( { cx, top + .06f, cz }, { volume.width + .12f, .12f, volume.depth + .12f } );
                    patch.farRoofs.box( { cx, top + .06f, cz }, { volume.width + .12f, .12f, volume.depth + .12f } );
                    const float nominalFloor = office ? 3.6f : civic ? 3.5f : 3.2f;
                    const int floors = std::max( 1, int( std::round( volume.height / nominalFloor ) ) );
                    const float floorHeight = volume.height / floors;
                    for( int axis = 0; axis < 2; ++axis ) for( int sign : { -1, 1 } )
                    {
                        const float span = axis ? volume.depth : volume.width;
                        const bool entrance = groundFloor && axis == entranceAxis && sign == entranceSign;
                        const Vector3F normal( axis ? float( sign ) : 0, 0, axis ? 0 : float( sign ) );
                        const Vector3F along( axis ? 0 : 1, 0, axis ? 1 : 0 );
                        const auto point = [&]( float s, float y, float inset ) {
                            return axis ? Vector3F( cx + sign * ( size.x * .5f - inset ), y, cz + s ) :
                                Vector3F( cx + s, y, cz + sign * ( size.z * .5f - inset ) );
                        };
                        const auto panel = [&]( Geometry &g, float left, float low, float right,
                                                float high, float inset = 0.f ) {
                            if( right - left <= .001f || high - low <= .001f ) return;
                            g.quad( point( left, low, inset ), point( right, low, inset ),
                                point( right, high, inset ), point( left, high, inset ), normal );
                        };
                        const auto reveal = [&]( float left, float low, float right, float high, float depth ) {
                            opaque.quad( point( left, low, 0 ), point( left, low, depth ),
                                point( left, high, depth ), point( left, high, 0 ), along );
                            opaque.quad( point( right, low, 0 ), point( right, high, 0 ),
                                point( right, high, depth ), point( right, low, depth ), -along );
                            opaque.quad( point( left, low, 0 ), point( right, low, 0 ),
                                point( right, low, depth ), point( left, low, depth ), { 0, 1, 0 } );
                            opaque.quad( point( left, high, 0 ), point( left, high, depth ),
                                point( right, high, depth ), point( right, high, 0 ), { 0, -1, 0 } );
                        };
                        const auto faceBox = [&]( Geometry &g, float s, float y, float width,
                                                  float height, float projection, float thickness ) {
                            g.box( point( s, y, -projection ), axis ? Vector3F( thickness, height, width ) :
                                Vector3F( width, height, thickness ) );
                        };
                        const float desiredBay = office ? 3.8f : civic ? 4.6f : 4.4f + ( lot.variant % 3 ) * .45f;
                        const int bays = std::max( 1, int( std::floor( span / desiredBay ) ) );
                        const float bayWidth = span / bays;
                        for( int floor = 0; floor < floors; ++floor )
                        {
                            const float y0 = volume.bottom + floor * floorHeight;
                            const float y1 = floor + 1 == floors ? top : y0 + floorHeight;
                            const bool retail = entrance && floor == 0 && ( office || civic || lot.variant % 3 != 0 );
                            const float low = y0 + ( retail ? .28f : office ? .62f : .78f );
                            const float high = y1 - ( retail ? .42f : office ? .48f : .62f );
                            auto &glass = patch.glass[( lot.variant / 7 + axis + ( sign > 0 ? 1 : 0 ) ) % 2];
                            if( high - low < .45f )
                            {
                                panel( opaque, -span * .5f, y0, span * .5f, y1 );
                                continue;
                            }
                            if( office )
                            {
                                // Curtain-wall kit: recessed ribbons, substantial spandrels
                                // and fine mullions rather than hundreds of boxed windows.
                                const float left = -span * .5f + .38f, right = span * .5f - .38f;
                                panel( opaque, -span * .5f, y0, span * .5f, low );
                                panel( opaque, -span * .5f, high, span * .5f, y1 );
                                panel( opaque, -span * .5f, low, left, high );
                                panel( opaque, right, low, span * .5f, high );
                                reveal( left, low, right, high, .085f );
                                // Each pane shares the ribbon aperture but has restrained
                                // seeded tint/roughness variation, including privacy glass.
                                for( int bay = 0; bay < bays; ++bay )
                                {
                                    const float paneLeft = std::max( left, -span * .5f + bay * bayWidth );
                                    const float paneRight = std::min( right, -span * .5f + ( bay + 1 ) * bayWidth );
                                    u32 variation = lot.variant ^ ( u32( floor + 1 ) * 0x9e3779b9u ) ^
                                        ( u32( bay + 1 ) * 0x85ebca6bu ) ^ u32( axis * 31 + sign );
                                    variation ^= variation >> 16; variation *= 0x7feb352du;
                                    variation ^= variation >> 15;
                                    const int surface = variation % 19 == 0 ? 2 : variation % 7 < 2 ? 1 : 0;
                                    panel( patch.glass[surface], paneLeft, low, paneRight, high, .085f );
                                }
                                panel( patch.farGlass, left, low, right, high, -.018f );
                                for( int bay = 1; bay < bays; ++bay )
                                {
                                    const float s = -span * .5f + bay * bayWidth;
                                    panel( patch.frames, s - .035f, low, s + .035f, high, .045f );
                                }
                                if( lot.variant % 3 == 1 )
                                    panel( patch.frames, left, ( low + high ) * .5f - .025f,
                                        right, ( low + high ) * .5f + .025f, .045f );
                                panel( patch.frames, -span * .5f, y1 - .065f, span * .5f, y1, -.025f );
                            }
                            else for( int bay = 0; bay < bays; ++bay )
                            {
                                const float s0 = -span * .5f + bay * bayWidth, s1 = s0 + bayWidth;
                                const float s = ( s0 + s1 ) * .5f;
                                const bool door = entrance && floor == 0 && bay == bays / 2;
                                const float ratio = retail ? .79f : civic ? .56f : .48f + ( lot.variant % 3 ) * .05f;
                                const float width = door ? std::min( 2.05f, bayWidth * .72f ) : bayWidth * ratio;
                                const float left = s - width * .5f, right = s + width * .5f;
                                const float bottom = door ? y0 + .16f : low;
                                const float head = door ? std::min( y1 - .35f, y0 + 2.65f ) : high;
                                const float depth = civic ? .24f : .16f;
                                panel( opaque, s0, y0, s1, bottom );
                                panel( opaque, s0, head, s1, y1 );
                                panel( opaque, s0, bottom, left, head );
                                panel( opaque, right, bottom, s1, head );
                                reveal( left, bottom, right, head, depth );
                                panel( glass, left, bottom, right, head, depth );
                                panel( patch.farGlass, left, bottom, right, head, -.018f );
                                panel( patch.frames, s - .032f, bottom, s + .032f, head, depth - .025f );
                                if( !retail && !door && axis == entranceAxis && sign == entranceSign )
                                {
                                    // A shallow projecting sill gives a grazing highlight.
                                    faceBox( patch.frames, s, bottom - .035f, width + .18f, .07f, .05f, .20f );
                                }
                            }
                            if( entrance && floor == 0 )
                            {
                                const float doorS = office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                const float head = office ? std::min( y1 - .42f, y0 + 2.7f ) :
                                    std::min( y1 - .35f, y0 + 2.65f );
                                const float doorHalfWidth = office ? .92f : std::min( 2.05f, bayWidth * .72f ) * .5f;
                                const float depth = office ? .06f : civic ? .215f : .135f;
                                panel( patch.frames, doorS - doorHalfWidth, y0 + .16f, doorS - doorHalfWidth + .06f, head, depth );
                                panel( patch.frames, doorS + doorHalfWidth - .06f, y0 + .16f, doorS + doorHalfWidth, head, depth );
                                panel( patch.frames, doorS - doorHalfWidth, head - .065f, doorS + doorHalfWidth, head, depth );
                                faceBox( patch.frames, doorS - .16f, y0 + 1.3f, .025f, .40f, -depth + .015f, .025f );
                                faceBox( patch.frames, doorS + .16f, y0 + 1.3f, .025f, .40f, -depth + .015f, .025f );
                                const float canopyWidth = std::min( span * .60f, civic ? 11.f : 7.f );
                                const float canopyDepth = civic ? 1.05f : .70f;
                                faceBox( patch.accents, doorS, y1 - .08f, canopyWidth, .16f,
                                    canopyDepth * .5f, canopyDepth );
                                faceBox( patch.frames, doorS, y1 + .10f, canopyWidth * .82f, .20f, .045f, .10f );
                                if( civic ) for( int bay = 0; bay <= bays; ++bay )
                                {
                                    const float s = std::clamp( -span * .5f + bay * bayWidth,
                                        -span * .5f + .22f, span * .5f - .22f );
                                    // Engaged stone columns articulate a portico; the closed
                                    // facade never advertises an unsupported walkable interior.
                                    faceBox( patch.frames, s, y0 + floorHeight * .5f, .32f,
                                        floorHeight - .20f, .12f, .36f );
                                    faceBox( patch.frames, s, y0 + .25f, .47f, .30f, .12f, .46f );
                                }
                            }
                        }
                    }
                    // Roof detail has bounded variants, rather than one box repeated
                    // at every lot centre. The occupied building masses stay unchanged.
                    const bool finalTier = std::abs( top - lot.height ) < .001f;
                    if( finalTier )
                    {
                        for( int side : { -1, 1 } )
                        {
                            patch.roofs.box( { cx, top + .29f, cz + side * ( size.z * .5f - .10f ) },
                                { size.x, .34f, .20f } );
                            patch.roofs.box( { cx + side * ( size.x * .5f - .10f ), top + .29f, cz },
                                { .20f, .34f, size.z - .40f } );
                            patch.farRoofs.box( { cx, top + .29f, cz + side * ( size.z * .5f - .10f ) },
                                { size.x, .34f, .20f } );
                            patch.farRoofs.box( { cx + side * ( size.x * .5f - .10f ), top + .29f, cz },
                                { .20f, .34f, size.z - .40f } );
                        }
                        if( quality != VehicleAppearanceQuality::Preview )
                        {
                            const int requestedUnits = office ? 2 + int( lot.variant % 3 ) : 1 + int( lot.variant % 2 );
                            const int units = std::min( requestedUnits,
                                std::max( 1, 1 + int( std::floor( ( size.x - 3.22f ) / 2.9f ) ) ) );
                            for( int unit = 0; unit < units; ++unit )
                            {
                                const float x = cx + ( unit - ( units - 1 ) * .5f ) * 2.9f;
                                const float z = cz + ( lot.variant % 2 ? .17f : -.17f ) * size.z;
                                const float h = office ? .85f : .60f;
                                patch.roofs.box( { x, top + .12f + h * .5f, z }, { 2.15f, h, 1.45f } );
                                for( int fin = 0; fin < 3; ++fin )
                                    patch.frames.box( { x, top + .15f + h + fin * .055f, z },
                                        { 2.22f, .025f, 1.48f } );
                            }
                            if( lot.variant % 3 == 1 )
                            {
                                for( int side : { -1, 1 } )
                                {
                                    patch.accents.box( { cx, top + .73f, cz + side * ( size.z * .5f - .40f ) },
                                        { size.x - .65f, .045f, .045f } );
                                    patch.accents.box( { cx + side * ( size.x * .5f - .40f ), top + .73f, cz },
                                        { .045f, .045f, size.z - .65f } );
                                }
                                for( int sx : { -1, 1 } ) for( int sz : { -1, 1 } )
                                    patch.accents.box( { cx + sx * ( size.x * .5f - .40f ), top + .50f,
                                        cz + sz * ( size.z * .5f - .40f ) }, { .045f, .46f, .045f } );
                            }
                        }
                    }
                }
            }
            // Contain exploration within the shared collision plane.
            const float boundary = city.extent + 20;
            for( int axis = 0; axis < 2; ++axis )
            {
                for( int side : { -1, 1 } )
                {
                    Vector3F p =
                        axis ? Vector3F( 0, .6f, side * boundary ) : Vector3F( side * boundary, .6f, 0 );
                    if(assets.cityTerrainRoads)
                    {
                        float height=0;if(!render::sampleTerrainHeight(*assets.roadAuthoring->getTerrainSnapshot(),
                            assets.roadAuthoring->getTerrainPlacement(),p,height))throw std::runtime_error("City boundary leaves its terrain.");
                        p.y+=height;
                    }
                    const Vector3F size =
                        axis ? Vector3F( boundary * 2, 1.2f, 1 ) : Vector3F( 1, 1.2f, boundary * 2 );
                    pavement.box( p, size );
                    reserveOwnerSlot(assets.actors);
                    auto wall = app->getGameManager()->createActor();
                    if(!wall) throw std::runtime_error("Cannot create procedural actor");
                    assets.actors.push_back(wall);
                    wall->setName( "City boundary" );
                    wall->setStatic( true );
                    wall->setPosition( p );
                    wall->addComponent<CollisionBox>()->setExtents( size );
                    wall->addComponent<Rigidbody>();
                    app->getGameManager()->getCurrentScene()->addActor( wall );
                }
            }
            if(!assets.cityTerrainRoads)mesh( assets, "City streets", asphalt, cityAsphalt );
            mesh( assets, "City sidewalks", pavement,
                  material( assets, { .42f, .43f, .44f, 1 }, .95f ) );
            if(!assets.cityTerrainRoads)mesh( assets, "City lane markings", markings,
                  material( assets, { .85f, .82f, .65f, 1 }, .8f ) );
            const ColourF colours[] = { { .78f, .68f, .57f, 1 },
                                        { .86f, .83f, .75f, 1 },
                                        { .58f, .65f, .69f, 1 } };
            const SurfaceTag facadeSurfaces[] = {
                SurfaceTag::Brick, SurfaceTag::Plaster, SurfaceTag::Concrete };
            std::array<SmartPtr<render::IMaterial>, 3> facades;
            for( int i = 0; i < 3; ++i )
            {
                // Complete bakes supply encoded albedo and linear normal/ORM data.
                // Each architectural kit has a different surface and physical scale.
                auto surface = forge->bakeSurface( facadeSurfaces[i], surfaceParams );
                if( surface.albedo.pixels.empty() || surface.normal.pixels.empty() || surface.orm.pixels.empty() )
                    throw std::runtime_error( "City facades require complete PBR maps." );
                if( i != 0 )
                {
                    // Large mineral panels need restrained weathering. The forge's
                    // full-range cracked plaster otherwise reads as mottled rock.
                    const float base = i == 1 ? .58f : .42f;
                    for( size_t pixel = 0; pixel < surface.albedo.pixels.size(); pixel += 4 )
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            const float encoded = surface.albedo.pixels[pixel + channel] / 255.f;
                            const float source = encoded <= .04045f ? encoded / 12.92f :
                                std::pow( ( encoded + .055f ) / 1.055f, 2.4f );
                            const float linear = base * .85f + source * .15f;
                            surface.albedo.pixels[pixel + channel] = u8( std::clamp(
                                ( 1.055f * std::pow( linear, 1.f / 2.4f ) - .055f ) * 255, 0.f, 255.f ) );
                        }
                }
                facades[i] = material( assets, colours[i], 1, 1 );
                maps( assets, facades[i], surface.albedo, surface.normal, surface.orm );
                facades[i]->setDiffuse( colours[i] );
                // Box projection uses facade height as well as width; XZ projection
                // had collapsed vertical walls into stretched texture columns.
                facades[i]->setUVProjection( 1 );
                facades[i]->setNormalStrength( i == 0 ? .2f : .1f );
                // Height-derived micro AO must not blanket-darken whole flat walls.
                facades[i]->setEditorFloat( "aoStrength", .35f );
                facades[i]->setEditorUInt( "uvFilter", 3 );
                facades[i]->setEditorFloat( "uvAniso", 8.f );
                facades[i]->setEditorFloat( "uvTriplanarScale", i == 0 ? 1.f : .25f );
            }
            const auto roofMaterial = material( assets, { .18f, .21f, .23f, 1 }, .78f );
            const auto frameMaterial = material( assets, { .68f, .67f, .61f, 1 }, .8f );
            const auto accentMaterial = material( assets, { .18f, .28f, .30f, 1 }, .55f );
            const std::array<SmartPtr<render::IMaterial>, 3> glassMaterials = {
                material( assets, { .12f, .18f, .23f, 1 }, .12f, 0 ),
                material( assets, { .18f, .23f, .27f, 1 }, .22f, 0 ),
                material( assets, { .36f, .40f, .41f, 1 }, .52f, 0 ) };
            for( const auto &[key, patch] : patches )
            {
                reserveOwnerSlot( assets.actors );
                auto root = app->getGameManager()->createActor();
                if( !root ) throw std::runtime_error( "Cannot create city LOD patch." );
                assets.actors.push_back( root );
                root->setName( "City LOD patch" );
                root->setPosition( { key.first * 128.f + 64, 0, key.second * 128.f + 64 } );
                app->getGameManager()->getCurrentScene()->addActor( root );
                auto nearRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                auto farRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                const auto add = [&]( const String &name, const Geometry &geometry,
                    SmartPtr<render::IMaterial> surface, auto &renderers ) {
                    if( auto actor = mesh( assets, name, geometry, surface, root ) )
                        renderers.push_back( actor->getComponent<scene::MeshRenderer>() );
                };
                if( quality != VehicleAppearanceQuality::Preview )
                {
                    for( int family = 0; family < 3; ++family )
                        add( "LOD0 city facade", patch.walls[family], facades[family], nearRenderers );
                    for( int glass = 0; glass < 3; ++glass )
                        add( "LOD0 city glazing", patch.glass[glass], glassMaterials[glass], nearRenderers );
                    add( "LOD0 city roof detail", patch.roofs, roofMaterial, nearRenderers );
                    add( "LOD0 city facade frames", patch.frames, frameMaterial, nearRenderers );
                    add( "LOD0 city entrance fascia", patch.accents, accentMaterial, nearRenderers );
                }
                for( int family = 0; family < 3; ++family )
                    add( "LOD1 city massing", patch.farWalls[family], facades[family], farRenderers );
                add( "LOD1 city roof silhouette", patch.farRoofs, roofMaterial, farRenderers );
                add( "LOD1 city glazing ribbons", patch.farGlass, glassMaterials[0], farRenderers );
                auto lod = root->addComponent<scene::LODGroup>();
                lod->setSize( 128 ); lod->setDetailBounds( patch.bounds );
                lod->setHysteresis( .18f ); lod->setCullBelowLastLOD( false );
                if( !nearRenderers.empty() ) lod->addLevel( .11f, nearRenderers );
                lod->addLevel( 0, farRenderers );
                String error;
                if( !lod->validate( &error ) ) throw std::runtime_error( "Invalid city LOD patch: " + error );
            }
            auto pavingMaterial=material( assets, assets.cityTerrainRoads?ColourF(.54f,.53f,.48f,1):ColourF(.40f,.41f,.38f,1), .92f );
            if(assets.cityTerrainRoads)
            {
                auto paving=forge->bakeSurface(SurfaceTag::Concrete,surfaceParams);
                maps(assets,pavingMaterial,paving.albedo,paving.normal,paving.orm);
                pavingMaterial->setDiffuse({.54f,.53f,.48f,1});pavingMaterial->setUVProjection(1);
                pavingMaterial->setEditorFloat("uvTriplanarScale",.5f);pavingMaterial->setNormalStrength(.12f);
                pavingMaterial->setEditorFloat("aoStrength",.3f);
            }
            mesh( assets, "City block plazas", courtyards, pavingMaterial );
            if(assets.cityTerrainRoads)
            {
                CityRoadTerrainMesh fitted;String error;
                if(!buildCityRoadFootways(assets.roadAuthoring->getSource(),assets.roadAuthoring->getBake(),
                    assets.roadAuthoring->getTerrainSnapshot(),assets.roadAuthoring->getTerrainPlacement(),fitted,error))
                    throw std::runtime_error(error.c_str());
                Geometry walks;
                walks.positions.reserve(fitted.vertices.size());walks.normals.reserve(fitted.vertices.size());
                walks.uv.reserve(fitted.vertices.size());walks.tangents.reserve(fitted.vertices.size());
                walks.indices.reserve(fitted.indices.size());
                for(const auto &vertex:fitted.vertices)
                {
                    walks.positions.push_back(vertex.position);walks.normals.push_back(vertex.normal);
                    walks.uv.push_back({vertex.uv.x,vertex.uv.y,0});
                    auto tangent=std::abs(vertex.normal.y)>.9f?Vector3F(1,0,0):Vector3F(-vertex.normal.z,0,vertex.normal.x);
                    tangent-=vertex.normal*tangent.dotProduct(vertex.normal);
                    tangent.normalise();walks.tangents.push_back({tangent.x,tangent.y,tangent.z,1});
                }
                for(const auto index:fitted.indices)walks.indices.push_back(index);
                if(auto actor=mesh(assets,"Terrain city footways",walks,pavingMaterial))
                {
                    // One native concave mesh shares every fitted top/kerb face;
                    // no per-paving-tile actor or simplified flat contact plane.
                    actor->setStatic(true);
                    auto collision=actor->addComponent<CollisionMesh>();
                    if(!collision)throw std::runtime_error("Cannot create terrain-city footway collision.");
                    auto resource=actor->getComponent<scene::Mesh>()->getMeshResource();
                    collision->setConvex(false);collision->setMeshResource(resource);
                    if(auto shape=dynamic_pointer_cast<physics::IMeshShape>(collision->getShape()))shape->setMeshResource(resource);
                    if(!actor->addComponent<Rigidbody>())throw std::runtime_error("Cannot create terrain-city footway rigidbody.");
                    app->getGameManager()->getCurrentScene()->registerAllUpdates(actor);
                }
            }
            mesh( assets, "City street furniture", streetMetal, material( assets, { .16f, .19f, .20f, 1 }, .48f, .6f ) );
            mesh( assets, "City lamp lenses", lampGlass, material( assets, { .80f, .82f, .76f, 1 }, .35f ) );
            mesh( assets, "City parks", parks, material( assets, { .20f, .34f, .14f, 1 }, 1 ) );
            buildCityStreetDressing( assets, city, quality );
            Geometry ground;
            ground.box( { 0, -.06f, 0 }, { city.extent * 2 + 80, .1f, city.extent * 2 + 80 } );
            if( !city.naturalRoads && !assets.cityTerrainRoads )
                mesh( assets, "City ground", ground, material( assets, { .28f, .30f, .26f, 1 }, 1 ) );
            // Bounded driving remains inside the collision walls. A surrounding ground
            // surface gives the skyline context instead of ending the city in the sky.
            Geometry hinterland;
            const float horizon = city.extent + 1400;
            if(assets.cityTerrainRoads)
            {
                const auto snapshot=assets.roadAuthoring->getTerrainSnapshot();
                const float radius=std::max(std::abs(snapshot->origin.x),std::abs(snapshot->origin.y));
                const auto plane=[](float x,float z){return 2+.005f*x+.003f*z-.2f;};
                const auto strip=[&](float x0,float z0,float x1,float z1){hinterland.quad(
                    {x0,plane(x0,z0),z0},{x1,plane(x1,z0),z0},{x1,plane(x1,z1),z1},{x0,plane(x0,z1),z1},{0,1,0});};
                strip(-horizon,-horizon,horizon,-radius);strip(-horizon,radius,horizon,horizon);
                strip(-horizon,-radius,-radius,radius);strip(radius,-radius,horizon,radius);
            }
            else hinterland.quad( { -horizon, -.12f, -horizon }, { horizon, -.12f, -horizon },
                                  { horizon, -.12f, horizon }, { -horizon, -.12f, horizon }, { 0, 1, 0 } );
            auto land = material( assets, { .34f, .48f, .25f, 1 }, 1 );
            auto soil = forge->bakeSurface( SurfaceTag::Dirt, surfaceParams );
            maps( assets, land, soil.albedo, soil.normal, soil.orm );
            land->setDiffuse( { .34f, .48f, .25f, 1 } );
            land->setUVProjection( 3 ); land->setEditorFloat( "uvTriplanarScale", .1f );
            land->setNormalStrength( .15f );
            land->setEditorFloat( "aoStrength", .4f );
            if( city.naturalRoads && !assets.cityTerrainRoads ) mesh( assets, "City ground", ground, land );
            mesh( assets, "City surrounding landscape", hinterland, land );
            if( assets.useFoliageWorld )
            {
                buildFoliageWorld( assets, seed, quality );
            }
            assets.circuit = {};
            for( size_t i = 0; i < city.route.size(); ++i )
            {
                const auto p = city.route[i], next = city.route[( i + 1 ) % city.route.size()];
                const auto position=assets.cityTerrainRoads?fittedRoute[i]:Vector3F(p.x,0,p.z);
                const auto nextPosition=assets.cityTerrainRoads?fittedRoute[(i+1)%fittedRoute.size()]:Vector3F(next.x,0,next.z);
                Vector3F forward=nextPosition-position;
                forward.normalise();
                assets.circuit.samples.push_back(
                    { position, { -forward.z, 0, forward.x }, assets.circuit.length } );
                assets.circuit.length += (nextPosition-position).length();
            }
            Geometry arrows;
            for( size_t i = 0; i < assets.circuit.samples.size(); i += 20 )
            {
                const auto &sample = assets.circuit.samples[i];
                const auto forward = Vector3F( sample.right.z, 0, -sample.right.x );
                const auto p = sample.position + Vector3F( 0, .04f, 0 );
                const auto tip = p + forward * 2;
                arrows.quad( p - sample.right * 1.5f, p + sample.right * 1.5f,
                             tip + sample.right * .01f, tip - sample.right * .01f, { 0, 1, 0 } );
            }
            if(!assets.cityTerrainRoads)mesh( assets, "City race route arrows", arrows,
                  material( assets, { .05f, .7f, .9f, 1 }, .6f ) );
        }

        void buildRoadAuthoringShowcase(SceneAssets &assets,SmartPtr<scene::IGameActor> vehicle,u32,
                                        VehicleAppearanceQuality)
        {
            auto app=core::IApplicationManager::instancePtr();auto manager=app->getGameManager();auto scene=manager->getCurrentScene();
            const auto root=[&](const String &name)
            {
                reserveOwnerSlot(assets.actors);auto actor=manager->createActor();
                if(!actor)throw std::runtime_error("Cannot create road authoring showcase root.");
                assets.actors.push_back(actor);actor->setName(name);actor->setStatic(true);
                actor->setPosition(Vector3F::zero());actor->setOrientation(QuaternionF::identity());actor->setScale(Vector3F::unit());
                scene->addActor(actor);scene->registerAllUpdates(actor);return actor;
            };
            auto terrainActor=root("Road authoring showcase terrain");assets.ground=terrainActor;
            assets.roadTerrain=terrainActor->addComponent<TerrainSystem>();
            if(!assets.roadTerrain || !terrainActor->addComponent<CollisionTerrain>() || !terrainActor->addComponent<Rigidbody>())
                throw std::runtime_error("Road showcase requires native terrain render and collision components.");
            if(!assets.roadTerrain->isLoaded())assets.roadTerrain->load(nullptr);
            auto terrainMaterial=terrainActor->addComponent<scene::Material>();if(terrainMaterial)terrainMaterial->setMaterialPath("DefaultTerrain.mat");
            assets.roadTerrain->setTreesEnabled(false);assets.roadTerrain->setGrassEnabled(false);
            render::TerrainData terrain;terrain.dimensions={97,97};terrain.spacing={2,2};terrain.origin={-96,-96};terrain.heights.resize(97*97);
            terrain.layers[0].tint={.34f,.40f,.22f,1};terrain.layers[1].tint={.30f,.25f,.17f,1};terrain.layerWeights.resize(97*97);
            terrain.appearance.macroScale=.025f;terrain.appearance.macroStrength=.22f;
            terrain.appearance.macroSeed=42;terrain.appearance.macroTint={.32f,.39f,.22f,1};
            for(int z=0;z<97;++z)for(int x=0;x<97;++x)
            {
                const float px=-96+x*2.f,pz=-96+z*2.f,r=std::hypot(px,pz),outside=std::clamp((r-40)/56.f,0.f,1.f);
                terrain.heights[size_t(z)*97+x]=3+.015f*px+.008f*pz+2.1f*outside*outside*std::sin(px*.035f)*std::cos(pz*.035f);
                terrain.layerWeights[size_t(z)*97+x]={1-outside*.35f,outside*.35f,0,0};
            }
            // This new root has no visible generation yet. Publish its initial CPU/
            // collision snapshot before the first deferred GPU upload can run.
            auto terrainRenderer=dynamic_pointer_cast<render::Terrain>(assets.roadTerrain->getTerrain());
            const bool deferred=terrainRenderer && terrainRenderer->isTerrainPublicationDeferred();
            if(deferred)terrainRenderer->setDeferredTerrainPublication(false);
            String error;const bool terrainApplied=assets.roadTerrain->applyTerrainData(terrain,error);
            if(deferred)terrainRenderer->setDeferredTerrainPublication(true);
            if(!terrainApplied)throw std::runtime_error(error.c_str());
            if(!assets.roadTerrain->getTerrain())throw std::runtime_error("Road showcase native terrain renderer is unavailable after component load.");
            auto authoringActor=root("Road authoring showcase source");assets.roadAuthoring=authoringActor->addComponent<CityRoadAuthoring>();
            if(!assets.roadAuthoring || !assets.roadAuthoring->bindTerrain(assets.roadTerrain,error))throw std::runtime_error(error.empty()?"Cannot bind showcase authoring terrain.":error.c_str());
            const auto id=[](unsigned value){char text[37];std::snprintf(text,sizeof(text),"f1a00000-2026-4000-8000-%012u",value);return SourceBudgetText(text);};
            procedural::CityRoadSplineSource source;source.version=2;source.id=id(1);
            const auto binding=assets.roadAuthoring->getTerrainBindingUUID();source.terrainResourceUuid.assign(binding.data(),binding.size());
            procedural::CityRoadRoundaboutSpec junction;junction.junctionId=id(2);junction.center={0,3.025f,0};junction.outerRadius=14;junction.islandRadius=8;
            junction.clockwise=true;junction.gradeX=.015f;junction.gradeZ=.008f;junction.maxGrade=.08f;
            const auto snapshot=assets.roadTerrain->getTerrainSnapshot();
            for(unsigned i=0;i<4;++i)
            {
                const Vector3F outward=i==0?Vector3F(1,0,0):i==1?Vector3F(0,0,1):i==2?Vector3F(-1,0,0):Vector3F(0,0,-1);
                const Vector3F side(-outward.z,0,outward.x);const float bend=i==3?1.5f:(i%2?10.f:-10.f);
                procedural::CityRoadSpline road;road.id=id(10+i);road.width=6;road.maxGrade=.08f;road.heightOffset=.025f;
                procedural::CityRoadRoundaboutPort port;port.approachId=road.id;port.outward=outward;port.width=6;port.length=18;junction.ports.push_back(port);
                auto start=outward*32.f,end=outward*85.f+side*bend;
                if(!snapshot || !render::sampleTerrainHeight(*snapshot,Transform3<real_Num>(),start,start.y) || !render::sampleTerrainHeight(*snapshot,Transform3<real_Num>(),end,end.y))
                    throw std::runtime_error("Showcase spline endpoint escaped terrain coverage.");
                auto out=outward*18.f,in=-outward*18.f-side*(bend*.5f);out.y=.015f*out.x+.008f*out.z;in.y=.015f*in.x+.008f*in.z;
                road.knots.push_back({id(20+i*2),start,{},out});road.knots.push_back({id(21+i*2),end,in,{}});source.roads.push_back(std::move(road));
            }
            source.roundabouts.push_back(junction);SourceBudgetText text;
            if(!procedural::serializeCityRoadSplineSource(source,text,error) || !assets.roadAuthoring->setSourceJSON(String(text.data(),text.size()),assets.roadAuthoring->getSourceRevision(),error))
                throw std::runtime_error(error.c_str());
            assets.triangles+=96*96*2+assets.roadAuthoring->getRuntimeTriangleCount();
            assets.useFoliageWorld=false;assets.circuit={};
            for(unsigned i=0;i<96;++i)
            {
                const float direction=junction.clockwise?1.f:-1.f,angle=direction*i*2*pi/96,next=direction*(i+1)*2*pi/96,radius=11;
                Vector3F p(std::cos(angle)*radius,0,std::sin(angle)*radius),q(std::cos(next)*radius,0,std::sin(next)*radius);
                bool pDriveable=false,qDriveable=false;
                if(!assets.roadAuthoring->getRuntimeSurface(p.x,p.z,p.y,pDriveable) || !pDriveable ||
                   !assets.roadAuthoring->getRuntimeSurface(q.x,q.z,q.y,qDriveable) || !qDriveable)
                    throw std::runtime_error("Showcase circulation route left the published driveable roundabout surface.");
                const auto forward=(q-p).normaliseCopy();assets.circuit.samples.push_back({p,{-forward.z,0,forward.x},assets.circuit.length});assets.circuit.length+=(q-p).length();
            }
            f32 spawnY=0;if(!render::sampleTerrainHeight(*snapshot,Transform3<real_Num>(),{0,0,-60},spawnY))throw std::runtime_error("Showcase spawn has no terrain.");
            vehicle->setPosition({0,spawnY+.445f,-60});vehicle->updateTransform();
            if(auto rigidbody=vehicle->getComponent<Rigidbody>())if(auto body=rigidbody->getRigidDynamic())body->setTransform(Transform3F(vehicle->getPosition(),QuaternionF::identity()));
        }

        void buildSky( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle )
        {
            auto app = core::IApplicationManager::instance();
            auto sky =
                app->getFactoryManager()->createObjectFromType<ISkyAtmosphere>( "ISkyAtmosphere" );
            if( !sky )
            {
                throw std::runtime_error( "WPProcedural atmosphere is unavailable." );
            }
            SkyState state;
            state.timeOfDay = 15;
            state.turbidity = 2.4f;
            sky->setState( state );
            sky->update();
            const auto result = sky->getResults();
            auto graphicsScene = app->getGraphicsSystem()->getGraphicsScene();

            auto faces=makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            auto reflectionFaces=makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            const auto probePosition = vehicle->getPosition() + Vector3F( 0, 2, 0 );
            const Vector3F corners[8] = { { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 },
                                          { -1, -1, 1 },  { 1, -1, 1 },  { 1, 1, 1 },  { -1, 1, 1 } };
            const int faceCorners[6][4] = { { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
                                            { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };
            for( int face = 0; face < 6; ++face )
            {
                TextureBuffer texture( 256, 256 );
                auto reflectionFace=makeSourceBudgetSnapshot<f32>();
                reflectionFace.resize(256*256*4);
                for( u32 y = 0; y < texture.height; ++y )
                {
                    for( u32 x = 0; x < texture.width; ++x )
                    {
                        float u = ( x + .5f ) / texture.width, v = ( y + .5f ) / texture.height;
                        // Image rows run top to bottom; the sky's bottom corners use UV.y=1.
                        auto left = corners[faceCorners[face][0]] * v +
                                    corners[faceCorners[face][3]] * ( 1 - v );
                        auto right = corners[faceCorners[face][1]] * v +
                                     corners[faceCorners[face][2]] * ( 1 - v );
                        auto dir = left * ( 1 - u ) + right * u;
                        dir.normalise();
                        auto c = sky->computeSkyColour( dir, result.sunDir, state.turbidity,
                                                        result.sunAltitude );
                        const auto haze = 1.f - std::max( dir.y, 0.f );
                        const auto glow =
                            std::pow( std::max( 0.f, dir.dotProduct( result.sunDir ) ), 128.f );
                        c.r = .06f + .35f * haze + c.r * .15f + glow * .6f;
                        c.g = .22f + .35f * haze + c.g * .15f + glow * .55f;
                        c.b = .58f + .24f * haze + c.b * .15f + glow * .4f;
                        // Keep the solar core as radiance in the reflection source.
                        // The visible LDR sky clips its display value; the probe does not.
                        const float solarCore =
                            std::pow( std::max( 0.f, dir.dotProduct( result.sunDir ) ), 2048.f );
                        c.r += solarCore * 16.f;
                        c.g += solarCore * 14.f;
                        c.b += solarCore * 10.f;
                        auto reflectionColour = c;
                        if( dir.y < 0 )
                        {
                            // Bake the start straight and surrounding grass into the lower
                            // hemisphere. This is a custom environment, not a realtime capture.
                            const auto hit = probePosition + dir * ( -probePosition.y / dir.y );
                            const bool road = std::abs( hit.x ) < 6 && hit.z > -90 && hit.z < 95;
                            const bool edge = std::abs( std::abs( hit.x ) - 5.7f ) < 0.12f && road;
                            reflectionColour.r = edge ? .7f : road ? .045f : .07f;
                            reflectionColour.g = edge ? .7f : road ? .05f : .13f;
                            reflectionColour.b = edge ? .7f : road ? .06f : .045f;
                            const float horizon = std::clamp( -dir.y * 20.f, 0.f, 1.f );
                            reflectionColour.r = c.r * ( 1 - horizon ) + reflectionColour.r * horizon;
                            reflectionColour.g = c.g * ( 1 - horizon ) + reflectionColour.g * horizon;
                            reflectionColour.b = c.b * ( 1 - horizon ) + reflectionColour.b * horizon;
                        }
                        auto pixel = texture.pixel( x, y );
                        auto reflectedPixel =
                            reflectionFace.data() + ( size_t( y ) * texture.width + x ) * 4;
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            float value = channel == 0 ? c.r : channel == 1 ? c.g : c.b;
                            pixel[channel] = u8( std::clamp(
                                std::pow( std::max( value, 0.f ), 1.f / 2.2f ) * 255.f, 0.f, 255.f ) );
                            const float reflectedValue = channel == 0   ? reflectionColour.r
                                                         : channel == 1 ? reflectionColour.g
                                                                        : reflectionColour.b;
                            reflectedPixel[channel] = std::max( reflectedValue, 0.f );
                        }
                        pixel[3] = 255;
                        reflectedPixel[3] = 1;
                    }
                }
                auto skyTexture = upload( assets, texture );
                faces.push_back( skyTexture );
                reflectionFaces.push_back(
                    uploadLinearRadiance( assets, reflectionFace, { 256, 256 } ) );
            }
            auto skyObject = graphicsScene->addGraphicsObjectByType<render::ISky>();
            assets.sky = skyObject;
            skyObject->setTextures( faces );
            skyObject->setDistance( 500 );
            skyObject->setVisible( true );

            reserveOwnerSlot(assets.textures);
            auto reflection =
                app->getGraphicsSystem()->getTextureManager()->createCubeMap( reflectionFaces );
            if( reflection )
            {
                assets.reflectionTexture = reflection;
                assets.textures.push_back( reflection );
                reflection->setName( assets.resourcePrefix + "/reflection" );
                // Eight RGBA32F levels at 128px per face, generated by ClawCubemap.
                assets.textureBytes += 6u * 21845u * 16u;
                auto probeResource = make_ptr<render::GraphicsCubemap>();
                probeResource->setTexture( reflection );
                probeResource->setTextureName( reflection->getName() );
                probeResource->setSceneManager( graphicsScene );
                reserveOwnerSlot(assets.actors);
                auto actor = app->getGameManager()->createActor();
                if(!actor) throw std::runtime_error("Cannot create procedural actor");
                assets.actors.push_back(actor);
                actor->setName( "Vehicle Reflection Cubemap" );
                actor->setPosition( probePosition );
                auto component = actor->addComponent<scene::Cubemap>();
                component->setProbeName( "Vehicle Outdoor Environment" );
                component->setSourceType( scene::Cubemap::SourceType::Custom );
                component->setCubemapPath( reflection->getName() );
                component->setResolution( 128 );
                component->setProjectionMode( scene::Cubemap::ProjectionMode::Infinite );
                component->setAutoEnableByDistance( false );
                component->setRenderCubemap( probeResource );
                component->setEnabled( true );
                const auto reflectionSlot = static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION );
                for( auto &vehicleMaterial : assets.vehicleMaterials )
                {
                    vehicleMaterial->setTexture( reflection, reflectionSlot );
                }
                if( assets.openCity )
                {
                    // The outdoor HDR sky also lights architectural specular response.
                    // Recessed opaque glazing still has no transmission/interior model.
                    for( auto &cityMaterial : assets.materials )
                        cityMaterial->setTexture( reflection, reflectionSlot );
                }
                // The actor's material also lets Cubemap manage the representative body material.
                actor->addComponent<scene::Material>()->setMaterial( assets.vehicleMaterials.front() );
                auto scene = app->getGameManager()->getCurrentScene();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );
                assets.reflectionActor = actor;
                WP_LOG(
                    "VehicleAdvanced: custom cubemap actor created; reflection applied to all vehicle "
                    "materials." );
            }

            graphicsScene->setAmbientLight( assets.openCity
                ? ColourF( .55f, .57f, .60f, 1 ) : ColourF( .28f, .32f, .38f, 1 ) );
            // The native HDR city path fogs in scene-linear space before its final
            // filmic transfer. Retain the legacy circuit presentation separately.
            auto horizon = sky->computeSkyColour( Vector3F( 1, 0, 0 ), result.sunDir, state.turbidity,
                                                  result.sunAltitude );
            const auto fogRadiance = [&]( float displayedLinear ) {
                const float colour = std::clamp( displayedLinear, 0.f, .95f );
                return assets.openCity ? colour : colour / ( 1 - colour );
            };
            graphicsScene->setFog(
                render::IGraphicsScene::FOG_LINEAR,
                ColourF( fogRadiance( .41f + horizon.r * .15f ), fogRadiance( .57f + horizon.g * .15f ),
                         fogRadiance( .82f + horizon.b * .15f ), 1 ),
                .0007f, 180, 800 );
            // Own the sky's matching sun. The showcase explicitly selects this light;
            // existing host lighting remains available to the scene's fallback policy.
            auto sun = graphicsScene->addGraphicsObjectByType<render::IGraphicsLight>();
            if( !sun )
            {
                throw std::runtime_error( "Cannot create procedural sunlight." );
            }
            assets.sun = sun;
            sun->setType( LightTypes::LT_DIRECTIONAL );
            sun->setDirection( -result.sunDir );
            sun->setDiffuseColour( ColourF( 1, .96f, .88f, 1 ) );
            sun->setPowerScale( assets.openCity ? 3.f : 1.8f );
            sun->setCastShadows( true );
            sun->setVisible( true );
        }
    }  // namespace

    size_t Circuit::nearest( const Vector3F &position ) const
    {
        size_t result = 0;
        float best = std::numeric_limits<float>::max();
        for( size_t i = 0; i < samples.size(); ++i )
        {
            auto d = ( samples[i].position - position ).length();
            if( d < best )
            {
                best = d;
                result = i;
            }
        }
        return result;
    }

    Circuit generateCircuit( u32 seed )
    {
        SourceBudgetVector<Vector3F> knots = { { 0, 0, 70 },    { 0, 0, 0 },     { 0, 0, -90 },
                                        { 20, 0, -165 }, { 80, 0, -180 }, { 140, 0, -150 },
                                        { 155, 0, -70 }, { 105, 0, -35 }, { 145, 0, 30 },
                                        { 135, 0, 105 }, { 50, 0, 130 },  { 0, 0, 95 } };
        // Seed changes the infield bends; the start straight and spawn remain fixed.
        std::mt19937 rng( seed );
        std::uniform_real_distribution<float> variation( -6, 6 );
        for( size_t i = 3; i < 11; ++i )
        {
            knots[i].x += variation( rng );
            knots[i].z += variation( rng );
        }
        SourceBudgetVector<Vector3F> dense;
        for( size_t i = 0; i < knots.size(); ++i )
        {
            for( int step = 0; step < 100; ++step )
            {
                auto a = knots[( i + knots.size() - 1 ) % knots.size()], b = knots[i],
                     c = knots[( i + 1 ) % knots.size()], d = knots[( i + 2 ) % knots.size()];
                float t = step / 100.f;
                dense.push_back( ( b * 2 + ( c - a ) * t + ( a * 2 - b * 5 + c * 4 - d ) * t * t +
                                   ( -a + b * 3 - c * 3 + d ) * t * t * t ) *
                                 .5f );
            }
        }
        SourceBudgetVector<float> distance( dense.size() + 1, 0 );
        for( size_t i = 0; i < dense.size(); ++i )
        {
            distance[i + 1] = distance[i] + ( dense[( i + 1 ) % dense.size()] - dense[i] ).length();
        }
        Circuit result;
        result.length = distance.back();
        auto count = static_cast<size_t>( std::ceil( result.length / 1.5f ) );
        size_t segment = 0;
        for( size_t i = 0; i < count; ++i )
        {
            float s = result.length * i / count;
            while( segment + 1 < dense.size() && distance[segment + 1] < s )
            {
                ++segment;
            }
            float t = ( s - distance[segment] ) / ( distance[segment + 1] - distance[segment] );
            result.samples.push_back(
                { dense[segment] + ( dense[( segment + 1 ) % dense.size()] - dense[segment] ) * t,
                  {},
                  s } );
        }
        // Make progress zero at the finish stripe, travelling in the car's -Z direction.
        auto start = result.nearest( { 0, 0, 0 } );
        std::rotate( result.samples.begin(), result.samples.begin() + start, result.samples.end() );
        for( size_t i = 0; i < count; ++i )
        {
            auto tangent = result.samples[( i + 1 ) % count].position -
                           result.samples[( i + count - 1 ) % count].position;
            tangent.normalise();
            result.samples[i].right = { -tangent.z, 0, tangent.x };
            result.samples[i].distance = result.length * i / count;
        }
        if( result.samples.size() < 3 || result.length < 500 )
        {
            throw std::runtime_error( "Invalid generated circuit." );
        }
        return result;
    }

    // Private module entry point shared with ProceduralRaceScene.cpp. Activation
    // is deferred while a saved scene is reconstructing its actor hierarchy.
    bool activateGeneratedScene( const SceneAssets &assets, SmartPtr<scene::IGameActor> owner, String &error )
    {
        error.clear();
        auto app=core::IApplicationManager::instancePtr();auto manager=app?app->getGameManager():nullptr;
        auto currentScene=manager?manager->getCurrentScene():nullptr;
        if(!owner || !currentScene){error="Generated scene activation has no owner/current scene.";return false;}
        if(currentScene->getSceneLoadingState()!=scene::IGameScene::SceneLoadingState::Loaded ||
           !Thread::getTaskFlag(Thread::Application_Flag))return false;
        try
        {
            // Children are not scene roots: Lua reparents generated scenery and
            // the Sample attaches render children before adding its car actor.
            // Require a live scene-owned ancestor, rather than hierarchy-enabled
            // alone (which also accepts removed, still-loaded actors).
            const auto inCurrentHierarchy=[&](SmartPtr<scene::IGameActor> candidate) {
                bool anchored=false;size_t depth=0;
                for(;candidate && depth<1024;candidate=candidate->getParent(),++depth)
                {
                    if(!candidate->isLoaded())return false;
                    const auto candidateScene=candidate->getScene();
                    if(candidateScene && candidateScene!=currentScene)return false;
                    anchored|=candidateScene==currentScene && candidate->getFlag(scene::IGameActor::ActorFlagInScene);
                }
                return !candidate && anchored;
            };
            // Initial Sample construction and scene restoration can precede
            // attaching the owner. Leave readiness false until that boundary.
            if(!inCurrentHierarchy(owner))return false;
            SourceBudgetVector<SmartPtr<scene::IGameActor>> cohort;
            cohort.reserve(assets.actors.size()+2);
            // Persistent anchors live outside assets.actors and must also have
            // completed lifecycle activation after an explicit terrain rebind.
            const auto terrainActor=assets.roadTerrain?assets.roadTerrain->getActor():nullptr;
            const auto sourceActor=assets.roadAuthoring?assets.roadAuthoring->getActor():nullptr;
            if(terrainActor)cohort.push_back(terrainActor);
            if(sourceActor && sourceActor!=terrainActor)cohort.push_back(sourceActor);
            for(const auto &generated:assets.actors)
                if(generated && generated!=terrainActor && generated!=sourceActor)cohort.push_back(generated);
            for(const auto &generated:cohort)
                if(!inCurrentHierarchy(generated))throw std::runtime_error("Generated actor is detached from the current scene: "+generated->getName());
            const auto actorState=app->isPlaying()?scene::IGameActor::State::Play:scene::IGameActor::State::Edit;
            const auto componentState=app->isPlaying()?scene::IComponent::State::Play:scene::IComponent::State::Edit;
            SourceBudgetVector<SmartPtr<scene::IGameActor>> ancestors;ancestors.reserve(16);
            // Resolve parent poses directly before a Rigidbody Enter event can
            // create native contacts. Do not pump unrelated scene updates.
            for(const auto &generated:cohort)
            {
                ancestors.clear();auto parent=generated;
                while(parent && ancestors.size()<1024){ancestors.push_back(parent);parent=parent->getParent();}
                if(parent)throw std::runtime_error("Generated scene activation found an invalid hierarchy depth/cycle.");
                for(auto it=ancestors.rbegin();it!=ancestors.rend();++it)if(auto transform=(*it)->getTransform())transform->update();
            }
            for(auto generated:cohort)
            {
                if(!generated->isLoaded())throw std::runtime_error("Generated actor is not loaded: "+generated->getName());
                generated->setState(actorState,true);
                // Same-state actors can still contain newly added components.
                for(auto component:generated->getComponents())if(component && component->isLoaded())component->setState(componentState);
                generated->updateTransform();
                if(generated->getState()!=actorState)throw std::runtime_error("Generated actor lifecycle activation is deferred: "+generated->getName());
            }
            auto physicsManager=app->getPhysicsManager();auto physics=physicsManager?physicsManager->getPhysicsScene():nullptr;
            if(!physics)throw std::runtime_error("Generated scene activation requires a native physics scene.");
            for(const auto &generated:cohort)if(generated->isEnabledInScene())
            {
                auto collision=generated->getComponent<scene::Collision>();if(!collision || !collision->isEnabled())continue;
                auto rigidbody=generated->getComponent<scene::Rigidbody>();auto body=rigidbody?rigidbody->getRigidStatic():nullptr;
                if(!generated->isStatic() || !rigidbody || !body || !collision->getShape() || !collision->getShape()->isAttached() ||
                   body->getScene()!=physics || body->getNumShapes()==0 || rigidbody->getState()!=componentState || collision->getState()!=componentState)
                    throw std::runtime_error("Generated static collision publication is incomplete: "+generated->getName());
                const auto expected=generated->getWorldTransform(),actual=body->getTransform();
                if((expected.getPosition()-actual.getPosition()).length()>.001f ||
                   (expected.getOrientation()*Vector3F::unitX()-actual.getOrientation()*Vector3F::unitX()).length()>.001f)
                    throw std::runtime_error("Generated static collision pose is stale: "+generated->getName());
            }
            return true;
        }
        catch(const std::exception &failure){error=failure.what();return false;}
    }

    void buildScene( SceneAssets &assets, SmartPtr<scene::IGameActor> actor, u32 seed,
                     VehicleAppearanceQuality quality )
    {
        assets.resourcePrefix = "__procedural/race/" + StringUtil::getUUID();
        const auto started = std::chrono::steady_clock::now();
        buildCar( assets, actor, seed, quality );
        if( assets.cityRoadShowcase )
        {
            buildRoadAuthoringShowcase(assets,actor,seed,quality);
        }
        else if( assets.openCity )
        {
            buildCity( assets, actor, seed, quality );
        }
        else
        {
            buildTrack( assets, seed, quality );
        }
        buildSky( assets, actor );
        TextureBuffer shade( 64, 64 );
        for( u32 y = 0; y < 64; ++y )
        {
            for( u32 x = 0; x < 64; ++x )
            {
                auto pixel = shade.pixel( x, y );
                const auto dx = ( x - 31.5f ) / 31.5f, dz = ( y - 31.5f ) / 31.5f;
                pixel[0] = pixel[1] = pixel[2] = 0;
                pixel[3] = u8( 130 * std::pow( std::max( 0.f, 1.f - dx * dx - dz * dz ), 2 ) );
            }
        }
        auto shadowMaterial = material( assets, ColourF::White, 1 );
        shadowMaterial->setTexture( upload( assets, shade ), 0 );
        shadowMaterial->setTransparent( true );
        shadowMaterial->setBlendMode( 1 );
        shadowMaterial->setDepthWrite( false );
        Geometry shadowGeometry;
        shadowGeometry.quad( { -1.6f, .025f, -3.7f }, { 1.6f, .025f, -3.7f }, { 1.6f, .025f, 3.7f },
                             { -1.6f, .025f, 3.7f }, { 0, 1, 0 } );
        assets.shadow = mesh( assets, "Vehicle contact shadow", shadowGeometry, shadowMaterial );
        if(assets.cityTerrainRoads)
            for(auto generated:assets.actors)generated->setFlag(IGameActor::ActorFlagDontSave,true);
        if( auto root = actor->getParent() )
        {
            auto scene = core::IApplicationManager::instance()->getGameManager()->getCurrentScene();
            // Serialize generated scenery only under the game's owned root.
            // Removing the old root after Editor restoration then removes all
            // scenery/colliders too, even when Lua's native references were lost.
            for( const auto &generated : assets.actors )
            {
                if( !generated->getParent() )
                {
                    scene->removeActor( generated );
                    root->addChild( generated );
                    scene->registerAllUpdates( generated );
                }
            }
        }

        String activationError;
        if(!activateGeneratedScene(assets,actor,activationError) && !activationError.empty())throw std::runtime_error(activationError.c_str());
        const auto generationMs =
            std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - started )
                .count();
        WP_LOG( String( "VehicleAdvanced generation milliseconds=" ) +
                StringUtil::toString( generationMs ) );
        WP_LOG( String( "VehicleAdvanced generated: seed=" ) + StringUtil::toString( seed ) +
                " hash=" + StringUtil::toString( assets.vehicle.contentHash ) +
                " triangles=" + StringUtil::toString( assets.triangles ) +
                " texture bytes=" + StringUtil::toString( assets.textureBytes ) +
                " circuit metres=" + StringUtil::toString( assets.circuit.length ) );
    }

    bool validateReflection( const SceneAssets &assets )
    {
        if( !assets.reflectionTexture )
        {
            return true;
        }
        auto component =
            assets.reflectionActor ? assets.reflectionActor->getComponent<scene::Cubemap>() : nullptr;
        auto probe = component ? component->getRenderCubemap() : nullptr;
        if( !component || !component->isEnabled() || !probe ||
            probe->getTexture() != assets.reflectionTexture || assets.vehicleMaterials.empty() )
        {
            return false;
        }
        const auto slot = static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION );
        for( const auto &material : assets.vehicleMaterials )
        {
            if( material->getTexture( slot ) != assets.reflectionTexture )
            {
                return false;
            }
        }
        void *native = nullptr;
        assets.reflectionTexture->getTextureFinal( &native );
        return native != nullptr;
    }

    void validateCircuit()
    {
        for( u32 seed = 0; seed < 100; ++seed )
        {
            auto circuit = generateCircuit( seed ), repeat = generateCircuit( seed );
            if( circuit.length != repeat.length || circuit.samples.size() != repeat.samples.size() )
            {
                throw std::runtime_error( "Circuit generation is not deterministic." );
            }
            const auto spacing = circuit.length / circuit.samples.size();
            for( size_t i = 0; i < circuit.samples.size(); ++i )
            {
                const auto &a = circuit.samples[i],
                           &b = circuit.samples[( i + 1 ) % circuit.samples.size()];
                const auto chord = ( b.position - a.position ).length();
                if( a.position != repeat.samples[i].position || a.position.y != 0 ||
                    std::abs( a.right.length() - 1 ) > 1e-4f || chord < spacing * .95f ||
                    chord > spacing * 1.01f )
                {
                    throw std::runtime_error( "Circuit closure, spacing, or frame validation failed." );
                }
            }
            if( circuit.samples.front().position.length() > spacing ||
                circuit.samples.front().right.x < .99f )
            {
                throw std::runtime_error( "Circuit spawn does not align with the start straight." );
            }
        }
        if( generateCircuit( 7 ).samples[100].position == generateCircuit( 8 ).samples[100].position )
        {
            throw std::runtime_error( "Seed does not alter the circuit." );
        }
    }

    void configurePhysics( const SceneAssets &assets, SmartPtr<scene::IGameActor> actor )
    {
        const auto &config = assets.vehicle.physics;
        auto car = actor->getComponent<scene::CarController>();
        auto vehicle = car->getVehicleController();
        auto body = actor->getComponent<scene::Rigidbody>();
        auto com = vector( config.massProperties.centreOfMass );
        car->setMass( float( config.massProperties.massKg ) );
        vehicle->setMass( float( config.massProperties.massKg ) );
        body->setMass( float( config.massProperties.massKg ) );
        car->setMOI( { float( config.massProperties.inertia.xx ),
                       float( config.massProperties.inertia.yy ),
                       float( config.massProperties.inertia.zz ) } );
        car->setDriveType( VehicleDriveType::RearWheelDrive );
        // Brush tyre forces use negative steering yaw. Adapt only the rendered
        // wheels; input and vehicle dynamics retain their existing convention.
        auto visualProperties = car->getProperties();
        visualProperties->setProperty( "Visual Steering Sign", -1.f );
        car->setProperties( visualProperties );
        auto controllerProperties = vehicle->getProperties();
        controllerProperties->setProperty( "Keyboard Input", false );
        controllerProperties->setProperty( "Steering Rate", 90.f );
        controllerProperties->setProperty( "Steering Wheelbase", float( config.wheelbaseM ) );
        controllerProperties->setProperty( "Steering Acceleration", 14.f );
        controllerProperties->setProperty( "Play Steering Scale",
                                           float( config.wheels[0].maxSteerRad * 180.0 / pi ) );
        vehicle->setProperties( controllerProperties );
        if( auto drive = vehicle->getDriveTrain() )
        {
            auto props = drive->getProperties();
            String ratios = "0,-2.5";
            for( auto ratio : config.drivetrain.forwardGearRatios )
            {
                ratios += "," + StringUtil::toString( ratio );
            }
            props->setProperty( "Gear Ratios", ratios );
            props->setProperty( "Final Drive Ratio", float( config.drivetrain.finalDriveRatio ) );
            props->setProperty( "Min RPM", float( config.drivetrain.idleRpm ) );
            props->setProperty( "Max RPM", float( config.drivetrain.redlineRpm ) );
            props->setProperty( "Max Torque", float( config.drivetrain.peakTorqueNm ) );
            props->setProperty( "Max Power", float( config.drivetrain.peakPowerW ) );
            props->setProperty( "Torque RPM", float( config.drivetrain.redlineRpm * .6 ) );
            props->setProperty( "Power RPM", float( config.drivetrain.redlineRpm * .85 ) );
            drive->setProperties( props );
        }
        // The chassis is centred on the computed COM. Wheel hubs and all render vertices use
        // the same origin. Keep ray mounts clear of the narrow monocoque collision box.
        actor->getComponent<scene::CollisionBox>()->setExtents( { .9f, .24f, 3.1f } );
        for( u32 i = 0; i < 4; ++i )
        {
            const auto &w = config.wheels[i];
            auto wheel = vehicle->getWheelController( i );
            auto p = vector( w.hubPosition ) - com;
            wheel->setLocalTransform( Transform3<real_Num>( p, QuaternionF::identity() ) );
            wheel->setRadius( float( w.tire.radiusM ) );
            wheel->setMass( float( config.massProperties.massKg *
                                   ( i < 2 ? config.massProperties.frontStaticWeightFraction
                                           : 1 - config.massProperties.frontStaticWeightFraction ) *
                                   .5 ) );
            wheel->setSpringRate( float( w.suspension.springRateNPerM * w.suspension.motionRatio *
                                         w.suspension.motionRatio ) );
            wheel->setDamping( float( w.suspension.damperCompressionNPerMps * w.suspension.motionRatio *
                                      w.suspension.motionRatio ) );
            wheel->setSuspensionTravel(
                float( w.suspension.bumpTravelM + w.suspension.reboundTravelM ) );
            wheel->setSuspensionDistance(
                float( w.suspension.bumpTravelM + w.suspension.reboundTravelM ) );
            wheel->setSteeringWheel( w.steerable );
            wheel->setPoweredWheel( w.driven );
            auto wheelProps = wheel->getProperties();
            wheelProps->setProperty( "Stable Contacts", true );
            wheelProps->setProperty( "Traction Control", true );
            wheelProps->setProperty( "Anti Lock Brakes", true );
            const auto &inertia = config.massProperties.inertia;
            const auto effectiveMass = 1.0 / ( 1.0 / config.massProperties.massKg +
                                               p.x * p.x / inertia.zz + p.z * p.z / inertia.xx );
            wheelProps->setProperty( "Contact Effective Mass", float( effectiveMass ) );
            wheelProps->setProperty( "Inertia", float( w.tire.wheelInertiaKgM2 ) );
            wheelProps->setProperty( "Brake Friction Torque", float( w.brakeTorqueNm ) );
            // Keep startup traction consistent with the assisted Grand Prix setup
            // and ProceduralRaceScene::updateSurfaceGrip's on-road multiplier.
            wheelProps->setProperty( "Grip", 1.0f );
            // A small rear reserve makes breakaway progressive under power.
            const auto axleGrip = i < 2 ? 1.0 : 1.06;
            wheelProps->setProperty(
                "Static Friction Coefficient",
                float( std::min( w.tire.peakLongitudinalFriction, w.tire.peakLateralFriction ) *
                       axleGrip ) );
            wheelProps->setProperty(
                "Sliding Friction Coefficient",
                float( std::min( w.tire.peakLongitudinalFriction, w.tire.peakLateralFriction ) *
                       axleGrip * .94 ) );
            wheelProps->setProperty( "Longitudinal Stiffness",
                                     float( w.tire.longitudinalStiffnessNPerSlip ) );
            wheelProps->setProperty( "Lateral Stiffness", float( w.tire.corneringStiffnessNPerRad *
                                                                 ( i < 2 ? 1.0 : 1.10 ) ) );
            wheel->setProperties( wheelProps );
            auto wheelActor = assets.wheels[i];
            wheelActor->setLocalPosition( p );
        }
    }
}  // namespace workphone::scene::race
