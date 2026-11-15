#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralSceneGeneration.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/Components/CityBuildingStyles.hpp>
#include <Workphone/Scene/Components/CitySurfaceLibrary.hpp>
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <Workphone/Interface/Procedural/OpenCityStreetDetails.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadSource.hpp>
#include <Workphone/Interface/Procedural/OpenCityLegacyRoute.hpp>
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
#include <Workphone/Graphics/GeneratedTextureSource.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include "CityBuildingKit.hpp"
#include "CityRoadCrossings.hpp"
#include "../RuntimeSceneMembership.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <map>
#include <tuple>
#include <cstdio>

namespace workphone::scene::race
{
    SceneAssets::SceneAssets( const SceneAssets &other ) :
        openCity( other.openCity ),
        useFoliageWorld( other.useFoliageWorld ),
        cityNaturalRoads( other.cityNaturalRoads ),
        cityRoadShowcase( other.cityRoadShowcase ),
        cityTerrainRoads( other.cityTerrainRoads ),
        roadAuthoring( other.roadAuthoring ),
        roadTerrain( other.roadTerrain ),
        terrainGameplaySource( other.terrainGameplaySource ),
        cityLayout( other.cityLayout ),
        cityBuildingSupportChecks( other.cityBuildingSupportChecks ),
        cityCulledParcels( other.cityCulledParcels ),
        cityBuildingSupportValid( other.cityBuildingSupportValid ),
        citySourceReused( other.citySourceReused ),
        cityTerrainRelief( other.cityTerrainRelief ),
        cityTerrainFitted( other.cityTerrainFitted ),
        cityTerrainFitReport( other.cityTerrainFitReport ),
        cityRaceRouteAvailable( other.cityRaceRouteAvailable ),
        cityRaceRouteError( other.cityRaceRouteError ),
        cityAppliedSourceRevision( other.cityAppliedSourceRevision ),
        cityStreetDetail( other.cityStreetDetail ),
        cityStreetDetailCount( other.cityStreetDetailCount ),
        cityBuildingStyle( other.cityBuildingStyle ),
        cityBuildingMeshes( other.cityBuildingMeshes ),
        cityBuildingModuleCount( other.cityBuildingModuleCount ),
        cityBuildingModulePlacements( other.cityBuildingModulePlacements ),
        cityBuildingKitDiagnostic( other.cityBuildingKitDiagnostic ),
        foliageTerrain( other.foliageTerrain ),
        cityBlocks( other.cityBlocks ),
        cityRoute( other.cityRoute ),
        vehicle( other.vehicle ),
        circuit( other.circuit ),
        wheels( other.wheels ),
        body( other.body ),
        shadow( other.shadow ),
        reflectionActor( other.reflectionActor ),
        ground( other.ground ),
        sceneryRoot( other.sceneryRoot ),
        reflectionTexture( other.reflectionTexture ),
        vehicleMaterials( other.vehicleMaterials ),
        materials( other.materials ),
        textures( other.textures ),
        meshes( other.meshes ),
        actors( other.actors ),
        treeLODs( makeSourceBudgetSnapshot( other.treeLODs ) ),
        vehicleLOD( other.vehicleLOD ),
        sky( other.sky ),
        sun( other.sun ),
        resourcePrefix( other.resourcePrefix ),
        exportSourceDirectory( other.exportSourceDirectory ),
        triangles( other.triangles ),
        textureBytes( other.textureBytes )
    {
    }

    SceneAssets &SceneAssets::operator=( const SceneAssets &other )
    {
        if( this != &other )
        {
            SceneAssets candidate( other );
            swap( candidate );
        }
        return *this;
    }

    void SceneAssets::swap( SceneAssets &other ) noexcept
    {
        using std::swap;
        swap( openCity, other.openCity );
        swap( useFoliageWorld, other.useFoliageWorld );
        swap( cityNaturalRoads, other.cityNaturalRoads );
        swap( foliageTerrain, other.foliageTerrain );
        swap( cityRoadShowcase, other.cityRoadShowcase );
        swap( roadAuthoring, other.roadAuthoring );
        swap( roadTerrain, other.roadTerrain );
        swap( terrainGameplaySource, other.terrainGameplaySource );
        swap( cityTerrainRoads, other.cityTerrainRoads );
        cityLayout.swap( other.cityLayout );
        swap( cityBuildingSupportChecks, other.cityBuildingSupportChecks );
        swap( cityCulledParcels, other.cityCulledParcels );
        swap( cityBuildingSupportValid, other.cityBuildingSupportValid );
        swap( citySourceReused, other.citySourceReused );
        swap( cityTerrainRelief, other.cityTerrainRelief );
        swap( cityTerrainFitted, other.cityTerrainFitted );
        swap( cityTerrainFitReport, other.cityTerrainFitReport );
        swap( cityRaceRouteAvailable, other.cityRaceRouteAvailable );
        swap( cityRaceRouteError, other.cityRaceRouteError );
        swap( cityAppliedSourceRevision, other.cityAppliedSourceRevision );
        swap( cityStreetDetail, other.cityStreetDetail );
        swap( cityStreetDetailCount, other.cityStreetDetailCount );
        swap( cityBuildingStyle, other.cityBuildingStyle );
        swap( cityBuildingMeshes, other.cityBuildingMeshes );
        swap( cityBuildingModuleCount, other.cityBuildingModuleCount );
        swap( cityBuildingModulePlacements, other.cityBuildingModulePlacements );
        swap( cityBuildingKitDiagnostic, other.cityBuildingKitDiagnostic );
        swap( cityBlocks, other.cityBlocks );
        swap( cityRoute, other.cityRoute );
        swap( vehicle, other.vehicle );
        circuit.swap( other.circuit );
        wheels.swap( other.wheels );
        swap( body, other.body );
        swap( shadow, other.shadow );
        swap( reflectionActor, other.reflectionActor );
        swap( ground, other.ground );
        swap( sceneryRoot, other.sceneryRoot );
        swap( reflectionTexture, other.reflectionTexture );
        vehicleMaterials.swap( other.vehicleMaterials );
        materials.swap( other.materials );
        textures.swap( other.textures );
        meshes.swap( other.meshes );
        actors.swap( other.actors );
        treeLODs.swap( other.treeLODs );
        swap( vehicleLOD, other.vehicleLOD );
        swap( sky, other.sky );
        swap( sun, other.sun );
        swap( resourcePrefix, other.resourcePrefix );
        swap( exportSourceDirectory, other.exportSourceDirectory );
        swap( triangles, other.triangles );
        swap( textureBytes, other.textureBytes );
    }

    namespace
    {
        template <class C>
        void reserveOwnerSlot( C &owners )
        {
            if( owners.size() != owners.capacity() )
            {
                return;
            }
            if( owners.size() == owners.max_size() )
            {
                throw std::length_error( "Scene inventory capacity overflow" );
            }
            const auto next =
                owners.capacity() > owners.max_size() / 2
                    ? owners.max_size()
                    : std::min( owners.max_size(), std::max<size_t>( 8, owners.capacity() * 2 ) );
            owners.reserve( next );
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
            reserveOwnerSlot( assets.materials );
            auto m = ApplicationUtil::createDefaultMaterial();
            if( !m )
            {
                throw std::runtime_error( "Cannot create procedural material" );
            }
            assets.materials.push_back( m );
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
            render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1,
            const char *citySurface = nullptr )
        {
            if( !bytes )
            {
                return nullptr;
            }
            if( !pixels || !width || !height || width > 16384 || height > 16384 ||
                u64( width ) * height * 4 != bytes )
            {
                throw std::invalid_argument( "Procedural upload requires exact bounded RGBA pixels." );
            }
            // Admit conversion before creating a manager-owned texture. The
            // caller's atlas and this BGRA snapshot remain independently owned.
            auto bgra = makeSourceBudgetSnapshot<u8>();
            bgra.reserve( bytes );
            bgra.resize( bytes );
            std::copy_n( pixels, bytes, bgra.begin() );
            for( size_t i = 0; i < bgra.size(); i += 4 )
            {
                std::swap( bgra[i], bgra[i + 2] );
            }
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            auto manager = graphics->getTextureManager();
            reserveOwnerSlot( assets.textures );
            auto name = assets.resourcePrefix;
            if( citySurface ) name += String( "/city-surface/" ) + citySurface;
            name += "/texture/" + StringUtil::toString( assets.textures.size() );
            // New sampled textures default to 2D/usage zero. Avoid allocating
            // createManual's complete blank image before replacing its pixels.
            auto texture = dynamic_pointer_cast<render::ITexture>( manager->create( name ) );
            if( !texture )
            {
                throw std::runtime_error( "Cannot create procedural texture." );
            }
            try
            {
                auto properties = texture->getProperties();
                properties->setProperty( "mipmapFilter", static_cast<s32>( filter ) );
                properties->setProperty( "mipmapAtlasColumns", atlasColumns );
                texture->setProperties( properties );
                String error;
                if( !texture->copyByteData( bgra, Vector2I( width, height ), error ) )
                {
                    throw std::runtime_error(
                        ( String( "Cannot stage procedural texture: " ) + error ).c_str() );
                }
                texture->load( nullptr );
                assets.textures.push_back( texture );
                assets.textureBytes += bytes;
            }
            catch( ... )
            {
                try
                {
                    manager->destroyResource( texture );
                }
                catch( ... )
                {
                }
                throw;
            }
            return texture;
        }

        SmartPtr<render::ITexture> upload(
            SceneAssets &assets, const TextureBuffer &buffer,
            render::TextureMipFilter filter = render::TextureMipFilter::Colour, u32 atlasColumns = 1,
            const char *citySurface = nullptr )
        {
            return uploadPixels( assets, buffer.data(), buffer.pixels.size(), buffer.width,
                                 buffer.height, filter, atlasColumns, citySurface );
        }

        SmartPtr<render::ITexture> uploadLinearRadiance( SceneAssets &assets, const Array<f32> &rgba,
                                                         Vector2I size )
        {
            auto graphics = core::IApplicationManager::instance()->getGraphicsSystem();
            reserveOwnerSlot( assets.textures );
            const auto name =
                assets.resourcePrefix + "/radiance/" + StringUtil::toString( assets.textures.size() );
            auto manager = graphics->getTextureManager();
            auto texture = dynamic_pointer_cast<render::ITexture>( manager->create( name ) );
            if( !texture )
            {
                throw std::runtime_error( "Cannot create HDR reflection texture" );
            }
            try
            {
                String error;
                if( !texture->copyLinearFloatData( rgba, size, error ) )
                {
                    throw std::runtime_error(
                        ( String( "Cannot stage HDR reflection face: " ) + error ).c_str() );
                }
                assets.textures.push_back( texture );
                assets.textureBytes += rgba.size() * sizeof( f32 );
            }
            catch( ... )
            {
                try
                {
                    manager->destroyResource( texture );
                }
                catch( ... )
                {
                }
                throw;
            }
            return texture;
        }

