#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
#include "Workphone/Scene/Components/CityFoliageLOD.hpp"
#include "Workphone/Scene/Components/CityHillsideVegetation.hpp"
#include "Workphone/Scene/Components/CityParkPlanting.hpp"
#include "Workphone/Scene/Components/CityReservePlanting.hpp"

        void buildFoliageWorld( SceneAssets &assets, u32 seed, VehicleAppearanceQuality quality,
                                const OpenCityLayout *city, const CityRoadTerrainMesh *cityFootways,
                                const SourceBudgetVector<cityParkLandscape::Layout> *parkLayouts,
                                const SourceBudgetVector<cityReserveLandscape::Site> *reserveSites )
        {
            auto app = core::IApplicationManager::instance();
            auto scene = app->getGraphicsSystem()->getGraphicsScene();
            const bool optionalHillside = cityFootways && !assets.useFoliageWorld;
            if( optionalHillside )
            {
                const auto type =
                    scene ? TypeManager::instance()->getName( scene->getTypeInfo() ) : nullptr;
                if( !type || String( type ) != "workphone::render::ClawScene" )
                {
                    return;
                }
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
                if( optionalHillside )
                {
                    return;
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
            const cityFoliageLOD::LeafSource leafSource( leafAtlas, leafTextureName );
            auto world = std::make_shared<render::FoliageWorldData>();
            world->revision = 1;
            // The bounded city overview needs a longer tree view range. Larger city
            // pages keep that range inside the existing 30-page-width pager gate.
            if( city )
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
                species->maximumDistance = city ? std::max( 600.f, city->extent * 5.f ) : 600.f;
                species->shadowDistance = 120;
                cityFoliageLOD::configure( assets, *species, leafSource, quality );
                world->species.push_back( species );
                for( const auto &lod : species->lods )
                {
                    assets.triangles += lod.indices.size() / 3;
                }
            }
            std::mt19937 random( seed );
            std::uniform_real_distribution<float> unit( 0, 1 );
            const int count = parkLayouts || ( cityFootways && !assets.useFoliageWorld ) ? 0
                              : quality == VehicleAppearanceQuality::Preview             ? 90
                                                                                         : 220;
            SourceBudgetVector<size_t> cityParks;
            if( city && count > 0 )
            {
                cityParks.reserve(
                    size_t( std::count_if( city->lots.begin(), city->lots.end(),
                                           []( const auto &lot ) { return lot.park; } ) ) );
                for( size_t index = 0; index < city->lots.size(); ++index )
                {
                    if( city->lots[index].park )
                    {
                        cityParks.push_back( index );
                    }
                }
                // A bounded preview must not always populate only the first city rows.
                // Every park receives a candidate before another round is distributed.
                std::sort( cityParks.begin(), cityParks.end(), [&]( size_t a, size_t b ) {
                    const auto ha = render::foliageStableHash( city->lots[a].id );
                    const auto hb = render::foliageStableHash( city->lots[b].id );
                    return ha != hb ? ha < hb : city->lots[a].id < city->lots[b].id;
                } );
            }
            for( int i = 0; i < count; ++i )
            {
                Vector3F p( -100 + unit( random ) * 380, 0, -250 + unit( random ) * 440 );
                u64 cityPlacementId = 0;
                if( city )
                {
                    if( cityParks.empty() )
                    {
                        continue;
                    }
                    const auto &lot = city->lots[cityParks[size_t( i ) % cityParks.size()]];
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
                cityHillside::addSpecies( assets, *world, leafTextureName, seed, leafSource, quality );
                if( assets.roadAuthoring->getSource().roads.size() ==
                    assets.roadAuthoring->getBake().roads.size() )
                {
                    cityHillside::Exclusions exclusions;
                    cityHillside::indexCity( exclusions, assets, *cityFootways );
                    if( parkLayouts )
                    {
                        cityParkPlanting::scatter( assets, *parkLayouts, exclusions, *world, seed,
                                                   quality );
                    }
                    cityHillside::scatter( assets, exclusions, *world, seed, quality );
                    if( reserveSites )
                    {
                        cityReservePlanting::scatter( assets, *reserveSites, exclusions, *world,
                                                      quality );
                    }
                }
            }
            if( !render::validateFoliageWorld( *world, error ) ||
                !assets.foliageTerrain->setFoliageSnapshot( world, error ) )
            {
                throw std::runtime_error( "Cannot publish racing foliage: " + error );
            }
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
