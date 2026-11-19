#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace sceneBuild
    {
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
            const std::array<Vector3F, 5> corners = { Vector3F( 0, 0, 0 ), Vector3F( 0, 0, -160 ),
                                                      Vector3F( 24, 0, -160 ), Vector3F( 24, 0, 32 ),
                                                      Vector3F( 0, 0, 32 ) };
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
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