        void maps( SceneAssets &assets, SmartPtr<render::IMaterial> m, const TextureBuffer &albedo,
                   const TextureBuffer &normal, const TextureBuffer &orm,
                   const char *citySurface = nullptr )
        {
            if( !albedo.pixels.empty() )
            {
                m->setDiffuse( ColourF::White );
                m->setTexture( upload( assets, albedo, render::TextureMipFilter::Colour, 1, citySurface ), 0 );
            }
            if( !normal.pixels.empty() )
            {
                m->setTexture( upload( assets, normal, render::TextureMipFilter::Normal, 1, citySurface ), 1 );
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
                                                        : render::TextureMipFilter::Data, 1, citySurface ),
                                   channel == 0   ? 22
                                   : channel == 1 ? 3
                                                  : 2 );
                }
                m->setRoughness( 1 );
                m->setMetalness( 1 );
            }
        }

        bool applyCitySurface( SceneAssets &assets, SmartPtr<render::IMaterial> material,
                               CitySurface family, u32 size, float normalStrength, u32 projection,
                               u32 seed = 0 )
        {
            SurfaceBakeResult surface;
            String diagnostic;
            if( !loadCitySurface( family, size, surface, diagnostic ) )
            {
                WP_LOG( String( "City surface uses procedural fallback: " ) + diagnostic );
                return false;
            }
            if( family == CitySurface::Asphalt && !assets.exportSourceDirectory.empty() )
            {
                std::string error;
                if( !texture_export::exportAsphaltSource( surface,
                        std::filesystem::u8path( assets.exportSourceDirectory.c_str() ), seed, error, true ) )
                    throw std::runtime_error( "City asphalt source export failed: " + error );
            }
            maps( assets, material, surface.albedo, surface.normal, surface.orm, citySurfaceAssetId( family ) );
            material->setUVProjection( projection );
            material->setTriplanarScale( 1.f / citySurfaceTileMetres( family ) );
            material->setNormalStrength( normalStrength );
            material->setEditorFloat( "aoStrength", .65f );
            material->setEditorUInt( "uvFilter", 3 );
            material->setEditorFloat( "uvAniso", 8.f );
            return true;
        }

        SmartPtr<render::IMaterial> raceAsphalt( SceneAssets &assets, SmartPtr<ITextureForge> forge,
                                                 const SurfaceBakeParams &params, u32 seed )
        {
            if( assets.openCity )
            {
                auto scanned = material( assets, ColourF::White, 1 );
                if( applyCitySurface( assets, scanned, CitySurface::Asphalt, params.size, .45f, 3, seed ) )
                    return scanned;
            }
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
                    u32 value = ( x % period ) * 0x9e3779b9u ^ ( y % period ) * 0x85ebca6bu ^ seed;
                    value ^= value >> 16;
                    value *= 0x7feb352du;
                    value ^= value >> 15;
                    value *= 0x846ca68bu;
                    return float( ( value ^ ( value >> 16 ) ) >> 8 ) / 16777215.f;
                };
                const auto smoothNoise = [&]( u32 x, u32 y, u32 cells ) {
                    const float u = float( x ) * cells / params.size,
                                v = float( y ) * cells / params.size;
                    const u32 ix = u32( u ), iy = u32( v );
                    const float fx = u - ix, fy = v - iy;
                    const float sx = fx * fx * ( 3 - 2 * fx ), sy = fy * fy * ( 3 - 2 * fy );
                    const float a =
                        noise( ix, iy, cells ) * ( 1 - sx ) + noise( ix + 1, iy, cells ) * sx;
                    const float b =
                        noise( ix, iy + 1, cells ) * ( 1 - sx ) + noise( ix + 1, iy + 1, cells ) * sx;
                    return a * ( 1 - sy ) + b * sy;
                };
                for( u32 y = 0; y < params.size; ++y )
                {
                    for( u32 x = 0; x < params.size; ++x )
                    {
                        const float grain = noise( x, y, params.size );
                        const float patch = smoothNoise( x, y, 3 ), aggregate = smoothNoise( x, y, 32 );
                        const float linear = .064f + ( patch - .5f ) * .016f +
                                             ( aggregate - .5f ) * .009f + ( grain - .5f ) * .006f;
                        auto pixel = asphalt.albedo.pixel( x, y );
                        for( u32 channel = 0; channel < 3; ++channel )
                        {
                            const float colour = linear * ( channel == 2 ? 1.02f : 1.f );
                            pixel[channel] = u8(
                                std::clamp( ( 1.055f * std::pow( colour, 1.f / 2.4f ) - .055f ) * 255,
                                            0.f, 255.f ) );
                        }
                        asphalt.height.values[size_t( y ) * params.size + x] =
                            .5f + ( grain - .5f ) * .012f;
                        auto orm = asphalt.orm.pixel( x, y );
                        orm[0] = 255;
                        orm[1] = u8( 224 + aggregate * 16 );
                        orm[2] = 0;
                        orm[3] = 255;
                    }
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

        SmartPtr<scene::IGameActor> publishMesh( SceneAssets &assets, const String &name,
                                                 SmartPtr<IMesh> generated, size_t triangles,
                                                 SmartPtr<render::IMaterial> m,
                                                 SmartPtr<scene::IGameActor> parent = nullptr )
        {
            if( !generated )
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
            generated->updateAABB( true );
            auto path = assets.resourcePrefix + "/mesh/" + StringUtil::toString( assets.meshes.size() ) +
                        ".meshbin";
            reserveOwnerSlot( assets.meshes );
            auto resource =
                dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve( path ).first );
            if( !resource )
            {
                throw std::runtime_error( "Cannot create procedural mesh resource" );
            }
            assets.meshes.push_back( resource );
            resource->setName( path );
            resource->setFilePath( path );
            resource->setMesh( generated );
            resource->setLoadingState( LoadingState::Loaded );
            assets.triangles += triangles;
            reserveOwnerSlot( assets.actors );
            auto actor = app->getGameManager()->createActor();
            if( !actor )
            {
                throw std::runtime_error( "Cannot create procedural actor" );
            }
            assets.actors.push_back( actor );
            actor->setName( name );
            if( parent )
            {
                parent->addChild( actor );
            }
            actor->addComponent<scene::Mesh>()->setMeshResource( resource );
            actor->addComponent<scene::MeshRenderer>();
            actor->addComponent<scene::Material>()->setMaterial( m );
            return actor;
        }

        SmartPtr<scene::IGameActor> mesh( SceneAssets &assets, const String &name, const Geometry &g,
                                          SmartPtr<render::IMaterial> m,
                                          SmartPtr<scene::IGameActor> parent = nullptr )
        {
            if( g.indices.empty() ) return nullptr;
            // DX11's material rasterizer uses counterclockwise front faces, so keep the
            // generated winding that matches the supplied lighting normals.
            return publishMesh( assets, name, MeshUtil::createMesh( g.positions, g.normals, g.tangents, g.uv, g.indices ),
                                g.indices.size() / 3, m, parent );
        }

        Vector3F vector( const VehiclePhysicsVector3 &p )
        {
            return { float( p.x ), float( p.y ), float( p.z ) };
        }

        void attachWorldActor( SmartPtr<IGameScene> scene, SmartPtr<IGameActor> owner,
                               SmartPtr<IGameActor> actor )
        {
            actor->setFlag( IGameActor::ActorFlagWorldSpaceTransform, true );
            owner->addChild( actor );
            actor->setScene( scene );
            scene->registerAllUpdates( actor );
        }

        void staticBox( SceneAssets &assets, const String &name, Vector3F centre, Vector3F size,
                        QuaternionF orientation = QuaternionF::identity() )
        {
            auto app = core::IApplicationManager::instance();
            reserveOwnerSlot( assets.actors );
            auto actor = app->getGameManager()->createActor();
            if( !actor )
            {
                throw std::runtime_error( "Cannot create procedural actor" );
            }
            assets.actors.push_back( actor );
            // Own these independently of batched render meshes and tree LODs.
            // The normal generated-scene cleanup also removes their physics bodies.
            actor->setName( "Trackside collision: " + name );
            actor->setStatic( true );
            actor->setPosition( centre );
            actor->setOrientation( orientation );
            // CollisionBox takes full dimensions, not half extents.
            actor->addComponent<scene::CollisionBox>()->setExtents( size );
            actor->addComponent<scene::Rigidbody>();
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
                reserveOwnerSlot( assets.vehicleMaterials );
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
                reserveOwnerSlot( assets.actors );
                auto wheel = app->getGameManager()->createActor();
                if( !wheel )
                {
                    throw std::runtime_error( "Cannot create procedural actor" );
                }
                assets.actors.push_back( wheel );
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
                auto renderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
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

        namespace cityParkLandscape { struct Layout; }
        namespace cityReserveLandscape { struct Site; }

        void buildFoliageWorld( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality,
                                const CityRoadTerrainMesh *cityFootways = nullptr,
                                const SourceBudgetVector<cityParkLandscape::Layout> *parkLayouts = nullptr,
                                const SourceBudgetVector<cityReserveLandscape::Site> *reserveSites = nullptr );

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
                    Array<scene::LODDetailBound> detailBounds{ Allocator<scene::LODDetailBound>(
                        sourceBudget ) };
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
                        std::copy_n( atlases[variant].rgba.begin() + size_t( y ) * atlas.width * 4,
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
                    reserveOwnerSlot( assets.actors );
                    auto root = app->getGameManager()->createActor();
                    if( !root )
                    {
                        throw std::runtime_error( "Cannot create procedural actor" );
                    }
                    assets.actors.push_back( root );
                    root->setName( "Tree LOD patch" );
                    root->setPosition( { key.first * 64.f + 32, 0, key.second * 64.f + 32 } );
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
                    reserveOwnerSlot( assets.treeLODs );
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

        float cityValueNoise( u32 seed, float x, float z );

#include "CityHillsideVegetation.hpp"
#include "CityParkLayout.hpp"
#include "CityParkPlanting.hpp"
#include "CityReserveLayout.hpp"
#include "CityReservePlanting.hpp"

        void buildFoliageWorld( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality,
                                const CityRoadTerrainMesh *cityFootways,
                                const SourceBudgetVector<cityParkLandscape::Layout> *parkLayouts,
                                const SourceBudgetVector<cityReserveLandscape::Site> *reserveSites )
        {
            auto app = core::IApplicationManager::instance();
            auto scene = app->getGraphicsSystem()->getGraphicsScene();
            const bool optionalHillside = cityFootways && !assets.useFoliageWorld;
            if( optionalHillside )
            {
                const auto type = scene ? TypeManager::instance()->getName( scene->getTypeInfo() ) : nullptr;
                if( !type || String( type ) != "workphone::render::ClawScene" ) return;
            }
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
                if( optionalHillside ) return;
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
            const auto city = assets.cityTerrainRoads ? assets.cityLayout
                              : assets.openCity
                                  ? OpenCityLayout::generate( seed, assets.cityBlocks, assets.cityRoute,
                                                              assets.cityNaturalRoads )
                                  : OpenCityLayout{};
            auto world = std::make_shared<render::FoliageWorldData>();
            world->revision = 1;
            // The bounded city overview needs a longer tree view range. Larger city
            // pages keep that range inside the existing 30-page-width pager gate.
            if( assets.openCity )
            {
                world->pageSize = 128;
            }
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
                species->maximumDistance =
                    assets.openCity ? std::max( 600.f, city.extent * 5.f ) : 600.f;
                species->shadowDistance = 120;
                world->species.push_back( species );
                for( const auto &lod : species->lods )
                {
                    assets.triangles += lod.indices.size() / 3;
                }
            }
            std::mt19937 random( seed );
            std::uniform_real_distribution<float> unit( 0, 1 );
            const int count = parkLayouts || ( cityFootways && !assets.useFoliageWorld ) ? 0 :
                              quality == VehicleAppearanceQuality::Preview ? 90 : 220;
            SourceBudgetVector<size_t> cityParks;
            if( assets.openCity && count > 0 )
            {
                cityParks.reserve( size_t( std::count_if(
                    city.lots.begin(), city.lots.end(), []( const auto &lot ) { return lot.park; } ) ) );
                for( size_t index = 0; index < city.lots.size(); ++index )
                {
                    if( city.lots[index].park )
                    {
                        cityParks.push_back( index );
                    }
                }
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
                    const u64 key =
                        render::foliageStableHash( lot.id ) ^ ( ( round + 1 ) * 0x9e3779b97f4a7c15ull );
                    cityPlacementId = render::foliageStableHash( key );
                    if( !cityPlacementId )
                    {
                        cityPlacementId = key;
                    }
                    const auto scalar = []( u64 value ) {
                        return float( render::foliageStableHash( value ) >> 40 ) / 16777216.f;
                    };
                    // The 90/220 candidate tiers fit one 16x16 cell permutation per park.
                    // Distinct rounds occupy distinct cells; hashes jitter within each cell.
                    // Fixed cells keep retained candidates stable across quality presets.
                    const auto cell = u32( round * 73 + render::foliageStableHash( lot.id ) ) & 255u;
                    const float u =
                        ( float( cell & 15u ) + .2f + .6f * scalar( key ^ 0xa341316cu ) ) / 16;
                    const float v =
                        ( float( cell >> 4 ) + .2f + .6f * scalar( key ^ 0xc8013ea4u ) ) / 16;
                    p = { lot.parcelX + ( u - .5f ) * std::max( 0.f, lot.parcelWidth - 4 ),
                          lot.groundHeight + .16f,
                          lot.parcelZ + ( v - .5f ) * std::max( 0.f, lot.parcelDepth - 4 ) };
                }
                else if( ( p - assets.circuit.samples[assets.circuit.nearest( p )].position ).length() <
                             22 ||
                         ( p.x < -15 && p.z > -85 && p.z < 15 ) )
                {
                    continue;
                }
                render::FoliageInstance instance;
                instance.id = cityPlacementId
                                  ? cityPlacementId
                                  : render::foliageStableHash( ( u64( seed ) << 32 ) | u32( i + 1 ) );
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
            if( cityFootways )
            {
                cityHillside::addSpecies( assets, *world, leafTextureName, seed );
                if( assets.roadAuthoring->getSource().roads.size() == assets.roadAuthoring->getBake().roads.size() )
                {
                    cityHillside::Exclusions exclusions;
                    cityHillside::indexCity( exclusions, assets, *cityFootways );
                    if( parkLayouts ) cityParkPlanting::scatter( assets, *parkLayouts, exclusions, *world, seed, quality );
                    cityHillside::scatter( assets, exclusions, *world, seed, quality );
                    if( reserveSites ) cityReservePlanting::scatter( assets, *reserveSites, exclusions, *world, quality );
                }
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
            const auto axisBox = []( Geometry &g, Vector3F p, float width, float height, float depth,
                                     bool xFace ) {
                g.box( p, xFace ? Vector3F( depth, height, width ) : Vector3F( width, height, depth ) );
            };
            const auto normalFor = []( OpenCityLayout::Frontage frontage ) {
                switch( frontage )
                {
                case OpenCityLayout::Frontage::West:
                    return Vector3F( -1, 0, 0 );
                case OpenCityLayout::Frontage::East:
                    return Vector3F( 1, 0, 0 );
                case OpenCityLayout::Frontage::South:
                    return Vector3F( 0, 0, 1 );
                default:
                    return Vector3F( 0, 0, -1 );
                }
            };
            SourceBudgetVector<const OpenCityLayout::Lot *> parkLots, frontages;
            parkLots.reserve( city.lots.size() );
            frontages.reserve( city.lots.size() );
            for( const auto &lot : city.lots )
            {
                if( !lot.park )
                {
                    frontages.push_back( &lot );
                    continue;
                }
                if( assets.cityTerrainRoads ) continue;
                parkLots.push_back( &lot );
                const float hx = lot.parcelWidth * .5f, hz = lot.parcelDepth * .5f;
                const Vector3F centre( lot.parcelX, lot.groundHeight + .166f, lot.parcelZ );
                // The perimeter loop stays inside the two-metre tree exclusion margin.
                // Corners belong to the transverse strips only, avoiding coplanar overlaps.
                for( int side : { -1, 1 } )
                {
                    horizontal( stone, centre + Vector3F( 0, 0, side * ( hz - 1.4f ) ),
                                lot.parcelWidth - 1.6f, 1.2f );
                    horizontal( stone, centre + Vector3F( side * ( hx - 1.4f ), 0, 0 ), 1.2f,
                                lot.parcelDepth - 4.f );
                    horizontal( stone, centre + Vector3F( 0, 0, side * ( hz - .56f ) ),
                                lot.parcelWidth - 1.f, .12f );
                    horizontal( stone, centre + Vector3F( side * ( hx - .56f ), 0, 0 ), .12f,
                                lot.parcelDepth - 1.24f );
                    horizontal( dark, centre + Vector3F( 0, .001f, side * ( hz - 2.f ) ),
                                lot.parcelWidth - 4.f, .025f );
                    horizontal( dark, centre + Vector3F( side * ( hx - 2.f ), .001f, 0 ), .025f,
                                lot.parcelDepth - 4.f );
                }
                if( !city.naturalRoads && !assets.cityTerrainRoads )
                {
                    const auto normal = normalFor( lot.frontage );
                    const auto tangent = Vector3F( normal.z, 0, -normal.x );
                    const bool xFace = std::abs( normal.x ) > .5f;
                    const auto front = centre + normal * ( xFace ? hx : hz );
                    auto inner = front - normal * .8f, outer = front + normal * 2.5f;
                    inner.y = .168f;
                    outer.y = .156f;
                    stone.quad( inner - tangent * .6f, outer - tangent * .6f, outer + tangent * .6f,
                                inner + tangent * .6f, { 0, 1, 0 } );
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
                const auto p =
                    Vector3F( lot.parcelX, lot.groundHeight + .15f, lot.parcelZ ) +
                    normal * ( half + ( city.naturalRoads || assets.cityTerrainRoads ? -.8f : 1.1f ) ) -
                    tangent * std::min( 6.f, span * .25f );
                for( int plank = -2; plank <= 2; ++plank )
                {
                    axisBox( wood, p + Vector3F( 0, .46f, 0 ) + normal * ( plank * .085f ), 1.8f, .055f,
                             .065f, xFace );
                }
                for( int row = 0; row < 3; ++row )
                {
                    axisBox( wood, p + Vector3F( 0, .70f + row * .13f, 0 ) - normal * .21f, 1.8f, .08f,
                             .045f, xFace );
                }
                for( int side : { -1, 1 } )
                {
                    axisBox( metal, p + tangent * ( side * .64f ) + Vector3F( 0, .215f, 0 ), .075f, .43f,
                             .43f, xFace );
                    metal.beam( p + tangent * ( side * .64f ) - normal * .22f + Vector3F( 0, .22f, 0 ),
                                p + tangent * ( side * .64f ) - normal * .22f + Vector3F( 0, .98f, 0 ),
                                .035f );
                }
                staticBox( assets, "City bench", p + Vector3F( 0, .515f, 0 ),
                           xFace ? Vector3F( .55f, 1.03f, 1.84f ) : Vector3F( 1.84f, 1.03f, .55f ) );
            }
            const size_t planterCount = city.naturalRoads || assets.cityTerrainRoads
                                            ? 0
                                            : std::min<size_t>( frontages.size(), preview ? 8 : 16 );
            for( size_t i = 0; i < planterCount; ++i )
            {
                const auto &lot = *frontages[i];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.width : lot.depth ) * .5f;
                const float span = xFace ? lot.depth : lot.width;
                const auto p = Vector3F( lot.x, .15f, lot.z ) + normal * ( half + 1.15f ) +
                               tangent * ( span * .35f );
                for( int side : { -1, 1 } )
                {
                    axisBox( stone, p + normal * ( side * .31f ) + Vector3F( 0, .275f, 0 ), 1.6f, .55f,
                             .08f, xFace );
                    axisBox( stone, p + tangent * ( side * .76f ) + Vector3F( 0, .275f, 0 ), .08f, .55f,
                             .54f, xFace );
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
                        const auto lower =
                            base + direction * ( .13f * a * a ) + Vector3F( 0, height * a, 0 );
                        const auto upper =
                            base + direction * ( .13f * b * b ) + Vector3F( 0, height * b, 0 );
                        const auto aWidth = across * ( .023f * ( 1 - .9f * a ) );
                        const auto bWidth = across * ( .023f * ( 1 - .9f * b ) );
                        auto faceNormal = ( lower + aWidth - ( lower - aWidth ) )
                                              .crossProduct( upper + bWidth - ( lower - aWidth ) );
                        faceNormal.normalise();
                        planting.quad( lower - aWidth, lower + aWidth, upper + bWidth, upper - bWidth,
                                       faceNormal );
                    }
                }
                staticBox( assets, "City planter", p + Vector3F( 0, .275f, 0 ),
                           xFace ? Vector3F( .7f, .55f, 1.6f ) : Vector3F( 1.6f, .55f, .7f ) );
            }
            const size_t binCount = city.naturalRoads || assets.cityTerrainRoads
                                        ? 0
                                        : std::min<size_t>( frontages.size(), preview ? 4 : 8 );
            for( size_t i = 0; i < binCount; ++i )
            {
                const auto &lot = *frontages[( i * 3 + 1 ) % frontages.size()];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.width : lot.depth ) * .5f;
                const float span = xFace ? lot.depth : lot.width;
                const auto p = Vector3F( lot.x, .15f, lot.z ) + normal * ( half + 1.1f ) -
                               tangent * ( span * .35f );
                metal.box( p + Vector3F( 0, .45f, 0 ), { .42f, .9f, .42f } );
                metal.box( p + Vector3F( 0, .93f, 0 ), { .47f, .06f, .47f } );
                axisBox( dark, p + Vector3F( 0, .75f, 0 ) + normal * .214f, .26f, .10f, .005f, xFace );
                staticBox( assets, "City waste bin", p + Vector3F( 0, .48f, 0 ), { .48f, .96f, .48f } );
            }
            const auto street = OpenCityStreetDetails::generate( city, 0, false );
            SourceBudgetVector<const OpenCityStreetDetails::Corner *> walks;
            walks.reserve( street.sidewalks.size() );
            for( const auto &walk : street.sidewalks )
            {
                walks.push_back( &walk );
            }
            std::sort( walks.begin(), walks.end(), []( const auto *a, const auto *b ) {
                return a->x * a->x + a->z * a->z < b->x * b->x + b->z * b->z;
            } );
            const size_t detailedWalks =
                assets.cityTerrainRoads ? 0 : std::min<size_t>( walks.size(), preview ? 32 : 64 );
            for( size_t i = 0; i < detailedWalks; ++i )
            {
                const auto &walk = *walks[i];
                const bool horizontalWalk = walk.width > walk.depth;
                const float length = horizontalWalk ? walk.width : walk.depth;
                const float step = preview ? 4.8f : 2.4f;
                for( float t = -length * .5f + step; t < length * .5f - .2f; t += step )
                {
                    horizontal( joints,
                                { walk.x + ( horizontalWalk ? t : 0 ), .156f,
                                  walk.z + ( horizontalWalk ? 0 : t ) },
                                horizontalWalk ? .025f : walk.width,
                                horizontalWalk ? walk.depth : .025f );
                }
                horizontal( joints, { walk.x, .156f, walk.z }, horizontalWalk ? length : .025f,
                            horizontalWalk ? .025f : length );
                if( i % 3 != 0 )
                {
                    continue;
                }
                const Vector3F centre( walk.x, .158f, walk.z );
                const auto vertex = [&]( Vector3F p, Vector3F uv ) {
                    const auto index = u32( metal.positions.size() );
                    metal.positions.push_back( p );
                    metal.normals.push_back( { 0, 1, 0 } );
                    metal.tangents.push_back( { 1, 0, 0, 1 } );
                    metal.uv.push_back( uv );
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
                {
                    metal.indices.push_back( c );
                    metal.indices.push_back( first + segment + 1 );
                    metal.indices.push_back( first + segment );
                }
                for( int groove = -2; groove <= 2; ++groove )
                {
                    const float z = groove * .09f;
                    horizontal( dark, centre + Vector3F( 0, .001f, z ),
                                2 * std::sqrt( .31f * .31f - z * z ) - .04f, .012f );
                }
            }
            mesh( assets, "City park paving and planter rims", stone,
                  material( assets, { .49f, .48f, .42f, 1 }, .9f ) );
            mesh( assets, "City bench frames bins and utility covers", metal,
                  material( assets, { .12f, .15f, .16f, 1 }, .62f, .55f ) );
            mesh( assets, "City bench timber", wood, material( assets, { .36f, .21f, .10f, 1 }, .8f ) );
            mesh( assets, "City paving joints and planter soil", dark,
                  material( assets, { .12f, .14f, .11f, 1 }, 1 ) );
            mesh( assets, "City concrete paving joints", joints,
                  material( assets, { .31f, .32f, .30f, 1 }, 1 ) );
            mesh( assets, "City ornamental planter grasses", planting,
                  material( assets, { .24f, .36f, .13f, 1 }, .92f ) );
        }

        struct CityStreetFixture
        {
            enum Kind { Bench, Planter, Bin, Lamp };
            Vector3F position, along, outward;
            Kind kind;
            float halfLength, halfDepth;
            float lowest = std::numeric_limits<float>::infinity();
            float highest = -std::numeric_limits<float>::infinity();
            double area = 0;
        };

        void measureStreetFixtureSupport( CityStreetFixture &site, const Vector3F &a,
                                          const Vector3F &b, const Vector3F &c )
        {
            // Clip the actual fitted triangle in the fixture's horizontal frame.
            // Y stays affine through clipping, so support is tested over the whole base.
            std::array<Vector3F, 8> polygon{}, clipped{};
            size_t count = 0;
            for( const auto &p : { a, b, c } )
            {
                const auto offset = p - site.position;
                polygon[count++] = { offset.dotProduct( site.along ), p.y,
                                     offset.dotProduct( site.outward ) };
            }
            for( int axis = 0; axis < 2 && count >= 3; ++axis )
            {
                for( const float side : { -1.f, 1.f } )
                {
                    const float extent = axis == 0 ? site.halfLength : site.halfDepth;
                    const auto distance = [=]( const Vector3F &v ) {
                        return extent - side * ( axis == 0 ? v.x : v.z );
                    };
                    size_t next = 0;
                    auto previous = polygon[count - 1];
                    float from = distance( previous );
                    for( size_t i = 0; i < count; ++i )
                    {
                        const auto current = polygon[i];
                        const float to = distance( current );
                        if( ( from < 0 && to > 0 ) || ( from > 0 && to < 0 ) )
                        {
                            clipped[next++] = previous + ( current - previous ) * ( from / ( from - to ) );
                        }
                        if( to >= 0 )
                        {
                            clipped[next++] = current;
                        }
                        previous = current;
                        from = to;
                    }
                    polygon.swap( clipped );
                    count = next;
                    if( count < 3 )
                    {
                        return;
                    }
                }
            }
            double area = 0;
            for( size_t i = 1; i + 1 < count; ++i )
            {
                const auto u = polygon[i] - polygon[0], v = polygon[i + 1] - polygon[0];
                area += double( u.x ) * v.z - double( u.z ) * v.x;
            }
            if( std::abs( area ) <= 1e-10 )
            {
                return;
            }
            site.area += std::abs( area ) * .5;
            for( size_t i = 0; i < count; ++i )
            {
                site.lowest = std::min( site.lowest, polygon[i].y );
                site.highest = std::max( site.highest, polygon[i].y );
            }
        }

        void buildTerrainCityStreetDressing( SceneAssets &assets, const CityRoadTerrainMesh &walks,
                                             const SourceBudgetVector<Vector3F> &existingLamps,
                                             VehicleAppearanceQuality quality,
                                             SourceBudgetVector<CityStreetFixture> &installed )
        {
            assets.cityStreetDetailCount = 0;
            const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
            if( detail == 0 || walks.indices.empty() )
            {
                return;
            }
            constexpr size_t maxCandidates = 4096;
            constexpr float halfLengths[] = { 1.05f, .89f, .28f, .25f };
            constexpr float halfDepths[] = { .30f, .30f, .28f, .25f };
            constexpr float maxRadius = 1.2f;
            const auto &bake = assets.roadAuthoring->getBake();
            const auto &source = assets.roadAuthoring->getSource();
            const float extent = assets.cityLayout.extent + 12;
            SourceBudgetVector<CityStreetFixture> candidates;
            const auto addCandidate = [&]( const Vector3F &edge, const Vector3F &along,
                                           const Vector3F &outward, CityStreetFixture::Kind kind ) {
                // The deepest base uses only the outer 0.77 m of the 2.4 m walk.
                const auto position = edge + outward * 1.98f;
                if( candidates.size() >= maxCandidates || std::abs( position.x ) > extent ||
                    std::abs( position.z ) > extent )
                {
                    return;
                }
                if( std::any_of( walks.crossings.begin(), walks.crossings.end(), [&]( const auto &crossing ) {
                        const auto offset = position - crossing.centre;
                        const auto extentAlong = halfLengths[kind] * std::abs( crossing.outward.dotProduct( along ) ) +
                            halfDepths[kind] * std::abs( crossing.outward.dotProduct( outward ) );
                        const auto extentAcross = halfLengths[kind] * std::abs( crossing.across.dotProduct( along ) ) +
                            halfDepths[kind] * std::abs( crossing.across.dotProduct( outward ) );
                        return std::abs( offset.dotProduct( crossing.outward ) ) < crossing.depth * .5f + extentAlong + .95f &&
                               std::abs( offset.dotProduct( crossing.across ) ) < crossing.width * .5f + extentAcross + 2.5f;
                    } ) ) return;
                const float lampClearance = kind == CityStreetFixture::Lamp ? 9.f : 1.65f;
                if( std::any_of( existingLamps.begin(), existingLamps.end(), [&]( const auto &lamp ) {
                        const float dx = lamp.x - position.x, dz = lamp.z - position.z;
                        return dx * dx + dz * dz < lampClearance * lampClearance;
                    } ) )
                {
                    return;
                }
                candidates.push_back( { position, along, outward, kind,
                                        halfLengths[kind], halfDepths[kind] } );
            };
            for( const auto &junction : source.roundabouts )
            {
                for( size_t i = 0; i < junction.ports.size(); ++i )
                {
                    const auto &port = junction.ports[i];
                    Vector3F along( port.outward.x, 0, port.outward.z );
                    along.normalise();
                    const auto centre = junction.center + along * ( junction.outerRadius + port.length * .55f );
                    for( const int side : { -1, 1 } )
                    {
                        const Vector3F outward( side * along.z, 0, -side * along.x );
                        const auto kind = ( i % 2 == 0 ) == ( side > 0 ) ? CityStreetFixture::Planter
                                                                         : CityStreetFixture::Bench;
                        addCandidate( centre + outward * ( port.width * .5f ), along, outward, kind );
                    }
                }
            }
            for( size_t road = 0; road < bake.roads.size() && candidates.size() < maxCandidates; ++road )
            {
                const auto &points = bake.roads[road].points;
                float next = 11.f + float( road % 4 ) * 2.f;
                size_t station = 0;
                for( size_t i = 1; i < points.size() && candidates.size() < maxCandidates; ++i )
                {
                    const auto &a = points[i - 1], &b = points[i];
                    while( next <= b.distance && next + 4 < points.back().distance &&
                           candidates.size() + 2 <= maxCandidates )
                    {
                        const float t = std::clamp( ( next - a.distance ) / ( b.distance - a.distance ),
                                                    0.f, 1.f );
                        next += 25.f;
                        ++station;
                        const auto centre = a.position + ( b.position - a.position ) * t;
                        for( const bool left : { true, false } )
                        {
                            const auto edgeA = left ? a.left : a.right, edgeB = left ? b.left : b.right;
                            auto along = edgeB - edgeA;
                            along.y = 0;
                            if( along.lengthSquared() < 1e-8f )
                            {
                                continue;
                            }
                            along.normalise();
                            const auto edge = edgeA + ( edgeB - edgeA ) * t;
                            auto outward = Vector3F( along.z, 0, -along.x );
                            if( ( edge - centre ).dotProduct( outward ) < 0 )
                            {
                                outward = -outward;
                            }
                            const auto kind = static_cast<CityStreetFixture::Kind>(
                                ( road + station * 3 + ( left ? 0 : 2 ) ) % 4 );
                            addCandidate( edge, along, outward, kind );
                        }
                    }
                }
            }
            if( candidates.empty() )
            {
                return;
            }

            // Index the comparatively few fixture centres and stream the already-bounded
            // footway mesh. Skirt faces have horizontal normals and supply no support.
            const auto cell = []( float coordinate ) { return int( std::floor( coordinate / 16 ) ); };
            const int firstCell = cell( -extent ), lastCell = cell( extent );
            SourceBudgetMap<std::pair<int, int>, SourceBudgetVector<size_t>> buckets;
            for( size_t i = 0; i < candidates.size(); ++i )
            {
                const auto &p = candidates[i].position;
                buckets[{ cell( p.x ), cell( p.z ) }].push_back( i );
            }
            for( size_t i = 0; i < walks.indices.size(); i += 3 )
            {
                const auto &a = walks.vertices[walks.indices[i]];
                const auto &b = walks.vertices[walks.indices[i + 1]];
                const auto &c = walks.vertices[walks.indices[i + 2]];
                if( a.normal.y < .9f || b.normal.y < .9f || c.normal.y < .9f )
                {
                    continue;
                }
                const float x0 = std::min( { a.position.x, b.position.x, c.position.x } ) - maxRadius;
                const float x1 = std::max( { a.position.x, b.position.x, c.position.x } ) + maxRadius;
                const float z0 = std::min( { a.position.z, b.position.z, c.position.z } ) - maxRadius;
                const float z1 = std::max( { a.position.z, b.position.z, c.position.z } ) + maxRadius;
                const int endX = std::min( cell( x1 ), lastCell ), endZ = std::min( cell( z1 ), lastCell );
                for( int x = std::max( cell( x0 ), firstCell ); x <= endX; ++x )
                {
                    for( int z = std::max( cell( z0 ), firstCell ); z <= endZ; ++z )
                    {
                        const auto found = buckets.find( { x, z } );
                        if( found == buckets.end() )
                        {
                            continue;
                        }
                        for( const auto index : found->second )
                        {
                            auto &site = candidates[index];
                            if( site.position.x >= x0 && site.position.x <= x1 &&
                                site.position.z >= z0 && site.position.z <= z1 )
                            {
                                measureStreetFixtureSupport( site, a.position, b.position, c.position );
                            }
                        }
                    }
                }
            }
            std::sort( candidates.begin(), candidates.end(), []( const auto &a, const auto &b ) {
                const auto &p = a.position, &q = b.position;
                const auto aDistance = p.x * p.x + p.z * p.z, bDistance = q.x * q.x + q.z * q.z;
                return std::tie( aDistance, p.x, p.z, a.kind, a.along.x, a.along.z ) <
                       std::tie( bDistance, q.x, q.z, b.kind, b.along.x, b.along.z );
            } );

            const size_t cap = quality == VehicleAppearanceQuality::Preview ? 40 : 88;
            const size_t limit = ( cap * detail + 99 ) / 100;
            SourceBudgetVector<CityStreetFixture> admitted;
            admitted.reserve( limit );
            for( auto &site : candidates )
            {
                const double baseArea = 4.0 * site.halfLength * site.halfDepth;
                if( std::abs( site.area - baseArea ) > .00005 || site.highest - site.lowest > .08f ||
                    std::any_of( admitted.begin(), admitted.end(), [&]( const auto &placed ) {
                        const float x = site.position.x - placed.position.x;
                        const float z = site.position.z - placed.position.z;
                        return x * x + z * z < 6.f * 6.f;
                    } ) )
                {
                    continue;
                }
                site.position.y = site.lowest;
                admitted.push_back( site );
                if( admitted.size() == limit )
                {
                    break;
                }
            }

            Geometry metal, timber, stone, planting, lenses;
            for( const auto &site : admitted )
            {
                const auto point = [&]( float u, float y, float v ) {
                    return site.position + site.along * u + Vector3F( 0, y, 0 ) + site.outward * v;
                };
                const auto box = [&]( Geometry &g, float u, float y, float v, float length,
                                      float height, float depth ) {
                    const float x0 = u - length * .5f, x1 = u + length * .5f;
                    const float y0 = y - height * .5f, y1 = y + height * .5f;
                    const float z0 = v - depth * .5f, z1 = v + depth * .5f;
                    const auto a = point( x0, y0, z0 ), b = point( x1, y0, z0 );
                    const auto c = point( x1, y0, z1 ), d = point( x0, y0, z1 );
                    const auto up = Vector3F( 0, y1 - y0, 0 );
                    g.quad( a + up, b + up, c + up, d + up, { 0, 1, 0 } );
                    g.quad( a, b, c, d, { 0, -1, 0 } );
                    g.quad( a, b, b + up, a + up, -site.outward );
                    g.quad( c, d, d + up, c + up, site.outward );
                    g.quad( d, a, a + up, d + up, -site.along );
                    g.quad( b, c, c + up, b + up, site.along );
                };
                switch( site.kind )
                {
                case CityStreetFixture::Bench:
                    for( int slat = -2; slat <= 2; ++slat )
                    {
                        box( timber, 0, .44f, slat * .075f, 1.94f, .045f, .064f );
                    }
                    for( int row = 0; row < 3; ++row )
                    {
                        box( timber, 0, .65f + row * .095f, .19f + row * .018f, 1.94f, .076f, .04f );
                    }
                    for( float side : { -.73f, .73f } )
                    {
                        box( metal, side, .18f, 0, .065f, .42f, .46f );
                        metal.beam( point( side, .33f, .16f ), point( side, .87f, .236f ), .035f );
                        metal.beam( point( side, .62f, .20f ), point( side, .62f, -.18f ), .023f );
                    }
                    break;
                case CityStreetFixture::Planter:
                    box( stone, 0, .19f, 0, 1.64f, .44f, .52f );
                    for( float side : { -1.f, 1.f } )
                    {
                        box( stone, 0, .42f, side * .24f, 1.72f, .12f, .10f );
                        box( stone, side * .81f, .42f, 0, .10f, .12f, .38f );
                    }
                    box( metal, 0, .404f, 0, 1.52f, .018f, .37f );
                    for( int tuft = -1; tuft <= 1; ++tuft )
                    {
                        for( int blade = 0; blade < 13; ++blade )
                        {
                            const float angle = blade * 2.39996323f + tuft;
                            const auto bend = site.along * std::cos( angle ) + site.outward * std::sin( angle );
                            const auto across = Vector3F( bend.z, 0, -bend.x ) * .024f;
                            const auto base = point( tuft * .47f, .41f, 0 ) + bend * .06f;
                            const float height = .23f + .022f * float( ( blade * 7 ) % 13 );
                            const auto middle = base + bend * .055f + Vector3F( 0, height * .62f, 0 );
                            const auto tip = base + bend * .16f + Vector3F( 0, height, 0 );
                            auto normal = across.crossProduct( middle - base );
                            normal.normalise();
                            const auto top = across * .55f;
                            planting.quad( base - across, base + across, middle + top, middle - top,
                                           normal );
                            planting.quad( middle - top, middle + top, tip + top * .13f,
                                           tip - top * .13f, normal );
                        }
                    }
                    break;
                case CityStreetFixture::Bin:
                    box( metal, 0, .33f, 0, .37f, .72f, .37f );
                    box( metal, 0, .79f, 0, .51f, .09f, .51f );
                    box( metal, 0, .71f, .13f, .41f, .18f, .22f );
                    for( int slat = -2; slat <= 2; ++slat )
                    {
                        for( float side : { -.20f, .20f } )
                        {
                            box( timber, slat * .077f, .33f, side, .056f, .61f, .036f );
                            box( timber, side, .33f, slat * .077f, .036f, .61f, .056f );
                        }
                    }
                    break;
                case CityStreetFixture::Lamp:
                    box( stone, 0, .035f, 0, .44f, .13f, .44f );
                    metal.beam( point( 0, .07f, 0 ), point( 0, .54f, 0 ), .082f );
                    metal.beam( point( 0, .53f, 0 ), point( 0, 3.43f, 0 ), .044f );
                    metal.beam( point( 0, 3.43f, 0 ), point( 0, 3.66f, -.52f ), .036f );
                    box( metal, 0, 3.67f, -.55f, .28f, .10f, .50f );
                    box( lenses, 0, 3.61f, -.55f, .24f, .035f, .42f );
                    break;
                }
            }

            const auto publish = [&]( const String &label, const Geometry &geometry, ColourF colour,
                                      float roughness, float metalness = 0 ) {
                if( !geometry.indices.empty() )
                {
                    mesh( assets, "Terrain city street detail " + label, geometry,
                          material( assets, colour, roughness, metalness ) );
                }
            };
            publish( "metal frames", metal, { .13f, .17f, .16f, 1 }, .66f, .5f );
            publish( "timber slats", timber, { .48f, .28f, .13f, 1 }, .85f );
            publish( "stone planters and plinths", stone, { .67f, .63f, .53f, 1 }, .88f );
            publish( "ornamental grasses", planting, { .31f, .43f, .16f, 1 }, .93f );
            publish( "pedestrian lamp lenses", lenses, { .86f, .83f, .72f, 1 }, .38f );
            assets.cityStreetDetailCount = static_cast<u32>( admitted.size() );
            installed.swap( admitted );
        }

#include "CityVergeTransitions.hpp"
#include "CityRoundaboutLandscape.hpp"

        void buildTerrainCityVerges( SceneAssets &assets, const CityRoadTerrainMesh &walks, u32 seed,
                                     VehicleAppearanceQuality quality )
        {
            const auto ground = assets.roadAuthoring->getTerrainSnapshot();
            if( !ground ) return;
            const auto &placement = assets.roadAuthoring->getTerrainPlacement();
            const auto &source = assets.roadAuthoring->getSource();
            const auto &roads = assets.roadAuthoring->getBake().roads;
            if( source.roads.size() != roads.size() ) return;
            const auto scalar = []( u64 key ) {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            };
            const auto locationKey = [seed]( Vector3F point ) {
                const auto x = static_cast<u32>( static_cast<s32>( std::floor( point.x * 2 ) ) );
                const auto z = static_cast<u32>( static_cast<s32>( std::floor( point.z * 2 ) ) );
                return render::foliageStableHash( ( u64( x ) << 32 ) | z ) ^ seed;
            };
            const auto distanceSquared = []( Vector3F p, Vector3F a, Vector3F b ) {
                const float x = b.x - a.x, z = b.z - a.z;
                const float d = x * x + z * z;
                const float t = d > 0 ? std::clamp( ( ( p.x - a.x ) * x + ( p.z - a.z ) * z ) / d, 0.f, 1.f ) : 0;
                const float rx = p.x - ( a.x + t * x ), rz = p.z - ( a.z + t * z );
                return rx * rx + rz * rz;
            };
            enum class VergePlant { Meadow, Fescue, GroundCover, Bramble, Heather };
            const auto woody = []( VergePlant plant ) {
                return plant == VergePlant::Bramble || plant == VergePlant::Heather;
            };
            struct Site
            {
                Vector3F position;
                u64 key;
                float radius;
                VergePlant plant;
                bool blocked = false;
            };
            const bool preview = quality == VehicleAppearanceQuality::Preview;
            const size_t limit = preview ? 580 : 1580;
            SourceBudgetVector<Site> sites;
            sites.reserve( limit );
            const auto plant = [&]( Vector3F centre, Vector3F along, float roadWidth, u64 key,
                                    bool outer, bool terminal ) {
                const Vector3F side( -along.z, 0, along.x );
                for( const float sign : { -1.f, 1.f } )
                {
                    const auto sideKey = key ^ ( sign > 0 ? 0xdfe493b9u : 0xbc6b2fbdu );
                    const float density = scalar( sideKey );
                    if( !terminal && density < ( outer ? .12f : .44f ) ) continue;
                    const unsigned members = ( terminal ? 8u : 4u ) + unsigned( density * ( outer ? 7.f : 4.f ) );
                    const float drift = ( scalar( sideKey + 11 ) - .5f ) * ( outer ? 4.2f : 2.4f );
                    const float patchDepth = scalar( sideKey + 17 ) * ( outer ? 3.3f : 1.3f );
                    const float stretch = terminal ? 8.8f : 3.8f + 6.2f * density;
                    const float habitat = scalar( sideKey + 23 );
                    for( unsigned i = 0; i < members && sites.size() < limit; ++i )
                    {
                        const auto variation = render::foliageStableHash( sideKey + i );
                        const float mix = scalar( variation );
                        VergePlant species = habitat < .34f ? VergePlant::Fescue : VergePlant::Meadow;
                        if( mix < .26f ) species = VergePlant::GroundCover;
                        if( outer && i >= 3 && mix > .75f )
                            species = habitat > .43f ? VergePlant::Bramble : VergePlant::Heather;
                        const auto size = scalar( variation + 3 );
                        const float radius = woody( species ) ? .58f + .85f * size : .22f + .45f * size;
                        // The first members form a low kerb fringe. The rest
                        // drift outward in an irregular patch, with scrub set back.
                        const float patch = i < 3 ? .48f * patchDepth : patchDepth;
                        const float swale = outer && density > .62f && i >= 3
                                                ? 1.2f + 2.3f * scalar( sideKey + 28 )
                                                : 0.f;
                        const float across = roadWidth * .5f + 2.63f + radius + patch +
                                             ( woody( species ) ? 1.6f : 0.f ) + swale +
                                             scalar( variation + 1 ) * ( i < 3 ? 1.7f : 3.2f );
                        const float offset = drift + ( scalar( variation + 2 ) - .5f ) * stretch;
                        auto point = centre + side * ( sign * across ) + along * offset;
                        if( !render::sampleTerrainHeight( *ground, placement, point, point.y ) ) continue;
                        bool supported = true;
                        for( const auto direction : { Vector3F( 1, 0, 0 ), Vector3F( -1, 0, 0 ),
                                                      Vector3F( 0, 0, 1 ), Vector3F( 0, 0, -1 ) } )
                        {
                            float height = 0;
                            supported = supported && render::sampleTerrainHeight(
                                *ground, placement, point + direction * radius, height ) &&
                                std::abs( height - point.y ) < .42f * radius;
                        }
                        if( supported ) sites.push_back( { point, variation, radius, species } );
                    }
                }
            };
            const float outerRadius = assets.cityLayout.extent * .76f;
            // A roundabout owns its approach pieces, so they never receive the
            // ordinary spline station pass below. Fill their supported outer verges
            // with irregular drifts as well as the occasional formal planting bed.
            const auto streetDetail = std::clamp( assets.cityStreetDetail, 0, 100 );
            const size_t approachLimit = std::min( limit / 5, size_t( 3 * streetDetail ) );
            for( const auto &junction : source.roundabouts )
            {
                for( const auto &port : junction.ports )
                {
                    if( sites.size() >= approachLimit ) break;
                    if( port.length < 10.f ) continue;
                    const auto direction = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                    const auto point = junction.center + direction * ( junction.outerRadius + port.length * .63f );
                    plant( point, direction, port.width, locationKey( point ) ^ 0x70a7bd31u, false, true );
                }
                if( sites.size() >= approachLimit ) break;
            }
            // Give the remaining budget to the outskirts first, including the last few
            // metres of a free approach viewed from outside the city.
            for( bool endsOnly : { true, false } )
                for( size_t r = 0; r < roads.size() && sites.size() < limit; ++r )
                {
                    const auto &points = roads[r].points;
                    if( points.size() < 2 ) continue;
                    if( endsOnly )
                    {
                        if( source.roads[r].closed ) continue;
                        for( unsigned end = 0; end < 2; ++end )
                        {
                            const auto &sample = end ? points.back() : points.front();
                            const auto &p = sample.position;
                            if( std::max( std::abs( p.x ), std::abs( p.z ) ) < outerRadius ) continue;
                            Vector3F inward( sample.tangent.x, 0, sample.tangent.z );
                            inward.normalise();
                            inward *= end ? -1.f : 1.f;
                            // Keep the endpoint's forward vehicle approach clear.
                            for( unsigned group = 0; group < 3; ++group )
                                plant( p + inward * ( 1.6f + group * 7.5f ), inward,
                                       source.roads[r].width,
                                       locationKey( p ) ^ ( u64( group + 1 ) << 19 ), true, true );
                        }
                    }
                    else
                    {
                        float station = 9.f + 8.f * scalar( locationKey( points.front().position ) );
                        for( size_t j = 1; j < points.size() && sites.size() < limit; ++j )
                        {
                            const auto &a = points[j - 1], &b = points[j];
                            const float length = b.distance - a.distance;
                            if( length <= 0 ) continue;
                            Vector3F direction( b.position.x - a.position.x, 0, b.position.z - a.position.z );
                            if( direction.lengthSquared() < 1e-8f ) continue;
                            direction.normalise();
                            station = std::max( station, a.distance );
                            while( station <= b.distance && sites.size() < limit )
                            {
                                const float t = ( station - a.distance ) / length;
                                const auto p = a.position + ( b.position - a.position ) * t;
                                const bool outer = std::max( std::abs( p.x ), std::abs( p.z ) ) >= outerRadius;
                                const auto key = locationKey( p ) ^ 0x93c9e1f0u;
                                plant( p, direction, source.roads[r].width, key, outer, false );
                                station += ( outer ? 8.f : 21.f ) + ( outer ? 16.f : 13.f ) * scalar( key );
                            }
                        }
                    }
                }

            constexpr float bucketSize = 12.f;
            constexpr float maxRadius = 1.65f;  // Largest plant plus separation from paving.
            using Bucket = std::pair<int, int>;
            SourceBudgetMap<Bucket, SourceBudgetVector<size_t>> buckets;
            const auto cell = [bucketSize]( float value ) { return int( std::floor( value / bucketSize ) ); };
            for( size_t i = 0; i < sites.size(); ++i )
                buckets[{ cell( sites[i].position.x ), cell( sites[i].position.z ) }].push_back( i );
            const auto visit = [&]( float minX, float minZ, float maxX, float maxZ, const auto &test ) {
                // Probe only populated rows. Long source segments cannot turn a
                // sparse planting pass into a scan over empty world cells.
                const int x0 = cell( minX ), x1 = cell( maxX ), z0 = cell( minZ ), z1 = cell( maxZ );
                for( auto entry = buckets.lower_bound( { x0, std::numeric_limits<int>::min() } );
                     entry != buckets.end() && entry->first.first <= x1; ++entry )
                    if( entry->first.second >= z0 && entry->first.second <= z1 )
                        for( const auto index : entry->second )
                            if( !sites[index].blocked ) sites[index].blocked = test( sites[index] );
            };
            for( size_t r = 0; r < roads.size(); ++r )
            {
                const auto &points = roads[r].points;
                const float reserve = source.roads[r].width * .5f + 2.45f;
                const float extent = reserve + maxRadius;
                for( size_t i = 1; i < points.size(); ++i )
                {
                    const auto a = points[i - 1].position, b = points[i].position;
                    visit( std::min( a.x, b.x ) - extent, std::min( a.z, b.z ) - extent,
                           std::max( a.x, b.x ) + extent, std::max( a.z, b.z ) + extent,
                           [&]( const Site &site ) {
                               const float clearance = reserve + site.radius;
                               return distanceSquared( site.position, a, b ) < clearance * clearance;
                           } );
                }
            }
            for( const auto &junction : source.roundabouts )
            {
                const auto centre = junction.center;
                const float reserve = junction.outerRadius + 2.45f;
                const float extent = reserve + maxRadius;
                visit( centre.x - extent, centre.z - extent, centre.x + extent, centre.z + extent,
                       [&]( const Site &site ) {
                           const float clearance = reserve + site.radius;
                           return distanceSquared( site.position, centre, centre ) < clearance * clearance;
                       } );
            }
            const auto cross = []( Vector3F a, Vector3F b, Vector3F p ) {
                return ( b.x - a.x ) * ( p.z - a.z ) - ( b.z - a.z ) * ( p.x - a.x );
            };
            for( size_t i = 0; i < walks.indices.size(); i += 3 )
            {
                const auto a = walks.vertices[walks.indices[i]].position;
                const auto b = walks.vertices[walks.indices[i + 1]].position;
                const auto c = walks.vertices[walks.indices[i + 2]].position;
                if( std::abs( cross( a, b, c ) ) < 1e-6f ) continue;
                visit( std::min( { a.x, b.x, c.x } ) - maxRadius, std::min( { a.z, b.z, c.z } ) - maxRadius,
                       std::max( { a.x, b.x, c.x } ) + maxRadius, std::max( { a.z, b.z, c.z } ) + maxRadius,
                       [&]( const Site &site ) {
                           const auto p = site.position;
                           const auto u = cross( a, b, p ), v = cross( b, c, p ), w = cross( c, a, p );
                           const float radius = site.radius + .2f;
                           return ( u <= 0 && v <= 0 && w <= 0 ) || ( u >= 0 && v >= 0 && w >= 0 ) ||
                                  std::min( { distanceSquared( p, a, b ), distanceSquared( p, b, c ),
                                              distanceSquared( p, c, a ) } ) < radius * radius;
                       } );
            }

            Geometry grasses[2], seeds, leaves, blooms[2], stems;
            std::array<Geometry, 3> canopies;
            const auto triangle = []( Geometry &shape, Vector3F a, Vector3F b, Vector3F c,
                                      Vector3F normal ) {
                const auto tangent = ( b - a ).normaliseCopy();
                const auto offset = static_cast<u32>( shape.positions.size() );
                for( const auto point : { a, b, c } )
                {
                    shape.positions.push_back( point );
                    shape.normals.push_back( normal );
                    shape.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                }
                shape.uv.push_back( { 0, 0, 0 } );
                shape.uv.push_back( { 1, 0, 0 } );
                shape.uv.push_back( { .5f, 1, 0 } );
                const auto forward = ( b - a ).crossProduct( c - a ).dotProduct( normal ) >= 0;
                shape.indices.push_back( offset );
                shape.indices.push_back( offset + ( forward ? 1 : 2 ) );
                shape.indices.push_back( offset + ( forward ? 2 : 1 ) );
            };
            const auto fanLeaf = [&]( Vector3F base, Vector3F tip, Vector3F across, float rib ) {
                const auto middle = base + ( tip - base ) * .52f;
                const auto ridge = middle + Vector3F( 0, rib, 0 );
                const auto left = middle - across, right = middle + across;
                const auto forward = ( tip - base ).normaliseCopy();
                const auto normal = ( Vector3F( 0, 1, 0 ) - forward * forward.y ).normaliseCopy();
                const auto tilt = across.normaliseCopy() * .38f;
                triangle( leaves, base, left, ridge, ( normal - tilt ).normaliseCopy() );
                triangle( leaves, ridge, left, tip, ( normal - tilt ).normaliseCopy() );
                triangle( leaves, base, ridge, right, ( normal + tilt ).normaliseCopy() );
                triangle( leaves, ridge, tip, right, ( normal + tilt ).normaliseCopy() );
            };
            const auto canopyLobe = [&]( Geometry &shape, Vector3F centre, float radius, float height,
                                        float angle, u64 key ) {
                constexpr u32 sides = 8, stride = sides + 1;
                const auto first = static_cast<u32>( shape.positions.size() );
                const auto vertex = [&]( Vector3F point, Vector3F normal, Vector3F tangent, float u, float v ) {
                    shape.positions.push_back( point );
                    shape.normals.push_back( normal );
                    shape.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                    shape.uv.push_back( { u, v, 0 } );
                };
                for( u32 ring = 0; ring < 3; ++ring )
                {
                    const float latitude = ( float( ring ) - 1.f ) * ( pi * .25f );
                    const float slope = std::sin( latitude ), span = std::cos( latitude );
                    for( u32 side = 0; side <= sides; ++side )
                    {
                        const auto sector = side % sides;
                        const float azimuth = angle + sector * ( 2.f * pi / sides );
                        const Vector3F direction( std::cos( azimuth ), 0, std::sin( azimuth ) );
                        const float ripple = .88f + .12f * scalar( key + sector + ring * sides );
                        const auto point = centre + direction * ( radius * span * ripple ) +
                                           Vector3F( 0, height * ( slope + .09f * ( ripple - .94f ) ), 0 );
                        const auto normal = ( direction * ( span * height ) +
                                              Vector3F( 0, radius * slope, 0 ) ).normaliseCopy();
                        vertex( point, normal, { -direction.z, 0, direction.x }, float( side ) / sides,
                                .25f * ( ring + 1 ) );
                    }
                }
                for( u32 side = 0; side < sides; ++side )
                {
                    const auto pole = static_cast<u32>( shape.positions.size() );
                    const float u = ( side + .5f ) / sides;
                    const float azimuth = angle + 2.f * pi * u;
                    const Vector3F tangent( -std::sin( azimuth ), 0, std::cos( azimuth ) );
                    vertex( centre - Vector3F( 0, height, 0 ), { 0, -1, 0 }, tangent, u, 0 );
                    vertex( centre + Vector3F( 0, height, 0 ), { 0, 1, 0 }, tangent, u, 1 );
                    const u32 a = first + side, b = a + stride, c = b + stride;
                    for( const auto index : { pole, a, a + 1, a, b, a + 1, a + 1, b, b + 1,
                                              b, c, b + 1, b + 1, c, c + 1, c, pole + 1, c + 1 } )
                        shape.indices.push_back( index );
                }
            };
            const auto flower = [&]( Vector3F head, float spread, float angle, unsigned tint ) {
                for( unsigned petal = 0; petal < 5; ++petal )
                {
                    const float azimuth = angle + petal * ( 2.f * pi / 5.f );
                    const Vector3F direction( std::cos( azimuth ), 0, std::sin( azimuth ) );
                    const Vector3F side( -direction.z, 0, direction.x );
                    const auto centre = head + direction * ( spread * .63f );
                    const auto lip = centre + Vector3F( 0, spread * .2f, 0 );
                    blooms[tint].quad( head, centre + side * ( spread * .25f ),
                                       lip + direction * ( spread * .37f ), centre - side * ( spread * .25f ),
                                       { 0, 1, 0 } );
                }
            };
            struct PlantClearance
            {
                Vector3F position;
                float radius;
            };
            SourceBudgetVector<PlantClearance> planted;
            planted.reserve( sites.size() );
            size_t remainingTriangles = preview ? 45000 : 156000;
            for( const auto &site : sites )
            {
                const auto p = site.position;
                if( site.blocked || std::any_of( assets.cityLayout.lots.begin(), assets.cityLayout.lots.end(),
                    [&]( const auto &lot ) {
                        return std::abs( p.x - lot.parcelX ) < lot.parcelWidth * .5f + site.radius &&
                               std::abs( p.z - lot.parcelZ ) < lot.parcelDepth * .5f + site.radius;
                    } ) )
                {
                    continue;
                }
                const bool shrub = woody( site.plant );
                const float separation = site.radius * ( shrub ? .73f : .37f );
                if( std::any_of( planted.begin(), planted.end(), [&]( const PlantClearance &other ) {
                        const float clearance = other.radius + separation;
                        return distanceSquared( p, other.position, other.position ) < clearance * clearance;
                    } ) ) continue;
                // Upper bounds include stems, seed clusters and petal geometry.
                const size_t triangleBudget = shrub ? 248 : site.plant == VergePlant::GroundCover ? 76 : 132;
                if( triangleBudget > remainingTriangles ) continue;
                remainingTriangles -= triangleBudget;
                planted.push_back( { p, separation } );
                const float rotation = scalar( site.key ) * 2.f * pi;
                if( shrub )
                {
                    const bool bramble = site.plant == VergePlant::Bramble;
                    const unsigned clusters = bramble ? 4 : 3;
                    const float height = site.radius * ( bramble ? .71f : .40f );
                    for( unsigned cluster = 0; cluster < clusters; ++cluster )
                    {
                        const auto key = render::foliageStableHash( site.key + 53 + cluster );
                        const float angle = rotation + cluster * 2.39996323f;
                        const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                        const float offset = cluster == 0 ? .05f : .27f + .15f * scalar( key );
                        const float width = site.radius * ( .32f + .12f * scalar( key + 1 ) );
                        auto crown = p + direction * ( site.radius * offset );
                        if( !render::sampleTerrainHeight( *ground, placement, crown, crown.y ) ) continue;
                        crown.y += height * ( cluster == 0 ? .68f : .29f + .19f * scalar( key + 2 ) );
                        const unsigned foliage = bramble ? unsigned( cluster == 0 || scalar( key + 3 ) > .53f ) : 2;
                        const float lobeHeight = height * ( .33f + .13f * scalar( key + 4 ) );
                        canopyLobe( canopies[foliage], crown, width, lobeHeight, angle, key );
                        if( cluster == 0 || ( bramble && cluster == 2 ) )
                            stems.beam( p, crown, .013f );
                        for( unsigned leaf = 0; leaf < ( bramble ? 4u : 6u ); ++leaf )
                        {
                            const auto leafKey = render::foliageStableHash( key + 18 + leaf );
                            const float turn = angle + ( leaf - ( bramble ? 1.5f : 2.5f ) ) * .64f +
                                               .29f * scalar( leafKey );
                            const Vector3F outward( std::cos( turn ), 0, std::sin( turn ) );
                            const auto base = crown + outward * ( width * .62f ) +
                                              Vector3F( 0, lobeHeight * ( .07f + .44f * scalar( leafKey + 1 ) ), 0 );
                            const auto tip = base + outward * ( width * ( .40f + .24f * scalar( leafKey + 2 ) ) ) +
                                             Vector3F( 0, lobeHeight * ( .76f * scalar( leafKey + 3 ) - .26f ), 0 );
                            const auto side = Vector3F( -outward.z, .28f - .56f * scalar( leafKey + 5 ), outward.x ) *
                                              ( width * ( bramble ? .12f : .082f ) );
                            const auto middle = ( base + tip ) * .5f;
                            const auto normal = side.crossProduct( tip - base ).normaliseCopy();
                            leaves.quad( base, middle - side, tip, middle + side, normal );
                        }
                        if( !bramble && cluster < 2 && scalar( site.key + 61 ) > .29f )
                            flower( crown + direction * ( width * .44f ) + Vector3F( 0, height * .22f, 0 ),
                                    .04f + .035f * scalar( key + 7 ), angle, 1 );
                    }
                    continue;
                }
                if( site.plant == VergePlant::GroundCover )
                {
                    for( unsigned leaf = 0; leaf < 8; ++leaf )
                    {
                        const float angle = rotation + leaf * 2.39996323f;
                        const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                        const float fraction = .35f + .46f * scalar( site.key + 71 + leaf );
                        auto tip = p + direction * ( site.radius * fraction );
                        if( !render::sampleTerrainHeight( *ground, placement, tip, tip.y ) ) continue;
                        tip.y += .035f + site.radius * .13f * scalar( site.key + 83 + leaf );
                        fanLeaf( p + Vector3F( 0, .045f, 0 ), tip,
                                 Vector3F( -direction.z, 0, direction.x ) * ( site.radius * fraction * .27f ), .019f );
                    }
                    if( scalar( site.key + 97 ) > .43f )
                        for( unsigned stem = 0; stem < 2; ++stem )
                        {
                            const float angle = rotation + stem * 2.39996323f;
                            const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                            const auto head = p + direction * ( site.radius * .23f ) +
                                              Vector3F( 0, .12f + .33f * site.radius * scalar( site.key + 101 + stem ), 0 );
                            stems.beam( p, head, .004f );
                            flower( head, .037f + .043f * scalar( site.key + 108 + stem ), angle, 0 );
                        }
                    continue;
                }
                const bool fescue = site.plant == VergePlant::Fescue;
                const unsigned blades = fescue ? 8 : 7;
                for( unsigned blade = 0; blade < blades; ++blade )
                {
                    const auto variation = render::foliageStableHash( site.key + blade + 37 );
                    const float angle = rotation + blade * 2.39996323f + .65f * scalar( variation + 11 );
                    const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                    const Vector3F across( -direction.z, 0, direction.x );
                    auto root = p + direction * ( site.radius * ( .08f + .34f * scalar( variation ) ) );
                    if( !render::sampleTerrainHeight( *ground, placement, root, root.y ) ) continue;
                    root.y -= .015f;
                    const bool shortBlade = blade >= ( blades + 1 ) / 2;
                    const float tier = shortBlade ? .51f + .26f * scalar( variation + 16 )
                                                  : .87f + .13f * scalar( variation + 16 );
                    const float height = tier * ( ( fescue ? .068f : .095f ) + site.radius *
                                                   ( .14f + ( fescue ? .24f : .35f ) * scalar( variation + 1 ) ) );
                    const float bend = site.radius * ( .24f + .29f * scalar( variation + 2 ) ) *
                                       ( shortBlade ? .77f : 1.f );
                    const auto lean = across * ( bend * .44f * ( scalar( variation + 8 ) - .5f ) );
                    auto lower = root + direction * ( bend * .22f ) + lean * .12f + Vector3F( 0, height * .46f, 0 );
                    auto upper = root + direction * ( bend * .65f ) + lean * .57f + Vector3F( 0, height, 0 );
                    auto tip = root + direction * bend + lean +
                               Vector3F( 0, height * ( .59f + .43f * scalar( variation + 4 ) ), 0 );
                    const auto followSurface = [&]( Vector3F &point ) {
                        float groundHeight = 0;
                        if( !render::sampleTerrainHeight( *ground, placement, point, groundHeight ) ) return false;
                        point.y = std::max( point.y, groundHeight + .025f );
                        return true;
                    };
                    if( !followSurface( lower ) || !followSurface( upper ) || !followSurface( tip ) ) continue;
                    const float width = fescue ? .012f + .007f * scalar( variation + 3 )
                                               : .014f + .009f * scalar( variation + 3 );
                    auto &grass = grasses[fescue == ( scalar( variation + 12 ) > .18f ) ? 1 : 0];
                    cityVerge::foldedGrassBlade( grass, std::array{ root, lower, upper, tip },
                                                { width * .71f, width, width * .61f }, across,
                                                [&]( Vector3F point ) {
                                                    float groundHeight = 0;
                                                    return render::sampleTerrainHeight( *ground, placement, point,
                                                                                        groundHeight );
                                                } );
                    if( !fescue && blade == 0 && scalar( variation + 5 ) > .36f )
                    {
                        const auto head = root + direction * ( bend * .66f ) + Vector3F( 0, height + .085f, 0 );
                        seeds.beam( root, head, .005f );
                        canopyLobe( seeds, head, .015f + .004f * scalar( variation + 7 ), .034f, angle, variation );
                    }
                }
            }
            SmartPtr<render::ITexture> canopyTexture;
            if( std::any_of( canopies.begin(), canopies.end(),
                             []( const Geometry &shape ) { return !shape.indices.empty(); } ) )
            {
                constexpr int resolution = 128;
                TextureBuffer leafMap( resolution, resolution );
                const auto paint = [&]( int x, int y, u8 light ) {
                    const auto index = ( size_t( ( y + resolution ) % resolution ) * resolution +
                                         ( x + resolution ) % resolution ) * 4;
                    leafMap.pixels[index] = light;
                    leafMap.pixels[index + 1] = light;
                    leafMap.pixels[index + 2] = light;
                    leafMap.pixels[index + 3] = 255;
                };
                for( int y = 0; y < resolution; ++y )
                    for( int x = 0; x < resolution; ++x )
                        paint( x, y, u8( 165 + 25 * scalar( size_t( y ) * resolution + x ) ) );
                for( u64 leaf = 0; leaf < 360; ++leaf )
                {
                    const auto key = render::foliageStableHash( seed ^ ( 0xc2a9au + leaf ) );
                    const int cx = int( resolution * scalar( key ) ), cy = int( resolution * scalar( key + 1 ) );
                    const float angle = 2.f * pi * scalar( key + 2 );
                    const float ax = std::cos( angle ), ay = std::sin( angle );
                    const float radius = 3.5f + 4.0f * scalar( key + 3 );
                    const float bladeWidth = 1.8f + 1.8f * scalar( key + 4 );
                    const float brightness = 182.f + 56.f * scalar( key + 5 );
                    const int reach = int( std::ceil( radius ) );
                    for( int y = -reach; y <= reach; ++y )
                        for( int x = -reach; x <= reach; ++x )
                        {
                            const float along = ( ax * x + ay * y ) / radius;
                            const float across = ( ax * y - ay * x ) / bladeWidth;
                            const float edge = along * along + across * across * ( 1 + .7f * std::abs( along ) );
                            if( edge < 1.f )
                            {
                                const float vein = std::abs( across ) < .15f ? 13.f : 0.f;
                                paint( cx + x, cy + y, u8( brightness - 23.f * edge + vein ) );
                            }
                        }
                }
                canopyTexture = upload( assets, leafMap );
            }
            const auto publish = [&]( const char *name, const Geometry &geometry, ColourF tint, bool twoSided,
                                      bool textured = false, bool castShadows = true ) {
                if( geometry.indices.empty() ) return;
                auto surface = material( assets, tint, .94f );
                if( !twoSided ) surface->setCullMode( 2 );
                if( textured ) surface->setTexture( canopyTexture, 0 );
                if( auto actor = mesh( assets, String( "Terrain city verge " ) + name, geometry, surface );
                    actor && !castShadows )
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
            };
            publish( "meadow grass", grasses[0], { .334f, .402f, .177f, 1 }, true, false, false );
            publish( "dry fescue", grasses[1], { .398f, .399f, .216f, 1 }, true, false, false );
            publish( "seed heads", seeds, { .447f, .388f, .238f, 1 }, false, false, false );
            publish( "shaded shrub foliage", canopies[0], { .29f, .39f, .158f, 1 }, false, true );
            publish( "shrub growth tips", canopies[1], { .355f, .442f, .204f, 1 }, false, true );
            publish( "heather foliage", canopies[2], { .36f, .41f, .245f, 1 }, false, true );
            publish( "broad leaves", leaves, { .303f, .389f, .176f, 1 }, true );
            publish( "meadow flowers", blooms[0], { .70f, .63f, .375f, 1 }, true );
            publish( "heather flowers", blooms[1], { .48f, .277f, .335f, 1 }, true );
            publish( "plant stems", stems, { .285f, .229f, .123f, 1 }, false );
        }

        void buildNaturalCityStreets( SceneAssets &assets, const OpenCityLayout &city, Geometry &asphalt,
                                      Geometry &pavement, Geometry &markings, Geometry &streetMetal,
                                      Geometry &lampGlass, VehicleAppearanceQuality quality )
        {
            const auto &network = city.roads;
            if( network.nodes.empty() || network.edges.empty() || network.nodes.size() > 160 ||
                network.edges.size() > 240 )
            {
                throw std::runtime_error( "Natural city requires a bounded connected road graph." );
            }
            const auto position = [&]( u32 node ) {
                if( node >= network.nodes.size() )
                {
                    throw std::runtime_error( "Natural road endpoint is out of range." );
                }
                const auto p = network.nodes[node];
                if( !std::isfinite( p.x ) || !std::isfinite( p.z ) )
                {
                    throw std::runtime_error( "Natural road endpoint is not finite." );
                }
                return Vector3F( p.x, 0, p.z );
            };
            SourceBudgetVector<u32> degree( network.nodes.size(), 0 );
            SourceBudgetVector<float> junctionHalfWidth( network.nodes.size(), 0.f );
            for( const auto &edge : network.edges )
            {
                if( edge.a == edge.b ||
                    ( position( edge.b ) - position( edge.a ) ).lengthSquared() < .01f )
                {
                    throw std::runtime_error( "Natural road has an invalid segment." );
                }
                ++degree[edge.a];
                ++degree[edge.b];
                const float halfWidth = procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f;
                junctionHalfWidth[edge.a] = std::max( junctionHalfWidth[edge.a], halfWidth );
                junctionHalfWidth[edge.b] = std::max( junctionHalfWidth[edge.b], halfWidth );
            }
            const auto stripe = []( Geometry &g, Vector3F a, Vector3F b, Vector3F across, float width,
                                    float y ) {
                a.y = b.y = y;
                const auto half = across * ( width * .5f );
                g.quad( a - half, b - half, b + half, a + half, { 0, 1, 0 } );
            };
            const auto trim = [&]( u32 node, u32 other, Vector3F direction, int side, bool paint,
                                   float ownHalfWidth ) {
                float distance = paint ? junctionHalfWidth[node] + 2.5f : 0;
                for( const auto &edge : network.edges )
                {
                    const u32 neighbour = edge.a == node ? edge.b : edge.b == node ? edge.a : node;
                    if( neighbour == node || neighbour == other )
                    {
                        continue;
                    }
                    auto q = position( neighbour ) - position( node );
                    q.normalise();
                    const float cosine = direction.dotProduct( q );
                    const float sine = direction.x * q.z - direction.z * q.x;
                    const float otherHalfWidth = procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f;
                    if( paint )
                    {
                        if( cosine > 0 && std::abs( sine ) > .001f )
                        {
                            const float outerPaintOffset = ownHalfWidth - .3f + .065f;
                            distance = std::max( distance,
                                ( otherHalfWidth + outerPaintOffset * cosine ) / std::abs( sine ) + .1f );
                        }
                    }
                    else if( sine * side > .001f )
                    {
                        // A branch on this side can cross the walk. Behind-facing
                        // continuations use the inner offset, avoiding enormous trim
                        // distances on gentle bends close to a straight continuation.
                        const float offset = cosine >= 0 ? ownHalfWidth + 2.5f : ownHalfWidth;
                        distance =
                            std::max( distance, ( otherHalfWidth + offset * cosine ) / std::abs( sine ) + .05f );
                    }
                }
                return distance;
            };
            const auto ribbonClear = [&]( Vector3F a, Vector3F b, Vector3F across ) {
                const auto centre = ( a + b ) * .5f;
                auto along = b - a;
                const float halfLength = along.length() * .5f;
                along.normalise();
                constexpr float halfWidth = 1.25f;
                const auto point = [&]( Vector3F p ) {
                    p -= centre;
                    return Vector2F( p.dotProduct( along ), p.dotProduct( across ) );
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
                        {
                            if( std::abs( start ) > extent )
                            {
                                intersects = false;
                                break;
                            }
                        }
                        else
                        {
                            float t0 = ( -extent - start ) / direction,
                                  t1 = ( extent - start ) / direction;
                            if( t0 > t1 )
                            {
                                std::swap( t0, t1 );
                            }
                            low = std::max( low, t0 );
                            high = std::min( high, t1 );
                            if( low > high )
                            {
                                intersects = false;
                                break;
                            }
                        }
                    }
                    if( intersects )
                    {
                        return false;
                    }
                    float distanceSquared = std::min( distanceToBox( p ), distanceToBox( q ) );
                    const float lengthSquared = delta.x * delta.x + delta.y * delta.y;
                    for( int sx : { -1, 1 } )
                    {
                        for( int sy : { -1, 1 } )
                        {
                            const Vector2F corner( sx * halfLength, sy * halfWidth );
                            const float t = std::clamp(
                                ( ( corner.x - p.x ) * delta.x + ( corner.y - p.y ) * delta.y ) /
                                    std::max( lengthSquared, 1e-8f ),
                                0.f, 1.f );
                            const auto offset = corner - ( p + delta * t );
                            distanceSquared =
                                std::min( distanceSquared, offset.x * offset.x + offset.y * offset.y );
                        }
                    }
                    const float roadHalf = procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f - .002f;
                    if( distanceSquared < roadHalf * roadHalf )
                    {
                        return false;
                    }
                }
                return true;
            };
            u32 lampCount = 0;
            for( const auto &edge : network.edges )
            {
                const auto a = position( edge.a ), b = position( edge.b );
                auto direction = b - a;
                const float length = direction.length();
                direction /= length;
                const Vector3F across( -direction.z, 0, direction.x );
                const float width = procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass );
                const float halfWidth = width * .5f;
                // Full segment rectangles and round node fans form the road's
                // centreline-buffer union, including split crossings and dead ends.
                stripe( asphalt, a, b, across, width, .012f );
                const float paintStart = trim( edge.a, edge.b, direction, 0, true, halfWidth );
                const float paintEnd = trim( edge.b, edge.a, -direction, 0, true, halfWidth );
                if( length > paintStart + paintEnd + 2 )
                {
                    const auto start = a + direction * paintStart, end = b - direction * paintEnd;
                    for( int side : { -1, 1 } )
                    {
                        stripe( markings, start + across * ( side * ( halfWidth - .3f ) ),
                                end + across * ( side * ( halfWidth - .3f ) ), across, .13f, .018f );
                    }
                    for( float t = paintStart; t + 2 < length - paintEnd; t += 5 )
                    {
                        stripe( markings, a + direction * t, a + direction * ( t + 2 ), across, .15f,
                                .018f );
                    }
                }
                for( int side : { -1, 1 } )
                {
                    const float start = trim( edge.a, edge.b, direction, side, false, halfWidth );
                    const float end = trim( edge.b, edge.a, -direction, -side, false, halfWidth );
                    if( length <= start + end + 1 )
                    {
                        continue;
                    }
                    auto p = a + direction * start + across * ( side * ( halfWidth + 1.25f ) );
                    auto q = b - direction * end + across * ( side * ( halfWidth + 1.25f ) );
                    if( !ribbonClear( p, q, across ) )
                    {
                        continue;
                    }
                    const auto half = across * 1.25f;
                    const Vector3F up( 0, .15f, 0 );
                    pavement.quad( p - half + up, q - half + up, q + half + up, p + half + up,
                                   { 0, 1, 0 } );
                    pavement.quad( p - half, p + half, q + half, q - half, { 0, -1, 0 } );
                    pavement.quad( p + half, p + half + up, q + half + up, q + half, across );
                    pavement.quad( p - half, q - half, q - half + up, p - half + up, -across );
                    pavement.quad( p - half, p - half + up, p + half + up, p + half, -direction );
                    pavement.quad( q - half, q + half, q + half + up, q - half + up, direction );
                    const auto centre = ( p + q ) * .5f;
                    staticBox( assets, "City sidewalk", centre + Vector3F( 0, .075f, 0 ),
                               { 2.5f, .15f, ( q - p ).length() },
                               QuaternionF::eulerDegrees(
                                   0, std::atan2( direction.x, direction.z ) * 180 / pi, 0 ) );
                    if( lampCount >= ( quality == VehicleAppearanceQuality::Preview ? 24u : 64u ) ||
                        ( q - p ).lengthSquared() < 400 )
                    {
                        continue;
                    }
                    const auto base = centre + Vector3F( 0, .15f, 0 ),
                               top = base + Vector3F( 0, 6.2f, 0 );
                    const auto arm = across * ( -side * 1.1f ) + Vector3F( 0, -.15f, 0 );
                    streetMetal.beam( base, top, .055f );
                    streetMetal.beam( top, top + arm, .045f );
                    lampGlass.box( top + arm, { .5f, .1f, .5f } );
                    staticBox( assets, "City lamp post", base + Vector3F( 0, 3.1f, 0 ),
                               { .12f, 6.2f, .12f } );
                    ++lampCount;
                }
            }
            for( u32 node = 0; node < network.nodes.size(); ++node )
            {
                if( !degree[node] )
                {
                    continue;
                }
                const auto p = position( node ) + Vector3F( 0, .012f, 0 );
                const auto base = u32( asphalt.positions.size() );
                const auto vertex = [&]( Vector3F v ) {
                    asphalt.positions.push_back( v );
                    asphalt.normals.push_back( { 0, 1, 0 } );
                    asphalt.tangents.push_back( { 1, 0, 0, 1 } );
                    asphalt.uv.push_back( { v.x, v.z, 0 } );
                };
                vertex( p );
                constexpr u32 slices = 24;
                for( u32 i = 0; i <= slices; ++i )
                {
                    vertex( p + Vector3F( std::cos( i * 2 * pi / slices ) * junctionHalfWidth[node], 0,
                                          std::sin( i * 2 * pi / slices ) * junctionHalfWidth[node] ) );
                }
                for( u32 i = 0; i < slices; ++i )
                {
                    asphalt.indices.push_back( base );
                    asphalt.indices.push_back( base + i + 2 );
                    asphalt.indices.push_back( base + i + 1 );
                }
            }
        }

        float cityValueNoise( u32 seed, float x, float z )
        {
            const auto xi = static_cast<s32>( std::floor( x ) );
            const auto zi = static_cast<s32>( std::floor( z ) );
            auto u = x - xi, v = z - zi;
            u = u * u * ( 3 - 2 * u );
            v = v * v * ( 3 - 2 * v );
            const auto sample = [=]( s32 dx, s32 dz ) {
                const auto key = ( u64( u32( xi + dx ) ) << 32 ) | u32( zi + dz );
                return float( render::foliageStableHash( key ^ ( u64( seed ) * 0x9e3779b97f4a7c15ULL ) ) >> 40 ) /
                       16777216.f;
            };
            const float a = sample( 0, 0 ) * ( 1 - u ) + sample( 1, 0 ) * u;
            const float b = sample( 0, 1 ) * ( 1 - u ) + sample( 1, 1 ) * u;
            return a * ( 1 - v ) + b * v;
        }

