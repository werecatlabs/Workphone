#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <stdexcept>
#include <map>

namespace workphone::scene::race
{
    namespace
    {
        using namespace procedural;
        constexpr float pi = 3.14159265358979323846f;
        struct Geometry
        {
            Array<Vector3F> positions, normals, uv;
            Array<u32> indices;
            Array<Vector4F> tangents;
            void quad( Vector3F a, Vector3F b, Vector3F c, Vector3F d, Vector3F normal, float u0 = 0,
                       float u1 = 1, float v0 = 0, float v1 = 1 )
            {
                const auto base = static_cast<u32>( positions.size() );
                auto tangent = b - a;
                if( tangent.lengthSquared() < 1e-8f )
                    tangent = Vector3F( 1, 0, 0 );
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
                    indices.push_back( base + i );
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
            auto m = ApplicationUtil::createDefaultMaterial();
            m->load( nullptr );
            m->setCullMode( 1 );
            m->setDepthTest( 2 );
            m->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
            m->setDiffuse( colour );
            m->setRoughness( roughness );
            m->setMetalness( metalness );
            assets.materials.push_back( m );
            return m;
        }
        SmartPtr<render::ITexture> upload(
            SceneAssets &assets, const TextureBuffer &buffer,
            render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1 )
        {
            if( buffer.pixels.empty() )
                return nullptr;
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            auto name =
                assets.resourcePrefix + "/texture/" + StringUtil::toString( assets.textures.size() );
            auto texture = graphics->getTextureManager()->createManual( name, "General", 2, buffer.width,
                                                                        buffer.height, 1, 0, 0 );
            if( !texture )
                throw std::runtime_error( "Cannot create procedural texture." );
            auto properties = texture->getProperties();
            properties->setProperty( "mipmapFilter", static_cast<s32>( filter ) );
            properties->setProperty( "mipmapAtlasColumns", atlasColumns );
            texture->setProperties( properties );
            // ClawTexture's native storage uses BGRA8, while procedural buffers use RGBA8.
            auto bgra = buffer.pixels;
            for( size_t i = 0; i < bgra.size(); i += 4 )
                std::swap( bgra[i], bgra[i + 2] );
            texture->copyData( bgra.data(), Vector2I( buffer.width, buffer.height ) );
            assets.textureBytes += buffer.sizeBytes();
            assets.textures.push_back( texture );
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
                m->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
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
        SmartPtr<scene::IGameActor> mesh( SceneAssets &assets, const String &name, const Geometry &g,
                                          SmartPtr<render::IMaterial> m,
                                          SmartPtr<scene::IGameActor> parent = nullptr )
        {
            if( g.indices.empty() )
                return nullptr;
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
            auto resource =
                dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve( path ).first );
            resource->setName( path );
            resource->setFilePath( path );
            resource->setMesh( generated );
            resource->setLoadingState( LoadingState::Loaded );
            assets.meshes.push_back( resource );
            assets.triangles += g.indices.size() / 3;
            auto actor = app->getGameManager()->createActor();
            actor->setName( name );
            assets.actors.push_back( actor );
            if( parent )
                parent->addChild( actor );
            actor->addComponent<scene::Mesh>()->setMeshResource( resource );
            actor->addComponent<scene::MeshRenderer>();
            actor->addComponent<scene::Material>()->setMaterial( m );
            if( !parent )
                app->getGameManager()->getCurrentScene()->addActor( actor );
            return actor;
        }
        Vector3F vector( const VehiclePhysicsVector3 &p )
        {
            return { float( p.x ), float( p.y ), float( p.z ) };
        }
        void staticBox( SceneAssets &assets, const String &name, Vector3F centre, Vector3F size,
                        QuaternionF orientation = QuaternionF::identity() )
        {
            const auto probeStart = std::chrono::steady_clock::now();
            auto app = core::IApplicationManager::instance();
            auto actor = app->getGameManager()->createActor();
            const auto probeActor = std::chrono::steady_clock::now();
            // Own these independently of batched render meshes and tree LODs.
            // The normal generated-scene cleanup also removes their physics bodies.
            assets.actors.push_back( actor );
            actor->setName( "Trackside collision: " + name );
            actor->setStatic( true );
            actor->setPosition( centre );
            actor->setOrientation( orientation );
            const auto probeTransform = std::chrono::steady_clock::now();
            // CollisionBox takes full dimensions, not half extents.
            actor->addComponent<scene::CollisionBox>()->setExtents( size );
            const auto probeShape = std::chrono::steady_clock::now();
            actor->addComponent<scene::Rigidbody>();
            const auto probeBody = std::chrono::steady_clock::now();
            app->getGameManager()->getCurrentScene()->addActor( actor );
            static double probeTimes[5] = {};
            static unsigned probeCount = 0;
            const std::chrono::steady_clock::time_point probes[] = {probeStart, probeActor, probeTransform, probeShape, probeBody, std::chrono::steady_clock::now()};
            for (int i = 0; i < 5; ++i) probeTimes[i] += std::chrono::duration<double, std::milli>(probes[i+1]-probes[i]).count();
            if (++probeCount % 100 == 0) printf("Collision creation timing %u: actor %.1f transform %.1f shape %.1f body %.1f scene %.1f\n", probeCount, probeTimes[0], probeTimes[1], probeTimes[2], probeTimes[3], probeTimes[4]);
        }