#include "CityGroundAppearance.hpp"

        SourceBudgetVector<Vector3F> prepareTerrainCity( SceneAssets &assets, OpenCityLayout &city,
                                                         SmartPtr<IGameActor> sourceOwner,
                                                         SmartPtr<IGameActor> vehicle, u32 seed )
        {
            auto app = core::IApplicationManager::instance();
            auto manager = app->getGameManager();
            auto scene = manager->getCurrentScene();
            struct NewAnchorGuard
            {
                SceneAssets &assets;
                SmartPtr<IGameManager> manager;
                SmartPtr<IGameActor> roads, terrain;
                bool accepted = false;
                ~NewAnchorGuard()
                {
                    if( accepted ) return;
                    // Failed initial grading must be retryable. Existing artist
                    // anchors are never enrolled in this temporary ownership.
                    if( roads ) assets.roadAuthoring = nullptr;
                    if( terrain ) assets.roadTerrain = nullptr;
                    for( auto actor : { roads, terrain } ) if( actor )
                    {
                        try { manager->destroyActor( actor ); }
                        catch( const std::exception &failure ) { WP_LOG( failure.what() ); }
                    }
                }
            } newAnchors{ assets, manager };
            const auto persistentActor = [&]( const String &name, SmartPtr<IGameActor> &owned ) {
                if( !detail::runtimeActorPublished( sourceOwner ) || sourceOwner->getScene() != scene )
                {
                    throw std::runtime_error( "Attach the city source owner to the current scene before generating terrain roads." );
                }
                auto actor = manager->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create persistent city source actor." );
                }
                owned = actor;
                actor->setName( name );
                actor->setStatic( true );
                actor->setPosition( Vector3F::zero() );
                actor->setOrientation( QuaternionF::identity() );
                actor->setScale( Vector3F::unit() );
                attachWorldActor( scene, sourceOwner, actor );
                return actor;
            };
            String error;
            const bool newSource = !assets.roadAuthoring;
            CityRoadTerrainFitResult initialFit;
            render::TerrainSnapshot originalTerrain;
            if( newSource )
            {
                persistentActor( "Editable generated city roads", newAnchors.roads );
                assets.roadAuthoring = newAnchors.roads->addComponent<CityRoadAuthoring>();
            }
            if( !assets.roadAuthoring )
            {
                throw std::runtime_error( "Terrain city requires a canonical road source." );
            }
            if( !assets.roadTerrain )
            {
                auto actor = persistentActor( "Generated city terrain", newAnchors.terrain );
                assets.roadTerrain = actor->addComponent<TerrainSystem>();
                if( !assets.roadTerrain || !actor->addComponent<CollisionTerrain>() ||
                    !actor->addComponent<Rigidbody>() )
                {
                    throw std::runtime_error(
                        "Terrain city requires native terrain and collision components." );
                }
                if( !assets.roadTerrain->isLoaded() )
                {
                    assets.roadTerrain->load( nullptr );
                }
                assets.roadTerrain->setTreesEnabled( false );
                assets.roadTerrain->setGrassEnabled( false );
                const float radius = std::ceil( ( city.extent + 96 ) / 4 ) * 4;
                const u32 side = u32( radius * .5f ) + 1;
                render::TerrainData data;
                data.dimensions = { s32( side ), s32( side ) };
                data.spacing = { 4, 4 };
                data.origin = { -radius, -radius };
                data.heights.resize( size_t( side ) * side );
                const float terrainPhase = float( seed % 997 ) * ( 6.28318530718f / 997.f );
                const float hillScale = std::max( 1.f, assets.cityTerrainRelief );
                const auto cityRelief = [&]( float x, float z ) {
                    // Preserve broad hills across the centre. The fitting pass
                    // below grades only road corridors and local junction pads.
                    return assets.cityTerrainRelief *
                        ( 14.f * ( std::sin( x / ( 240.f * hillScale ) + terrainPhase ) *
                                       std::cos( z / ( 300.f * hillScale ) - terrainPhase ) -
                                   std::sin( terrainPhase ) * std::cos( terrainPhase ) ) +
                          5.f * ( std::sin( ( x + z ) / ( 180.f * hillScale ) + terrainPhase ) -
                                  std::sin( terrainPhase ) ) );
                };
                for( u32 z = 0; z < side; ++z )
                {
                    for( u32 x = 0; x < side; ++x )
                    {
                        const float px = -radius + x * 4, pz = -radius + z * 4;
                        const float outside = std::clamp(
                            ( std::max( std::abs( px ), std::abs( pz ) ) - city.extent - 20 ) / 76, 0.f,
                            1.f );
                        data.heights[size_t( z ) * side + x] =
                            2 + .005f * px + .003f * pz +
                            cityRelief( px, pz ) +
                            outside * outside * 3 * std::sin( px * .019f ) * std::cos( pz * .017f );
                    }
                }
                cityGround::seedLayers( data, app->getProjectPath(), seed );
                // Prepare both candidates before publishing the new terrain.
                // The planar planning surface establishes XZ topology only;
                // the fitter derives road profiles and local pads from the hills.
                procedural::OpenCityRoadSourceOptions initialOptions;
                initialOptions.sourceId = assets.roadAuthoring->getSource().id;
                initialOptions.terrainResourceUuid = "00000000-0000-4000-8000-000000000001";
                initialOptions.seed = seed;
                initialOptions.maximumRoundabouts = 4;
                procedural::OpenCityRoadSourceResult planned;
                if( !procedural::buildOpenCityRoadSource( city, initialOptions,
                        []( const Vector3F &, float &h ) { h = 2; return true; }, planned, error ) )
                    throw std::runtime_error( error.c_str() );
                cityGround::seedWeights( data, city.extent, seed, planned.source );
                CityRoadTerrainFitOptions fitOptions;
                originalTerrain = std::make_shared<render::TerrainData>( std::move( data ) );
                if( !prepareCityRoadTerrainFit( planned.source, originalTerrain,
                        Transform3<real_Num>(), fitOptions, initialFit, error ) )
                    throw std::runtime_error( error.c_str() );
                assets.cityTerrainFitted = true;
                assets.cityTerrainFitReport = initialFit.report;
                auto renderer =
                    dynamic_pointer_cast<render::Terrain>( assets.roadTerrain->getTerrain() );
                const bool deferred = renderer && renderer->isTerrainPublicationDeferred();
                if( deferred )
                {
                    renderer->setDeferredTerrainPublication( false );
                }
                const bool applied = assets.roadTerrain->applyTerrainData( *initialFit.terrain, error );
                if( deferred )
                {
                    renderer->setDeferredTerrainPublication( true );
                }
                if( !applied )
                {
                    throw std::runtime_error( error.c_str() );
                }
            }
            if( assets.roadAuthoring->getTerrain() != assets.roadTerrain &&
                !assets.roadAuthoring->bindTerrain( assets.roadTerrain, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            const auto &currentSource = assets.roadAuthoring->getSource();
            const bool seedSource =
                newSource || ( currentSource.roads.empty() && currentSource.roundabouts.empty() &&
                               currentSource.revision <= 2 );
            assets.citySourceReused = !seedSource;
            if( !seedSource && !assets.roadAuthoring->rebuild( error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
            const auto placement = assets.roadAuthoring->getTerrainPlacement();
            if( !terrain )
            {
                throw std::runtime_error( "Terrain city has no retained terrain snapshot." );
            }
            const auto height = [&]( const Vector3F &point, float &y ) {
                return render::sampleTerrainHeight( *terrain, placement, point, y );
            };
            SourceBudgetVector<Vector3F> route;
            if( seedSource )
            {
                procedural::OpenCityRoadSourceOptions options;
                options.seed = seed;
                options.sourceId = assets.roadAuthoring->getSource().id;
                const auto binding = assets.roadAuthoring->getTerrainBindingUUID();
                options.terrainResourceUuid.assign( binding.data(), binding.size() );
                options.terrainRevision = terrain->revision;
                options.maximumRoundabouts = 4;
                procedural::OpenCityRoadSourceResult generated;
                if( initialFit.terrain )
                {
                    generated.source = std::move( initialFit.source );
                    generated.source.terrainResourceUuid = options.terrainResourceUuid;
                    generated.source.terrainRevision = terrain->revision;
                    if( !procedural::fitOpenCityRoadRoute( city, generated.source, height,
                                                         generated.route, error ) )
                        throw std::runtime_error( error.c_str() );
                }
                else if( !procedural::buildOpenCityRoadSource( city, options, height, generated, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                SourceBudgetText json;
                if( !procedural::serializeCityRoadSplineSource( generated.source, json, error ) ||
                    !assets.roadAuthoring->setSourceJSON( String( json.data(), json.size() ),
                                                          assets.roadAuthoring->getSourceRevision(),
                                                          error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                if( originalTerrain && !assets.roadAuthoring->rememberInitialTerrainGrading( originalTerrain, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                route = std::move( generated.route );
            }
            else
            {
                assets.cityRaceRouteAvailable = procedural::fitOpenCityRoadRoute(
                    city, assets.roadAuthoring->getSource(), height, route, error );
                if( !assets.cityRaceRouteAvailable && city.naturalRoads )
                {
                    // Saved sources precede the new district layout and do not
                    // store generator provenance or their pinned race path.
                    // Keep their edits and validate the exact previous path;
                    // never replace their roads or bypass surface coverage.
                    OpenCityLayout legacy;
                    legacy.route = procedural::legacyOpenCityNaturalRoute(
                        seed, city.blocks, assets.cityRoute );
                    String legacyError;
                    assets.cityRaceRouteAvailable = procedural::fitOpenCityRoadRoute(
                        legacy, assets.roadAuthoring->getSource(), height, route, legacyError );
                    if( assets.cityRaceRouteAvailable )
                    {
                        error.clear();
                        assets.cityRaceRouteError.clear();
                        WP_LOG( "Terrain city: retained the validated revision-2 natural race route." );
                    }
                    else
                    {
                        error += String( " Legacy route: " ) + legacyError;
                    }
                }
                if( !assets.cityRaceRouteAvailable )
                {
                    assets.cityRaceRouteError = error;
                }
            }
            if( route.size() > 1 && ( route.front() - route.back() ).lengthSquared() < 1e-8f )
            {
                route.pop_back();
            }
            OpenCityLayout supported;
            CityRoadParcelSupportOptions parcelOptions;
            parcelOptions.shrinkForRelief = true;
            if( !prepareCityRoadParcels( city, assets.roadAuthoring->getSource(),
                                         assets.roadAuthoring->getBake(), terrain, placement, supported,
                                         assets.cityCulledParcels, assets.cityBuildingSupportChecks,
                                         error, parcelOptions ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            city = std::move( supported );
            city.route.clear();
            for( const auto &point : route )
            {
                city.route.push_back( { point.x, point.z } );
            }
            assets.cityLayout = city;
            assets.cityAppliedSourceRevision = assets.roadAuthoring->getSourceRevision();
            assets.ground = assets.roadTerrain->getActor();
            assets.triangles += assets.roadAuthoring->getRuntimeTriangleCount() +
                                u64( terrain->dimensions.x - 1 ) * ( terrain->dimensions.y - 1 ) * 2;
            if( !vehicle )
            {
                newAnchors.accepted = true;
                return route;
            }
            Vector3F spawn = route.empty() ? Vector3F( 0, 0, 0 ) : route.front();
            bool driveable = false;
            float y = 0;
            if( !assets.roadAuthoring->getRuntimeSurface( spawn.x, spawn.z, y, driveable ) ||
                !driveable )
            {
                if( !height( spawn, y ) )
                {
                    throw std::runtime_error( "Edited city has no supported vehicle spawn." );
                }
            }
            spawn.y = y + .42f;
            vehicle->setPosition( spawn );
            vehicle->updateTransform();
            if( auto rigidbody = vehicle->getComponent<Rigidbody>() )
            {
                if( auto body = rigidbody->getRigidDynamic() )
                {
                    body->setTransform( Transform3F( spawn, vehicle->getOrientation() ) );
                }
            }
            newAnchors.accepted = true;
            return route;
        }

#include "CityCarriagewayUtilities.hpp"
#include "CityRoadEdgeSurface.hpp"
#include "CityBuildingGroundContact.hpp"
#include "CityEntranceWayfinding.hpp"
#include "CityRoundaboutSigns.hpp"
#include "CityFootwayAppearance.hpp"
#include "CityCrossingLandings.hpp"
#include "CityCrossingBollards.hpp"
#include "CityParkLandscape.hpp"
#include "CityReserveLandscape.hpp"
#include "CityParkAmenities.hpp"
#include "CityStormDrains.hpp"
#include "CityHinterland.hpp"
#include "CityBuildingGlazing.hpp"
#include "CityShopfront.hpp"
#include "CityShopInterior.hpp"
#include "CityFacadeDetail.hpp"

        void buildCity( SceneAssets &assets, SmartPtr<IGameActor> sourceOwner,
                        SmartPtr<IGameActor> vehicle, u32 seed, VehicleAppearanceQuality quality )
        {
            auto city = OpenCityLayout::generate( seed, assets.cityBlocks, assets.cityRoute,
                                                  assets.cityNaturalRoads );
            SourceBudgetVector<Vector3F> fittedRoute;
            if( assets.cityTerrainRoads )
            {
                fittedRoute = prepareTerrainCity( assets, city, sourceOwner, vehicle, seed );
            }
            const auto buildingCount = std::count_if( city.lots.begin(), city.lots.end(),
                                                      []( const auto &lot ) { return !lot.park; } );
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
            // Shared tiled facade, paving and soil maps retain their world scale;
            // only the largest cities use smaller source images. Vehicle and
            // carriageway bakes keep their independent quality settings.
            surfaceParams.size = quality == VehicleAppearanceQuality::Preview || buildingCount > 256 ? 256 : 512;
            SmartPtr<render::IMaterial> cityAsphalt;
            if( !assets.cityTerrainRoads )
            {
                auto asphaltParams = surfaceParams;
                asphaltParams.size = quality == VehicleAppearanceQuality::Preview ? 256 : 512;
                cityAsphalt = raceAsphalt( assets, forge, asphaltParams, seed );
            }
            // World projection gives segments and intersections the same physical texture scale.
            if( cityAsphalt )
            {
                cityAsphalt->setUVProjection( 3 );
                const auto albedo = cityAsphalt->getTexture( 0 );
                if( !albedo || albedo->getName().find( "/city-surface/" ) == String::npos )
                    cityAsphalt->setEditorFloat( "uvTriplanarScale", 1.f / 8.f );
            }
            Geometry asphalt, pavement, markings, parks, courtyards, streetMetal, lampGlass;
            SourceBudgetVector<Vector3F> terrainStreetLamps;
            SourceBudgetVector<cityGroundContact::Entry> buildingEntries;
            SourceBudgetVector<cityEntranceWayfinding::Mount> entranceSigns;
            constexpr u32 paneAtlasColumns = cityBuildingGlazing::atlasColumns;
            constexpr float panePadding = cityBuildingGlazing::padding;

            struct CityPatch
            {
                Geometry walls[3], farWalls[3], glass[3], farGlass, roofs, farRoofs, frames, accents;
                Geometry windowPanes, timber, tiles, farTiles, officeJoinery;
                Geometry shopfrontFascia[static_cast<size_t>( cityShopfront::Style::Count )], shopfrontLetters, shopfrontDisplays;
                cityShopInterior::Surfaces shopInterior;
                cityFacadeDetail::GeometryBatches masonry;
                Array<LODDetailBound> bounds{ Allocator<LODDetailBound>( sourceBudget ) };
            };

            SourceBudgetMap<std::pair<int, int>, CityPatch> patches;
            size_t emittedShopfronts = 0;
            size_t emittedDisplayBays = 0;
            bool emittedMasonry = false;
            const size_t displayBayLimit = buildingCount > 256 ? 96 : 144;
            size_t nominalMansardSilhouettes = 0, emittedMansardSilhouettes = 0;
            CityBuildingKit kit( assets, buildingCount );
            const bool detailedBuildings = quality != VehicleAppearanceQuality::Preview;
            const bool decorateShopfronts = detailedBuildings && !kit.hasModule( CityBuildingPart::Window ) &&
                                            !kit.hasModule( CityBuildingPart::Entrance );
            const bool dressOfficeMasonry = detailedBuildings && !kit.hasModule( CityBuildingPart::Wall ) &&
                !kit.hasModule( CityBuildingPart::Window ) && !kit.hasModule( CityBuildingPart::Corner );
            const int buildingStyle = std::clamp( assets.cityBuildingStyle, 0, cityBuildingStyleMax );
            const auto streetDetails = OpenCityStreetDetails::generate(
                city, seed, quality != VehicleAppearanceQuality::Preview );
            // Match the existing flat physics proxy. Preserve library topology,
            // kerbs and markings without introducing unsupported road-height bumps.
            auto append = []( Geometry &g, const RoadMesh &source, bool flat = false,
                              float height = .012f ) {
                const auto base = static_cast<u32>( g.positions.size() );
                for( const auto &v : source.vertices )
                {
                    g.positions.push_back( { float( v.position.x ),
                                             flat ? height : float( v.position.y ),
                                             float( v.position.z ) } );
                    g.normals.push_back( flat ? Vector3F( 0, 1, 0 )
                                              : Vector3F( float( v.normal.x ), float( v.normal.y ),
                                                          float( v.normal.z ) ) );
                    g.uv.push_back( { float( v.uv.x ), float( v.uv.y ), 0 } );
                    g.tangents.push_back( { 1, 0, 0, 1 } );
                }
                if( source.indices.size() % 3 )
                {
                    throw std::runtime_error( "Incomplete city road triangles." );
                }
                for( size_t triangle = 0; triangle < source.indices.size(); triangle += 3 )
                {
                    if( source.indices[triangle] >= source.vertices.size() ||
                        source.indices[triangle + 1] >= source.vertices.size() ||
                        source.indices[triangle + 2] >= source.vertices.size() )
                    {
                        throw std::runtime_error( "Invalid city road triangle index." );
                    }
                    auto a = base + source.indices[triangle], b = base + source.indices[triangle + 1],
                         c = base + source.indices[triangle + 2];
                    if( a >= g.positions.size() || b >= g.positions.size() || c >= g.positions.size() )
                    {
                        throw std::runtime_error( "Invalid city road triangle index." );
                    }
                    // Marking meshes can carry the opposite winding to their authored
                    // up normal. Flattened city surfaces always face out of the ground.
                    if( flat && ( g.positions[b] - g.positions[a] )
                                        .crossProduct( g.positions[c] - g.positions[a] )
                                        .y < 0 )
                    {
                        std::swap( b, c );
                    }
                    g.indices.push_back( a );
                    g.indices.push_back( b );
                    g.indices.push_back( c );
                }
            };
            const int half = city.blocks / 2;
            u32 segmentSeed = seed;
            if( assets.cityTerrainRoads )
            {
                for( const auto &lot : city.lots )
                {
                    if( lot.park ) continue;  // Parks use the retained ground and its collider.
                    const float top = lot.groundHeight + .15f, bottom = lot.groundMinimumHeight - .25f;
                    const Vector3F centre( lot.parcelX, ( top + bottom ) * .5f, lot.parcelZ );
                    const Vector3F size( lot.parcelWidth, top - bottom, lot.parcelDepth );
                    // A zero-setback facade already closes the upper overlap on all four
                    // sides. The pad's visual top meets its base while its collider stays intact.
                    const bool covered = !lot.park && std::abs( lot.x - lot.parcelX ) < .001f &&
                                         std::abs( lot.z - lot.parcelZ ) < .001f &&
                                         std::abs( lot.width - lot.parcelWidth ) < .001f &&
                                         std::abs( lot.depth - lot.parcelDepth ) < .001f;
                    courtyards.box( centre - Vector3F( 0, covered ? .075f : 0, 0 ),
                                    size - Vector3F( 0, covered ? .15f : 0, 0 ) );
                    staticBox( assets, "City terrain foundation", centre, size );
                }
                SourceBudgetVector<Vector3F> lampCandidates;
                const auto &source = assets.roadAuthoring->getSource();
                const auto &bake = assets.roadAuthoring->getBake();
                for( size_t road = 0; road < bake.roads.size(); ++road )
                {
                    const auto &points = bake.roads[road].points;
                    float next = 24;
                    for( size_t i = 1; i < points.size(); ++i )
                    {
                        const auto &a = points[i - 1], &b = points[i];
                        while( next <= b.distance )
                        {
                            const float t = std::clamp(
                                ( next - a.distance ) / ( b.distance - a.distance ), 0.f, 1.f );
                            next += 60;
                            const auto centre = a.position + ( b.position - a.position ) * t;
                            auto forward = b.position - a.position;
                            forward.y = 0;
                            forward.normalise();
                            const Vector3F across( forward.z, 0, -forward.x );
                            for( int side : { -1, 1 } )
                            {
                                auto point =
                                    centre +
                                    across * ( side * ( source.roads[road].width * .5f + 1.3f ) );
                                bool clear = true;
                                for( const auto &junction : source.roundabouts )
                                {
                                    if( std::hypot( point.x - junction.center.x,
                                                    point.z - junction.center.z ) <
                                        junction.outerRadius + 2 )
                                    {
                                        clear = false;
                                    }
                                }
                                for( size_t other = 0; clear && other < bake.roads.size(); ++other )
                                {
                                    const auto &samples = bake.roads[other].points;
                                    for( size_t j = 1; j < samples.size(); ++j )
                                    {
                                        const auto u = samples[j - 1].position, v = samples[j].position;
                                        const float dx = v.x - u.x, dz = v.z - u.z;
                                        const float s = std::clamp(
                                            ( ( point.x - u.x ) * dx + ( point.z - u.z ) * dz ) /
                                                ( dx * dx + dz * dz ),
                                            0.f, 1.f );
                                        const float x = point.x - u.x - s * dx,
                                                    z = point.z - u.z - s * dz,
                                                    r = source.roads[other].width * .5f + .5f;
                                        if( x * x + z * z < r * r )
                                        {
                                            clear = false;
                                            break;
                                        }
                                    }
                                }
                                float height = 0;
                                if( clear && lampCandidates.size() < 2048 &&
                                    render::sampleTerrainHeight(
                                        *assets.roadAuthoring->getTerrainSnapshot(),
                                        assets.roadAuthoring->getTerrainPlacement(), point, height ) )
                                {
                                    point.y = height;
                                    lampCandidates.push_back( point );
                                }
                            }
                        }
                    }
                }
                std::sort( lampCandidates.begin(), lampCandidates.end(),
                           []( const auto &a, const auto &b ) {
                               return a.lengthSquared() < b.lengthSquared();
                           } );
                for( size_t i = 0;
                     i < std::min<size_t>( lampCandidates.size(),
                                           quality == VehicleAppearanceQuality::Preview ? 24 : 64 );
                     ++i )
                {
                    const auto ground = lampCandidates[i], base = ground + Vector3F( 0, .15f, 0 ),
                               top = base + Vector3F( 0, 6.2f, 0 );
                    if( assets.cityStreetDetail > 0 )
                    {
                        terrainStreetLamps.push_back( ground );
                    }
                    courtyards.box( ground + Vector3F( 0, .075f, 0 ), { .4f, .15f, .4f } );
                    streetMetal.beam( base, top, .055f );
                    const Vector3F arm( .8f, -.1f, .45f );
                    streetMetal.beam( top, top + arm, .04f );
                    lampGlass.box( top + arm, { .5f, .1f, .5f } );
                    staticBox( assets, "City lamp foundation", ground + Vector3F( 0, .075f, 0 ),
                               { .4f, .15f, .4f } );
                    staticBox( assets, "City lamp post", base + Vector3F( 0, 3.1f, 0 ),
                               { .12f, 6.2f, .12f } );
                }
            }
            else if( city.naturalRoads )
            {
                buildNaturalCityStreets( assets, city, asphalt, pavement, markings, streetMetal,
                                         lampGlass, quality );
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
                for( const auto &walk : streetDetails.sidewalks )
                {
                    addWalk( walk );
                }
                for( const auto &walk : streetDetails.corners )
                {
                    addWalk( walk );
                }
                // The inner block slabs meet the sidewalks at the same authored grade.
                for( int x = -half; x < half; ++x )
                {
                    for( int z = -half; z < half; ++z )
                    {
                        const Vector3F centre( ( x + .5f ) * city.spacing, .075f,
                                               ( z + .5f ) * city.spacing );
                        const Vector3F size( city.spacing - 17, .15f, city.spacing - 17 );
                        courtyards.box( centre, size );
                    }
                }
                // The adjoining plaza/walk pieces form one solid interior square per
                // block. Use that exact union to avoid thousands of scene-root actors.
                for( const auto &surface : streetDetails.collisionSurfaces( city ) )
                {
                    staticBox( assets, "City sidewalk",
                               { surface.x, ( surface.bottom + surface.top ) * .5f, surface.z },
                               { surface.width, surface.top - surface.bottom, surface.depth } );
                }
                for( const auto &item : streetDetails.furniture )
                {
                    const Vector3F p( item.x, item.baseY, item.z );
                    if( item.kind == OpenCityStreetDetails::FurnitureKind::Lamp )
                    {
                        const Vector3F top = p + Vector3F( 0, 6.2f, 0 );
                        const Vector3F arm( std::sin( item.yaw ) * 1.25f, -.15f,
                                            std::cos( item.yaw ) * 1.25f );
                        streetMetal.beam( p, top, .055f );
                        streetMetal.beam( top, top + arm, .045f );
                        lampGlass.box( top + arm, { .5f, .10f, .5f } );
                        staticBox( assets, "City lamp post", p + Vector3F( 0, 3.1f, 0 ),
                                   { .12f, 6.2f, .12f } );
                    }
                    else
                    {
                        streetMetal.box( p + Vector3F( 0, .45f, 0 ), { .13f, .9f, .13f } );
                    }
                }
            }
            for( const auto &lot : city.lots )
            {
                if( lot.park )
                {
                    if( !assets.cityTerrainRoads )
                        parks.box( { lot.parcelX, lot.groundHeight + .155f, lot.parcelZ },
                                   { lot.parcelWidth - 1, .01f, lot.parcelDepth - 1 } );
                    continue;
                }
                kit.beginBuilding();
                const auto key =
                    std::make_pair( int( std::floor( lot.x / 128 ) ), int( std::floor( lot.z / 128 ) ) );
                const Vector3F origin( key.first * 128.f + 64, 0, key.second * 128.f + 64 );
                auto &patch = patches[key];
                patch.bounds.push_back(
                    { Vector3F( lot.x, lot.groundHeight + ( lot.height + 6.f ) * .5f, lot.z ) - origin,
                      std::max( { lot.width + 2.5f, lot.depth + 2.5f, lot.height + 6.f } ) } );
                const auto volumes = OpenCityLayout::buildingVolumes( lot );
                // One stable kit per parcel, shared by every occupied tier. District mix
                // retains neighbourhood character while varying both roofline and frontage.
                const int residentialStyles[] = { 1, 4, 4, 5, 7, 9 };
                const int commercialStyles[] = { 3, 3, 6, 6, 8 };
                const int civicStyles[] = { 2, 2, 5, 8 };
                const int style = buildingStyle ? buildingStyle
                    : lot.district == OpenCityLayout::District::Commercial
                        ? commercialStyles[lot.variant % 5]
                    : lot.district == OpenCityLayout::District::Civic
                        ? civicStyles[lot.variant % 4]
                        : residentialStyles[lot.variant % 6];
                const bool office = style == 3, civic = style == 2;
                const bool terrace = style == 4, parisian = style == 5, warehouse = style == 6;
                const bool mediterranean = style == 7, artdeco = style == 8, timber = style == 9;
                // Fine ornaments share a fixed city-wide allowance, divided fairly
                // before emission. Roof forms, occupied shells and glazing never
                // depend on this optional-detail budget. Dense ten-block cities
                // therefore cannot multiply railings or sash bars without bound.
                size_t facadeDetailVertices = 0, roofDetailVertices = 0;
                size_t masonryVerticesLeft = 250000u / std::max<size_t>( 1, buildingCount );
                cityFacadeDetail::Counts masonry;
                const size_t facadeDetailLimit = ( buildingCount > 256 ? 110000u : 220000u ) /
                                                 std::max<size_t>( 1, buildingCount );
                const size_t roofDetailLimit = ( buildingCount > 256 ? 50000u : 100000u ) /
                                               std::max<size_t>( 1, buildingCount );
                const auto admitFacadeDetail = [&]( size_t vertices ) {
                    if( vertices > facadeDetailLimit - facadeDetailVertices ) return false;
                    facadeDetailVertices += vertices;
                    return true;
                };
                const auto admitRoofDetail = [&]( size_t vertices ) {
                    if( vertices > roofDetailLimit - roofDetailVertices ) return false;
                    roofDetailVertices += vertices;
                    return true;
                };
                const int entranceAxis = lot.frontage == OpenCityLayout::Frontage::West ||
                                                 lot.frontage == OpenCityLayout::Frontage::East
                                             ? 1
                                             : 0;
                const int entranceSign = lot.frontage == OpenCityLayout::Frontage::West ||
                                                 lot.frontage == OpenCityLayout::Frontage::North
                                             ? -1
                                             : 1;
                for( size_t tier = 0; tier < volumes.size(); ++tier )
                {
                    const auto &volume = volumes[tier];
                    const Vector3F centre( volume.x, volume.bottom + volume.height * .5f, volume.z );
                    const Vector3F size( volume.width, volume.height, volume.depth );
                    const bool groundFloor = volume.bottom == lot.groundHeight;
                    const u32 family = civic || parisian || mediterranean || artdeco || timber ? 1u
                                       : office ? ( buildingStyle == 0 && groundFloor && lot.variant % 3 == 0 ? 1u : 2u )
                                                : ( buildingStyle == 0 && lot.variant % 5 == 0 ? 1u : 0u );
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
                    patch.roofs.box( { cx, top + .06f, cz },
                                     { volume.width + .12f, .12f, volume.depth + .12f } );
                    patch.farRoofs.box( { cx, top + .06f, cz },
                                        { volume.width + .12f, .12f, volume.depth + .12f } );
                    const float nominalFloor = warehouse ? 4.1f : office ? 3.6f : civic || parisian ? 3.5f : 3.2f;
                    const int floors = std::max( 1, int( std::round( volume.height / nominalFloor ) ) );
                    const float floorHeight = volume.height / floors;
                    for( int axis : { entranceAxis, 1 - entranceAxis } )
                    {
                        for( int sign : { entranceSign, -entranceSign } )
                        {
                            const float span = axis ? volume.depth : volume.width;
                            const bool streetFace = axis == entranceAxis && sign == entranceSign;
                            const bool entrance = groundFloor && streetFace;
                            const Vector3F normal( axis ? float( sign ) : 0, 0,
                                                   axis ? 0 : float( sign ) );
                            const Vector3F along( axis ? 0 : 1, 0, axis ? 1 : 0 );
                            const float yaw = axis ? float( sign ) * pi * .5f : sign < 0 ? pi : 0;
                            const auto point = [&]( float s, float y, float inset ) {
                                return axis
                                           ? Vector3F( cx + sign * ( size.x * .5f - inset ), y, cz + s )
                                           : Vector3F( cx + s, y, cz + sign * ( size.z * .5f - inset ) );
                            };
                            const auto panel = [&]( Geometry &g, float left, float low, float right,
                                                    float high, float inset = 0.f ) {
                                if( right - left <= .001f || high - low <= .001f )
                                {
                                    return;
                                }
                                g.quad( point( left, low, inset ), point( right, low, inset ),
                                        point( right, high, inset ), point( left, high, inset ),
                                        normal );
                            };
                            const auto module = [&]( CityBuildingPart role, Geometry &geometry,
                                                     float left, float low, float right, float high,
                                                     float inset, float thickness ) {
                                return detailedBuildings && right > left && high > low &&
                                       kit.append( role, geometry,
                                                   point( ( left + right ) * .5f,
                                                          ( low + high ) * .5f, inset ),
                                                   { right - left, high - low, thickness }, yaw );
                            };
                            const auto wallPanel = [&]( float left, float low, float right, float high ) {
                                if( right - left >= 1.25f && high - low >= .60f &&
                                    module( CityBuildingPart::Wall, opaque, left, low, right, high,
                                            .06f, .12f ) )
                                {
                                    return;
                                }
                                panel( opaque, left, low, right, high );
                            };
                            const auto reveal = [&]( float left, float low, float right, float high,
                                                     float depth ) {
                                opaque.quad( point( left, low, 0 ), point( left, low, depth ),
                                             point( left, high, depth ), point( left, high, 0 ), along );
                                opaque.quad( point( right, low, 0 ), point( right, high, 0 ),
                                             point( right, high, depth ), point( right, low, depth ),
                                             -along );
                                opaque.quad( point( left, low, 0 ), point( right, low, 0 ),
                                             point( right, low, depth ), point( left, low, depth ),
                                             { 0, 1, 0 } );
                                opaque.quad( point( left, high, 0 ), point( left, high, depth ),
                                             point( right, high, depth ), point( right, high, 0 ),
                                             { 0, -1, 0 } );
                            };
                            const auto faceBox = [&]( Geometry &g, float s, float y, float width,
                                                      float height, float projection, float thickness ) {
                                g.box( point( s, y, -projection ),
                                       axis ? Vector3F( thickness, height, width )
                                            : Vector3F( width, height, thickness ) );
                            };
                            const float desiredBay = terrace || parisian || timber ? 4.2f
                                                     : warehouse ? 4.2f
                                                     : artdeco ? 4.2f
                                                     : office ? 3.8f
                                                     : civic ? 4.6f
                                                             : 4.4f + ( lot.variant % 3 ) * .45f;
                            const int nominalBays = std::max( 1, int( std::floor( span / desiredBay ) ) );
                            const bool denseFacade = !office && buildingCount > 128;
                            const float density = denseFacade ? 128.f / float( buildingCount ) : 1.f;
                            // Preserve the six-block frontage exactly. Larger cities use
                            // fewer, normally sized apertures, with priority given to the
                            // street elevation. Storey heights and roof forms stay intact.
                            const int bays = denseFacade
                                ? std::max( 1, int( std::floor( nominalBays * ( streetFace ? std::sqrt( density ) : density ) ) ) )
                                : nominalBays;
                            const float bayWidth = span / bays;
                            const float windowBayWidth = denseFacade ? span / nominalBays : bayWidth;
                            if( assets.cityTerrainRoads && entrance )
                            {
                                const float doorOffset = office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                const float doorHalfWidth = office ? std::min( .92f, ( span * .5f - .38f ) * .8f ) :
                                                            std::min( 2.05f, bayWidth * .72f ) * .5f;
                                buildingEntries.push_back( { &lot, point( doorOffset, volume.bottom, 0 ) + origin,
                                                              normal, along, span, doorOffset, doorHalfWidth } );
                            }
                            for( int floor = 0; floor < floors; ++floor )
                            {
                                const float y0 = volume.bottom + floor * floorHeight;
                                const float y1 = floor + 1 == floors ? top : y0 + floorHeight;
                                const bool retail = entrance && floor == 0 &&
                                                    ( office || civic || warehouse || parisian || artdeco ||
                                                      ( !terrace && !timber && !mediterranean && lot.variant % 3 != 0 ) );
                                const float low = y0 + ( retail ? .28f : office ? .62f : .78f );
                                const float high = y1 - ( retail ? .42f : office ? .48f : .62f );
                                auto &glass =
                                    patch.glass[( lot.variant / 7 + axis + ( sign > 0 ? 1 : 0 ) ) % 2];
                                const auto windowPane = [&]( Geometry &plain, float left, float bottom,
                                                            float right, float head, float depth, int bay,
                                                            bool usePlain ) {
                                    if( usePlain || !detailedBuildings )
                                    {
                                        panel( plain, left, bottom, right, head, depth );
                                        return;
                                    }
                                    if( right - left <= .001f || head - bottom <= .001f ) return;
                                    const u64 building = ( u64( lot.variant ) << 32 ) | seed;
                                    const u64 pane = ( u64( tier ) << 40 ) | ( u64( floor ) << 24 ) |
                                                     ( u64( bay ) << 3 ) | u64( axis * 2 + ( sign > 0 ) );
                                    const u32 variant = u32( render::foliageStableHash( building ^ pane ) );
                                    const u32 roll = variant % 20;
                                    const u32 face = u32( render::foliageStableHash( building ^ ( pane & 7u ) ) );
                                    const bool officeBlind = bays == 2 ? ( u32( bay + 2 * floor ) + face ) % 4 == 0
                                                                      : roll < 4;
                                    u32 tile = variant % 4;
                                    if( !retail && office && officeBlind ) tile = 6 + ( variant >> 9 ) % 2;
                                    if( !retail && !office && roll < 6 ) tile = 4 + ( variant >> 9 ) % 4;
                                    const float u0 = ( tile + panePadding ) / paneAtlasColumns;
                                    const float u1 = ( tile + 1.f - panePadding ) / paneAtlasColumns;
                                    patch.windowPanes.quad( point( left, bottom, depth ), point( right, bottom, depth ),
                                                            point( right, head, depth ), point( left, head, depth ),
                                                            normal, u0, u1, panePadding, 1.f - panePadding );
                                };
                                bool entranceModule = false;
                                if( high - low < .45f )
                                {
                                    wallPanel( -span * .5f, y0, span * .5f, y1 );
                                    continue;
                                }
                                const float shopDoor = office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                const bool builtInShopfront = decorateShopfronts && retail && ( office || warehouse || artdeco ) &&
                                    y1 - y0 >= 3.05f && cityShopfront::fasciaWidth( span, shopDoor ) >= 2.3f &&
                                    admitFacadeDetail( cityShopfront::faceBudget );
                                const auto shopStyle = builtInShopfront ? cityShopfront::choose( lot.id, seed, warehouse, artdeco )
                                                                       : cityShopfront::Style::Market;
                                u32 displayCount = 0, interiorCount = 0;
                                const auto interior = [&]( float left, float right, float bottom, float head, float depth, int bay ) {
                                    if( !builtInShopfront || interiorCount + displayCount >= 2 || right - left < 1.15f ||
                                        head - bottom < 1.5f || emittedDisplayBays >= displayBayLimit ) return false;
                                    const u32 variation = u32( render::foliageStableHash( lot.id ^ ( u64( bay + 1 ) << 32 ) ^ interiorCount ) );
                                    cityShopInterior::append( patch.shopInterior, shopStyle, normal, along, point,
                                                              left, bottom, right, head, depth, variation );
                                    ++interiorCount;
                                    ++emittedDisplayBays;
                                    return true;
                                };
                                const auto display = [&]( float left, float right, float bottom, float head, float depth, int bay ) {
                                    if( !builtInShopfront || interiorCount + displayCount >= 2 || right - left < 1.05f || head - bottom < 1.38f ||
                                        !admitFacadeDetail( cityShopfront::displayBudget ) ) return;
                                    const u32 variation = u32( render::foliageStableHash( lot.id ^ ( u64( bay + 1 ) << 32 ) ^ displayCount ) );
                                    cityShopfront::appendDisplay( patch.shopfrontDisplays, patch.officeJoinery, shopStyle,
                                                                  faceBox, left, right, bottom, head, depth, variation );
                                    ++displayCount;
                                };
                                if( office )
                                {
                                    if( dressOfficeMasonry && ( floor > 0 || !groundFloor ) )
                                        cityFacadeDetail::officeStorey( patch.masonry, masonry, point, normal,
                                            along, span, bays, y0, low, high, y1, masonryVerticesLeft );
                                    // Curtain-wall kit: recessed ribbons, substantial spandrels
                                    // and fine mullions rather than hundreds of boxed windows.
                                    const float left = -span * .5f + .38f, right = span * .5f - .38f;
                                    const float doorHalfWidth = std::min( .92f, right * .8f );
                                    const float doorHead = std::min( high, y0 + 2.7f );
                                    const float paneDepth = builtInShopfront ? cityShopfront::glassDepth : .085f;
                                    if( entrance && floor == 0 )
                                    {
                                        entranceModule = module( CityBuildingPart::Entrance,
                                                                 patch.accents, -doorHalfWidth, y0 + .16f,
                                                                 doorHalfWidth, doorHead,
                                                                 .022f, .14f );
                                    }
                                    if( entranceModule )
                                    {
                                        wallPanel( -span * .5f, y0, -doorHalfWidth, low );
                                        panel( opaque, -doorHalfWidth, y0, doorHalfWidth, y0 + .16f );
                                        wallPanel( doorHalfWidth, y0, span * .5f, low );
                                        reveal( left, low, -doorHalfWidth, high, .085f );
                                        reveal( doorHalfWidth, low, right, high, .085f );
                                        reveal( -doorHalfWidth, y0 + .16f, doorHalfWidth, doorHead,
                                                .085f );
                                    }
                                    else
                                    {
                                        wallPanel( -span * .5f, y0, span * .5f, low );
                                        reveal( left, low, right, high, paneDepth );
                                    }
                                    wallPanel( -span * .5f, high, span * .5f, y1 );
                                    panel( opaque, -span * .5f, low, left, high );
                                    panel( opaque, right, low, span * .5f, high );
                                    const auto pane = [&]( float paneLeft, float paneRight, int surface, int bay ) {
                                        if( paneRight - paneLeft <= .001f ) return false;
                                        const bool custom = module( CityBuildingPart::Window, patch.frames,
                                                                    paneLeft, low, paneRight, high, .050f, .07f );
                                        if( builtInShopfront && !custom )
                                        {
                                            const float leftEdge = std::clamp( -doorHalfWidth - .16f, paneLeft, paneRight );
                                            const float rightEdge = std::clamp( doorHalfWidth + .16f, paneLeft, paneRight );
                                            const auto displayPane = [&]( float a, float b ) {
                                                if( interior( a, b, low, high, paneDepth, bay ) ) return;
                                                windowPane( patch.glass[surface], a, low, b, high, paneDepth, bay, false );
                                                display( a, b, low, high, paneDepth, bay );
                                            };
                                            displayPane( paneLeft, leftEdge );
                                            windowPane( patch.glass[surface], leftEdge, low, rightEdge, high, paneDepth, bay, false );
                                            displayPane( rightEdge, paneRight );
                                        }
                                        else
                                            windowPane( patch.glass[surface], paneLeft, low, paneRight, high,
                                                        custom ? .097f : paneDepth, bay, custom );
                                        return custom;
                                    };
                                    // Keep the continuous ribbon aperture and individual pane treatments.
                                    bool previousModule = false, anyWindowModule = false;
                                    for( int bay = 0; bay < bays; ++bay )
                                    {
                                        const float paneLeft =
                                            std::max( left, -span * .5f + bay * bayWidth );
                                        const float paneRight =
                                            std::min( right, -span * .5f + ( bay + 1 ) * bayWidth );
                                        u32 variation =
                                            lot.variant ^ ( u32( floor + 1 ) * 0x9e3779b9u ) ^
                                            ( u32( bay + 1 ) * 0x85ebca6bu ) ^ u32( axis * 31 + sign );
                                        variation ^= variation >> 16;
                                        variation *= 0x7feb352du;
                                        variation ^= variation >> 15;
                                        const int surface = variation % 19 == 0 ? 2
                                                            : variation % 7 < 2 ? 1
                                                                                : 0;
                                        bool windowModule = false;
                                        if( entranceModule && paneRight > -doorHalfWidth && paneLeft < doorHalfWidth )
                                        {
                                            windowModule = pane( paneLeft, std::min( paneRight, -doorHalfWidth ), surface, bay );
                                            windowModule |= pane( std::max( paneLeft, doorHalfWidth ), paneRight, surface, bay );
                                            panel( patch.glass[surface], std::max( paneLeft, -doorHalfWidth ),
                                                   doorHead, std::min( paneRight, doorHalfWidth ), high, .085f );
                                        }
                                        else
                                        {
                                            windowModule = pane( paneLeft, paneRight, surface, bay );
                                        }
                                        if( bay > 0 && !windowModule && !previousModule )
                                        {
                                            const float s = -span * .5f + bay * bayWidth;
                                            const float mullionBase = entranceModule && std::abs( s ) < doorHalfWidth + .035f
                                                                          ? doorHead : low;
                                            panel( patch.frames, s - .035f, mullionBase, s + .035f, high, .045f );
                                        }
                                        previousModule = windowModule;
                                        anyWindowModule |= windowModule;
                                    }
                                    panel( patch.farGlass, left, low, right, high, -.018f );
                                    if( detailedBuildings && !anyWindowModule && ( streetFace || floor < 3 ) )
                                    {
                                        // Narrow framing modules sit in front of the recessed panes.
                                        for( int bay = 0; bay < bays; ++bay )
                                        {
                                            const float a = std::max( left, -span * .5f + bay * bayWidth );
                                            const float b = std::min( right, -span * .5f + ( bay + 1 ) * bayWidth );
                                            const int divisions = std::clamp( int( std::ceil( ( b - a ) / 1.50f ) ), 1, 3 );
                                            for( int division = 1; division < divisions; ++division )
                                            {
                                                const float s = a + ( b - a ) * float( division ) / divisions;
                                                if( retail && std::abs( s ) <= doorHalfWidth + .09f ) continue;
                                                if( !admitFacadeDetail( 24 ) ) break;
                                                faceBox( patch.officeJoinery, s, ( low + high ) * .5f,
                                                         .050f, high - low, -.047f, .060f );
                                            }
                                        }
                                        if( admitFacadeDetail( retail ? 72 : 48 ) )
                                        {
                                            faceBox( patch.officeJoinery, 0, high - .028f, right - left,
                                                     .056f, -.047f, .062f );
                                            if( retail )
                                            {
                                                for( float side : { -1.f, 1.f } )
                                                    faceBox( patch.officeJoinery, side * ( right + doorHalfWidth ) * .5f,
                                                             low + .028f, right - doorHalfWidth, .056f, -.047f, .062f );
                                            }
                                            else
                                                faceBox( patch.officeJoinery, 0, low + .028f, right - left,
                                                         .056f, -.047f, .062f );
                                        }
                                    }
                                    if( !builtInShopfront && !entranceModule && !anyWindowModule && lot.variant % 3 == 1 )
                                    {
                                        panel( patch.frames, left, ( low + high ) * .5f - .025f, right,
                                               ( low + high ) * .5f + .025f, .045f );
                                    }
                                    panel( patch.frames, -span * .5f, y1 - .065f, span * .5f, y1,
                                           -.025f );
                                }
                                else
                                {
                                    for( int bay = 0; bay < bays; ++bay )
                                    {
                                        const float s0 = -span * .5f + bay * bayWidth,
                                                    s1 = s0 + bayWidth;
                                        const float s = ( s0 + s1 ) * .5f;
                                        const bool door = entrance && floor == 0 && bay == bays / 2;
                                        const float ratio = warehouse ? .74f
                                                            : artdeco ? .52f
                                                            : timber ? .50f
                                                            : retail ? .79f
                                                            : civic ? .56f
                                                                    : .48f + ( lot.variant % 3 ) * .05f;
                                        const float width =
                                            door ? std::min( 2.05f, bayWidth * .72f ) : windowBayWidth * ratio;
                                        const float left = s - width * .5f, right = s + width * .5f;
                                        const float bottom = door ? y0 + .16f : low;
                                        const float head =
                                            door ? std::min( y1 - .35f, y0 + 2.65f ) : high;
                                        const float depth = builtInShopfront && !door ? cityShopfront::glassDepth : civic ? .24f : .16f;
                                        wallPanel( s0, y0, s1, bottom );
                                        wallPanel( s0, head, s1, y1 );
                                        panel( opaque, s0, bottom, left, head );
                                        panel( opaque, right, bottom, s1, head );
                                        reveal( left, bottom, right, head, depth );
                                        const bool hasModule = module(
                                            door ? CityBuildingPart::Entrance : CityBuildingPart::Window,
                                            door ? patch.accents : patch.frames, left, bottom, right, head,
                                            depth * .45f, depth );
                                        entranceModule |= door && hasModule;
                                        const bool bayWindow = terrace && streetFace && !door && !hasModule &&
                                                               floor < 2 && ( bay + lot.variant ) % 2 == 0 &&
                                                               admitFacadeDetail( 132 );
                                        const bool room = !door && !hasModule && !bayWindow &&
                                                          interior( left, right, bottom, head, depth, bay );
                                        if( ( !door || !hasModule ) && !bayWindow && !room )
                                            windowPane( glass, left, bottom, right, head, depth, bay, door || hasModule );
                                        if( !room && !door ) display( left, right, bottom, head, depth, bay );
                                        if( !hasModule )
                                        {
                                            panel( patch.frames, s - .032f, bottom, s + .032f, head,
                                                   depth - .025f );
                                        }
                                        panel( patch.farGlass, left, bottom, right, head, -.018f );
                                        if( !retail && !door && axis == entranceAxis &&
                                            sign == entranceSign )
                                        {
                                            // A shallow projecting sill gives a grazing highlight.
                                            faceBox( patch.frames, s, bottom - .035f, width + .18f, .07f,
                                                     .05f, .20f );
                                        }
                                        if( detailedBuildings && !hasModule && !door )
                                        {
                                            if( bayWindow )
                                            {
                                                // Three glazed faces form a shallow oriel. The
                                                // occupied wall aperture remains open behind it.
                                                const float projection = .42f, shoulder = width * .12f;
                                                panel( glass, left + shoulder, bottom, right - shoulder, head, -projection );
                                                glass.quad( point( left, bottom, 0 ), point( left + shoulder, bottom, -projection ),
                                                            point( left + shoulder, head, -projection ), point( left, head, 0 ),
                                                            ( normal * shoulder - along * projection ).normaliseCopy() );
                                                glass.quad( point( right - shoulder, bottom, -projection ), point( right, bottom, 0 ),
                                                            point( right, head, 0 ), point( right - shoulder, head, -projection ),
                                                            ( normal * shoulder + along * projection ).normaliseCopy() );
                                                for( float jamb : { left + shoulder, s, right - shoulder } )
                                                    faceBox( patch.frames, jamb, ( bottom + head ) * .5f, .055f,
                                                             head - bottom, projection + .015f, .065f );
                                                faceBox( patch.frames, s, bottom - .075f, width + .12f, .15f, projection * .5f, projection + .14f );
                                                faceBox( patch.frames, s, head + .06f, width + .14f, .12f, projection * .5f, projection + .16f );
                                            }
                                            if( warehouse && admitFacadeDetail( 120 ) )
                                            {
                                                // Tall steel factory sash with an actual multi-pane grid.
                                                for( int pane = 1; pane < 3; ++pane )
                                                    faceBox( patch.accents, left + width * pane / 3.f,
                                                             ( bottom + head ) * .5f, .045f, head - bottom,
                                                             -depth + .035f, .055f );
                                                for( int pane = 1; pane < 3; ++pane )
                                                    faceBox( patch.accents, s, bottom + ( head - bottom ) * pane / 3.f,
                                                             width, .045f, -depth + .035f, .055f );
                                                faceBox( patch.frames, s, head + .07f, width + .22f, .14f, .06f, .20f );
                                            }
                                            if( mediterranean && admitFacadeDetail( 240 ) )
                                            {
                                                const float shutter = std::min( .48f, ( bayWidth - width ) * .34f );
                                                for( float side : { -1.f, 1.f } )
                                                {
                                                    const float shutterS = s + side * ( width * .5f + shutter * .55f + .05f );
                                                    faceBox( patch.accents, shutterS, ( bottom + head ) * .5f,
                                                             shutter, head - bottom, .07f, .08f );
                                                    for( int slat = 1; slat <= 4; ++slat )
                                                        faceBox( patch.timber, shutterS, bottom + ( head - bottom ) * slat / 5.f,
                                                                 shutter * .86f, .045f, .12f, .035f );
                                                }
                                            }
                                            if( ( parisian || mediterranean ) && streetFace && floor > 0 &&
                                                ( parisian ? floor == 1 || floor + 1 == floors
                                                           : ( bay + floor + lot.variant ) % 3 == 0 ) &&
                                                admitFacadeDetail( 216 ) )
                                            {
                                                // Balconettes alternate with deeper slabs; rail spacing
                                                // stays capped independently of parcel dimensions.
                                                const float balconyWidth = width + .42f;
                                                const float projection = parisian ? .46f : .60f;
                                                faceBox( patch.frames, s, bottom - .16f, balconyWidth, .14f,
                                                         projection * .5f, projection + .12f );
                                                faceBox( patch.accents, s, bottom + .65f, balconyWidth, .055f,
                                                         projection, .055f );
                                                for( int rail = 0; rail <= 4; ++rail )
                                                    faceBox( patch.accents, s + balconyWidth * ( rail / 4.f - .5f ),
                                                             bottom + .25f, .035f, .80f, projection, .035f );
                                                for( float side : { -1.f, 1.f } )
                                                    faceBox( patch.accents, s + side * balconyWidth * .5f, bottom + .65f,
                                                             .045f, .055f, projection * .5f, projection );
                                            }
                                        }
                                    }
                                }
                                if( detailedBuildings && ( warehouse || artdeco || timber ) )
                                {
                                    for( int bay = 0; bay <= bays; ++bay )
                                    {
                                        const float s = std::clamp( -span * .5f + bay * bayWidth,
                                                                    -span * .5f + .16f, span * .5f - .16f );
                                        if( timber )
                                        {
                                            if( floor == 0 && admitFacadeDetail( 24 ) )
                                                faceBox( patch.timber, s, volume.bottom + volume.height * .5f, .15f,
                                                         volume.height, .065f, .16f );
                                            if( bay < bays && floor > 0 && admitFacadeDetail( 24 ) )
                                                patch.timber.beam( point( s + .16f, y0 + .14f, -.10f ),
                                                                    point( s + std::min( bayWidth * .28f, .9f ), low - .08f, -.10f ), .05f );
                                        }
                                        else if( warehouse && floor == 0 && admitFacadeDetail( 48 ) )
                                        {
                                            faceBox( opaque, s, volume.bottom + volume.height * .5f, .32f, volume.height, .11f, .24f );
                                            faceBox( patch.frames, s, top - .12f, .46f, .16f, .12f, .28f );
                                        }
                                        else if( artdeco && floor == 0 && admitFacadeDetail( 72 ) )
                                        {
                                            for( int flute = -1; flute <= 1; ++flute )
                                                faceBox( patch.frames, s + flute * .095f, volume.bottom + volume.height * .5f,
                                                         .055f, volume.height, .13f + ( flute == 0 ? .04f : 0 ), .13f );
                                        }
                                    }
                                    if( timber && admitFacadeDetail( 24 ) )
                                        faceBox( patch.timber, 0, y1 - .10f, span, .20f, .055f, .16f );
                                }
                                if( entrance && floor == 0 && !entranceModule )
                                {
                                    const float doorS =
                                        office ? 0.f : -span * .5f + ( bays / 2 + .5f ) * bayWidth;
                                    const float head = office ? std::min( y1 - .42f, y0 + 2.7f )
                                                              : std::min( y1 - .35f, y0 + 2.65f );
                                    const float doorHalfWidth =
                                        office ? .92f : std::min( 2.05f, bayWidth * .72f ) * .5f;
                                    const float depth = office ? .06f : civic ? .215f : .135f;
                                    panel( patch.frames, doorS - doorHalfWidth, y0 + .16f,
                                           doorS - doorHalfWidth + .06f, head, depth );
                                    panel( patch.frames, doorS + doorHalfWidth - .06f, y0 + .16f,
                                           doorS + doorHalfWidth, head, depth );
                                    panel( patch.frames, doorS - doorHalfWidth, head - .065f,
                                           doorS + doorHalfWidth, head, depth );
                                    faceBox( patch.frames, doorS - .16f, y0 + 1.3f, .025f, .40f,
                                             -depth + .015f, .025f );
                                    faceBox( patch.frames, doorS + .16f, y0 + 1.3f, .025f, .40f,
                                             -depth + .015f, .025f );
                                    if( office && admitFacadeDetail( 72 ) )
                                    {
                                        for( float side : { -1.f, 1.f } )
                                            faceBox( patch.officeJoinery, doorS + side * doorHalfWidth * .5f,
                                                     y0 + .37f, doorHalfWidth - .073f, .19f,
                                                     -depth + .021f, .035f );
                                        faceBox( patch.officeJoinery, doorS, ( y0 + .28f + head ) * .5f,
                                                 .046f, head - y0 - .28f, -depth + .026f, .054f );
                                    }
                                    const float canopyWidth =
                                        std::min( span * .60f, civic ? 11.f : terrace || timber ? 3.2f : 7.f );
                                    const float canopyDepth = civic ? 1.05f : timber ? .95f : .70f;
                                    if( builtInShopfront )
                                    {
                                        cityShopfront::appendFace( patch.shopfrontFascia[static_cast<u32>( shopStyle )],
                                            patch.officeJoinery, patch.shopfrontLetters, shopStyle, normal, along,
                                            point, faceBox, doorS, doorHalfWidth, span, y0, high, y1 );
                                        ++emittedShopfronts;
                                        if( interiorCount )
                                            WP_LOG( String( "City commercial display rooms lot=" ) + StringUtil::toString( lot.id ) +
                                                    ", style=" + cityShopInterior::styleName( shopStyle ) +
                                                    ", bays=" + StringUtil::toString( interiorCount ) );
                                    }
                                    else
                                    {
                                        if( mediterranean && admitFacadeDetail( 168 ) )
                                        {
                                            for( int rafter = 0; rafter <= 5; ++rafter )
                                                faceBox( patch.timber, doorS + canopyWidth * ( rafter / 5.f - .5f ),
                                                         y1 - .08f, .10f, .14f, canopyDepth * .5f, canopyDepth );
                                            faceBox( patch.timber, doorS, y1 - .18f, canopyWidth + .16f, .16f,
                                                     canopyDepth * .82f, .13f );
                                        }
                                        else
                                            faceBox( timber ? patch.timber : patch.accents, doorS, y1 - .08f, canopyWidth, .16f,
                                                     canopyDepth * .5f, canopyDepth );
                                        faceBox( patch.frames, doorS, y1 + .10f, canopyWidth * .82f, .20f,
                                                 .045f, .10f );
                                    }
                                    if( assets.cityTerrainRoads && detailedBuildings && assets.cityStreetDetail > 0 &&
                                        ( office || civic || warehouse || artdeco ) &&
                                        assets.cityBuildingMeshes[size_t( CityBuildingPart::Wall )].empty() &&
                                        assets.cityBuildingMeshes[size_t( CityBuildingPart::Entrance )].empty() &&
                                        assets.cityBuildingMeshes[size_t( CityBuildingPart::Corner )].empty() )
                                    {
                                        // The lettering is supported by the real metal fascia, outside the
                                        // central shop name and clear of door, glass, and moving leaves.
                                        const float fascia = builtInShopfront ? cityShopfront::fasciaWidth( span, doorS ) : canopyWidth;
                                        const float side = Vector3F::unitY().crossProduct( normal ).dotProduct( along ) >= 0 ? 1.f : -1.f;
                                        if( fascia >= ( builtInShopfront ? 2.55f : 2.18f ) )
                                            entranceSigns.push_back( { lot.id,
                                                point( doorS + side * ( fascia * .5f - ( builtInShopfront ? .40f : .33f ) ),
                                                       y1 - ( builtInShopfront ? .198f : .08f ),
                                                       -( builtInShopfront ? .152f : canopyDepth + .002f ) ) + origin,
                                                normal, along * -side, builtInShopfront ? 1.24f : 1.f,
                                                lot.district == OpenCityLayout::District::Commercial } );
                                    }
                                    if( civic )
                                    {
                                        for( int bay = 0; bay <= bays; ++bay )
                                        {
                                            const float s =
                                                std::clamp( -span * .5f + bay * bayWidth,
                                                            -span * .5f + .22f, span * .5f - .22f );
                                            // Engaged stone columns articulate a portico; the closed
                                            // facade never advertises an unsupported walkable interior.
                                            faceBox( patch.frames, s, y0 + floorHeight * .5f, .32f,
                                                     floorHeight - .20f, .12f, .36f );
                                            faceBox( patch.frames, s, y0 + .25f, .47f, .30f, .12f,
                                                     .46f );
                                        }
                                    }
                                    if( ( artdeco || warehouse || terrace || timber ) &&
                                        admitFacadeDetail( artdeco ? 192 : warehouse ? 168 : terrace ? 72 : 48 ) )
                                    {
                                        for( float side : { -1.f, 1.f } )
                                        {
                                            const float jamb = doorS + side * ( doorHalfWidth + .18f );
                                            faceBox( timber ? patch.timber : patch.frames, jamb,
                                                     ( y0 + head ) * .5f, timber ? .17f : .23f,
                                                     head - y0, .13f, .24f );
                                            if( artdeco )
                                            {
                                                for( int step = 0; step < 3; ++step )
                                                    faceBox( patch.frames, jamb + side * step * .18f,
                                                             y0 + ( head - y0 + .55f - step * .20f ) * .5f,
                                                             .10f, head - y0 + .55f - step * .20f,
                                                             .22f - step * .035f, .16f );
                                            }
                                        }
                                        if( warehouse )
                                            for( int rib = 0; rib < 5; ++rib )
                                                faceBox( patch.accents, doorS, y0 + .42f + rib * .39f,
                                                         doorHalfWidth * 1.8f, .055f, -depth + .025f, .045f );
                                        if( terrace )
                                            faceBox( patch.frames, doorS, head + .13f, doorHalfWidth * 2 + .68f,
                                                     .20f, .15f, .34f );
                                    }
                                }
                            }
                            if( detailedBuildings && ( !office || entrance ) )
                            {
                                // Stone courses frame the elevations without filling an aperture.
                                const float courseY = civic && entrance
                                                          ? volume.bottom + floorHeight + .08f
                                                          : top - .19f;
                                if( courseY < top - .08f )
                                {
                                    faceBox( patch.frames, 0, courseY, span + .08f, .13f, .075f, .17f );
                                }
                                if( parisian && admitFacadeDetail( floors > 1 ? 96 : 72 ) )
                                {
                                    for( int course = 0; course < 3; ++course )
                                        faceBox( patch.frames, 0, top - .35f + course * .12f,
                                                 span + .10f + course * .09f, .10f,
                                                 .08f + course * .04f, .22f + course * .08f );
                                    if( floors > 1 )
                                        faceBox( patch.frames, 0, volume.bottom + floorHeight + .10f,
                                                 span + .14f, .20f, .08f, .24f );
                                }
                            }
                        }
                    }
                    // Corner piers give masonry real depth. Explicit corner modules can also
                    // dress the primary commercial tier and the simplified preview.
                    if( groundFloor )
                    {
                        const float pierHeight = std::min( volume.height - .25f, floorHeight * 2 );
                        auto &cornerGeometry = detailedBuildings ? patch.frames : patch.farRoofs;
                        for( const float xSide : { -1.f, 1.f } )
                        {
                            for( const float zSide : { -1.f, 1.f } )
                            {
                                const Vector3F corner( cx + xSide * ( size.x * .5f - .14f ),
                                                       volume.bottom + .18f + pierHeight * .5f,
                                                       cz + zSide * ( size.z * .5f - .14f ) );
                                const float yaw = zSide > 0 ? xSide * pi * .25f
                                                           : xSide * pi * .75f;
                                if( !kit.append( CityBuildingPart::Corner, cornerGeometry, corner,
                                                 { .50f, pierHeight, .50f }, yaw ) &&
                                    detailedBuildings && ( civic || ( !office && lot.variant % 3 == 1 ) ) )
                                {
                                    patch.frames.box( corner, { .39f, pierHeight, .39f } );
                                    patch.frames.box( corner + Vector3F( 0, -pierHeight * .5f + .14f, 0 ),
                                                      { .49f, .28f, .49f } );
                                    patch.frames.box( corner + Vector3F( 0, pierHeight * .5f - .10f, 0 ),
                                                      { .47f, .20f, .47f } );
                                }
                            }
                        }
                    }
                    if( tier + 1 != volumes.size() )
                    {
                        continue;
                    }

                    // Keep the building's occupied tier and collision intact. Each LOD retains
                    // its roof silhouette, while the street-level kit adds smaller features.
                    const float deck = top + .12f;
                    auto &moduleRoof = detailedBuildings ? patch.roofs : patch.farRoofs;
                    const bool replacedRoof = kit.append( CityBuildingPart::Roof, moduleRoof,
                                                          { cx, deck + 1.25f, cz },
                                                          { size.x, 2.5f, size.z }, 0 );
                    const bool hipped = style == 1 || mediterranean;
                    const bool gabled = terrace || timber;
                    const bool lantern = civic || ( office && lot.variant % 4 == 1 );
                    // Timber houses present their gable to the street. Other pitched
                    // roofs keep a ridge along the longer occupied dimension.
                    const bool longX = timber ? entranceAxis == 1 : size.x >= size.z;
                    const auto roofPoint = [&]( float u, float y, float v ) {
                        return longX ? Vector3F( cx + u, deck + y, cz + v )
                                     : Vector3F( cx + v, deck + y, cz + u );
                    };
                    const float halfLong = ( longX ? size.x : size.z ) * .5f;
                    const float halfShort = ( longX ? size.z : size.x ) * .5f;
                    const float rise = parisian ? 2.65f
                        : timber ? std::clamp( halfShort * .55f, 1.8f, 3.8f )
                        : terrace ? std::clamp( halfShort * .43f, 1.5f, 3.0f )
                        : std::clamp( halfShort * .31f, .9f, 2.5f );
                    const auto roofBox = [&]( Geometry &g, float u, float y, float v,
                                              float width, float height, float depth ) {
                        g.box( roofPoint( u, y, v ), longX ? Vector3F( width, height, depth )
                                                                         : Vector3F( depth, height, width ) );
                    };
                    const auto slope = [&]( Geometry &g, Vector3F a, Vector3F b,
                                            Vector3F c, Vector3F d ) {
                        auto normal = ( b - a ).crossProduct( d - a ).normaliseCopy();
                        g.quad( a, b, c, d, normal.y < 0 ? -normal : normal );
                    };
                    const int roofBays = std::clamp( int( halfLong * 2 / 4.2f ), 2, 6 );
                    const int longRoofDormers = std::clamp( int( ( halfLong * 2 - 3.f ) / 3.2f ), 1, 6 );
                    const int endRoofBays = std::clamp( int( ( halfShort * 2 - 3.f ) / 3.2f ), 1, 4 );
                    const int sawTeeth = std::clamp( int( halfShort * 2 / 4.f ), 2, 4 );
                    const auto mansardDormers = [&]( auto visit, bool countSilhouettes = false ) {
                        const bool frontageEnd = entranceAxis == ( longX ? 1 : 0 );
                        // Visit the actual street frontage first so the finite raised
                        // sash budget is useful even when the street meets a short end.
                        for( bool endFace : { frontageEnd, !frontageEnd } )
                        {
                            const int nominalCount = endFace ? endRoofBays : longRoofDormers;
                            const float available = std::max( 0.f, ( endFace ? halfShort : halfLong ) - 1.5f );
                            for( float side : { float( entranceSign ), float( -entranceSign ) } )
                            {
                                const bool streetFace = endFace == frontageEnd && side == float( entranceSign );
                                const int minimum = std::min( nominalCount, streetFace ? 2 : 1 );
                                const int count = buildingCount > 128
                                    ? std::clamp( int( std::floor( nominalCount * ( 128.f / float( buildingCount ) ) ) ),
                                                  minimum, nominalCount ) : nominalCount;
                                if( countSilhouettes )
                                {
                                    nominalMansardSilhouettes += nominalCount;
                                    emittedMansardSilhouettes += count;
                                }
                                for( int bay = 0; bay < count; ++bay )
                                    visit( endFace, ( bay + .5f ) * ( 2 * available / count ) - available, side );
                            }
                        }
                    };
                    const auto dormerPoint = [&]( bool endFace, float along, float y, float side, float inset ) {
                        return endFace ? roofPoint( side * ( halfLong - inset ), y, along )
                                       : roofPoint( along, y, side * ( halfShort - inset ) );
                    };
                    const auto dormerBox = [&]( Geometry &g, bool endFace, float along, float y, float side,
                                               float inset, float width, float height, float depth ) {
                        if( endFace )
                            roofBox( g, side * ( halfLong - inset ), y, along, depth, height, width );
                        else
                            roofBox( g, along, y, side * ( halfShort - inset ), width, height, depth );
                    };
                    const auto roofSilhouette = [&]( Geometry &roof ) {
                        if( parisian )
                        {
                            // A steep lower skirt and a shallow capped upper deck give a
                            // mansard an unmistakable silhouette at both render distances.
                            const float inset = std::min( 1.35f, halfShort * .28f );
                            for( float side : { -1.f, 1.f } )
                            {
                                slope( roof, roofPoint( -halfLong, 0, side * halfShort ),
                                       roofPoint( halfLong, 0, side * halfShort ),
                                       roofPoint( halfLong - inset, rise, side * ( halfShort - inset ) ),
                                       roofPoint( -halfLong + inset, rise, side * ( halfShort - inset ) ) );
                                slope( roof, roofPoint( side * halfLong, 0, -halfShort ),
                                       roofPoint( side * halfLong, 0, halfShort ),
                                       roofPoint( side * ( halfLong - inset ), rise, halfShort - inset ),
                                       roofPoint( side * ( halfLong - inset ), rise, -halfShort + inset ) );
                            }
                            mansardDormers( [&]( bool endFace, float along, float side ) {
                                dormerBox( roof, endFace, along, 1.18f, side, .43f, 1.26f, 1.66f, .76f );
                                dormerBox( roof, endFace, along, 2.07f, side, .43f, 1.44f, .12f, .92f );
                            }, true );
                            roofBox( roof, 0, rise + .04f, 0, ( halfLong - inset ) * 2,
                                     .08f, ( halfShort - inset ) * 2 );
                            return;
                        }
                        if( warehouse )
                        {
                            const float toothWidth = 2 * halfShort / sawTeeth;
                            for( int tooth = 0; tooth < sawTeeth; ++tooth )
                            {
                                const float v0 = -halfShort + tooth * toothWidth, v1 = v0 + toothWidth;
                                slope( roof, roofPoint( -halfLong, .08f, v0 ), roofPoint( halfLong, .08f, v0 ),
                                       roofPoint( halfLong, 1.45f, v1 ), roofPoint( -halfLong, 1.45f, v1 ) );
                                const Vector3F faceNormal = longX ? Vector3F( 0, 0, 1 ) : Vector3F( 1, 0, 0 );
                                roof.quad( roofPoint( -halfLong, .08f, v1 ), roofPoint( halfLong, .08f, v1 ),
                                           roofPoint( halfLong, 1.45f, v1 ), roofPoint( -halfLong, 1.45f, v1 ), faceNormal );
                                for( float side : { -1.f, 1.f } )
                                {
                                    const auto a = roofPoint( side * halfLong, .08f, v0 );
                                    const auto b = roofPoint( side * halfLong, .08f, v1 );
                                    const auto c = roofPoint( side * halfLong, 1.45f, v1 );
                                    roof.quad( ( a + b ) * .5f, b, c, a,
                                               longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side ) );
                                }
                            }
                            return;
                        }
                        if( artdeco )
                        {
                            float bottom = 0;
                            for( int step = 0; step < 3; ++step )
                            {
                                const float fraction = .78f - step * .20f;
                                const float height = .85f + step * .10f;
                                roof.box( { cx, deck + bottom + height * .5f, cz },
                                          { size.x * fraction, height, size.z * fraction } );
                                roof.box( { cx, deck + bottom + height + .04f, cz },
                                          { size.x * fraction + .14f, .08f, size.z * fraction + .14f } );
                                bottom += height;
                            }
                            return;
                        }
                        if( gabled )
                        {
                            for( float side : { -1.f, 1.f } )
                            {
                                slope( roof, roofPoint( -halfLong, 0, side * halfShort ),
                                       roofPoint( halfLong, 0, side * halfShort ),
                                       roofPoint( halfLong, rise, 0 ), roofPoint( -halfLong, rise, 0 ) );
                                const auto a = roofPoint( side * halfLong, 0, -halfShort );
                                const auto b = roofPoint( side * halfLong, 0, halfShort );
                                const auto c = roofPoint( side * halfLong, rise, 0 );
                                roof.quad( ( a + b ) * .5f, b, c, a,
                                           longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side ) );
                            }
                            return;
                        }
                        if( hipped )
                        {
                            const float ridge = std::max( .1f, halfLong - halfShort * .75f );
                            for( float side : { -1.f, 1.f } )
                            {
                                const auto a = roofPoint( -halfLong, 0, side * halfShort );
                                const auto b = roofPoint( halfLong, 0, side * halfShort );
                                const auto c = roofPoint( ridge, rise, 0 );
                                const auto d = roofPoint( -ridge, rise, 0 );
                                auto n = ( b - a ).crossProduct( d - a );
                                n.normalise();
                                roof.quad( a, b, c, d, n.y < 0 ? -n : n );
                                const auto e = roofPoint( side * halfLong, 0, -halfShort );
                                const auto f = roofPoint( side * halfLong, 0, halfShort );
                                const auto g = roofPoint( side * ridge, rise, 0 );
                                auto endNormal = ( f - e ).crossProduct( g - e );
                                endNormal.normalise();
                                // Split the triangular hip at its eave midpoint to keep
                                // indexed quads nondegenerate in the standard mesh stream.
                                const auto mid = ( e + f ) * .5f;
                                roof.quad( mid, f, g, e, endNormal.y < 0 ? -endNormal : endNormal );
                            }
                            return;
                        }
                        const float parapet = office ? .52f : .42f;
                        for( float side : { -1.f, 1.f } )
                        {
                            roof.box( { cx, deck + parapet * .5f, cz + side * ( size.z * .5f - .10f ) },
                                      { size.x, parapet, .20f } );
                            roof.box( { cx + side * ( size.x * .5f - .10f ), deck + parapet * .5f, cz },
                                      { .20f, parapet, size.z - .40f } );
                        }
                        if( lantern )
                        {
                            const float height = civic ? 1.35f : 1.75f;
                            roof.box( { cx, deck + height * .5f, cz },
                                      { size.x * .43f, height, size.z * .36f } );
                            roof.box( { cx, deck + height + .09f, cz },
                                      { size.x * .43f + .28f, .18f, size.z * .36f + .28f } );
                        }
                    };
                    if( detailedBuildings && !replacedRoof )
                    {
                        roofSilhouette( mediterranean ? patch.tiles : artdeco ? patch.walls[family] : patch.roofs );
                    }
                    if( detailedBuildings || !replacedRoof )
                    {
                        roofSilhouette( mediterranean ? patch.farTiles : artdeco ? patch.farWalls[family] : patch.farRoofs );
                    }
                    if( !detailedBuildings || replacedRoof )
                    {
                        continue;
                    }
                    if( parisian )
                    {
                        mansardDormers( [&]( bool endFace, float along, float side ) {
                            // Every elevation has glazing. Only the optional raised
                            // sash consumes the existing per-building trim allowance.
                            const Vector3F normal = endFace == longX ? Vector3F( side, 0, 0 )
                                                                                    : Vector3F( 0, 0, side );
                            patch.glass[1].quad( dormerPoint( endFace, along - .46f, .62f, side, .035f ),
                                                 dormerPoint( endFace, along + .46f, .62f, side, .035f ),
                                                 dormerPoint( endFace, along + .46f, 1.82f, side, .035f ),
                                                 dormerPoint( endFace, along - .46f, 1.82f, side, .035f ), normal );
                            if( !admitRoofDetail( 120 ) ) return;
                            dormerBox( patch.frames, endFace, along, .59f, side, .035f, 1.12f, .10f, .12f );
                            dormerBox( patch.frames, endFace, along, 1.85f, side, .035f, 1.14f, .10f, .12f );
                            for( float jamb : { -.5f, 0.f, .5f } )
                                dormerBox( patch.frames, endFace, along + jamb, 1.22f, side, .015f, .065f, 1.30f, .07f );
                        } );
                    }
                    else if( warehouse )
                    {
                        const float toothWidth = 2 * halfShort / sawTeeth;
                        for( int tooth = 1; tooth <= sawTeeth; ++tooth )
                        {
                            const float v = -halfShort + tooth * toothWidth + .018f;
                            patch.glass[1].quad( roofPoint( -halfLong + .12f, .31f, v ), roofPoint( halfLong - .12f, .31f, v ),
                                                 roofPoint( halfLong - .12f, 1.35f, v ), roofPoint( -halfLong + .12f, 1.35f, v ),
                                                 longX ? Vector3F( 0, 0, 1 ) : Vector3F( 1, 0, 0 ) );
                            if( !admitRoofDetail( 24 * ( roofBays + 2 ) ) ) continue;
                            roofBox( patch.frames, 0, 1.44f, v, halfLong * 2 + .06f, .08f, .12f );
                            for( int post = 0; post <= roofBays; ++post )
                                roofBox( patch.accents, -halfLong + .14f + post * ( 2 * halfLong - .28f ) / roofBays,
                                         .82f, v + .025f, .055f, 1.12f, .065f );
                        }
                    }
                    else if( artdeco )
                    {
                        if( !admitRoofDetail( 240 ) ) continue;
                        for( int flute = -2; flute <= 2; ++flute )
                        {
                            const float height = 3.08f - std::abs( flute ) * .20f;
                            for( float side : { -1.f, 1.f } )
                                patch.frames.box( { cx + flute * .24f, deck + height * .5f,
                                                     cz + side * ( size.z * .39f + .03f ) },
                                                   { .10f, height, .10f } );
                        }
                    }
                    else if( gabled )
                    {
                        for( float side : { -1.f, 1.f } )
                        {
                            const float u = side * ( halfLong + .015f );
                            const auto a = roofPoint( u, 0, -halfShort ), b = roofPoint( u, 0, halfShort ),
                                       c = roofPoint( u, rise, 0 );
                            opaque.quad( ( a + b ) * .5f, b, c, a,
                                         longX ? Vector3F( side, 0, 0 ) : Vector3F( 0, 0, side ) );
                            if( !admitRoofDetail( timber ? 144 : 48 ) ) continue;
                            auto &verge = timber ? patch.timber : patch.frames;
                            verge.beam( a, c, timber ? .09f : .075f );
                            verge.beam( b, c, timber ? .09f : .075f );
                            if( timber )
                            {
                                verge.beam( roofPoint( u + side * .02f, .04f, 0 ),
                                            roofPoint( u + side * .02f, rise, 0 ), .07f );
                                verge.beam( a, b, .08f );
                                for( float brace : { -.5f, .5f } )
                                    verge.beam( roofPoint( u + side * .02f, .08f, brace * halfShort ),
                                                roofPoint( u + side * .02f, rise * .48f, 0 ), .055f );
                            }
                        }
                        const int chimneys = terrace ? std::clamp( roofBays / 2, 1, 3 ) : 1;
                        for( int chimney = 0; chimney < chimneys; ++chimney )
                        {
                            if( !admitRoofDetail( 96 ) ) break;
                            const float u = ( chimney + .5f ) * ( 2 * halfLong / chimneys ) - halfLong;
                            roofBox( opaque, u, rise + .45f, 0, .62f, 1.05f, .72f );
                            roofBox( patch.frames, u, rise + .96f, 0, .78f, .14f, .86f );
                            for( float pot : { -.16f, .16f } )
                                roofBox( patch.tiles, u + pot, rise + 1.14f, 0, .16f, .26f, .18f );
                        }
                    }
                    else if( hipped )
                    {
                        const float u = halfLong * ( lot.variant % 2 ? .34f : -.34f );
                        const auto base = roofPoint( u, rise * .63f, halfShort * .20f );
                        patch.walls[family].box( base + Vector3F( 0, .53f, 0 ), { .54f, 1.24f, .61f } );
                        patch.frames.box( base + Vector3F( 0, 1.16f, 0 ), { .66f, .12f, .72f } );
                        if( mediterranean && admitRoofDetail( 408 ) )
                        {
                            const float ridge = std::max( .1f, halfLong - halfShort * .75f );
                            patch.tiles.beam( roofPoint( -ridge, rise + .045f, 0 ),
                                              roofPoint( ridge, rise + .045f, 0 ), .085f );
                            // Bounded raised tile courses catch grazing light without
                            // allocating one mesh or actor for each roof tile.
                            for( float side : { -1.f, 1.f } )
                            {
                                for( int course = 1; course <= 8; ++course )
                                {
                                    const float t = course / 9.f;
                                    const float u = ridge + ( halfLong - ridge ) * t;
                                    const float y = rise * ( 1 - t ) + .035f, v = side * halfShort * t;
                                    patch.tiles.beam( roofPoint( -u, y, v ), roofPoint( u, y, v ), .035f );
                                }
                            }
                        }
                    }
                    else if( lantern )
                    {
                        const float width = size.x * .43f, depth = size.z * .36f;
                        const float y = deck + ( civic ? .85f : 1.08f );
                        for( const float side : { -1.f, 1.f } )
                        {
                            patch.glass[1].box( { cx, y, cz + side * ( depth * .5f + .012f ) },
                                                 { width - .48f, .62f, .024f } );
                            patch.glass[1].box( { cx + side * ( width * .5f + .012f ), y, cz },
                                                 { .024f, .62f, depth - .48f } );
                            for( int post = -1; post <= 1; ++post )
                            {
                                patch.frames.box( { cx + post * width * .22f, y,
                                                    cz + side * ( depth * .5f + .027f ) },
                                                  { .075f, .66f, .044f } );
                            }
                        }
                    }
                    else
                    {
                        const int requestedUnits = office ? 3 : 1 + int( lot.variant % 2 );
                        const int units = std::min( requestedUnits, 1 + int( std::max( 0.f, size.x - 2.5f ) / 2.45f ) );
                        for( int unit = 0; unit < units; ++unit )
                        {
                            const float x = cx + ( unit - ( units - 1 ) * .5f ) * 2.45f;
                            const float z = cz + ( lot.variant % 2 ? .17f : -.17f ) * size.z;
                            const float h = office ? .82f : .54f;
                            patch.roofs.box( { x, deck + h * .5f, z }, { 1.95f, h, 1.28f } );
                            for( int fin = -2; fin <= 2; ++fin )
                            {
                                patch.frames.box( { x, deck + h + .025f, z + fin * .21f },
                                                  { 1.86f, .045f, .082f } );
                            }
                        }
                    }
                }
                if( masonry.storeys )
                {
                    emittedMasonry = true;
                    WP_LOG( String( "City masonry facade lot=" ) + StringUtil::toString( lot.id ) +
                            ", faces=" + StringUtil::toString( masonry.storeys ) +
                            ", panels=" + StringUtil::toString( masonry.panels ) +
                            ", profiles=" + StringUtil::toString( masonry.profiles ) +
                            ", mullions=" + StringUtil::toString( masonry.mullions ) );
                }
            }
            if( nominalMansardSilhouettes != emittedMansardSilhouettes )
            {
                WP_LOG( String( "City mansard density: buildings=" ) + StringUtil::toString( u64( buildingCount ) ) +
                        " emitted/nominal dormers across LODs=" + StringUtil::toString( u64( emittedMansardSilhouettes ) ) +
                        "/" + StringUtil::toString( u64( nominalMansardSilhouettes ) ) +
                        " silhouette triangles avoided=" +
                        StringUtil::toString( u64( nominalMansardSilhouettes - emittedMansardSilhouettes ) * 24 ) );
            }
            kit.finish();
            // Contain exploration within the shared collision plane.
            const float boundary = city.extent + 20;
            std::array<Geometry, 4> terrainBoundaries;
            for( int axis = 0; axis < 2; ++axis )
            {
                for( int side : { -1, 1 } )
                {
                    Vector3F p =
                        axis ? Vector3F( 0, .6f, side * boundary ) : Vector3F( side * boundary, .6f, 0 );
                    if( assets.cityTerrainRoads )
                    {
                        float height = 0;
                        const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                        const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                        if( !render::sampleTerrainHeight( *terrain, placement, p, height ) )
                        {
                            throw std::runtime_error( "City boundary leaves its terrain." );
                        }
                        p.y += height;
                        const Vector3F along = axis ? Vector3F( 1, 0, 0 ) : Vector3F( 0, 0, 1 );
                        const Vector3F across( along.z, 0, along.x );
                        const Vector3F up( 0, 1.2f, 0 ), toe( 0, .22f, 0 );
                        auto &wall = terrainBoundaries[axis * 2 + ( side > 0 )];
                        const auto segments = std::clamp( int( std::ceil( boundary * 2.f / 4.f ) ), 1, 512 );
                        Vector3F lastLeft, lastRight;
                        for( int i = 0; i <= segments; ++i )
                        {
                            const auto centre = p + along * ( boundary * ( 2.f * i / segments - 1.f ) );
                            auto left = centre - across * .5f, right = centre + across * .5f;
                            if( !render::sampleTerrainHeight( *terrain, placement, left, left.y ) ||
                                !render::sampleTerrainHeight( *terrain, placement, right, right.y ) )
                            {
                                wall = Geometry{};
                                break;
                            }
                            if( i )
                            {
                                wall.quad( lastLeft + up, left + up, right + up, lastRight + up, { 0, 1, 0 } );
                                wall.quad( lastLeft - toe, left - toe, left + up, lastLeft + up, -across );
                                wall.quad( right - toe, lastRight - toe, lastRight + up, right + up, across );
                            }
                            lastLeft = left;
                            lastRight = right;
                        }
                        if( !wall.indices.empty() ) continue;
                    }
                    const Vector3F size =
                        axis ? Vector3F( boundary * 2, 1.2f, 1 ) : Vector3F( 1, 1.2f, boundary * 2 );
                    pavement.box( p, size );
                    reserveOwnerSlot( assets.actors );
                    auto wall = app->getGameManager()->createActor();
                    if( !wall )
                    {
                        throw std::runtime_error( "Cannot create procedural actor" );
                    }
                    assets.actors.push_back( wall );
                    wall->setName( "City boundary" );
                    wall->setStatic( true );
                    wall->setPosition( p );
                    wall->addComponent<CollisionBox>()->setExtents( size );
                    wall->addComponent<Rigidbody>();
                }
            }
            if( !assets.cityTerrainRoads )
            {
                mesh( assets, "City streets", asphalt, cityAsphalt );
            }
            auto pavingMaterial = material( assets, { .54f, .53f, .48f, 1 }, .92f );
            if( !applyCitySurface( assets, pavingMaterial, CitySurface::Paving, surfaceParams.size,
                                   .45f, 1 ) )
            {
                auto paving = forge->bakeSurface( SurfaceTag::Concrete, surfaceParams );
                maps( assets, pavingMaterial, paving.albedo, paving.normal, paving.orm );
                pavingMaterial->setDiffuse( { .54f, .53f, .48f, 1 } );
                pavingMaterial->setUVProjection( 1 );
                pavingMaterial->setTriplanarScale( .5f );
                pavingMaterial->setNormalStrength( .12f );
                pavingMaterial->setEditorFloat( "aoStrength", .3f );
            }
            mesh( assets, "City sidewalks", pavement, pavingMaterial );
            for( auto &wall : terrainBoundaries )
            {
                if( auto actor = mesh( assets, "Terrain city boundary", wall, pavingMaterial ) )
                {
                    actor->setStatic( true );
                    auto collision = actor->addComponent<CollisionMesh>();
                    if( !collision ) throw std::runtime_error( "Cannot create city boundary collision." );
                    auto resource = actor->getComponent<scene::Mesh>()->getMeshResource();
                    collision->setConvex( false );
                    collision->setMeshResource( resource );
                    if( auto shape = dynamic_pointer_cast<physics::IMeshShape>( collision->getShape() ) )
                        shape->setMeshResource( resource );
                    if( !actor->addComponent<Rigidbody>() )
                        throw std::runtime_error( "Cannot create city boundary rigidbody." );
                }
                wall = Geometry{};
            }
            if( !assets.cityTerrainRoads )
            {
                mesh( assets, "City lane markings", markings,
                      material( assets, { .85f, .82f, .65f, 1 }, .8f ) );
            }
            const ColourF colours[] = { { .78f, .68f, .57f, 1 },
                                        { .86f, .83f, .75f, 1 },
                                        { .58f, .65f, .69f, 1 } };
            const SurfaceTag facadeSurfaces[] = { SurfaceTag::Brick, SurfaceTag::Plaster,
                                                  SurfaceTag::Concrete };
            std::array<SmartPtr<render::IMaterial>, 3> facades;
            const CitySurface facadeFamilies[] = { CitySurface::Brick, CitySurface::Plaster,
                                                   CitySurface::Concrete };
            for( int i = 0; i < 3; ++i )
            {
                facades[i] = material( assets, colours[i], 1, 1 );
                if( applyCitySurface( assets, facades[i], facadeFamilies[i], surfaceParams.size,
                                      i == 0 ? .6f : i == 1 ? .12f : .25f, 1 ) )
                {
                    // Scanned albedo already carries the surface colour. Restrained
                    // architectural tints preserve reflectance and mortar contrast.
                    facades[i]->setDiffuse( i == 1 ? ColourF( .96f, .94f, .90f, 1 )
                                            : i == 2 ? ColourF( .93f, .96f, 1, 1 )
                                                     : ColourF::White );
                    if( i == 1 ) facades[i]->setEditorFloat( "aoStrength", .25f );
                    continue;
                }
                // Complete bakes supply encoded albedo and linear normal/ORM data.
                // Each architectural kit has a different surface and physical scale.
                auto surface = forge->bakeSurface( facadeSurfaces[i], surfaceParams );
                if( surface.albedo.pixels.empty() || surface.normal.pixels.empty() ||
                    surface.orm.pixels.empty() )
                {
                    throw std::runtime_error( "City facades require complete PBR maps." );
                }
                if( i != 0 )
                {
                    // Large mineral panels need restrained weathering. The forge's
                    // full-range cracked plaster otherwise reads as mottled rock.
                    const float base = i == 1 ? .58f : .42f;
                    for( size_t pixel = 0; pixel < surface.albedo.pixels.size(); pixel += 4 )
                    {
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            const float encoded = surface.albedo.pixels[pixel + channel] / 255.f;
                            const float source = encoded <= .04045f
                                                     ? encoded / 12.92f
                                                     : std::pow( ( encoded + .055f ) / 1.055f, 2.4f );
                            const float linear = base * .85f + source * .15f;
                            surface.albedo.pixels[pixel + channel] = u8(
                                std::clamp( ( 1.055f * std::pow( linear, 1.f / 2.4f ) - .055f ) * 255,
                                            0.f, 255.f ) );
                        }
                    }
                }
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
            const auto storefrontMetal = material( assets, { .37f, .39f, .40f, 1 }, .53f, .32f );
            const auto accentMaterial = material( assets, { .18f, .28f, .30f, 1 }, .55f );
            const auto timberMaterial = material( assets, { .24f, .15f, .085f, 1 }, .83f );
            const auto tileMaterial = material( assets, { .57f, .24f, .13f, 1 }, .82f );
            const auto [glassMaterials, paneMaterial] = cityBuildingGlazing::build( assets, detailedBuildings );
            const auto shops = emittedShopfronts ? cityShopfront::materials( assets ) : cityShopfront::Materials{};
            const auto interiors = emittedDisplayBays ? cityShopInterior::materials( assets ) : cityShopInterior::Materials{};
            const auto masonry = emittedMasonry ? cityFacadeDetail::materials( assets, facades[2], seed )
                                                : cityFacadeDetail::Materials{};
            for( auto &[key, patch] : patches )
            {
                reserveOwnerSlot( assets.actors );
                auto root = app->getGameManager()->createActor();
                if( !root )
                {
                    throw std::runtime_error( "Cannot create city LOD patch." );
                }
                assets.actors.push_back( root );
                root->setName( "City LOD patch" );
                root->setPosition( { key.first * 128.f + 64, 0, key.second * 128.f + 64 } );
                auto nearRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                auto farRenderers = makeSourceBudgetSnapshot<SmartPtr<scene::Renderer>>();
                const auto add = [&]( const String &name, Geometry &geometry,
                                      SmartPtr<render::IMaterial> surface, auto &renderers, bool shadows = true ) {
                    if( auto actor = mesh( assets, name, geometry, surface, root ) )
                    {
                        auto renderer = actor->getComponent<scene::MeshRenderer>();
                        if( !shadows ) renderer->setCastShadows( scene::Renderer::CastShadows::Off );
                        renderers.push_back( renderer );
                    }
                    // MeshUtil owns a packed copy now. Release grown staging arrays
                    // before publishing the next patch instead of doubling all city
                    // vertex/index storage until the end of this function.
                    geometry = Geometry{};
                };
                if( quality != VehicleAppearanceQuality::Preview )
                {
                    for( int family = 0; family < 3; ++family )
                    {
                        add( "LOD0 city facade", patch.walls[family], facades[family], nearRenderers );
                    }
                    for( int glass = 0; glass < 3; ++glass )
                    {
                        add( "LOD0 city glazing", patch.glass[glass], glassMaterials[glass],
                             nearRenderers );
                    }
                    add( "LOD0 city glazing", patch.windowPanes, paneMaterial, nearRenderers );
                    add( "LOD0 city roof detail", patch.roofs, roofMaterial, nearRenderers );
                    add( "LOD0 city facade frames", patch.frames, frameMaterial, nearRenderers );
                    add( "LOD0 city masonry panels", patch.masonry.panels, masonry.stone, nearRenderers );
                    add( "LOD0 city masonry sill and head profiles", patch.masonry.profiles, masonry.dressed, nearRenderers );
                    add( "LOD0 city masonry capped mullions", patch.masonry.mullions, masonry.metal, nearRenderers );
                    add( "LOD0 city storefront joinery", patch.officeJoinery, storefrontMetal, nearRenderers );
                    for( size_t style = 0; style < shops.fascia.size(); ++style )
                        add( "LOD0 city commercial fascia and canopies", patch.shopfrontFascia[style], shops.fascia[style], nearRenderers );
                    add( "LOD0 city commercial lettering", patch.shopfrontLetters, shops.lettering, nearRenderers );
                    add( "LOD0 city commercial window displays", patch.shopfrontDisplays, shops.display, nearRenderers );
                    add( "LOD0 city commercial display room lining", patch.shopInterior.lining, interiors.lining, nearRenderers, false );
                    add( "LOD0 city commercial display room rear wall", patch.shopInterior.rearWall, interiors.rearWall, nearRenderers, false );
                    add( "LOD0 city commercial display room timber", patch.shopInterior.timber, interiors.timber, nearRenderers, false );
                    add( "LOD0 city commercial display room fittings", patch.shopInterior.fittings, interiors.fittings, nearRenderers, false );
                    add( "LOD0 city commercial display room exhibits", patch.shopInterior.display, interiors.display, nearRenderers, false );
                    add( "LOD0 city commercial display room lights", patch.shopInterior.luminaires, interiors.luminaires, nearRenderers, false );
                    add( "LOD0 city commercial display glazing", patch.shopInterior.glazing, interiors.glazing, nearRenderers, false );
                    add( "LOD0 city entrance fascia", patch.accents, accentMaterial, nearRenderers );
                    add( "LOD0 city timber details", patch.timber, timberMaterial, nearRenderers );
                    add( "LOD0 city terracotta roofs", patch.tiles, tileMaterial, nearRenderers );
                }
                for( int family = 0; family < 3; ++family )
                {
                    add( "LOD1 city massing", patch.farWalls[family], facades[family], farRenderers );
                }
                add( "LOD1 city roof silhouette", patch.farRoofs, roofMaterial, farRenderers );
                add( "LOD1 city terracotta silhouette", patch.farTiles, tileMaterial, farRenderers );
                add( "LOD1 city glazing ribbons", patch.farGlass, glassMaterials[0], farRenderers );
                auto lod = root->addComponent<scene::LODGroup>();
                lod->setSize( 128 );
                lod->setDetailBounds( patch.bounds );
                lod->setHysteresis( .18f );
                lod->setCullBelowLastLOD( false );
                if( !nearRenderers.empty() )
                {
                    lod->addLevel( .11f, nearRenderers );
                }
                lod->addLevel( 0, farRenderers );
                String error;
                if( !lod->validate( &error ) )
                {
                    throw std::runtime_error( "Invalid city LOD patch: " + error );
                }
            }
            mesh( assets, "City block plazas", courtyards, pavingMaterial );
            if( assets.cityTerrainRoads )
            {
                CityRoadTerrainMesh fitted;
                String error;
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                const auto roadFootways = assets.roadAuthoring->getRuntimeFootways();
                const bool sharedFootways = roadFootways && terrain &&
                    roadFootways->sourceRevision == assets.roadAuthoring->getActiveRevision() &&
                    roadFootways->terrainRevision == terrain->revision;
                if( sharedFootways )
                {
                    fitted = *roadFootways;
                }
                else if( !buildCityRoadFootways( assets.roadAuthoring->getSource(), assets.roadAuthoring->getBake(),
                                                terrain, assets.roadAuthoring->getTerrainPlacement(), fitted, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                const auto walks = cityFootwayAppearance::prepare( assets, fitted, pavingMaterial, seed );
                if( auto actor = publishMesh( assets, "Terrain city footways", walks.mesh,
                                              fitted.indices.size() / 3, walks.slab ) )
                {
                    // One native concave mesh shares every fitted top/kerb face;
                    // no per-paving-tile actor or simplified flat contact plane.
                    if( walks.curb )
                    {
                        auto stone = actor->addComponent<scene::Material>();
                        if( !stone ) throw std::runtime_error( "Cannot create terrain-city kerb material binding." );
                        stone->setIndex( 1 );
                        stone->setMaterial( walks.curb );
                    }
                    actor->setStatic( true );
                    auto collision = actor->addComponent<CollisionMesh>();
                    if( !collision )
                    {
                        throw std::runtime_error( "Cannot create terrain-city footway collision." );
                    }
                    auto resource = actor->getComponent<scene::Mesh>()->getMeshResource();
                    collision->setConvex( false );
                    collision->setMeshResource( resource );
                    if( auto shape = dynamic_pointer_cast<physics::IMeshShape>( collision->getShape() ) )
                    {
                        shape->setMeshResource( resource );
                    }
                    if( !actor->addComponent<Rigidbody>() )
                    {
                        throw std::runtime_error( "Cannot create terrain-city footway rigidbody." );
                    }
                }
                if( sharedFootways )
                {
                    cityCrossingLandings::build( assets, fitted );
                    cityStormDrains::build( assets, fitted, seed, quality );
                }
                SourceBudgetVector<CityStreetFixture> installedFurniture;
                buildTerrainCityStreetDressing( assets, fitted, terrainStreetLamps, quality, installedFurniture );
                cityCrossingBollards::build( assets, fitted, installedFurniture, terrainStreetLamps, quality );
                cityCarriagewayUtilities::build( assets, fitted, seed, quality );
                cityRoundaboutLandscape::build( assets, seed );
                SourceBudgetVector<cityParkLandscape::Layout> parkLayouts;
                SourceBudgetVector<cityReserveLandscape::Site> reserveSites;
                {
                    const auto layers = cityCarriagewayUtilities::activeLayers( assets );
                    const cityRoadEdges::Surface roadSurface( layers.road );
                    SourceBudgetVector<cityRoadEdges::Bounds> pedestrianCorridors;
                    cityGroundContact::build( assets, buildingEntries, fitted, pavingMaterial, forge,
                        layers.road ? &roadSurface : nullptr, walks.mesh, seed, pedestrianCorridors );
                    cityEntranceWayfinding::build( assets, entranceSigns );
                    cityRoundaboutSigns::build( assets, fitted, terrainStreetLamps, quality );
                    cityRoadEdges::buildGutters( assets, fitted, roadSurface, layers.roadMaterial );
                    buildTerrainCityVergeTransitions( assets, fitted, seed, quality, roadSurface );
                    parkLayouts = cityParkLandscape::build( assets, city, fitted, walks.mesh, roadSurface,
                                                           pavingMaterial, seed, surfaceParams.size, quality );
                    if( layers.road )
                        reserveSites = cityReserveLandscape::build( assets, city, fitted, walks.mesh, roadSurface,
                            buildingEntries, parkLayouts, pedestrianCorridors, seed, quality );
                }
                cityParkAmenities::build( assets, parkLayouts, seed );
                buildTerrainCityVerges( assets, fitted, seed, quality );
                buildFoliageWorld( assets, seed, quality, &fitted, &parkLayouts, &reserveSites );
            }
            mesh( assets, "City street furniture", streetMetal,
                  material( assets, { .16f, .19f, .20f, 1 }, .48f, .6f ) );
            mesh( assets, "City lamp lenses", lampGlass,
                  material( assets, { .80f, .82f, .76f, 1 }, .35f ) );
            auto land = material( assets, { .34f, .48f, .25f, 1 }, 1 );
            if( !applyCitySurface( assets, land, CitySurface::Ground, surfaceParams.size, .5f, 3 ) )
            {
                auto soil = forge->bakeSurface( SurfaceTag::Dirt, surfaceParams );
                maps( assets, land, soil.albedo, soil.normal, soil.orm );
                land->setDiffuse( { .34f, .48f, .25f, 1 } );
                land->setUVProjection( 3 );
                land->setTriplanarScale( .1f );
                land->setNormalStrength( .15f );
                land->setEditorFloat( "aoStrength", .4f );
            }
            mesh( assets, "City parks", parks, land );
            buildCityStreetDressing( assets, city, quality );
            Geometry ground;
            ground.box( { 0, -.06f, 0 }, { city.extent * 2 + 80, .1f, city.extent * 2 + 80 } );
            if( !city.naturalRoads && !assets.cityTerrainRoads )
            {
                mesh( assets, "City ground", ground, material( assets, { .28f, .30f, .26f, 1 }, 1 ) );
            }
            // Bounded driving remains inside the collision walls. A surrounding ground
            // surface gives the skyline context instead of ending the city in the sky.
            Geometry hinterland;
            if( assets.cityTerrainRoads )
            {
                const auto snapshot = assets.roadAuthoring->getTerrainSnapshot();
                if( snapshot )
                    cityHinterland::build( hinterland, *snapshot, assets.roadAuthoring->getTerrainPlacement(),
                                           city.extent, seed, quality );
            }
            else
            {
                const float horizon = city.extent + 1400;
                hinterland.quad( { -horizon, -.12f, -horizon }, { horizon, -.12f, -horizon },
                                 { horizon, -.12f, horizon }, { -horizon, -.12f, horizon },
                                 { 0, 1, 0 } );
            }
            if( city.naturalRoads && !assets.cityTerrainRoads )
            {
                mesh( assets, "City ground", ground, land );
            }
            mesh( assets, "City surrounding landscape", hinterland, land );
            if( assets.useFoliageWorld && !assets.cityTerrainRoads )
            {
                buildFoliageWorld( assets, seed, quality );
            }
            assets.circuit = {};
            for( size_t i = 0; i < city.route.size(); ++i )
            {
                const auto p = city.route[i], next = city.route[( i + 1 ) % city.route.size()];
                const auto position = assets.cityTerrainRoads ? fittedRoute[i] : Vector3F( p.x, 0, p.z );
                const auto nextPosition = assets.cityTerrainRoads
                                              ? fittedRoute[( i + 1 ) % fittedRoute.size()]
                                              : Vector3F( next.x, 0, next.z );
                Vector3F forward = nextPosition - position;
                forward.normalise();
                assets.circuit.samples.push_back(
                    { position, { -forward.z, 0, forward.x }, assets.circuit.length } );
                assets.circuit.length += ( nextPosition - position ).length();
            }
            Geometry arrows;
            for( size_t i = 0; i < assets.circuit.samples.size(); i += 20 )
            {
                const auto &sample = assets.circuit.samples[i];
                const auto forward = Vector3F( sample.right.z, 0, -sample.right.x );
                const auto p = sample.position + Vector3F( 0, .04f, 0 );
                const auto tip = p + forward * 2;
                arrows.quad( p - sample.right * 1.5f, p + sample.right * 1.5f, tip + sample.right * .01f,
                             tip - sample.right * .01f, { 0, 1, 0 } );
            }
            if( !assets.cityTerrainRoads )
            {
                mesh( assets, "City race route arrows", arrows,
                      material( assets, { .05f, .7f, .9f, 1 }, .6f ) );
                assets.cityLayout = std::move( city );
            }
        }

        void buildRoadAuthoringShowcase( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32,
                                         VehicleAppearanceQuality )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto manager = app->getGameManager();
            auto scene = manager->getCurrentScene();
            if( !detail::runtimeActorPublished( vehicle ) || vehicle->getScene() != scene )
            {
                throw std::runtime_error( "Attach the road showcase owner to the current scene before generating." );
            }
            const auto root = [&]( const String &name ) {
                reserveOwnerSlot( assets.actors );
                auto actor = manager->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create road authoring showcase root." );
                }
                assets.actors.push_back( actor );
                actor->setName( name );
                actor->setStatic( true );
                actor->setPosition( Vector3F::zero() );
                actor->setOrientation( QuaternionF::identity() );
                actor->setScale( Vector3F::unit() );
                actor->setFlag( IGameActor::ActorFlagDontSave, true );
                attachWorldActor( scene, vehicle, actor );
                return actor;
            };
            auto terrainActor = root( "Road authoring showcase terrain" );
            assets.ground = terrainActor;
            assets.roadTerrain = terrainActor->addComponent<TerrainSystem>();
            if( !assets.roadTerrain || !terrainActor->addComponent<CollisionTerrain>() ||
                !terrainActor->addComponent<Rigidbody>() )
            {
                throw std::runtime_error(
                    "Road showcase requires native terrain render and collision components." );
            }
            if( !assets.roadTerrain->isLoaded() )
            {
                assets.roadTerrain->load( nullptr );
            }
            auto terrainMaterial = terrainActor->addComponent<scene::Material>();
            if( terrainMaterial )
            {
                terrainMaterial->setMaterialPath( "DefaultTerrain.mat" );
            }
            assets.roadTerrain->setTreesEnabled( false );
            assets.roadTerrain->setGrassEnabled( false );
            render::TerrainData terrain;
            terrain.dimensions = { 97, 97 };
            terrain.spacing = { 2, 2 };
            terrain.origin = { -96, -96 };
            terrain.heights.resize( 97 * 97 );
            terrain.layers[0].tint = { .34f, .40f, .22f, 1 };
            terrain.layers[1].tint = { .30f, .25f, .17f, 1 };
            terrain.layerWeights.resize( 97 * 97 );
            terrain.appearance.macroScale = .025f;
            terrain.appearance.macroStrength = .22f;
            terrain.appearance.macroSeed = 42;
            terrain.appearance.macroTint = { .32f, .39f, .22f, 1 };
            for( int z = 0; z < 97; ++z )
            {
                for( int x = 0; x < 97; ++x )
                {
                    const float px = -96 + x * 2.f, pz = -96 + z * 2.f, r = std::hypot( px, pz ),
                                outside = std::clamp( ( r - 40 ) / 56.f, 0.f, 1.f );
                    terrain.heights[size_t( z ) * 97 + x] =
                        3 + .015f * px + .008f * pz +
                        2.1f * outside * outside * std::sin( px * .035f ) * std::cos( pz * .035f );
                    terrain.layerWeights[size_t( z ) * 97 + x] = { 1 - outside * .35f, outside * .35f, 0,
                                                                   0 };
                }
            }
            // This new root has no visible generation yet. Publish its initial CPU/
            // collision snapshot before the first deferred GPU upload can run.
            auto terrainRenderer =
                dynamic_pointer_cast<render::Terrain>( assets.roadTerrain->getTerrain() );
            const bool deferred = terrainRenderer && terrainRenderer->isTerrainPublicationDeferred();
            if( deferred )
            {
                terrainRenderer->setDeferredTerrainPublication( false );
            }
            String error;
            const bool terrainApplied = assets.roadTerrain->applyTerrainData( terrain, error );
            if( deferred )
            {
                terrainRenderer->setDeferredTerrainPublication( true );
            }
            if( !terrainApplied )
            {
                throw std::runtime_error( error.c_str() );
            }
            if( !assets.roadTerrain->getTerrain() )
            {
                throw std::runtime_error(
                    "Road showcase native terrain renderer is unavailable after component load." );
            }
            auto authoringActor = root( "Road authoring showcase source" );
            assets.roadAuthoring = authoringActor->addComponent<CityRoadAuthoring>();
            if( !assets.roadAuthoring ||
                !assets.roadAuthoring->bindTerrain( assets.roadTerrain, error ) )
            {
                throw std::runtime_error( error.empty() ? "Cannot bind showcase authoring terrain."
                                                        : error.c_str() );
            }
            const auto id = []( unsigned value ) {
                char text[37];
                std::snprintf( text, sizeof( text ), "f1a00000-2026-4000-8000-%012u", value );
                return SourceBudgetText( text );
            };
            procedural::CityRoadSplineSource source;
            source.version = 2;
            source.id = id( 1 );
            const auto binding = assets.roadAuthoring->getTerrainBindingUUID();
            source.terrainResourceUuid.assign( binding.data(), binding.size() );
            procedural::CityRoadRoundaboutSpec junction;
            junction.junctionId = id( 2 );
            junction.center = { 0, 3.025f, 0 };
            junction.outerRadius = 14;
            junction.islandRadius = 8;
            junction.clockwise = true;
            junction.gradeX = .015f;
            junction.gradeZ = .008f;
            junction.maxGrade = .08f;
            const auto snapshot = assets.roadTerrain->getTerrainSnapshot();
            for( unsigned i = 0; i < 4; ++i )
            {
                const Vector3F outward = i == 0   ? Vector3F( 1, 0, 0 )
                                         : i == 1 ? Vector3F( 0, 0, 1 )
                                         : i == 2 ? Vector3F( -1, 0, 0 )
                                                  : Vector3F( 0, 0, -1 );
                const Vector3F side( -outward.z, 0, outward.x );
                const float bend = i == 3 ? 1.5f : ( i % 2 ? 10.f : -10.f );
                procedural::CityRoadSpline road;
                road.id = id( 10 + i );
                road.width = 6;
                road.maxGrade = .08f;
                road.heightOffset = .025f;
                procedural::CityRoadRoundaboutPort port;
                port.approachId = road.id;
                port.outward = outward;
                port.width = 6;
                port.length = 18;
                junction.ports.push_back( port );
                auto start = outward * 32.f, end = outward * 85.f + side * bend;
                if( !snapshot ||
                    !render::sampleTerrainHeight( *snapshot, Transform3<real_Num>(), start, start.y ) ||
                    !render::sampleTerrainHeight( *snapshot, Transform3<real_Num>(), end, end.y ) )
                {
                    throw std::runtime_error( "Showcase spline endpoint escaped terrain coverage." );
                }
                auto out = outward * 18.f, in = -outward * 18.f - side * ( bend * .5f );
                out.y = .015f * out.x + .008f * out.z;
                in.y = .015f * in.x + .008f * in.z;
                road.knots.push_back( { id( 20 + i * 2 ), start, {}, out } );
                road.knots.push_back( { id( 21 + i * 2 ), end, in, {} } );
                source.roads.push_back( std::move( road ) );
            }
            source.roundabouts.push_back( junction );
            SourceBudgetText text;
            if( !procedural::serializeCityRoadSplineSource( source, text, error ) ||
                !assets.roadAuthoring->setSourceJSON( String( text.data(), text.size() ),
                                                      assets.roadAuthoring->getSourceRevision(),
                                                      error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            assets.triangles += 96 * 96 * 2 + assets.roadAuthoring->getRuntimeTriangleCount();
            assets.useFoliageWorld = false;
            assets.circuit = {};
            for( unsigned i = 0; i < 96; ++i )
            {
                const float direction = junction.clockwise ? 1.f : -1.f,
                            angle = direction * i * 2 * pi / 96,
                            next = direction * ( i + 1 ) * 2 * pi / 96, radius = 11;
                Vector3F p( std::cos( angle ) * radius, 0, std::sin( angle ) * radius ),
                    q( std::cos( next ) * radius, 0, std::sin( next ) * radius );
                bool pDriveable = false, qDriveable = false;
                if( !assets.roadAuthoring->getRuntimeSurface( p.x, p.z, p.y, pDriveable ) ||
                    !pDriveable ||
                    !assets.roadAuthoring->getRuntimeSurface( q.x, q.z, q.y, qDriveable ) ||
                    !qDriveable )
                {
                    throw std::runtime_error(
                        "Showcase circulation route left the published driveable roundabout surface." );
                }
                const auto forward = ( q - p ).normaliseCopy();
                assets.circuit.samples.push_back(
                    { p, { -forward.z, 0, forward.x }, assets.circuit.length } );
                assets.circuit.length += ( q - p ).length();
            }
            f32 spawnY = 0;
            if( !render::sampleTerrainHeight( *snapshot, Transform3<real_Num>(), { 0, 0, -60 },
                                              spawnY ) )
            {
                throw std::runtime_error( "Showcase spawn has no terrain." );
            }
            vehicle->setPosition( { 0, spawnY + .445f, -60 } );
            vehicle->updateTransform();
            if( auto rigidbody = vehicle->getComponent<Rigidbody>() )
            {
                if( auto body = rigidbody->getRigidDynamic() )
                {
                    body->setTransform( Transform3F( vehicle->getPosition(), QuaternionF::identity() ) );
                }
            }
        }

#include "CityDaylightSky.hpp"

        void buildSky( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32 seed )
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
            if( assets.openCity )
            {
                // Morning sun reaches the south and west street elevations before
                // the cooler sky fill lifts the opposite faces.
                state.timeOfDay = 9.65f;
                state.turbidity = 2.7f;
                state.latitude = 48.f;
                state.dayOfYear = 117.f;
            }
            sky->setState( state );
            sky->update();
            const auto result = sky->getResults();
            auto graphicsScene = app->getGraphicsSystem()->getGraphicsScene();

            auto faces = makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            auto reflectionFaces = makeSourceBudgetSnapshot<SmartPtr<render::ITexture>>();
            const auto probePosition = vehicle->getPosition() + Vector3F( 0, 2, 0 );
            const Vector3F corners[8] = { { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 },
                                          { -1, -1, 1 },  { 1, -1, 1 },  { 1, 1, 1 },  { -1, 1, 1 } };
            const int faceCorners[6][4] = { { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 },
                                            { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };
            constexpr u32 reflectionSize = 256;
            const u32 samplesPerSide = assets.openCity ? 2 : 1;
            const u32 visibleSize = reflectionSize * samplesPerSide;
            // Downsample the same angular field for the HDR probe in linear space.
            // Keep the probe and its prefilter at their existing allocation size.
            const float reflectionWeight = 1.f / float( samplesPerSide * samplesPerSide );
            for( int face = 0; face < 6; ++face )
            {
                TextureBuffer texture( visibleSize, visibleSize );
                auto reflectionFace = makeSourceBudgetSnapshot<f32>();
                reflectionFace.resize( reflectionSize * reflectionSize * 4, 0.f );
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
                        const auto sunAlignment = std::clamp( dir.dotProduct( result.sunDir ), 0.f, 1.f );
                        ColourF c;
                        float sunTransmission = 1.f;
                        if( assets.openCity )
                        {
                            const auto city = cityDaylight::sample( dir, result.sunDir, sunAlignment, seed );
                            c = city.radiance;
                            sunTransmission = city.sunTransmission;
                        }
                        else
                        {
                            c = sky->computeSkyColour( dir, result.sunDir, state.turbidity,
                                                       result.sunAltitude );
                            const auto haze = 1.f - std::max( dir.y, 0.f );
                            const auto glow = std::pow( sunAlignment, 128.f );
                            c.r = .06f + .35f * haze + c.r * .15f + glow * .6f;
                            c.g = .22f + .35f * haze + c.g * .15f + glow * .55f;
                            c.b = .58f + .24f * haze + c.b * .15f + glow * .4f;
                        }
                        // Keep the solar core as radiance in the reflection source.
                        // Only the visible sky maps its display value to an 8-bit texture.
                        const float solarCore = std::pow( sunAlignment, 2048.f ) * sunTransmission;
                        c.r += solarCore * 16.f;
                        c.g += solarCore * 14.f;
                        c.b += solarCore * 10.f;
                        auto reflectionColour = c;
                        if( assets.openCity && dir.y < 0 )
                        {
                            const auto ground = std::clamp( -dir.y * 5.5f, 0.f, 1.f );
                            reflectionColour.r = c.r * ( 1 - ground ) + .115f * ground;
                            reflectionColour.g = c.g * ( 1 - ground ) + .139f * ground;
                            reflectionColour.b = c.b * ( 1 - ground ) + .106f * ground;
                        }
                        else if( dir.y < 0 )
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
                            reflectionFace.data() +
                            ( size_t( y / samplesPerSide ) * reflectionSize + x / samplesPerSide ) * 4;
                        for( size_t channel = 0; channel < 3; ++channel )
                        {
                            float value = channel == 0 ? c.r : channel == 1 ? c.g : c.b;
                            pixel[channel] = assets.openCity ? cityDaylight::encodeDisplay( value ) :
                                u8( std::clamp( std::pow( std::max( value, 0.f ), 1.f / 2.2f ) * 255.f, 0.f, 255.f ) );
                            const float reflectedValue = channel == 0   ? reflectionColour.r
                                                         : channel == 1 ? reflectionColour.g
                                                                        : reflectionColour.b;
                            reflectedPixel[channel] += std::max( reflectedValue, 0.f ) * reflectionWeight;
                        }
                        pixel[3] = 255;
                        reflectedPixel[3] = 1;
                    }
                }
                auto skyTexture = upload( assets, texture );
                faces.push_back( skyTexture );
                reflectionFaces.push_back(
                    uploadLinearRadiance( assets, reflectionFace, { reflectionSize, reflectionSize } ) );
            }
            auto skyObject = graphicsScene->addGraphicsObjectByType<render::ISky>();
            assets.sky = skyObject;
            skyObject->setTextures( faces );
            skyObject->setDistance( 500 );
            skyObject->setVisible( true );

            reserveOwnerSlot( assets.textures );
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
                reserveOwnerSlot( assets.actors );
                auto actor = app->getGameManager()->createActor();
                if( !actor )
                {
                    throw std::runtime_error( "Cannot create procedural actor" );
                }
                assets.actors.push_back( actor );
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
                    {
                        cityMaterial->setTexture( reflection, reflectionSlot );
                    }
                }
                // The actor's material also lets Cubemap manage the representative body material.
                actor->addComponent<scene::Material>()->setMaterial( assets.vehicleMaterials.front() );
                assets.reflectionActor = actor;
                WP_LOG(
                    "VehicleAdvanced: custom cubemap actor created; reflection applied to all vehicle "
                    "materials." );
            }

            graphicsScene->setAmbientLight( assets.openCity ? ColourF( .74f, .83f, .94f, 1 )
                                                            : ColourF( .28f, .32f, .38f, 1 ) );
            if( assets.openCity )
            {
                graphicsScene->setFog( render::IGraphicsScene::FOG_EXP2,
                    ColourF( citySkyHorizon.x, citySkyHorizon.y, citySkyHorizon.z, 1 ), .00172f, 0, 950 );
            }
            else
            {
                const auto horizon = sky->computeSkyColour( Vector3F( 1, 0, 0 ), result.sunDir,
                                                            state.turbidity, result.sunAltitude );
                const auto fogRadiance = []( float displayedLinear ) {
                    const float colour = std::clamp( displayedLinear, 0.f, .95f );
                    return colour / ( 1 - colour );
                };
                graphicsScene->setFog( render::IGraphicsScene::FOG_LINEAR,
                    ColourF( fogRadiance( .41f + horizon.r * .15f ), fogRadiance( .57f + horizon.g * .15f ),
                             fogRadiance( .82f + horizon.b * .15f ), 1 ), .0007f, 180, 800 );
            }
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
            sun->setDiffuseColour( assets.openCity ? ColourF( 1, .955f, .885f, 1 ) : ColourF( 1, .96f, .88f, 1 ) );
            sun->setPowerScale( assets.openCity ? 4.72f : 1.8f );
            sun->setCastShadows( true );
            sun->setVisible( true );
        }

        void buildTerrainGameplayScene( SceneAssets &assets )
        {
            const auto source = assets.terrainGameplaySource;
            String error;
            if( !source || !render::validateTerrainData( *source, error ) )
            {
                throw std::runtime_error( error.empty() ? "Terrain gameplay source is unavailable."
                                                        : error.c_str() );
            }

            // This bounded reference frame supports the existing driving telemetry.
            // Native terrain supplies every visible and collidable ground triangle;
            // there is no generated road mesh or fallback collision plane.
            const std::array<Vector3F, 5> corners = {
                Vector3F( 0, 0, 0 ), Vector3F( 0, 0, -160 ), Vector3F( 24, 0, -160 ),
                Vector3F( 24, 0, 32 ), Vector3F( 0, 0, 32 ) };
            Circuit circuit;
            circuit.samples.reserve( 256 );
            for( size_t i = 0; i < corners.size(); ++i )
            {
                const auto delta = corners[( i + 1 ) % corners.size()] - corners[i];
                const auto length = delta.length();
                const auto tangent = delta / length;
                const Vector3F right( -tangent.z, 0, tangent.x );
                const auto steps = static_cast<u32>( std::ceil( length / 2.f ) );
                for( u32 step = 0; step < steps; ++step )
                {
                    auto position = corners[i] + delta * ( float( step ) / steps );
                    f32 height = 0;
                    // Include the vehicle-width strip so a reference over a hole
                    // cannot be mistaken for a playable terrain route.
                    for( const auto offset : { -4.f, 0.f, 4.f } )
                    {
                        if( !render::sampleTerrainHeight( *source, Transform3<real_Num>(),
                                                         position + right * offset, height ) ||
                            std::abs( height ) > .001f )
                        {
                            throw std::runtime_error(
                                "Terrain gameplay requires a flat, solid route at y=0 inside "
                                "x=[-4,28], z=[-164,36]." );
                        }
                    }
                    circuit.samples.push_back(
                        { position, right, circuit.length + length * float( step ) / steps } );
                }
                circuit.length += length;
            }

            auto manager = core::IApplicationManager::instance()->getGameManager();
            reserveOwnerSlot( assets.actors );
            auto ground = manager->createActor();
            if( !ground )
            {
                throw std::runtime_error( "Cannot create terrain gameplay ground." );
            }
            assets.actors.push_back( ground );
            assets.ground = ground;
            ground->setName( "Terrain gameplay ground" );
            ground->setStatic( true );
            ground->setPosition( Vector3F::zero() );
            ground->setOrientation( QuaternionF::identity() );
            ground->setScale( Vector3F::unit() );
            auto terrain = ground->addComponent<TerrainSystem>();
            if( !terrain || !ground->addComponent<CollisionTerrain>() ||
                !ground->addComponent<Rigidbody>() )
            {
                throw std::runtime_error(
                    "Terrain gameplay requires native terrain, collision and rigidbody components." );
            }
            if( !terrain->isLoaded() )
            {
                terrain->load( nullptr );
            }
            terrain->setTreesEnabled( false );
            terrain->setGrassEnabled( false );
            if( !terrain->getTerrain() || !terrain->applyTerrainData( *source, error ) )
            {
                throw std::runtime_error( error.empty() ? "Terrain gameplay renderer is unavailable."
                                                        : error.c_str() );
            }
            // Preserve the renderer's deferred publication boundary. Configuration
            // waits for its matching native render/query/collision generation.
            assets.circuit.swap( circuit );
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

    bool activateGeneratedScene( const SceneAssets &assets, SmartPtr<scene::IGameActor> owner,
                                 String &error )
    {
        error.clear();
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        auto currentScene = manager ? manager->getCurrentScene() : nullptr;
        if( !owner || !currentScene )
        {
            error = "Generated scene activation has no owner/current scene.";
            return false;
        }
        if( currentScene->getSceneLoadingState() != scene::IGameScene::SceneLoadingState::Loaded ||
            !Thread::getTaskFlag( Thread::Application_Flag ) )
        {
            return false;
        }
        try
        {
            // Children are not scene roots: Lua reparents generated scenery and
            // the Sample attaches render children before adding its car actor.
            // Require a live scene-owned ancestor, rather than hierarchy-enabled
            // alone (which also accepts removed, still-loaded actors).
            const auto inCurrentHierarchy = [&]( SmartPtr<scene::IGameActor> candidate ) {
                bool anchored = false;
                size_t depth = 0;
                for( ; candidate && depth < 1024; candidate = candidate->getParent(), ++depth )
                {
                    if( !candidate->isLoaded() )
                    {
                        return false;
                    }
                    const auto candidateScene = candidate->getScene();
                    if( candidateScene && candidateScene != currentScene )
                    {
                        return false;
                    }
                    anchored |= candidateScene == currentScene &&
                                candidate->getFlag( scene::IGameActor::ActorFlagInScene );
                }
                return !candidate && anchored;
            };
            // Initial Sample construction and scene restoration can precede
            // attaching the owner. Leave readiness false until that boundary.
            if( !inCurrentHierarchy( owner ) )
            {
                return false;
            }
            SourceBudgetVector<SmartPtr<scene::IGameActor>> cohort;
            cohort.reserve( assets.actors.size() + 2 );
            // Persistent anchors live outside assets.actors and must also have
            // completed lifecycle activation after an explicit terrain rebind.
            const auto terrainActor = assets.roadTerrain ? assets.roadTerrain->getActor() : nullptr;
            const auto sourceActor = assets.roadAuthoring ? assets.roadAuthoring->getActor() : nullptr;
            if( terrainActor )
            {
                cohort.push_back( terrainActor );
            }
            if( sourceActor && sourceActor != terrainActor )
            {
                cohort.push_back( sourceActor );
            }
            for( const auto &generated : assets.actors )
            {
                if( generated && generated != terrainActor && generated != sourceActor )
                {
                    cohort.push_back( generated );
                }
            }
            for( const auto &generated : cohort )
            {
                if( !inCurrentHierarchy( generated ) )
                {
                    throw std::runtime_error( "Generated actor is detached from the current scene: " +
                                              generated->getName() );
                }
            }
            const auto actorState =
                app->isPlaying() ? scene::IGameActor::State::Play : scene::IGameActor::State::Edit;
            const auto componentState =
                app->isPlaying() ? scene::IComponent::State::Play : scene::IComponent::State::Edit;
            SourceBudgetVector<SmartPtr<scene::IGameActor>> ancestors;
            ancestors.reserve( 16 );
            // Resolve parent poses directly before a Rigidbody Enter event can
            // create native contacts. Do not pump unrelated scene updates.
            for( const auto &generated : cohort )
            {
                ancestors.clear();
                auto parent = generated;
                while( parent && ancestors.size() < 1024 )
                {
                    ancestors.push_back( parent );
                    parent = parent->getParent();
                }
                if( parent )
                {
                    throw std::runtime_error(
                        "Generated scene activation found an invalid hierarchy depth/cycle." );
                }
                for( auto it = ancestors.rbegin(); it != ancestors.rend(); ++it )
                {
                    if( auto transform = ( *it )->getTransform() )
                    {
                        transform->update();
                    }
                }
            }
            for( auto generated : cohort )
            {
                if( !generated->isLoaded() )
                {
                    throw std::runtime_error( "Generated actor is not loaded: " + generated->getName() );
                }
                generated->setState( actorState, false );
                // Same-state actors can still contain newly added components.
                for( auto component : generated->getComponents() )
                {
                    if( component && component->isLoaded() )
                    {
                        component->setState( componentState );
                    }
                }
                generated->updateTransform();
                if( generated->getState() != actorState )
                {
                    throw std::runtime_error( "Generated actor lifecycle activation is deferred: " +
                                              generated->getName() );
                }
            }
            auto physicsManager = app->getPhysicsManager();
            auto physics = physicsManager ? physicsManager->getPhysicsScene() : nullptr;
            if( !physics )
            {
                throw std::runtime_error(
                    "Generated scene activation requires a native physics scene." );
            }
            if( assets.terrainGameplaySource )
            {
                const auto terrain =
                    assets.ground ? assets.ground->getComponent<TerrainSystem>() : nullptr;
                const auto collision =
                    assets.ground ? assets.ground->getComponent<CollisionTerrain>() : nullptr;
                if( !terrain || !collision )
                {
                    throw std::runtime_error( "Terrain gameplay ground lost its native components." );
                }
                const auto renderer = dynamic_cast<render::Terrain *>( terrain->getTerrain().get() );
                const auto requested = terrain->getTerrainSnapshot();
                render::TerrainSnapshot active;
                Transform3<real_Num> placement;
                u64 serial = 0;
                bool detached = true;
                terrain->getTerrainIntegrationState( active, placement, serial, detached );
                if( !renderer || !requested || !serial || detached || active != requested ||
                    renderer->hasPendingTerrainPublication() ||
                    renderer->getTerrainSnapshot() != requested ||
                    renderer->getActiveTerrainSnapshot() != requested ||
                    collision->getTerrainSnapshot() != requested )
                {
                    // A newly generated deferred terrain has no native shape until
                    // the render transaction publishes all consumers together.
                    return false;
                }
            }
            for( const auto &generated : cohort )
            {
                if( generated->isEnabledInScene() )
                {
                    auto collision = generated->getComponent<scene::Collision>();
                    if( !collision || !collision->isEnabled() )
                    {
                        continue;
                    }
                    auto rigidbody = generated->getComponent<scene::Rigidbody>();
                    auto body = rigidbody ? rigidbody->getRigidStatic() : nullptr;
                    if( !generated->isStatic() || !rigidbody || !body || !collision->getShape() ||
                        !collision->getShape()->isAttached() || body->getScene() != physics ||
                        body->getNumShapes() == 0 || rigidbody->getState() != componentState ||
                        collision->getState() != componentState )
                    {
                        throw std::runtime_error(
                            "Generated static collision publication is incomplete: " +
                            generated->getName() );
                    }
                    const auto expected = generated->getWorldTransform(), actual = body->getTransform();
                    if( ( expected.getPosition() - actual.getPosition() ).length() > .001f ||
                        ( expected.getOrientation() * Vector3F::unitX() -
                          actual.getOrientation() * Vector3F::unitX() )
                                .length() > .001f )
                    {
                        throw std::runtime_error( "Generated static collision pose is stale: " +
                                                  generated->getName() );
                    }
                }
            }
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }

    namespace
    {
        void buildFlatGround( SceneAssets &assets )
        {
            reserveOwnerSlot( assets.actors );
            auto ground = core::IApplicationManager::instance()->getGameManager()->createActor();
            if( !ground )
            {
                throw std::runtime_error( "Cannot create procedural collision actor." );
            }
            assets.actors.push_back( ground );
            assets.ground = ground;
            ground->setName( "Circuit collision plane" );
            ground->setStatic( true );
            ground->setPosition( { 0, -.5f, 0 } );
            ground->addComponent<CollisionBox>()->setExtents( { 1100, 1, 1100 } );
            ground->addComponent<Rigidbody>();
        }

        void attachGeneratedActors( SceneAssets &assets, SmartPtr<IGameActor> owner )
        {
            auto manager = core::IApplicationManager::instance()->getGameManager();
            auto scene = manager->getCurrentScene();
            reserveOwnerSlot( assets.actors );
            auto scenery = manager->createActor();
            if( !scenery )
            {
                throw std::runtime_error( "Cannot create procedural scenery group." );
            }
            assets.sceneryRoot = scenery;
            assets.actors.push_back( scenery );
            scenery->setName( "Procedural scenery" );
            scenery->setStatic( true );
            scenery->setFlag( IGameActor::ActorFlagDontSave, true );
            scenery->setScene( scene );

            // Build the hierarchy before entering Edit/Play. The world's poses
            // remain relative to this identity group, independent of the owner.
            for( auto actor : assets.actors )
            {
                if( actor == scenery )
                {
                    continue;
                }
                actor->setFlag( IGameActor::ActorFlagDontSave, true );
                if( !actor->getParent() )
                {
                    scenery->addChild( actor );
                }
                actor->setScene( scene );
                scene->registerAllUpdates( actor );
            }
            if( !owner->getScene() )
            {
                scenery->setScene( nullptr );
            }
            attachWorldActor( scene, owner, scenery );
        }

        void buildVehicleShadow( SceneAssets &assets )
        {
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
            shadowGeometry.quad( { -1.6f, .025f, -3.7f }, { 1.6f, .025f, -3.7f },
                                 { 1.6f, .025f, 3.7f }, { -1.6f, .025f, 3.7f }, { 0, 1, 0 } );
            assets.shadow = mesh( assets, "Vehicle contact shadow", shadowGeometry, shadowMaterial );
        }
    }  // namespace

    void buildCircuitScene( SceneAssets &assets, SmartPtr<IGameActor> owner, u32 seed,
                            VehicleAppearanceQuality quality )
    {
        assets.resourcePrefix = "__procedural/circuit/" + StringUtil::getUUID();
        if( assets.terrainGameplaySource )
        {
            buildTerrainGameplayScene( assets );
        }
        else
        {
            buildFlatGround( assets );
            buildTrack( assets, seed, quality );
        }
        attachGeneratedActors( assets, owner );
    }

    void buildCityScene( SceneAssets &assets, SmartPtr<IGameActor> owner, u32 seed,
                         VehicleAppearanceQuality quality )
    {
        assets.resourcePrefix = "__procedural/city/" + StringUtil::getUUID();
        assets.openCity = true;
        if( !assets.cityTerrainRoads )
        {
            buildFlatGround( assets );
        }
        buildCity( assets, owner, nullptr, seed, quality );
        attachGeneratedActors( assets, owner );
    }

    void buildScene( SceneAssets &assets, SmartPtr<IGameActor> actor, u32 seed,
                     VehicleAppearanceQuality quality, SmartPtr<IGameActor> citySourceOwner )
    {
        assets.resourcePrefix = "__procedural/race/" + StringUtil::getUUID();
        const auto started = std::chrono::steady_clock::now();
        if( !assets.ground && !assets.cityRoadShowcase && !assets.cityTerrainRoads &&
            !assets.terrainGameplaySource )
        {
            buildFlatGround( assets );
        }
        buildCar( assets, actor, seed, quality );
        if( assets.terrainGameplaySource )
        {
            buildTerrainGameplayScene( assets );
        }
        else if( assets.cityRoadShowcase )
        {
            buildRoadAuthoringShowcase( assets, actor, seed, quality );
        }
        else if( assets.openCity )
        {
            buildCity( assets, citySourceOwner ? citySourceOwner : actor, actor, seed, quality );
        }
        else
        {
            buildTrack( assets, seed, quality );
        }
        buildSky( assets, actor, seed );
        buildVehicleShadow( assets );
        attachGeneratedActors( assets, actor );

        String activationError;
        if( !activateGeneratedScene( assets, actor, activationError ) && !activationError.empty() )
        {
            throw std::runtime_error( activationError.c_str() );
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

    void clearScene( SceneAssets &assets, SmartPtr<IGameActor> owner )
    {
        SceneAssets retired;
        assets.swap( retired );
        // Release producer storage before retirement callbacks need new allocations.
        retired.vehicle = {};
        retired.circuit = {};
        retired.cityLayout = {};
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;

        if( retired.vehicleLOD )
        {
            if( auto system =
                    dynamic_pointer_cast<LODSystem>( retired.vehicleLOD->getComponentSystem() ) )
            {
                system->clearViewOverride();
            }
            if( owner )
            {
                owner->removeComponentInstance( retired.vehicleLOD );
            }
        }

        if( manager )
        {
            if( retired.sceneryRoot )
            {
                try
                {
                    retired.sceneryRoot->removeChildren();
                }
                catch( const std::bad_alloc & )
                {
                    // Retire actors individually if the bulk snapshot cannot be admitted.
                }
            }
            for( auto it = retired.actors.rbegin(); it != retired.actors.rend(); ++it )
            {
                if( *it == retired.sceneryRoot )
                {
                    continue;
                }
                try
                {
                    manager->destroyActor( *it );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
            if( retired.sceneryRoot )
            {
                try
                {
                    manager->destroyActor( retired.sceneryRoot );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
        }

        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if( graphics )
        {
            auto scene = graphics->getGraphicsScene();
            if( retired.foliageTerrain )
            {
                String error;
                retired.foliageTerrain->setFoliageSnapshot( {}, error );
                if( scene && ( !retired.cityTerrainRoads || !retired.roadTerrain ||
                               retired.roadTerrain->getTerrain() != retired.foliageTerrain ) )
                {
                    scene->removeGraphicsObject( retired.foliageTerrain );
                }
            }

            if( scene && retired.sky )
            {
                scene->removeGraphicsObject( retired.sky );
            }

            if( scene && retired.sun )
            {
                scene->removeGraphicsObject( retired.sun );
            }

            auto materials = graphics->getMaterialManager();
            if( materials )
            {
                for( auto material : retired.materials )
                {
                    try
                    {
                        materials->destroyResource( material );
                    }
                    catch( const std::bad_alloc & )
                    {
                    }
                }
            }

            auto textures = graphics->getTextureManager();
            if( textures )
            {
                for( auto texture : retired.textures )
                {
                    // TextureManager detaches the resource before notifying it.
                    try
                    {
                        textures->destroyResource( texture );
                    }
                    catch( const std::bad_alloc & )
                    {
                    }
                }
            }
        }

        auto meshes = app ? dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) : nullptr;
        if( meshes )
        {
            for( auto mesh : retired.meshes )
            {
                try
                {
                    meshes->removeMeshResource( mesh );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
        }
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
        auto car = actor->getComponent<scene::FourWheelVehicle>();
        auto vehicle = car->getVehicleController();
        auto body = actor->getComponent<scene::Rigidbody>();
        auto com = vector( config.massProperties.centreOfMass );
        const bool hasHandlingPreset = car->hasHandlingPreset();
        if( !hasHandlingPreset )
        {
            car->setMass( float( config.massProperties.massKg ) );
            vehicle->setMass( float( config.massProperties.massKg ) );
            body->setMass( float( config.massProperties.massKg ) );
            car->setDriveType( VehicleDriveType::RearWheelDrive );
            // Brush tyre forces use negative steering yaw. Adapt the rendered wheels.
            auto visualProperties = car->getProperties();
            visualProperties->setProperty( "Visual Steering Sign", -1.f );
            car->setProperties( visualProperties );
        }
        car->setMOI( { float( config.massProperties.inertia.xx ),
                       float( config.massProperties.inertia.yy ),
                       float( config.massProperties.inertia.zz ) } );
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
            wheelProps->setProperty( "Grip", 1.25f );
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
        car->refreshHandlingPreset();
    }
}  // namespace workphone::scene::race