        void railCollision( SceneAssets &assets, Vector3F start, Vector3F end )
        {
            auto direction = end - start;
            direction.y = 0;
            const auto length = direction.length();
            if( length <= 1e-4f )
                return;
            const auto yaw = std::atan2( direction.x, direction.z ) * 180.f / pi;
            staticBox( assets, "Guardrail beam", ( start + end ) * .5f + Vector3F( 0, .5f, 0 ),
                       { .2f, .5f, length }, QuaternionF::eulerDegrees( 0, yaw, 0 ) );
        }
        void pineGeometry( Geometry &trunks, Geometry &leaves, Vector3F p, float h )
        {
            trunks.box( p + Vector3F( 0, h * .3f, 0 ), { h * .05f, h * .6f, h * .05f } );
            for( int layer = 0; layer < 3; ++layer )
            {
                float y = h * ( .4f + .17f * layer ), radius = h * ( .27f - .06f * layer );
                for( int side = 0; side < 8; ++side )
                {
                    float a = 2 * pi * side / 8, b = 2 * pi * ( side + 1 ) / 8;
                    auto p0 = p + Vector3F( std::cos( a ) * radius, y, std::sin( a ) * radius );
                    auto p1 = p + Vector3F( std::cos( b ) * radius, y, std::sin( b ) * radius );
                    auto top = p + Vector3F( 0, y + h * .32f, 0 );
                    auto normal = ( p1 - p0 ).crossProduct( top - p0 );
                    normal.normalise();
                    leaves.quad( p0, p1, top, top, normal );
                }
            }
        }
        MeshImposterAtlas bakePineImposter()
        {
            Geometry trunks, canopy;
            pineGeometry( trunks, canopy, {}, 1 );
            Array<MeshImposterTriangle> triangles;
            auto append = [&]( const Geometry &g, ColourF colour ) {
                for( size_t i = 0; i < g.indices.size(); i += 3 )
                    triangles.push_back( { { g.positions[g.indices[i]], g.positions[g.indices[i + 1]],
                                             g.positions[g.indices[i + 2]] },
                                           colour } );
            };
            append( trunks, { .12f, .055f, .018f, 1 } );
            append( canopy, { .18f, .32f, .13f, 1 } );
            return generateMeshImposters( triangles, { -.28f, 0, -.28f }, { .28f, 1.08f, .28f } );
        }
        void pineImposterGeometry( Geometry &g, const MeshImposterAtlas &atlas, Vector3F p, float h )
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
                const float u0 = ( float( i * atlas.tileWidth ) + 1 ) / atlas.width;
                const float u1 = ( float( ( i + 1 ) * atlas.tileWidth ) - 2 ) / atlas.width;
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
                throw std::runtime_error( "VehicleAdvanced requires WPProcedural IVehicleGenerator." );
            VehicleGenerationConfig config;
            config.seed = seed;
            config.appearance.quality = quality;
            config.appearance.customMaxResolution = quality == VehicleAppearanceQuality::Preview ? 256
                                                    : quality == VehicleAppearanceQuality::Standard
                                                        ? 512
                                                        : 1024;
            assets.vehicle = generator->generate( config );
            if( !assets.vehicle.isValid() )
                throw std::runtime_error( "Procedural vehicle generation failed." );
            const auto &appearance = assets.vehicle.appearance;
            std::array<SmartPtr<render::IMaterial>, size_t( VehicleMaterialSlot::Count )> materials;
            for( size_t i = 0; i < materials.size(); ++i )
            {
                const auto &d = appearance.materials[i];
                auto m = material( assets, { d.baseColor.r, d.baseColor.g, d.baseColor.b, d.opacity },
                                   d.roughness, d.metallic );
                m->setEmissive( { d.emissiveColor.r, d.emissiveColor.g, d.emissiveColor.b, 1 } );
                auto props = m->getProperties();
                props->setProperty( "emissionIntensity", d.emissiveIntensity );
                props->setProperty( "enableEmission", d.emissiveIntensity > 0 );
                props->setProperty( "clearCoat", d.clearcoat );
                props->setProperty( "clearCoatRoughness", d.clearcoatRoughness );
                props->setProperty( "normalMapWeight", d.normalScale );
                m->setProperties( props );
                m->setSpecular( ColourF( .04f, .04f, .04f, 1 ) );
                m->setDiffuse( { d.baseColor.r, d.baseColor.g, d.baseColor.b, d.opacity } );
                m->setRoughness( d.roughness );
                m->setMetalness( d.metallic );
                m->setEmissive( { d.emissiveColor.r, d.emissiveColor.g, d.emissiveColor.b, 1 } );
                m->setEmissionEnabled( d.emissiveIntensity > 0 );
                if( d.doubleSided )
                    m->setCullMode( 1 );
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
                auto wheel = app->getGameManager()->createActor();
                wheel->setName( String( "Generated wheel " ) + StringUtil::toString( i ) );
                actor->addChild( wheel );
                wheel->addComponent<scene::WheelController>();
                assets.wheels[i] = wheel;
                assets.actors.push_back( wheel );
            }
            auto vehicleLOD = actor->addComponent<scene::LODGroup>();
            assets.vehicleLOD = vehicleLOD;
            vehicleLOD->setSize( 4.5f );
            vehicleLOD->setHysteresis( .15f );
            vehicleLOD->setCullBelowLastLOD( false );
            const size_t firstLOD = quality == VehicleAppearanceQuality::Preview ? 1 : 0;
            for( size_t lod = firstLOD; lod < assets.vehicle.geometry.lods.size(); ++lod )
            {
                Array<SmartPtr<scene::Renderer>> renderers;
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
                        auto part = mesh( assets, "LOD" + StringUtil::toString( lod ) + " " +
                            String( section.name.c_str() ), parts[i], m,
                                          rotating ? assets.wheels[i] : actor );
                        if( !assets.body && !rotating )
                            assets.body = part;
                        if( part )
                            renderers.push_back( part->getComponent<scene::MeshRenderer>() );
                    }
                }
                vehicleLOD->addLevel(
                    lod + 1 == assets.vehicle.geometry.lods.size() ? 0.f : ( lod == 0 ? .32f : .085f ),
                    renderers );
            }
            generator = nullptr;
        }
        void buildTrack( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality )
        {
            assets.circuit = generateCircuit( seed );
            auto app = core::IApplicationManager::instance();
            auto forge =
                app->getFactoryManager()->createObjectFromType<ITextureForge>( "ITextureForge" );
            if( !forge )
                throw std::runtime_error( "WPProcedural texture forge is unavailable." );
            forge->setSeed( seed );
            SurfaceBakeParams params;
            params.size = quality == VehicleAppearanceQuality::Preview ? 256 : 512;
            auto asphalt = forge->bakeSurface( SurfaceTag::Asphalt, params );
            // Keep the forge's seeded aggregate detail subtle at driving distance.
            // Its general-purpose height/rut contrast is too strong for a race surface.
            for( size_t i = 0; i < asphalt.albedo.pixels.size(); i += 4 )
            {
                for( size_t channel = 0; channel < 3; ++channel )
                    asphalt.albedo.pixels[i + channel] =
                        static_cast<u8>( 60 + asphalt.albedo.pixels[i + channel] / 6 );
                for( size_t channel = 0; channel < 2; ++channel )
                    asphalt.normal.pixels[i + channel] = static_cast<u8>(
                        128 + ( int( asphalt.normal.pixels[i + channel] ) - 128 ) / 10 );
            }
            auto roadMat = material( assets, { .07f, .075f, .085f, 1 }, .9f );
            maps( assets, roadMat, asphalt.albedo, asphalt.normal, asphalt.orm );
            roadMat->setNormalStrength( .2f );
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
                // Only change sampler fields. A full material property round-trip reloads
                // texture names, which can discard anonymous procedural texture bindings.
                auto properties = make_ptr<Properties>();
                properties->setProperty( "uvFilter", u32( 3 ) );
                properties->setProperty( "uvAniso", 8.f );
                if( sampled != roadMat )
                    properties->setProperty( "uvTriplanarScale", sampled == grass ? .08f : .25f );
                sampled->setProperties( properties );
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
                for( int z = 0; z < 2; ++z )
                    ( ( x + z ) % 2 ? checkerDark : checkerWhite )
                        .quad( { float( x - 6 ), .02f, float( z ) },
                               { float( x - 5 ), .02f, float( z ) },
                               { float( x - 5 ), .02f, float( z + 1 ) },
                               { float( x - 6 ), .02f, float( z + 1 ) }, { 0, 1, 0 } );
            mesh( assets, "Start finish black", checkerDark, dark );
            mesh( assets, "Start finish white", checkerWhite, white );
            Geometry gantry, pits;
            gantry.box( { -8, 3, 3 }, { .6f, 6, .6f } );
            gantry.box( { 8, 3, 3 }, { .6f, 6, .6f } );
            gantry.box( { 0, 5.6f, 3 }, { 16.5f, 1, .5f } );
            staticBox( assets, "Gantry left post", { -8, 3, 3 }, { .6f, 6, .6f } );
            staticBox( assets, "Gantry right post", { 8, 3, 3 }, { .6f, 6, .6f } );
            staticBox( assets, "Gantry overhead beam", { 0, 5.6f, 3 }, { 16.5f, 1, .5f } );
            mesh( assets, "Start gantry", gantry, dark );
            for( int i = 0; i < 6; ++i )
            {
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
            // Batch trees into 64-metre patches. The shared data-oriented LODSystem
            // chooses one renderer set per patch, without per-tree update callbacks.
            std::mt19937 rng( seed );
            std::uniform_real_distribution<float> unit( 0, 1 );
            Geometry hills;
            struct TreePatch
            {
                Geometry trunks, leaves, imposters;
                Array<scene::LODDetailBound> detailBounds;
            };
            std::map<std::pair<int, int>, TreePatch> patches;
            const auto atlas = bakePineImposter();
            TextureBuffer treeTexture( atlas.width, atlas.height );
            treeTexture.pixels.assign( atlas.rgba.begin(), atlas.rgba.end() );
            auto imposterMaterial = material( assets, ColourF::White, 1 );
            auto imposterTexture = upload( assets, treeTexture, render::TextureMipFilter::Cutout, 3 );
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
            nearTreeMaterial->setTexture( upload( assets, palette, render::TextureMipFilter::None ), 0 );
            int treeCount = quality == VehicleAppearanceQuality::Preview ? 90 : 220;
            for( int i = 0; i < treeCount; ++i )
            {
                Vector3F p( -100 + unit( rng ) * 380, 0, -250 + unit( rng ) * 440 );
                auto nearest = assets.circuit.nearest( p );
                if( ( p - points[nearest].position ).length() < 22 ||
                    ( p.x < -15 && p.z > -85 && p.z < 15 ) )
                    continue;
                float h = 4 + unit( rng ) * 7;
                staticBox( assets, "Tree trunk", p + Vector3F( 0, h * .3f, 0 ),
                           { h * .05f, h * .6f, h * .05f } );
                const auto key =
                    std::make_pair( int( std::floor( p.x / 64 ) ), int( std::floor( p.z / 64 ) ) );
                const Vector3F centre( key.first * 64.f + 32, 0, key.second * 64.f + 32 );
                auto &patch = patches[key];
                patch.detailBounds.push_back( { p - centre + Vector3F( 0, h * .5f, 0 ), h } );
                pineGeometry( patch.trunks, patch.leaves, p - centre, h );
                pineImposterGeometry( patch.imposters, atlas, p - centre, h );
            }
            for( const auto &[key, patch] : patches )
            {
                auto root = app->getGameManager()->createActor();
                root->setName( "Tree LOD patch" );
                assets.actors.push_back( root );
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
                        nearTrees.indices.push_back( base + index );
                };
                append( patch.trunks, .25f );
                append( patch.leaves, .75f );
                auto trees = mesh( assets, "LOD0 pine batch", nearTrees, nearTreeMaterial, root );
                auto cards =
                    mesh( assets, "LOD1 pine imposters", patch.imposters, imposterMaterial, root );
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
                    throw std::runtime_error( "Invalid generated tree LOD group: " + error );
                assets.treeLODs.push_back( lod );
            }
            // Keep the track and all authored tree positions on the flat contact plane.
            // Outside that footprint, a continuous ridge replaces overlapping cone silhouettes.
            const auto height = [seed]( float x, float z ) {
                const float radius = std::hypot( x - 80, z + 25 );
                const float t = std::clamp( ( radius - 300 ) / 160, 0.f, 1.f );
                const float phase = float( seed % 1000 ) * .073f;
                return -.15f +
                       t * t * ( 3 - 2 * t ) *
                           ( 58 + 19 * std::sin( x * .009f + phase ) +
                             13 * std::cos( z * .012f - phase ) + 8 * std::sin( ( x + z ) * .018f ) );
            };
            const int grid = quality == VehicleAppearanceQuality::Preview ? 24 : 32;
            for( int z = 0; z <= grid; ++z )
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
                        const u32 a = z * ( grid + 1 ) + x, b = a + 1;
                        const u32 c = a + grid + 1, d = c + 1;
                        for( auto index : { a, c, d, a, d, b } )
                            hills.indices.push_back( index );
                    }
                }
            mesh( assets, "Distant terrain", hills, grass );
        }
        void buildCity( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality )
        {
            const auto city = OpenCityLayout::generate( seed, assets.cityBlocks, assets.cityRoute );
            auto app = core::IApplicationManager::instance();
            auto roads = app->getFactoryManager()->createObjectFromType<IRoadSystem>( "IRoadSystem" );
            if( !roads )
                throw std::runtime_error( "Open city requires the WPProcedural road service." );
            roads->setSeed(seed);
            Geometry asphalt, pavement, markings, buildings[3], windows, parks;
            // Match the existing flat physics proxy. Preserve library topology,
            // kerbs and markings without introducing unsupported road-height bumps.
            auto append = [](Geometry &g, const RoadMesh &source, bool flat = false) {
                const auto base = static_cast<u32>(g.positions.size());
                for(const auto &v : source.vertices)
                {
                    g.positions.push_back({float(v.position.x), flat ? .012f : float(v.position.y), float(v.position.z)});
                    g.normals.push_back(flat ? Vector3F(0,1,0) : Vector3F(float(v.normal.x),float(v.normal.y),float(v.normal.z)));
                    g.uv.push_back({float(v.uv.x),float(v.uv.y),0});
                    g.tangents.push_back({1,0,0,1});
                }
                for(auto index : source.indices) g.indices.push_back(base + index);
            };
            const int half = city.blocks / 2;
            u32 segmentSeed = seed;
            for(int row = -half; row <= half; ++row)
                for(int col = -half; col < half; ++col)
                    for(int axis = 0; axis < 2; ++axis)
                    {
                        RoadSegmentSpec spec;
                        const float a = col * city.spacing + 6, b = (col + 1) * city.spacing - 6;
                        spec.start = axis ? Vector3<real_Num>(row * city.spacing,0,a) : Vector3<real_Num>(a,0,row * city.spacing);
                        spec.end = axis ? Vector3<real_Num>(row * city.spacing,0,b) : Vector3<real_Num>(b,0,row * city.spacing);
                        spec.roadClass = RoadClass::Arterial;
                        spec.seed = segmentSeed++;
                        spec.generateDressing = false;
                        const auto road = roads->generateSegment(spec);
                        append(asphalt, road.roadSurface, true);
                        for(const auto &part : road.sidewalks) append(pavement, part);
                        for(const auto &part : road.kerbs) append(pavement, part);
                        append(markings, road.markings);
                    }
            for(int x = -half; x <= half; ++x)
                for(int z = -half; z <= half; ++z)
                {
                    IntersectionSpec spec;
                    spec.center = {x * city.spacing,0,z * city.spacing};
                    spec.roadClass = RoadClass::Arterial;
                    spec.seed = segmentSeed++;
                    const auto junction = roads->generateIntersection(spec);
                    append(asphalt, junction.surface, true);
                    append(markings, junction.markings);
                }
            size_t lotIndex = 0;
            for(const auto &lot : city.lots)
            {
                if(lot.park)
                {
                    parks.box({lot.x,.02f,lot.z},{lot.width,.04f,lot.depth});
                    continue;
                }
                auto &walls = buildings[lotIndex++ % 3];
                const Vector3F size(lot.width,lot.height,lot.depth);
                walls.box({lot.x,lot.height * .5f,lot.z},size);
                auto collision = app->getGameManager()->createActor();
                assets.actors.push_back(collision);
                collision->setName("City building collision");
                collision->setStatic(true);
                collision->setPosition({lot.x,lot.height * .5f,lot.z});
                collision->addComponent<CollisionBox>()->setExtents(size);
                collision->addComponent<Rigidbody>();
                app->getGameManager()->getCurrentScene()->addActor(collision);
                // Batched facades keep city draw calls independent of building count.
                const float floorStep = quality == VehicleAppearanceQuality::Preview ? 6.f : 3.5f;
                for(float y = 2; y < lot.height - 1; y += floorStep)
                    for(float x = -lot.width / 2 + 3; x < lot.width / 2 - 2; x += 4)
                        for(int face : {-1,1})
                            windows.box({lot.x+x,y,lot.z+face*(lot.depth*.5f+.03f)},{1.4f,1.6f,.06f});
                for(float y = 2; y < lot.height - 1; y += floorStep)
                    for(float z = -lot.depth / 2 + 3; z < lot.depth / 2 - 2; z += 4)
                        for(int face : {-1,1})
                            windows.box({lot.x+face*(lot.width*.5f+.03f),y,lot.z+z},{.06f,1.6f,1.4f});
            }
            // Contain exploration within the shared collision plane.
            const float boundary = city.extent + 20;
            for(int axis=0;axis<2;++axis)
                for(int side : {-1,1})
                {
                    const Vector3F p = axis ? Vector3F(0,.6f,side*boundary) : Vector3F(side*boundary,.6f,0);
                    const Vector3F size = axis ? Vector3F(boundary*2,1.2f,1) : Vector3F(1,1.2f,boundary*2);
                    pavement.box(p,size);
                    auto wall=app->getGameManager()->createActor();
                    assets.actors.push_back(wall);
                    wall->setName("City boundary");
                    wall->setStatic(true);
                    wall->setPosition(p);
                    wall->addComponent<CollisionBox>()->setExtents(size);
                    wall->addComponent<Rigidbody>();
                    app->getGameManager()->getCurrentScene()->addActor(wall);
                }
            mesh(assets,"City streets",asphalt,material(assets,{.07f,.075f,.085f,1},.9f));
            mesh(assets,"City sidewalks",pavement,material(assets,{.42f,.43f,.44f,1},.95f));
            mesh(assets,"City lane markings",markings,material(assets,{.85f,.82f,.65f,1},.8f));
            const ColourF colours[] = {{.42f,.38f,.32f,1},{.54f,.52f,.47f,1},{.24f,.29f,.34f,1}};
            for(int i=0;i<3;++i) mesh(assets,"City buildings",buildings[i],material(assets,colours[i],.85f));
            mesh(assets,"City windows",windows,material(assets,{.055f,.13f,.19f,1},.22f,.35f));
            mesh(assets,"City parks",parks,material(assets,{.16f,.32f,.12f,1},1));
            Geometry ground;
            ground.box({0,-.06f,0},{city.extent*2+80,.1f,city.extent*2+80});
            mesh(assets,"City ground",ground,material(assets,{.28f,.30f,.26f,1},1));
            assets.circuit = {};
            for(size_t i=0;i<city.route.size();++i)
            {
                const auto p = city.route[i], next = city.route[(i+1)%city.route.size()];
                Vector3F forward(next.x-p.x,0,next.z-p.z);
                forward.normalise();
                assets.circuit.samples.push_back({{p.x,0,p.z},{-forward.z,0,forward.x},assets.circuit.length});
                assets.circuit.length += std::hypot(next.x-p.x,next.z-p.z);
            }
            Geometry arrows;
            for(size_t i=0;i<assets.circuit.samples.size();i+=20)
            {
                const auto &sample=assets.circuit.samples[i];
                const auto forward=Vector3F(sample.right.z,0,-sample.right.x);
                const auto p=sample.position+Vector3F(0,.04f,0);
                const auto tip=p+forward*2;
                arrows.quad(p-sample.right*1.5f,p+sample.right*1.5f,tip,tip,{0,1,0});
            }
            mesh(assets,"City race route arrows",arrows,material(assets,{.05f,.7f,.9f,1},.6f));
        }
        void buildSky( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle )
        {
            auto app = core::IApplicationManager::instance();
            auto sky =
                app->getFactoryManager()->createObjectFromType<ISkyAtmosphere>( "ISkyAtmosphere" );
            if( !sky )
                throw std::runtime_error( "WPProcedural atmosphere is unavailable." );
            SkyState state;
            state.timeOfDay = 15;
            state.turbidity = 2.4f;
            sky->setState( state );
            sky->update();
            const auto result = sky->getResults();
            auto graphicsScene = app->getGraphicsSystem()->getGraphicsScene();

            Array<SmartPtr<render::ITexture>> faces;
            Array<SmartPtr<render::ITexture>> reflectionFaces;
            const auto probePosition = vehicle->getPosition() + Vector3F( 0, 2, 0 );
            const Vector3F corners[8] = { { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 },
                                          { -1, -1, 1 },  { 1, -1, 1 },  { 1, 1, 1 },  { -1, 1, 1 } };
            const int faceCorners[6][4] = { { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
                                            { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };
            for( int face = 0; face < 6; ++face )
            {
                TextureBuffer texture( 256, 256 );
                TextureBuffer reflectionFace( 256, 256 );
                for( u32 y = 0; y < texture.height; ++y )
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
                        auto reflectedPixel = reflectionFace.pixel( x, y );
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            float value = channel == 0 ? c.r : channel == 1 ? c.g : c.b;
                            pixel[channel] = u8( std::clamp(
                                std::pow( std::max( value, 0.f ), 1.f / 2.2f ) * 255.f, 0.f, 255.f ) );
                            const float reflectedValue = channel == 0   ? reflectionColour.r
                                                         : channel == 1 ? reflectionColour.g
                                                                        : reflectionColour.b;
                            reflectedPixel[channel] = u8( std::clamp(
                                std::pow( std::max( reflectedValue, 0.f ), 1.f / 2.2f ) * 255.f, 0.f,
                                255.f ) );
                        }
                        pixel[3] = 255;
                        reflectedPixel[3] = 255;
                    }
                auto skyTexture = upload( assets, texture );
                faces.push_back( skyTexture );
                reflectionFaces.push_back( upload( assets, reflectionFace ) );
            }
            auto skyObject = graphicsScene->addGraphicsObjectByType<render::ISky>();
            assets.sky = skyObject;
            skyObject->setTextures( faces );
            skyObject->setDistance( 500 );
            skyObject->setVisible( true );

            auto reflection =
                app->getGraphicsSystem()->getTextureManager()->createCubeMap( reflectionFaces );
            if( reflection )
            {
                reflection->setName( assets.resourcePrefix + "/reflection" );

                assets.reflectionTexture = reflection;
                assets.textures.push_back( reflection );
                // Eight RGBA32F levels at 128px per face, generated by ClawCubemap.
                assets.textureBytes += 6u * 21845u * 16u;
                auto probeResource = make_ptr<render::GraphicsCubemap>();
                probeResource->setTexture( reflection );
                probeResource->setTextureName( reflection->getName() );
                probeResource->setSceneManager( graphicsScene );
                auto actor = app->getGameManager()->createActor();
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
                    vehicleMaterial->setTexture( reflection, reflectionSlot );
                // The actor's material also lets Cubemap manage the representative body material.
                actor->addComponent<scene::Material>()->setMaterial( assets.vehicleMaterials.front() );
                auto scene = app->getGameManager()->getCurrentScene();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );
                assets.reflectionActor = actor;
                assets.actors.push_back( actor );
                WP_LOG(
                    "VehicleAdvanced: custom cubemap actor created; reflection applied to all vehicle "
                    "materials." );
            }

            graphicsScene->setAmbientLight( ColourF( .28f, .32f, .38f, 1 ) );
            // Use the same horizon palette as the unlit sky. Lit surfaces pass through
            // Reinhard, so invert that transform to converge to the sky's displayed colour.
            auto horizon = sky->computeSkyColour( Vector3F( 1, 0, 0 ), result.sunDir,
                                                  state.turbidity, result.sunAltitude );
            const auto fogRadiance = []( float displayedLinear ) {
                const float colour = std::clamp( displayedLinear, 0.f, .95f );
                return colour / ( 1 - colour );
            };
            graphicsScene->setFog( render::IGraphicsScene::FOG_LINEAR,
                ColourF( fogRadiance( .41f + horizon.r * .15f ),
                         fogRadiance( .57f + horizon.g * .15f ),
                         fogRadiance( .82f + horizon.b * .15f ), 1 ), .0007f, 180, 800 );
            // Keep the host scene's lights; reuse its directional lighting if present.
            for( auto light : graphicsScene->getGraphicsObjectsByType<render::IGraphicsLight>() )
                if( light->getType() == LightTypes::LT_DIRECTIONAL && light->isVisible() )
                    return;
            auto sun = graphicsScene->addGraphicsObjectByType<render::IGraphicsLight>();
            assets.sun = sun;
            sun->setType( LightTypes::LT_DIRECTIONAL );
            sun->setDirection( -result.sunDir );
            sun->setDiffuseColour( ColourF( 1, .96f, .88f, 1 ) );
            sun->setPowerScale( 1.8f );
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
        std::vector<Vector3F> knots = { { 0, 0, 70 },    { 0, 0, 0 },     { 0, 0, -90 },
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
        std::vector<Vector3F> dense;
        for( size_t i = 0; i < knots.size(); ++i )
            for( int step = 0; step < 100; ++step )
            {
                auto a = knots[( i + knots.size() - 1 ) % knots.size()], b = knots[i],
                     c = knots[( i + 1 ) % knots.size()], d = knots[( i + 2 ) % knots.size()];
                float t = step / 100.f;
                dense.push_back( ( b * 2 + ( c - a ) * t + ( a * 2 - b * 5 + c * 4 - d ) * t * t +
                                   ( -a + b * 3 - c * 3 + d ) * t * t * t ) *
                                 .5f );
            }
        std::vector<float> distance( dense.size() + 1, 0 );
        for( size_t i = 0; i < dense.size(); ++i )
            distance[i + 1] = distance[i] + ( dense[( i + 1 ) % dense.size()] - dense[i] ).length();
        Circuit result;
        result.length = distance.back();
        auto count = static_cast<size_t>( std::ceil( result.length / 1.5f ) );
        size_t segment = 0;
        for( size_t i = 0; i < count; ++i )
        {
            float s = result.length * i / count;
            while( segment + 1 < dense.size() && distance[segment + 1] < s )
                ++segment;
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
            throw std::runtime_error( "Invalid generated circuit." );
        return result;
    }
    void buildScene( SceneAssets &assets, SmartPtr<scene::IGameActor> actor, u32 seed,
                     VehicleAppearanceQuality quality )
    {
        assets.resourcePrefix = "__procedural/race/" + StringUtil::getUUID();
        const auto started = std::chrono::steady_clock::now();
        buildCar( assets, actor, seed, quality );
        if( assets.openCity )
            buildCity( assets, seed, quality );
        else
            buildTrack( assets, seed, quality );
        buildSky( assets, actor );
        TextureBuffer shade( 64, 64 );
        for( u32 y = 0; y < 64; ++y )
            for( u32 x = 0; x < 64; ++x )
            {
                auto pixel = shade.pixel( x, y );
                const auto dx = ( x - 31.5f ) / 31.5f, dz = ( y - 31.5f ) / 31.5f;
                pixel[0] = pixel[1] = pixel[2] = 0;
                pixel[3] = u8( 130 * std::pow( std::max( 0.f, 1.f - dx * dx - dz * dz ), 2 ) );
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
        if( assets.openCity )
            if( auto root = actor->getParent() )
            {
                auto scene = core::IApplicationManager::instance()->getGameManager()->getCurrentScene();
                // Serialize the generated city only under the game's owned root.
                // Removing the old root after Editor restoration then removes all
                // streets/colliders too, even when Lua's native references were lost.
                for( const auto &generated : assets.actors )
                    if( !generated->getParent() )
                    {
                        scene->removeActor( generated );
                        root->addChild( generated );
                        scene->registerAllUpdates( generated );
                    }
            }

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
            return true;
        auto component =
            assets.reflectionActor ? assets.reflectionActor->getComponent<scene::Cubemap>() : nullptr;
        auto probe = component ? component->getRenderCubemap() : nullptr;
        if( !component || !component->isEnabled() || !probe ||
            probe->getTexture() != assets.reflectionTexture || assets.vehicleMaterials.empty() )
            return false;
        const auto slot = static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION );
        for( const auto &material : assets.vehicleMaterials )
            if( material->getTexture( slot ) != assets.reflectionTexture )
                return false;
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
                throw std::runtime_error( "Circuit generation is not deterministic." );
            const auto spacing = circuit.length / circuit.samples.size();
            for( size_t i = 0; i < circuit.samples.size(); ++i )
            {
                const auto &a = circuit.samples[i],
                           &b = circuit.samples[( i + 1 ) % circuit.samples.size()];
                const auto chord = ( b.position - a.position ).length();
                if( a.position != repeat.samples[i].position || a.position.y != 0 ||
                    std::abs( a.right.length() - 1 ) > 1e-4f || chord < spacing * .95f ||
                    chord > spacing * 1.01f )
                    throw std::runtime_error( "Circuit closure, spacing, or frame validation failed." );
            }
            if( circuit.samples.front().position.length() > spacing ||
                circuit.samples.front().right.x < .99f )
                throw std::runtime_error( "Circuit spawn does not align with the start straight." );
        }
        if( generateCircuit( 7 ).samples[100].position == generateCircuit( 8 ).samples[100].position )
            throw std::runtime_error( "Seed does not alter the circuit." );
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
                ratios += "," + StringUtil::toString( ratio );
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
            wheelProps->setProperty( "Grip", 0.7f );
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
